// Fill out your copyright notice in the Description page of Project Settings.


#include "SocketManager.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"


// ===== FSocketReceiver 구현 =====
FSocketReceiver::FSocketReceiver(FSocket* InSocket, USocketManager* InOwner)
    : Socket(InSocket), Owner(InOwner), bStopThread(false), Thread(nullptr)
{
    Thread = FRunnableThread::Create(this, TEXT("FSocketReceiver"), 0, TPri_BelowNormal);
}

FSocketReceiver::~FSocketReceiver()
{
    Shutdown();
}

bool FSocketReceiver::Init()
{
    return Socket != nullptr;
}

uint32 FSocketReceiver::Run()
{
    UE_LOG(LogTemp, Log, TEXT("[SocketReceiver] Receiver thread running"));

    while (!bStopThread)
    {
        uint32 Pending = 0;
        if (Socket && Socket->HasPendingData(Pending) && Pending > 0)
        {
            TArray<uint8> Buffer;
            Buffer.SetNumUninitialized(Pending);
            int32 Read = 0;

            // 데이터 수신 시도
            if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), Read) || Read == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SocketReceiver] Connection lost or recv failed"));

                // 게임 스레드에서 연결 끊김 처리
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        if (Owner)
                        {
                            Owner->OnConnectionLost();
                        }
                    });
                break;
            }

            // Buffer 크기 조정 (실제 읽은 크기만큼)
            Buffer.SetNum(Read);

            UE_LOG(LogTemp, Log, TEXT("[SocketReceiver] Received %d bytes"), Read);

            // 게임 스레드로 데이터 전달
            AsyncTask(ENamedThreads::GameThread, [this, Buffer]()
                {
                    if (Owner)
                    {
                        Owner->OnDataReceived.Broadcast(Buffer);
                    }
                });
        }

        // CPU 사용량 줄이기 위한 대기
        FPlatformProcess::Sleep(0.01f);
    }

    UE_LOG(LogTemp, Log, TEXT("[SocketReceiver] Receiver thread stopped"));
    return 0;
}

void FSocketReceiver::Stop()
{
    bStopThread = true;
}

// ===== USocketManager 구현 =====
void USocketManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Socket = nullptr;
    ReceiverRunnable = nullptr;
    bIsConnected = false;

    UE_LOG(LogTemp, Log, TEXT("[SocketManager] Subsystem initialized"));
}

void USocketManager::Deinitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Shutting down..."));
    Disconnect();
    Super::Deinitialize();
}

bool USocketManager::Connect(const FString& Address, int32 Port)
{
    // 이미 연결되어 있으면 기존 연결 유지
    if (IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Already connected to %s:%d - keeping existing connection"), *Address, Port);
        return true;
    }

    // 기존 연결이 있다면 정리 후 새로 연결
    if (Socket)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Cleaning up existing socket before new connection"));
        Disconnect();
    }

    UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Connecting to %s:%d..."), *Address, Port);

    // 소켓 서브시스템 가져오기
    ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!Subsys)
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Socket subsystem not available"));
        return false;
    }

    // 소켓 생성
    Socket = Subsys->CreateSocket(NAME_Stream, TEXT("AuthSocket"), false);
    if (!Socket)
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Failed to create socket"));
        return false;
    }

    // IP 주소 파싱
    FIPv4Address IP;
    if (!FIPv4Address::Parse(Address, IP))
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Invalid IP address: %s"), *Address);
        Disconnect();
        return false;
    }

    // 서버 주소 설정
    TSharedRef<FInternetAddr> ServerAddr = Subsys->CreateInternetAddr();
    ServerAddr->SetIp(IP.Value);
    ServerAddr->SetPort(Port);

    // 논블로킹 모드 설정
    Socket->SetNonBlocking(true);

    // 서버 연결 시도
    bool bConnected = Socket->Connect(*ServerAddr);
    if (bConnected)
    {
        // 수신 스레드 시작
        ReceiverRunnable = new FSocketReceiver(Socket, this);
        bIsConnected = true;

        UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Successfully connected to %s:%d"), *Address, Port);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Failed to connect to %s:%d"), *Address, Port);
        Disconnect();
    }

    return bConnected;
}

void USocketManager::Disconnect()
{
    UE_LOG(LogTemp, Log, TEXT("[SocketManager] Disconnecting..."));

    bIsConnected = false;

    // 수신 스레드 정리
    if (ReceiverRunnable)
    {
        ReceiverRunnable->Shutdown();
        delete ReceiverRunnable;
        ReceiverRunnable = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SocketManager] Receiver thread cleaned up"));
    }

    // 소켓 정리
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SocketManager] Socket closed and destroyed"));
    }
}


bool USocketManager::IsConnected() const
{
    return bIsConnected && Socket && Socket->GetConnectionState() == SCS_Connected;
}

void USocketManager::OnConnectionLost()
{
    UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Connection lost detected!"));
    bIsConnected = false;

    // 필요시 자동 재연결이나 이벤트 브로드캐스트 추가 가능
}

bool USocketManager::Send(const TArray<uint8>& Data)
{
    if (!IsConnected())
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Cannot send - not connected"));
        return false;
    }

    int32 BytesSent = 0;
    bool bSendResult = Socket->Send(Data.GetData(), Data.Num(), BytesSent);

    if (bSendResult && BytesSent == Data.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("[SocketManager] Successfully sent %d bytes"), BytesSent);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Send failed: sent %d of %d bytes"), BytesSent, Data.Num());

        // 전송 실패 시 연결 상태 재확인
        if (!IsConnected())
        {
            OnConnectionLost();
        }
        return false;
    }
}
