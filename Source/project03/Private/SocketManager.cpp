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
    while (!bStopThread)
    {
        uint32 Pending = 0;
        if (Socket && Socket->HasPendingData(Pending))
        {
            TArray<uint8> Buffer;
            Buffer.SetNumUninitialized(Pending);
            int32 Read = 0;

            // Read==0 �̰ų� Recv ���� �� ���� Ż��
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

// ===== USocketManager ���� =====
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
    // 1) ���� ���� & ���� ������ ���� ����
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

    // ���� ����(192.168.0.12)�� ���� �����ϵ��� IP ����
    FIPv4Address IP;
    {
        const FString RemoteIPString = TEXT("192.168.0.12");
        FIPv4Address::Parse(RemoteIPString, IP);
    }

    // �������� InternetAddr �� ��ġ��ŵ�ϴ�
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
