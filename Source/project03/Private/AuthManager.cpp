// Fill out your copyright notice in the Description page of Project Settings.


#include "AuthManager.h"
#include "SocketManager.h"
//#include "Misc/SecureHash.h"   // FMD5
#include "Async/Async.h"
#include "Engine/Engine.h"

//void UAuthManager::Initialize(FSubsystemCollectionBase& Collection)
//{
//    Super::Initialize(Collection);
//    SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
//    if (SocketMgr)
//    {
//        // ���� ���� �õ� (echo server)
//        bool bOK = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
//        UE_LOG(LogTemp, Warning, TEXT("[Auth] Connect to echo server: %s"), bOK ? TEXT("OK") : TEXT("FAIL"));
//
//        SocketMgr->OnDataReceived.AddDynamic(this, &UAuthManager::HandleSocketData);
//    }
//    else
//    {
//        UE_LOG(LogTemp, Error, TEXT("[Auth] SocketManager not found"));
//    }
//}

FString UAuthManager::HashPassword(const FString& Password) const
{
    // ���� ����: �� �״�� �ؽ�
    return FMD5::HashAnsiString(*Password);
}

void UAuthManager::Login(const FString& Username, const FString& Password)
{
    // SocketMgr�� ���� ���ٸ� �� �ڸ����� ������ ����
    if (!SocketMgr)
    {
        SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
        if (!SocketMgr)
        {
            UE_LOG(LogTemp, Error, TEXT("[Auth] SocketManager not found in Login()"));
            return;
        }
        bool bOK = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
        UE_LOG(LogTemp, Warning, TEXT("[Auth] Connect to echo server: %s"), bOK ? TEXT("OK") : TEXT("FAIL"));
        SocketMgr->OnDataReceived.AddDynamic(this, &UAuthManager::HandleSocketData);
    }

    // ��Ŷ ���� (RequestType + Username + Password)
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Login);

    // Username
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password (��)
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // ����
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Login packet send: %s (len=%d)"),
        bSent ? TEXT("OK") : TEXT("FAIL"), Packet.Num());
}

void UAuthManager::RegisterUser(const FString& Username, const FString& Password)
{
    // SocketMgr�� ���� ���ٸ� �� �ڸ����� ������ ����
    if (!SocketMgr)
    {
        SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
        if (!SocketMgr)
        {
            UE_LOG(LogTemp, Error, TEXT("[Auth] SocketManager not found in RegisterUser()"));
            return;
        }
        bool bOK = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
        UE_LOG(LogTemp, Warning, TEXT("[Auth] Connect to echo server: %s"), bOK ? TEXT("OK") : TEXT("FAIL"));
        SocketMgr->OnDataReceived.AddDynamic(this, &UAuthManager::HandleSocketData);
    }

    // ��Ŷ ���� (RequestType + Username + Password)
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Register);

    // Username
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password (��)
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // ����
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Register packet send: %s (len=%d)"),
        bSent ? TEXT("OK") : TEXT("FAIL"), Packet.Num());
}

void UAuthManager::HandleSocketData(const TArray<uint8>& Data)
{
    // ������ ù ����Ʈ�� 1(����) / 0(����) �� ����
    bool bSuccess = (Data.Num() > 0 && Data[0] == 1);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Response byte=%d �� %s"), Data.Num() > 0 ? Data[0] : -1,
           bSuccess ? TEXT("SUCCESS") : TEXT("FAIL"));
    OnAuthResponse.Broadcast(bSuccess);
}