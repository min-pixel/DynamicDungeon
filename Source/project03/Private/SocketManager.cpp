// Fill out your copyright notice in the Description page of Project Settings.


#include "SocketManager.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"


// ===== FSocketReceiver ���� =====
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

            // ������ ���� �õ�
            if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), Read) || Read == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SocketReceiver] Connection lost or recv failed"));

                // ���� �����忡�� ���� ���� ó��
                AsyncTask(ENamedThreads::GameThread, [this]()
                    {
                        if (Owner)
                        {
                            Owner->OnConnectionLost();
                        }
                    });
                break;
            }

            // Buffer ũ�� ���� (���� ���� ũ�⸸ŭ)
            Buffer.SetNum(Read);

            UE_LOG(LogTemp, Log, TEXT("[SocketReceiver] Received %d bytes"), Read);

            // ���� ������� ������ ����
            AsyncTask(ENamedThreads::GameThread, [this, Buffer]()
                {
                    if (Owner)
                    {
                        Owner->OnDataReceived.Broadcast(Buffer);
                    }
                });
        }

        // CPU ��뷮 ���̱� ���� ���
        FPlatformProcess::Sleep(0.01f);
    }

    UE_LOG(LogTemp, Log, TEXT("[SocketReceiver] Receiver thread stopped"));
    return 0;
}

void FSocketReceiver::Stop()
{
    bStopThread = true;
}

// ===== USocketManager ���� =====
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
    // �̹� ����Ǿ� ������ ���� ���� ����
    if (IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Already connected to %s:%d - keeping existing connection"), *Address, Port);
        return true;
    }

    // ���� ������ �ִٸ� ���� �� ���� ����
    if (Socket)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Cleaning up existing socket before new connection"));
        Disconnect();
    }

    UE_LOG(LogTemp, Warning, TEXT("[SocketManager] Connecting to %s:%d..."), *Address, Port);

    // ���� ����ý��� ��������
    ISocketSubsystem* Subsys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!Subsys)
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Socket subsystem not available"));
        return false;
    }

    // ���� ����
    Socket = Subsys->CreateSocket(NAME_Stream, TEXT("AuthSocket"), false);
    if (!Socket)
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Failed to create socket"));
        return false;
    }

    // IP �ּ� �Ľ�
    FIPv4Address IP;
    if (!FIPv4Address::Parse(Address, IP))
    {
        UE_LOG(LogTemp, Error, TEXT("[SocketManager] Invalid IP address: %s"), *Address);
        Disconnect();
        return false;
    }

    // ���� �ּ� ����
    TSharedRef<FInternetAddr> ServerAddr = Subsys->CreateInternetAddr();
    ServerAddr->SetIp(IP.Value);
    ServerAddr->SetPort(Port);

    // ����ŷ ��� ����
    Socket->SetNonBlocking(true);

    // ���� ���� �õ�
    bool bConnected = Socket->Connect(*ServerAddr);
    if (bConnected)
    {
        // ���� ������ ����
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

    // ���� ������ ����
    if (ReceiverRunnable)
    {
        ReceiverRunnable->Shutdown();
        delete ReceiverRunnable;
        ReceiverRunnable = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[SocketManager] Receiver thread cleaned up"));
    }

    // ���� ����
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

    // �ʿ�� �ڵ� �翬���̳� �̺�Ʈ ��ε�ĳ��Ʈ �߰� ����
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

        // ���� ���� �� ���� ���� ��Ȯ��
        if (!IsConnected())
        {
            OnConnectionLost();
        }
        return false;
    }
}
