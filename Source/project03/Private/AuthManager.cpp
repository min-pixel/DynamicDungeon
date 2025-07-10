// Fill out your copyright notice in the Description page of Project Settings.

#include "AuthManager.h"
#include "SocketManager.h"
#include "Async/Async.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "Engine/Engine.h"

void UAuthManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Initializing - connecting to server..."));

    // SocketManager 가져오기
    SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
    if (!SocketMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] SocketManager subsystem not found!"));
        bIsConnected = false;
        return;
    }

    // 게임 시작 시 서버에 연결
    bool bConnectResult = SocketMgr->Connect(TEXT("192.168.0.12"), 3000);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Initial server connection: %s"),
        bConnectResult ? TEXT("SUCCESS") : TEXT("FAILED"));

    if (bConnectResult)
    {
        // 데이터 수신 이벤트 바인딩
        SocketMgr->OnDataReceived.AddDynamic(this, &UAuthManager::HandleSocketData);
        bIsConnected = true;
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Ready to handle auth requests"));
    }
    else
    {
        bIsConnected = false;
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to establish initial connection"));
    }
}

void UAuthManager::Deinitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Shutting down..."));

    // 연결 해제
    if (SocketMgr)
    {
        SocketMgr->Disconnect();
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Disconnected from server"));
    }

    bIsConnected = false;
    SocketMgr = nullptr;

    Super::Deinitialize();
}

bool UAuthManager::EnsureConnection()
{
    // SocketManager 확인
    if (!SocketMgr)
    {
        SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
        if (!SocketMgr)
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] SocketManager subsystem not available"));
            return false;
        }
    }

    // 연결 상태 확인
    if (!SocketMgr->IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Connection lost, attempting to reconnect..."));

        // 재연결 시도
        bool bReconnected = SocketMgr->Connect(TEXT("192.168.0.12"), 3000);
        if (bReconnected)
        {
            // 이벤트 재바인딩 (혹시 모르니)
            if (!SocketMgr->OnDataReceived.IsAlreadyBound(this, &UAuthManager::HandleSocketData))
            {
                SocketMgr->OnDataReceived.AddDynamic(this, &UAuthManager::HandleSocketData);
            }
            bIsConnected = true;
            UE_LOG(LogTemp, Log, TEXT("[AuthManager] Reconnection successful"));
        }
        else
        {
            bIsConnected = false;
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] Reconnection failed"));
        }

        return bReconnected;
    }

    // 이미 연결됨
    bIsConnected = true;
    return true;
}

FString UAuthManager::HashPassword(const FString& Password) const
{
    // 간단 스킵: 평문 그대로 해시
    return FMD5::HashAnsiString(*Password);
}

void UAuthManager::Login(const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login attempt for user: %s"), *Username);

    // 연결 상태 확인
    if (!EnsureConnection())
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Cannot login - connection failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }

    // 패킷 조립
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Login);

    // Username 인코딩
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password 인코딩
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // 패킷 전송
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login packet sent: %s (size: %d bytes)"),
        bSent ? TEXT("SUCCESS") : TEXT("FAILED"), Packet.Num());

    if (!bSent)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to send login packet"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
    // 성공적으로 전송했다면 HandleSocketData에서 서버 응답 대기
}

void UAuthManager::RegisterUser(const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration attempt for user: %s"), *Username);

    // 연결 상태 확인
    if (!EnsureConnection())
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Cannot register - connection failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }

    // 패킷 조립
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Register);

    // Username 인코딩
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password 인코딩
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // 패킷 전송
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration packet sent: %s (size: %d bytes)"),
        bSent ? TEXT("SUCCESS") : TEXT("FAILED"), Packet.Num());

    if (!bSent)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to send registration packet"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
    // 성공적으로 전송했다면 HandleSocketData에서 서버 응답 대기
}

void UAuthManager::HandleSocketData(const TArray<uint8>& Data)
{
    if (Data.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Received empty data packet"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[AuthManager] Received %d bytes, first byte: %d"), Data.Num(), Data[0]);

    // 첫 바이트로 응답 타입 확인
    uint8 ResponseType = Data[0];

    if (ResponseType == 0)
    {
        // 로그인/회원가입 실패
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Authentication failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }
    else if (ResponseType == 1 && Data.Num() == 1)
    {
        // 회원가입 성공 (기존 방식: 1바이트만)
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration successful"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(true, EmptyData);
        return;
    }
    else if (ResponseType == 1 && Data.Num() > 5)
    {
        // 로그인 성공 + 캐릭터 데이터
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login successful with character data"));

        // 데이터 크기 읽기 (리틀 엔디안)
        if (Data.Num() < 5)
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] Invalid data format - too short"));
            FCharacterLoginData EmptyData;
            OnAuthResponse.Broadcast(false, EmptyData);
            return;
        }

        uint32 DataSize = 0;
        DataSize |= static_cast<uint32>(Data[1]);
        DataSize |= static_cast<uint32>(Data[2]) << 8;
        DataSize |= static_cast<uint32>(Data[3]) << 16;
        DataSize |= static_cast<uint32>(Data[4]) << 24;

        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Character data size: %d bytes"), DataSize);

        // 데이터 유효성 검사
        if (static_cast<uint32>(Data.Num()) < 5 + DataSize)
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] Invalid data format - expected %d bytes but got %d"),
                5 + DataSize, Data.Num());
            FCharacterLoginData EmptyData;
            OnAuthResponse.Broadcast(false, EmptyData);
            return;
        }

        // UTF-8로 인코딩된 JSON 데이터를 FString으로 변환
        // 방법 1: UTF8_TO_TCHAR 사용
        TArray<uint8> JsonBytes;
        for (uint32 i = 0; i < DataSize; ++i)
        {
            JsonBytes.Add(Data[5 + i]);
        }
        JsonBytes.Add(0); // null terminator

        FString JsonString = UTF8_TO_TCHAR((const char*)JsonBytes.GetData());

        // 디버깅을 위해 받은 JSON 출력
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Received JSON: %s"), *JsonString);

        // HEX 덤프 (디버깅용)
        FString HexDump;
        for (uint32 i = 0; i < FMath::Min(DataSize, 100u); ++i)
        {
            HexDump += FString::Printf(TEXT("%02X "), Data[5 + i]);
        }
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] First 100 bytes (HEX): %s"), *HexDump);

        // JSON 파싱
        FCharacterLoginData CharacterData = ParseCharacterData(JsonString);

        // 파싱 결과 확인
        if (CharacterData.CharacterId == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Warning: CharacterId is 0, parsing may have failed"));
        }

        // UI에 결과 전달
        OnAuthResponse.Broadcast(true, CharacterData);
        return;
    }
    else
    {
        // 알 수 없는 형식
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Unknown response format"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
}

FCharacterLoginData UAuthManager::ParseCharacterData(const FString& JsonString)
{
    FCharacterLoginData Result;

    // 기본값으로 초기화
    Result.CharacterId = 0;
    Result.CharacterName = TEXT("DefaultName");
    Result.PlayerClass = 0;
    Result.Level = 1;
    Result.MaxHealth = 100.0f;
    Result.MaxStamina = 100.0f;
    Result.MaxKnowledge = 100.0f;
    Result.Gold = 0;

    // JSON 파싱
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // 각 필드를 안전하게 읽기
        JsonObject->TryGetNumberField(TEXT("character_id"), Result.CharacterId);
        JsonObject->TryGetStringField(TEXT("character_name"), Result.CharacterName);
        JsonObject->TryGetNumberField(TEXT("player_class"), Result.PlayerClass);
        JsonObject->TryGetNumberField(TEXT("level"), Result.Level);

        // float 값들은 double로 읽어서 변환
        double TempValue;
        if (JsonObject->TryGetNumberField(TEXT("max_health"), TempValue))
            Result.MaxHealth = static_cast<float>(TempValue);
        if (JsonObject->TryGetNumberField(TEXT("max_stamina"), TempValue))
            Result.MaxStamina = static_cast<float>(TempValue);
        if (JsonObject->TryGetNumberField(TEXT("max_knowledge"), TempValue))
            Result.MaxKnowledge = static_cast<float>(TempValue);

        JsonObject->TryGetNumberField(TEXT("gold"), Result.Gold);

        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] JSON Parse Success: ID=%d, Name=%s, Class=%d, Level=%d, Gold=%d"),
            Result.CharacterId, *Result.CharacterName, Result.PlayerClass, Result.Level, Result.Gold);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to parse JSON: %s"), *JsonString);
    }

    return Result;
}