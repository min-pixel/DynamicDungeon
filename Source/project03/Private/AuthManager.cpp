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

    // SocketManager ��������
    SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
    if (!SocketMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] SocketManager subsystem not found!"));
        bIsConnected = false;
        return;
    }

    // ���� ���� �� ������ ����
    bool bConnectResult = SocketMgr->Connect(TEXT("192.168.0.12"), 3000);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Initial server connection: %s"),
        bConnectResult ? TEXT("SUCCESS") : TEXT("FAILED"));

    if (bConnectResult)
    {
        // ������ ���� �̺�Ʈ ���ε�
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

    // ���� ����
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
    // SocketManager Ȯ��
    if (!SocketMgr)
    {
        SocketMgr = GetGameInstance()->GetSubsystem<USocketManager>();
        if (!SocketMgr)
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] SocketManager subsystem not available"));
            return false;
        }
    }

    // ���� ���� Ȯ��
    if (!SocketMgr->IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Connection lost, attempting to reconnect..."));

        // �翬�� �õ�
        bool bReconnected = SocketMgr->Connect(TEXT("192.168.0.12"), 3000);
        if (bReconnected)
        {
            // �̺�Ʈ ����ε� (Ȥ�� �𸣴�)
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

    // �̹� �����
    bIsConnected = true;
    return true;
}

FString UAuthManager::HashPassword(const FString& Password) const
{
    // ���� ��ŵ: �� �״�� �ؽ�
    return FMD5::HashAnsiString(*Password);
}

void UAuthManager::Login(const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login attempt for user: %s"), *Username);

    // ���� ���� Ȯ��
    if (!EnsureConnection())
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Cannot login - connection failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }

    // ��Ŷ ����
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Login);

    // Username ���ڵ�
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password ���ڵ�
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // ��Ŷ ����
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login packet sent: %s (size: %d bytes)"),
        bSent ? TEXT("SUCCESS") : TEXT("FAILED"), Packet.Num());

    if (!bSent)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to send login packet"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
    // ���������� �����ߴٸ� HandleSocketData���� ���� ���� ���
}

void UAuthManager::RegisterUser(const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration attempt for user: %s"), *Username);

    // ���� ���� Ȯ��
    if (!EnsureConnection())
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Cannot register - connection failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }

    // ��Ŷ ����
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::Register);

    // Username ���ڵ�
    FTCHARToUTF8 U8Name(*Username);
    uint16 NameLen = U8Name.Length();
    Packet.Append(reinterpret_cast<uint8*>(&NameLen), sizeof(NameLen));
    Packet.Append((uint8*)U8Name.Get(), NameLen);

    // Password ���ڵ�
    FTCHARToUTF8 U8Pass(*Password);
    uint16 PassLen = U8Pass.Length();
    Packet.Append(reinterpret_cast<uint8*>(&PassLen), sizeof(PassLen));
    Packet.Append((uint8*)U8Pass.Get(), PassLen);

    // ��Ŷ ����
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration packet sent: %s (size: %d bytes)"),
        bSent ? TEXT("SUCCESS") : TEXT("FAILED"), Packet.Num());

    if (!bSent)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to send registration packet"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
    // ���������� �����ߴٸ� HandleSocketData���� ���� ���� ���
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

    // ù ����Ʈ�� ���� Ÿ�� Ȯ��
    uint8 ResponseType = Data[0];

    if (ResponseType == 0)
    {
        // �α���/ȸ������ ����
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Authentication failed"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
        return;
    }
    else if (ResponseType == 1 && Data.Num() == 1)
    {
        // ȸ������ ���� (���� ���: 1����Ʈ��)
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Registration successful"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(true, EmptyData);
        return;
    }
    else if (ResponseType == 1 && Data.Num() > 5)
    {
        // �α��� ���� + ĳ���� ������
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Login successful with character data"));

        // ������ ũ�� �б� (��Ʋ �����)
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

        // ������ ��ȿ�� �˻�
        if (static_cast<uint32>(Data.Num()) < 5 + DataSize)
        {
            UE_LOG(LogTemp, Error, TEXT("[AuthManager] Invalid data format - expected %d bytes but got %d"),
                5 + DataSize, Data.Num());
            FCharacterLoginData EmptyData;
            OnAuthResponse.Broadcast(false, EmptyData);
            return;
        }

        // UTF-8�� ���ڵ��� JSON �����͸� FString���� ��ȯ
        // ��� 1: UTF8_TO_TCHAR ���
        TArray<uint8> JsonBytes;
        for (uint32 i = 0; i < DataSize; ++i)
        {
            JsonBytes.Add(Data[5 + i]);
        }
        JsonBytes.Add(0); // null terminator

        FString JsonString = UTF8_TO_TCHAR((const char*)JsonBytes.GetData());

        // ������� ���� ���� JSON ���
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Received JSON: %s"), *JsonString);

        // HEX ���� (������)
        FString HexDump;
        for (uint32 i = 0; i < FMath::Min(DataSize, 100u); ++i)
        {
            HexDump += FString::Printf(TEXT("%02X "), Data[5 + i]);
        }
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] First 100 bytes (HEX): %s"), *HexDump);

        // JSON �Ľ�
        FCharacterLoginData CharacterData = ParseCharacterData(JsonString);

        // �Ľ� ��� Ȯ��
        if (CharacterData.CharacterId == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Warning: CharacterId is 0, parsing may have failed"));
        }

        // UI�� ��� ����
        OnAuthResponse.Broadcast(true, CharacterData);
        return;
    }
    else
    {
        // �� �� ���� ����
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Unknown response format"));
        FCharacterLoginData EmptyData;
        OnAuthResponse.Broadcast(false, EmptyData);
    }
}

FCharacterLoginData UAuthManager::ParseCharacterData(const FString& JsonString)
{
    FCharacterLoginData Result;

    // �⺻������ �ʱ�ȭ
    Result.CharacterId = 0;
    Result.CharacterName = TEXT("DefaultName");
    Result.PlayerClass = 0;
    Result.Level = 1;
    Result.MaxHealth = 100.0f;
    Result.MaxStamina = 100.0f;
    Result.MaxKnowledge = 100.0f;
    Result.Gold = 0;

    // JSON �Ľ�
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // �� �ʵ带 �����ϰ� �б�
        JsonObject->TryGetNumberField(TEXT("character_id"), Result.CharacterId);
        JsonObject->TryGetStringField(TEXT("character_name"), Result.CharacterName);
        JsonObject->TryGetNumberField(TEXT("player_class"), Result.PlayerClass);
        JsonObject->TryGetNumberField(TEXT("level"), Result.Level);

        // float ������ double�� �о ��ȯ
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