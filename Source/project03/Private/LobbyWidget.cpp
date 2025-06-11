// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/Button.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"
#include "ShopWidget.h"
#include "GameFramework/HUD.h"


void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    if (StartGameButton)
    {
        StartGameButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);
    }

    if (GoToShopButton)
    {
        GoToShopButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnGoToShopClicked);
    }

    if (LeftArrowButton)
    {
        LeftArrowButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLeftArrowClicked);
    }

    if (RightArrowButton)
    {
        RightArrowButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnRightArrowClicked);
    }

    if (CloseShopButton)
    {
        CloseShopButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnCloseShopButtonClicked);
    }
    
}

void ULobbyWidget::InitializeLobby(AMyDCharacter* Player)
{
    PlayerCharacter = Player;

    // (1) 플레이어 인벤토리 컴포넌트 연결 (없으면 더미 생성)
    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
    {
        InventoryComponentRef = PlayerCharacter->InventoryComponent;
    }
    else
    {
        // 더미 생성
        InventoryComponentRef = NewObject<UInventoryComponent>(this);
        InventoryComponentRef->RegisterComponent();
        InventoryComponentRef->Capacity = 32;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // GameInstance에서 저장된 인벤토리를 복원한다
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && InventoryComponentRef)
    {
        const int32 SavedCount = GameInstance->SavedInventoryItems.Num();
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        for (int32 i = 0; i < SavedCount; ++i)
        {
            if (i < InventoryComponentRef->InventoryItemsStruct.Num())
            {
                InventoryComponentRef->InventoryItemsStruct[i] = GameInstance->SavedInventoryItems[i];
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Restored Inventory from GameInstance (%d items)"), SavedCount);
    }

    if (GameInstance && EquipmentWidgetInstance)
    {
        EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
    }

    // (2) 창고 인벤토리 컴포넌트 생성 (항상 새로 만든다)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 50; // 창고는 더 크게
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

        if (GameInstance && GameInstance->SavedStorageItems.Num() > 0)
        {
            // 저장된 데이터가 있을 때만 복원
            StorageComponentRef->InventoryItemsStruct = GameInstance->SavedStorageItems;
            UE_LOG(LogTemp, Warning, TEXT("Restored Storage Inventory from SavedStorageItems"));
        }
        else
        {
            // 저장된 게 없으면 (처음 시작이면)
            UE_LOG(LogTemp, Warning, TEXT("No SavedStorageItems found, starting empty Storage"));
        }

        if (StorageWidgetInstance)
        {
            StorageWidgetInstance->InventoryRef = StorageComponentRef;
            StorageWidgetInstance->bIsChestInventory = true;
            StorageWidgetInstance->RefreshInventoryStruct();
        }

        UE_LOG(LogTemp, Warning, TEXT("Created Storage InventoryComponent"));
    }

    // (3) 인벤토리 위젯 설정
    if (InventoryWidgetInstance)
    {
        InventoryWidgetInstance->InventoryRef = InventoryComponentRef;
        InventoryWidgetInstance->bIsChestInventory = false;
        InventoryWidgetInstance->AddToViewport(1);
        InventoryWidgetInstance->RefreshInventoryStruct();
    }

    // (4) 창고 위젯 설정
    if (StorageWidgetInstance)
    {
        StorageWidgetInstance->InventoryRef = StorageComponentRef;
        StorageWidgetInstance->bIsChestInventory = true;
        StorageWidgetInstance->AddToViewport(1);
        StorageWidgetInstance->RefreshInventoryStruct();
    }

    // (5) 장비창 위젯 설정
    if (EquipmentWidgetInstance)
    {
        EquipmentWidgetInstance->AddToViewport(1);
        EquipmentWidgetInstance->RefreshEquipmentSlots();
    }

    UE_LOG(LogTemp, Warning, TEXT("Lobby Initialized (Dummy Safe Mode)"));
}
//void ULobbyWidget::InitializeLobby(AMyDCharacter* Player)
//{
//    PlayerCharacter = Player;
//    // 초기 설정 필요 시 여기에 작성
//
//     // 1. 인벤토리 컴포넌트 연결 (플레이어 없으면 더미 생성)
//    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
//    {
//        StorageInventoryComponent = PlayerCharacter->InventoryComponent;
//    }
//    else
//    {
//        // 플레이어가 없으면 가짜 인벤토리 컴포넌트 생성
//        StorageInventoryComponent = NewObject<UInventoryComponent>(this);
//        StorageInventoryComponent->RegisterComponent(); // 컴포넌트로 등록
//        StorageInventoryComponent->Capacity = 20; // 임의 용량 설정
//        StorageInventoryComponent->InventoryItemsStruct.SetNum(StorageInventoryComponent->Capacity);
//    }
//
//
//    if (InventoryWidgetClass)
//    {
//        InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
//        if (InventoryWidgetInstance)
//        {
//            InventoryWidgetInstance->InventoryRef = PlayerCharacter->GetInventoryComponent(); // 플레이어 인벤토리 연결
//            InventoryWidgetInstance->RefreshInventoryStruct();
//            InventoryWidgetInstance->AddToViewport(1);
//            InventoryWidgetInstance->SetPositionInViewport(FVector2D(100, 300), false); // 원하는 위치로 조정
//        }
//    }
//
//    if (StorageWidgetClass)
//    {
//        StorageInventoryComponent = NewObject<UInventoryComponent>(this); // 스토리지용 임시 인벤토리 컴포넌트 생성
//        if (StorageInventoryComponent)
//        {
//            StorageInventoryComponent->Capacity = 30; // 예시: 창고는 30칸
//            StorageInventoryComponent->RegisterComponent();
//
//            StorageWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), StorageWidgetClass);
//            if (StorageWidgetInstance)
//            {
//                StorageWidgetInstance->InventoryRef = StorageInventoryComponent;
//                StorageWidgetInstance->bIsChestInventory = true;
//                StorageWidgetInstance->RefreshInventoryStruct();
//                StorageWidgetInstance->AddToViewport(1);
//                StorageWidgetInstance->SetPositionInViewport(FVector2D(800, 300), false); // 원하는 위치로 조정
//            }
//        }
//    }
//
//    if (EquipmentWidgetClass)
//    {
//        EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);
//        if (EquipmentWidgetInstance)
//        {
//            if (EquipmentWidgetClass && PlayerCharacter && PlayerCharacter->GetEquipmentWidget())
//            {
//                EquipmentWidgetInstance = PlayerCharacter->GetEquipmentWidget();
//            } // 플레이어 장비 연결
//            EquipmentWidgetInstance->RefreshEquipmentSlots();
//            EquipmentWidgetInstance->AddToViewport(1);
//            EquipmentWidgetInstance->SetPositionInViewport(FVector2D(500, 300), false); // 원하는 위치로 조정
//        }
//    }
//
//}

void ULobbyWidget::OnStartGameClicked()
{
    //UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));

    // 게임 준비 처리
    // 예: 플레이어 상태 변경, 서버에 알림 등

    if (EquipmentWidgetInstance)
    {
        UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
        if (GameInstance)
        {
            GameInstance->SavedInventoryItems = InventoryComponentRef->InventoryItemsStruct;
            GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->EquipmentSlots;
            GameInstance->SavedStorageItems = StorageComponentRef->InventoryItemsStruct;
            GameInstance->InitializeCharacterData(AvailableClasses[CurrentClassIndex]);
            UE_LOG(LogTemp, Warning, TEXT("Saved Equipment Slots to GameInstance"));
        }
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("Opening level now..."));
        UGameplayStatics::OpenLevel(PC, FName("/Game/DynamicDugeon"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("PlayerController is null, cannot open level."));
    }
    
}

void ULobbyWidget::OnGoToShopClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Go to Shop button clicked"));

   

    // 기존 창 숨김
    if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
    if (EquipmentWidgetInstance) EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

    if (!ShopWidgetInstance && ShopWidgetClass)
    {
        ShopWidgetInstance = CreateWidget<UShopWidget>(GetWorld(), ShopWidgetClass);

         
        if (ShopWidgetInstance)
        {
            // 상점 아이템 세팅
            TArray<FItemData> Items;

            for (TObjectIterator<UClass> It; It; ++It)
            {
                if (It->IsChildOf(AItem::StaticClass()) &&
                    !It->HasAnyClassFlags(CLASS_Abstract) &&
                    *It != AItem::StaticClass())
                {
                    AItem* DefaultItem = (*It)->GetDefaultObject<AItem>();
                    if (DefaultItem)
                    {
                        FItemData Data = DefaultItem->ToItemData();
                        Items.Add(Data);
                    }
                }
            }

            ShopWidgetInstance->ShopItemList = Items;
            ShopWidgetInstance->TargetInventory = InventoryComponentRef;
            ShopWidgetInstance->SlotWidgetClass = InventoryWidgetInstance->SlotWidgetClass;

            ShopWidgetInstance->PopulateShopItems();
            ShopWidgetInstance->AddToViewport(2);


        }
    }
}

void ULobbyWidget::OnLeftArrowClicked()
{
    CurrentClassIndex = (CurrentClassIndex - 1 + AvailableClasses.Num()) % AvailableClasses.Num();
    UpdateClassDisplay();
}

void ULobbyWidget::OnRightArrowClicked()
{
    CurrentClassIndex = (CurrentClassIndex + 1) % AvailableClasses.Num();
    UpdateClassDisplay();
}

void ULobbyWidget::UpdateClassDisplay()
{
    FString ClassName;
    switch (AvailableClasses[CurrentClassIndex])
    {
    case EPlayerClass::Warrior: ClassName = TEXT("W"); break;
    case EPlayerClass::Rogue:   ClassName = TEXT("R"); break;
    case EPlayerClass::Mage:    ClassName = TEXT("M"); break;
    }

    ClassText->SetText(FText::FromString(ClassName));
}

void ULobbyWidget::OnCloseShopButtonClicked()
{
    if (ShopWidgetInstance)
    {
        ShopWidgetInstance->RemoveFromParent();
        ShopWidgetInstance = nullptr;
    }

    if (InventoryWidgetInstance)
        InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);

    if (EquipmentWidgetInstance)
        EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Warning, TEXT("Shop closed via LobbyWidget"));
}