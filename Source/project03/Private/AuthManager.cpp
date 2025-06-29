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
//        // 서버 연결 시도 (echo server)
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
    // 간단 스텁: 평문 그대로 해시
    return FMD5::HashAnsiString(*Password);
}

void UAuthManager::Login(const FString& Username, const FString& Password)
{
    // SocketMgr가 아직 없다면 이 자리에서 꺼내고 연결
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

    // 패킷 조립 (RequestType + Username + Password)
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Login);

    // Username
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password (평문)
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // 전송
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Login packet send: %s (len=%d)"),
        bSent ? TEXT("OK") : TEXT("FAIL"), Packet.Num());
}

void UAuthManager::RegisterUser(const FString& Username, const FString& Password)
{
    // SocketMgr가 아직 없다면 이 자리에서 꺼내고 연결
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

    // 패킷 조립 (RequestType + Username + Password)
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Register);

    // Username
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password (평문)
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // 전송
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Register packet send: %s (len=%d)"),
        bSent ? TEXT("OK") : TEXT("FAIL"), Packet.Num());
}

void UAuthManager::HandleSocketData(const TArray<uint8>& Data)
{
    // 서버가 첫 바이트로 1(성공) / 0(실패) 를 보냄
    bool bSuccess = (Data.Num() > 0 && Data[0] == 1);
    UE_LOG(LogTemp, Warning, TEXT("[Auth] Response byte=%d → %s"), Data.Num() > 0 ? Data[0] : -1,
           bSuccess ? TEXT("SUCCESS") : TEXT("FAIL"));
    OnAuthResponse.Broadcast(bSuccess);
}