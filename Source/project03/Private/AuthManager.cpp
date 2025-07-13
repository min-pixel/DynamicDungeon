// Fill out your copyright notice in the Description page of Project Settings.

#include "AuthManager.h"
#include "SocketManager.h"
#include "Async/Async.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "DynamicDungeonInstance.h"  
#include "PlayerCharacterData.h"
#include "Engine/Engine.h"
#include "Item.h"
#include "ItemDataD.h"
#include "Armor.h"
#include "ScrollItem.h"
#include "Weapon.h"
#include "Potion.h"

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

    // �̹� ����Ǿ� ������ �翬������ ����
    if (SocketMgr->IsConnected())
    {
        UE_LOG(LogTemp, Log, TEXT("[AuthManager] Already connected - skipping reconnection"));
        bIsConnected = true;
        return true;
    }

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

    // SaveDataResponse ó�� �߰�
    if (ResponseType == 2) // SaveDataResponse
    {
        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Save data response received"));
        bool bSuccess = (Data.Num() > 0 && Data[0] > 0);
        OnSaveDataResponse.Broadcast(bSuccess);
        return;
    }

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

        if (UDynamicDungeonInstance* GI = Cast<UDynamicDungeonInstance>(GetGameInstance()))
        {
            GI->CurrentCharacterData.CharacterId = CharacterData.CharacterId;
            GI->CurrentCharacterData.PlayerName = CharacterData.CharacterName;
            GI->CurrentCharacterData.PlayerClass = static_cast<EPlayerClass>(CharacterData.PlayerClass);
            GI->CurrentCharacterData.Level = CharacterData.Level;
            GI->CurrentCharacterData.MaxHealth = CharacterData.MaxHealth;
            GI->CurrentCharacterData.MaxStamina = CharacterData.MaxStamina;
            GI->CurrentCharacterData.MaxKnowledge = CharacterData.MaxKnowledge;
            GI->CurrentCharacterData.Gold = CharacterData.Gold;
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

        if (UDynamicDungeonInstance* GI = Cast<UDynamicDungeonInstance>(GetGameInstance()))
        {
            const TArray<TSharedPtr<FJsonValue>>* InventoryArray;
            if (JsonObject->TryGetArrayField(TEXT("inventory_items"), InventoryArray))
            {
                GI->SavedInventoryItems.Empty();

                for (const auto& ItemValue : *InventoryArray)
                {
                    const TSharedPtr<FJsonObject>* ItemObject;
                    if (ItemValue->TryGetObject(ItemObject))
                    {
                        FItemData NewItem;
                        int32 SlotIndex;
                        (*ItemObject)->TryGetNumberField(TEXT("slot_index"), SlotIndex);

                        FString ItemClassName;
                        (*ItemObject)->TryGetStringField(TEXT("item_class"), ItemClassName);

                        // 3�� ���: ��� UClass ��ü�� ��ȸ�Ͽ� �̸����� ã��
                        UClass* ItemClass = nullptr;
                        for (TObjectIterator<UClass> It; It; ++It)
                        {
                            UClass* Class = *It;
                            if (Class && Class->IsChildOf(AItem::StaticClass()))
                            {
                                FString ClassName = Class->GetName();

                                // ��Ȯ�� Ŭ���� �̸� ��Ī
                                if (ClassName.Equals(ItemClassName))
                                {
                                    ItemClass = Class;
                                    UE_LOG(LogTemp, Log, TEXT("Found exact match for class: %s"), *ItemClassName);
                                    break;
                                }

                                // "A" ���λ簡 �ִ� ��쵵 üũ (��: "RobeTop" -> "ARobeTop")
                                if (ClassName.Equals(FString("A") + ItemClassName))
                                {
                                    ItemClass = Class;
                                    UE_LOG(LogTemp, Log, TEXT("Found class with A prefix: A%s"), *ItemClassName);
                                    break;
                                }
                            }
                        }

                        if (!ItemClass)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Failed to find item class: %s"), *ItemClassName);
                        }

                        NewItem.ItemClass = ItemClass;

                        if (!NewItem.ItemIcon && ItemClass)
                        {
                            // �ش� Ŭ������ DefaultObject ���� �������� ����
                            if (const AItem* DefObj = ItemClass->GetDefaultObject<AItem>())
                            {
                                NewItem.ItemIcon = DefObj->ItemIcon;
                               
                                
                            }

                        }

                        // ������ �ʵ� �Ľ�
                        (*ItemObject)->TryGetStringField(TEXT("item_name"), NewItem.ItemName);

                        int32 ItemType;
                        (*ItemObject)->TryGetNumberField(TEXT("item_type"), ItemType);
                        NewItem.ItemType = static_cast<EItemType>(ItemType);

                        (*ItemObject)->TryGetNumberField(TEXT("grade"), NewItem.Grade);
                        (*ItemObject)->TryGetNumberField(TEXT("count"), NewItem.Count);

                        if (NewItem.Price == 0 && ItemClass)
                        {
                            const AItem* DefObj = ItemClass->GetDefaultObject<AItem>();
                            if (DefObj)
                            {
                                const int32 BasePrice = DefObj->Price;          // C-��� ����
                                static const float GradeMul[3] = { 1.0f, 1.5f, 2.0f };   // C, B, A
                                int32 G = FMath::Clamp<int32>(NewItem.Grade, 0, 2);
                                NewItem.Price = FMath::RoundToInt(BasePrice * GradeMul[G]);
                            }
                        }

                        int32 PotionEffect;
                        (*ItemObject)->TryGetNumberField(TEXT("potion_effect"), PotionEffect);
                        NewItem.PotionEffect = static_cast<EPotionEffectType>(PotionEffect);

                        // �迭 ũ�� ����
                        if (GI->SavedInventoryItems.Num() <= SlotIndex)
                        {
                            GI->SavedInventoryItems.SetNum(SlotIndex + 1);
                        }

                        GI->SavedInventoryItems[SlotIndex] = NewItem;

                        UE_LOG(LogTemp, Warning, TEXT("Loaded item: %s at slot %d (Class: %s)"),
                            *NewItem.ItemName, SlotIndex, ItemClass ? TEXT("Found") : TEXT("Not Found"));
                    }
                }

                UE_LOG(LogTemp, Warning, TEXT("Total inventory items loaded: %d"),
                    GI->SavedInventoryItems.Num());
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("[AuthManager] JSON Parse Success: ID=%d, Name=%s, Class=%d, Level=%d, Gold=%d"),
            Result.CharacterId, *Result.CharacterName, Result.PlayerClass, Result.Level, Result.Gold);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to parse JSON: %s"), *JsonString);
    }

    return Result;
}

//������ ���� �Լ�
void UAuthManager::SaveGameData(int32 CharacterId, const FString& JsonData)
{
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Save data request for character: %d"), CharacterId);

    // ���� ���� Ȯ��
    if (!EnsureConnection())
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Cannot save data - connection failed"));
        OnSaveDataResponse.Broadcast(false);
        return;
    }

    // JSON �����͸� UTF-8�� ��ȯ
    FTCHARToUTF8 U8Json(*JsonData);
    uint32 JsonLen = U8Json.Length();

    // ��Ŷ ����: [RequestType][DataSize][JsonData]
    TArray<uint8> Packet;
    Packet.Add((uint8)EAuthRequestType::SaveData);

    // ������ ũ�� (4����Ʈ, ��Ʋ �����)
    Packet.Add((JsonLen >> 0) & 0xFF);
    Packet.Add((JsonLen >> 8) & 0xFF);
    Packet.Add((JsonLen >> 16) & 0xFF);
    Packet.Add((JsonLen >> 24) & 0xFF);

    // JSON ������
    Packet.Append((uint8*)U8Json.Get(), JsonLen);

    // ��Ŷ ����
    bool bSent = SocketMgr->Send(Packet);
    UE_LOG(LogTemp, Warning, TEXT("[AuthManager] Save data packet sent: %s (size: %d bytes)"),
        bSent ? TEXT("SUCCESS") : TEXT("FAILED"), Packet.Num());

    if (!bSent)
    {
        UE_LOG(LogTemp, Error, TEXT("[AuthManager] Failed to send save data packet"));
        OnSaveDataResponse.Broadcast(false);
    }
    // ���������� �����ߴٸ� HandleSocketData���� ���� ���� ���
}