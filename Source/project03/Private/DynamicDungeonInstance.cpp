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
    // 기본값 설정
    bCanInteract = false;
    LobbyGold = 1000;

    // Seamless Travel을 위한 전환 맵 설정
    // 빈 맵이나 로딩 화면 맵을 지정
    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UDynamicDungeonInstance::OnPreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDynamicDungeonInstance::OnPostLoadMap);

}

void UDynamicDungeonInstance::Init()
{
    Super::Init();

    //// 서브시스템 얻기
    //USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    //if (SocketMgr)
    //{
    //    // 로컬 테스트용으로 127.0.0.1:3000 에 연결 시도
    //    bool bConnected = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
    //    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Connect: %s"), bConnected ? TEXT("Success") : TEXT("Fail"));

    //    // Dynamic Multicast Delegate 바인딩 (람다 대신 UFUNCTION)
    //    SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("[SocketTest] SocketManager 서브시스템을 찾을 수 없습니다!"));
    //}

    // SocketManager 연결은 AuthManager에서만 담당하도록 변경
    // 여기서는 연결하지 않고, 필요시에만 참조만 얻어옴
    USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    if (SocketMgr)
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicDungeonInstance] SocketManager found, but not connecting here"));

        // 데이터 수신 이벤트만 바인딩 (AuthManager와 공유)
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
    CurrentCharacterData.PlayerName = TEXT("DefaultName"); // 나중에 입력받게 해도 됨

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

    // 빈 인벤토리 / 장비 초기화
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

    // AuthManager 가져오기
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (!AuthMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] AuthManager not found - cannot save data"));
        return;
    }

    // 유효한 캐릭터 데이터가 없으면 저장 안 함
    if (CurrentCharacterData.PlayerName.IsEmpty() || CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] No valid character data to save"));
        return;
    }

    // 모든 게임 데이터를 JSON으로 수집
    FString JsonData = CollectAllGameData();

    if (JsonData.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] Failed to collect game data"));
        return;
    }

    // 서버에 저장 요청
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
    // JSON 객체 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    // 캐릭터 기본 정보
    JsonObject->SetNumberField(TEXT("character_id"), CurrentCharacterData.CharacterId); 
    JsonObject->SetStringField(TEXT("character_name"), CurrentCharacterData.PlayerName);
    JsonObject->SetNumberField(TEXT("player_class"), static_cast<int32>(CurrentCharacterData.PlayerClass));
    JsonObject->SetNumberField(TEXT("level"), CurrentCharacterData.Level);
    JsonObject->SetNumberField(TEXT("max_health"), CurrentCharacterData.MaxHealth);
    JsonObject->SetNumberField(TEXT("max_stamina"), CurrentCharacterData.MaxStamina);
    JsonObject->SetNumberField(TEXT("max_knowledge"), CurrentCharacterData.MaxKnowledge);
    JsonObject->SetNumberField(TEXT("gold"), LobbyGold);

    // 인벤토리 아이템 배열
    TArray<TSharedPtr<FJsonValue>> InventoryArray;
    for (int32 i = 0; i < SavedInventoryItems.Num(); ++i)
    {
        const FItemData& Item = SavedInventoryItems[i];
        if (Item.ItemClass) // 아이템이 있는 슬롯만
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

    // 장비 아이템 배열
    TArray<TSharedPtr<FJsonValue>> EquipmentArray;
    for (int32 i = 0; i < SavedEquipmentItems.Num(); ++i)
    {
        const FItemData& Item = SavedEquipmentItems[i];
        if (Item.ItemClass) // 아이템이 있는 슬롯만
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

    // 창고 아이템 배열
    TArray<TSharedPtr<FJsonValue>> StorageArray;
    for (int32 i = 0; i < SavedStorageItems.Num(); ++i)
    {
        const FItemData& Item = SavedStorageItems[i];
        if (Item.ItemClass) // 아이템이 있는 슬롯만
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

    // JSON을 문자열로 변환
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

    // 이미 저장 중이면 완료까지 대기
    if (bIsSaving)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save already in progress, waiting..."));
        WaitForSaveCompletion();
        return;
    }

    // 데이터 저장 시도
    SafeShutdown();
}


void UDynamicDungeonInstance::SafeShutdown()
{
    // 유효한 캐릭터 데이터가 없으면 바로 종료
    if (CurrentCharacterData.PlayerName.IsEmpty() || CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] No valid character data - skipping save"));
        bSaveCompleted = true;
        return;
    }

    // AuthManager 확인
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (!AuthMgr)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameInstance] AuthManager not found - cannot save on shutdown"));
        bSaveCompleted = true;
        return;
    }

    // 저장 상태 설정
    bIsSaving = true;
    bSaveCompleted = false;

    // 저장 완료 이벤트 바인딩
    if (!AuthMgr->OnSaveDataResponse.IsAlreadyBound(this, &UDynamicDungeonInstance::OnSaveDataCompleted))
    {
        AuthMgr->OnSaveDataResponse.AddDynamic(this, &UDynamicDungeonInstance::OnSaveDataCompleted);
    }

    // 데이터 저장 요청
    SaveAllDataToServer();

    // 최대 대기 시간 타이머 설정
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

    // 저장 완료까지 동기적으로 대기
    WaitForSaveCompletion();
}

void UDynamicDungeonInstance::OnSaveDataCompleted(bool bSuccess)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Save data completed: %s"),
        bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    bSaveCompleted = true;
    bIsSaving = false;

    // 타이머 정리
    if (GetWorld() && SaveWaitTimer.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(SaveWaitTimer);
    }

    // 이벤트 언바인딩
    UAuthManager* AuthMgr = GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &UDynamicDungeonInstance::OnSaveDataCompleted);
    }
}

// 저장 완료까지 동기적으로 대기하는 함수
void UDynamicDungeonInstance::WaitForSaveCompletion()
{
    const float CheckInterval = 0.1f; // 100ms마다 체크
    float ElapsedTime = 0.0f;

    while (!bSaveCompleted && ElapsedTime < MaxSaveWaitTime)
    {
        // 게임 스레드에서 대기
        FPlatformProcess::Sleep(CheckInterval);
        ElapsedTime += CheckInterval;

        // 게임 루프 처리 (중요: 네트워크 패킷 처리를 위해)
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