// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonInstance.h"
#include "SocketManager.h"
#include "AuthManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Item.h" 

UDynamicDungeonInstance ::UDynamicDungeonInstance
()
{
    // �⺻�� ����
    bCanInteract = false;
    LobbyGold = 1000;

    // Seamless Travel�� ���� ��ȯ �� ����
    // �� ���̳� �ε� ȭ�� ���� ����
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UDynamicDungeonInstance::OnPreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDynamicDungeonInstance::OnPostLoadMap);

}

void UDynamicDungeonInstance::Init()
{
    Super::Init();

    //// ����ý��� ���
    //USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    //if (SocketMgr)
    //{
    //    // ���� �׽�Ʈ������ 127.0.0.1:3000 �� ���� �õ�
    //    bool bConnected = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
    //    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Connect: %s"), bConnected ? TEXT("Success") : TEXT("Fail"));

    //    // Dynamic Multicast Delegate ���ε� (���� ��� UFUNCTION)
    //    SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("[SocketTest] SocketManager ����ý����� ã�� �� �����ϴ�!"));
    //}

    // SocketManager ������ AuthManager������ ����ϵ��� ����
    // ���⼭�� �������� �ʰ�, �ʿ�ÿ��� ������ ����
    USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    if (SocketMgr)
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicDungeonInstance] SocketManager found, but not connecting here"));

        // ������ ���� �̺�Ʈ�� ���ε� (AuthManager�� ����)
        if (!SocketMgr->OnDataReceived.IsAlreadyBound(this, &UDynamicDungeonInstance::OnSocketDataReceived))
        {
            SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[DynamicDungeonInstance] SocketManager subsystem not found!"));
    }

}

void UDynamicDungeonInstance::OnSocketDataReceived(const TArray<uint8>& Data)
{
    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Received %d bytes"), Data.Num());
}

void UDynamicDungeonInstance::SetCanInteract(bool NewState)
{
    bCanInteract = NewState;
}

void UDynamicDungeonInstance::InitializeCharacterData(EPlayerClass SelectedClass)
{
    CurrentCharacterData.PlayerClass = SelectedClass;
    CurrentCharacterData.Level = 1;
    CurrentCharacterData.PlayerName = TEXT("DefaultName"); // ���߿� �Է¹ް� �ص� ��

    switch (SelectedClass)
    {
    case EPlayerClass::Warrior:
        CurrentCharacterData.MaxHealth = 150.f;
        CurrentCharacterData.MaxStamina = 100.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Rogue:
        CurrentCharacterData.MaxHealth = 100.f;
        CurrentCharacterData.MaxStamina = 150.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Mage:
        CurrentCharacterData.MaxHealth = 100.f;
        CurrentCharacterData.MaxStamina = 10.f;
        CurrentCharacterData.MaxKnowledge = 200.f;
        break;
    }

    // �� �κ��丮 / ��� �ʱ�ȭ
    CurrentCharacterData.InventoryItems.Empty();
    CurrentCharacterData.EquippedItems.Empty();
}


void UDynamicDungeonInstance::OnPreLoadMap(const FString& MapName)
{
    UE_LOG(LogTemp, Log, TEXT("PreLoadMap: %s"), *MapName);
}

void UDynamicDungeonInstance::OnPostLoadMap(UWorld* LoadedWorld)
{

    if (!LoadedWorld)
    {
        UE_LOG(LogTemp, Warning, TEXT("[OnPostLoadMap] LoadedWorld is null, skipping"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("PostLoadMap: %s"), *LoadedWorld->GetMapName());
}

void UDynamicDungeonInstance::SaveAllDataToServer()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Starting save process..."));

    // AuthManager ��������
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (!AuthMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] AuthManager not found - cannot save data"));
        return;
    }

    // ��ȿ�� ĳ���� �����Ͱ� ������ ���� �� ��
    if (CurrentCharacterData.PlayerName.IsEmpty() || CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] No valid character data to save"));
        return;
    }

    // ��� ���� �����͸� JSON���� ����
    FString JsonData = CollectAllGameData();

    if (JsonData.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] Failed to collect game data"));
        return;
    }

    // ������ ���� ��û
    if (CurrentCharacterData.CharacterId == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] Invalid CharacterId: 0"));
        return;
    }
    
    //int32 CharacterId = 1; 
    AuthMgr->SaveGameData(CurrentCharacterData.CharacterId, JsonData);

    UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save request sent to server"));
}

FString UDynamicDungeonInstance::CollectAllGameData()
{
    // JSON ��ü ����
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    // ĳ���� �⺻ ����
    JsonObject->SetNumberField(TEXT("character_id"), CurrentCharacterData.CharacterId); 
    JsonObject->SetStringField(TEXT("character_name"), CurrentCharacterData.PlayerName);
    JsonObject->SetNumberField(TEXT("player_class"), static_cast<int32>(CurrentCharacterData.PlayerClass));
    JsonObject->SetNumberField(TEXT("level"), CurrentCharacterData.Level);
    JsonObject->SetNumberField(TEXT("max_health"), CurrentCharacterData.MaxHealth);
    JsonObject->SetNumberField(TEXT("max_stamina"), CurrentCharacterData.MaxStamina);
    JsonObject->SetNumberField(TEXT("max_knowledge"), CurrentCharacterData.MaxKnowledge);
    JsonObject->SetNumberField(TEXT("gold"), LobbyGold);

    // �κ��丮 ������ �迭
    TArray<TSharedPtr<FJsonValue>> InventoryArray;
    for (int32 i = 0; i < SavedInventoryItems.Num(); ++i)
    {
        const FItemData& Item = SavedInventoryItems[i];
        if (Item.ItemClass) // �������� �ִ� ���Ը�
        {
            TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject);
            ItemObj->SetNumberField(TEXT("slot_index"), i);
            ItemObj->SetStringField(TEXT("item_class"), Item.ItemClass->GetName());
            ItemObj->SetStringField(TEXT("item_name"), Item.ItemName);
            ItemObj->SetNumberField(TEXT("item_type"), static_cast<int32>(Item.ItemType));
            ItemObj->SetNumberField(TEXT("grade"), Item.Grade);
            ItemObj->SetNumberField(TEXT("count"), Item.Count);
            ItemObj->SetNumberField(TEXT("potion_effect"), static_cast<int32>(Item.PotionEffect));

            InventoryArray.Add(MakeShareable(new FJsonValueObject(ItemObj)));
        }
    }
    JsonObject->SetArrayField(TEXT("inventory_items"), InventoryArray);

    // ��� ������ �迭
    TArray<TSharedPtr<FJsonValue>> EquipmentArray;
    for (int32 i = 0; i < SavedEquipmentItems.Num(); ++i)
    {
        const FItemData& Item = SavedEquipmentItems[i];
        if (Item.ItemClass) // �������� �ִ� ���Ը�
        {
            TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject);
            ItemObj->SetNumberField(TEXT("slot_index"), i);
            ItemObj->SetStringField(TEXT("item_class"), Item.ItemClass->GetName());
            ItemObj->SetStringField(TEXT("item_name"), Item.ItemName);
            ItemObj->SetNumberField(TEXT("item_type"), static_cast<int32>(Item.ItemType));
            ItemObj->SetNumberField(TEXT("grade"), Item.Grade);
            ItemObj->SetNumberField(TEXT("count"), Item.Count);
            ItemObj->SetNumberField(TEXT("potion_effect"), static_cast<int32>(Item.PotionEffect));

            EquipmentArray.Add(MakeShareable(new FJsonValueObject(ItemObj)));
        }
    }
    JsonObject->SetArrayField(TEXT("equipment_items"), EquipmentArray);

    // â�� ������ �迭
    TArray<TSharedPtr<FJsonValue>> StorageArray;
    for (int32 i = 0; i < SavedStorageItems.Num(); ++i)
    {
        const FItemData& Item = SavedStorageItems[i];
        if (Item.ItemClass) // �������� �ִ� ���Ը�
        {
            TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject);
            ItemObj->SetNumberField(TEXT("slot_index"), i);
            ItemObj->SetStringField(TEXT("item_class"), Item.ItemClass->GetName());
            ItemObj->SetStringField(TEXT("item_name"), Item.ItemName);
            ItemObj->SetNumberField(TEXT("item_type"), static_cast<int32>(Item.ItemType));
            ItemObj->SetNumberField(TEXT("grade"), Item.Grade);
            ItemObj->SetNumberField(TEXT("count"), Item.Count);
            ItemObj->SetNumberField(TEXT("potion_effect"), static_cast<int32>(Item.PotionEffect));

            StorageArray.Add(MakeShareable(new FJsonValueObject(ItemObj)));
        }
    }
    JsonObject->SetArrayField(TEXT("storage_items"), StorageArray);

    // JSON�� ���ڿ��� ��ȯ
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Collected JSON data: %s"), *OutputString);

    return OutputString;
}

void UDynamicDungeonInstance::SaveDataAndShutdown()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Game is shutting down - attempting to save data..."));

    if (bShutdownRequested) { return; }  
    bShutdownRequested = true;

    // �̹� ���� ���̸� �Ϸ���� ���
    if (bIsSaving)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save already in progress, waiting..."));
        WaitForSaveCompletion();
        return;
    }

    // ������ ���� �õ�
    SafeShutdown();
}


void UDynamicDungeonInstance::SafeShutdown()
{
    // ��ȿ�� ĳ���� �����Ͱ� ������ �ٷ� ����
    if (CurrentCharacterData.PlayerName.IsEmpty() || CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] No valid character data - skipping save"));
        bSaveCompleted = true;
        return;
    }

    // AuthManager Ȯ��
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (!AuthMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] AuthManager not found - cannot save on shutdown"));
        bSaveCompleted = true;
        return;
    }

    // ���� ���� ����
    bIsSaving = true;
    bSaveCompleted = false;

    // ���� �Ϸ� �̺�Ʈ ���ε�
    if (!AuthMgr->OnSaveDataResponse.IsAlreadyBound(this, &UDynamicDungeonInstance::OnSaveDataCompleted))
    {
        AuthMgr->OnSaveDataResponse.AddDynamic(this, &UDynamicDungeonInstance::OnSaveDataCompleted);
    }

    // ������ ���� ��û
    SaveAllDataToServer();

    // �ִ� ��� �ð� Ÿ�̸� ����
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            SaveWaitTimer,
            [this]()
            {
                UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save timeout reached - forcing shutdown"));
                bSaveCompleted = true;
                bIsSaving = false;
            },
            MaxSaveWaitTime,
            false
        );
    }

    // ���� �Ϸ���� ���������� ���
    WaitForSaveCompletion();
}

void UDynamicDungeonInstance::OnSaveDataCompleted(bool bSuccess)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save data completed: %s"),
        bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    bSaveCompleted = true;
    bIsSaving = false;

    // Ÿ�̸� ����
    if (GetWorld() && SaveWaitTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(SaveWaitTimer);
    }

    // �̺�Ʈ ����ε�
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &UDynamicDungeonInstance::OnSaveDataCompleted);
    }
}

// ���� �Ϸ���� ���������� ����ϴ� �Լ�
void UDynamicDungeonInstance::WaitForSaveCompletion()
{
    const float CheckInterval = 0.1f; // 100ms���� üũ
    float ElapsedTime = 0.0f;

    while (!bSaveCompleted && ElapsedTime < MaxSaveWaitTime)
    {
        // ���� �����忡�� ���
        FPlatformProcess::Sleep(CheckInterval);
        ElapsedTime += CheckInterval;

        // ���� ���� ó�� (�߿�: ��Ʈ��ũ ��Ŷ ó���� ����)
        if (GetWorld())
        {
            GetWorld()->Tick(LEVELTICK_All, CheckInterval);
        }
    }

    if (bSaveCompleted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save completed after %.2f seconds"), ElapsedTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save timed out after %.2f seconds"), ElapsedTime);
    }
}