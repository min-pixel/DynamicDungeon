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
    while (!bStopThread)
    {
        uint32 Pending = 0;
        if (Socket && Socket->HasPendingData(Pending))
        {
            TArray<uint8> Buffer;
            Buffer.SetNumUninitialized(Pending);
            int32 Read = 0;

            // Read==0 이거나 Recv 실패 시 루프 탈출
            if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), Read) || Read == 0)
            {
                break;
            }

            AsyncTask(ENamedThreads::GameThread, [this, Buffer]()
                {
                    if (Owner) Owner->OnDataReceived.Broadcast(Buffer);
                });
        }
        FPlatformProcess::Sleep(0.01f);
    }
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
}

void USocketManager::Deinitialize()
{
    if (ReceiverRunnable)
    {
        ReceiverRunnable->Stop();
        delete ReceiverRunnable;
        ReceiverRunnable = nullptr;
    }
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
    Super::Deinitialize();
}

bool USocketManager::Connect(const FString& Address, int32 Port)
{
    // 1) 이전 소켓 & 수신 쓰레드 안전 정리
    if (ReceiverRunnable)
    {
        ReceiverRunnable->Shutdown();
        delete ReceiverRunnable;
        ReceiverRunnable = nullptr;
    }
    if (Socket)
    {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }

    ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    Socket = Subsys->CreateSocket(NAME_Stream, TEXT("AuthSocket"), false);

    /*FIPv4Address IP;
    FIPv4Address::Parse(Address, IP);*/

    // 원격 서버(192.168.0.12)에 고정 연결하도록 IP 지정
    FIPv4Address IP;
    {
        const FString RemoteIPString = TEXT("192.168.0.12");
        FIPv4Address::Parse(RemoteIPString, IP);
    }

    // 변수명을 InternetAddr 로 일치시킵니다
    TSharedRef<FInternetAddr> InternetAddr = Subsys->CreateInternetAddr();
    InternetAddr->SetIp(IP.Value);
    InternetAddr->SetPort(Port);

    Socket->SetNonBlocking(true);

    bool bOK = Socket->Connect(*InternetAddr);
    if (bOK)
    {
        ReceiverRunnable = new FSocketReceiver(Socket, this);
    }
    return bOK;
}

bool USocketManager::Send(const TArray<uint8>& Data)
{
    if (!Socket) return false;
    int32 Sent = 0;
    return Socket->Send(Data.GetData(), Data.Num(), Sent);
}
