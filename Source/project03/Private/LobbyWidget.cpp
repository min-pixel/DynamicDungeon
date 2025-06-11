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

    // (1) �÷��̾� �κ��丮 ������Ʈ ���� (������ ���� ����)
    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
    {
        InventoryComponentRef = PlayerCharacter->InventoryComponent;
    }
    else
    {
        // ���� ����
        InventoryComponentRef = NewObject<UInventoryComponent>(this);
        InventoryComponentRef->RegisterComponent();
        InventoryComponentRef->Capacity = 32;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // GameInstance���� ����� �κ��丮�� �����Ѵ�
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

    // (2) â�� �κ��丮 ������Ʈ ���� (�׻� ���� �����)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 50; // â��� �� ũ��
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

        if (GameInstance && GameInstance->SavedStorageItems.Num() > 0)
        {
            // ����� �����Ͱ� ���� ���� ����
            StorageComponentRef->InventoryItemsStruct = GameInstance->SavedStorageItems;
            UE_LOG(LogTemp, Warning, TEXT("Restored Storage Inventory from SavedStorageItems"));
        }
        else
        {
            // ����� �� ������ (ó�� �����̸�)
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

    // (3) �κ��丮 ���� ����
    if (InventoryWidgetInstance)
    {
        InventoryWidgetInstance->InventoryRef = InventoryComponentRef;
        InventoryWidgetInstance->bIsChestInventory = false;
        InventoryWidgetInstance->AddToViewport(1);
        InventoryWidgetInstance->RefreshInventoryStruct();
    }

    // (4) â�� ���� ����
    if (StorageWidgetInstance)
    {
        StorageWidgetInstance->InventoryRef = StorageComponentRef;
        StorageWidgetInstance->bIsChestInventory = true;
        StorageWidgetInstance->AddToViewport(1);
        StorageWidgetInstance->RefreshInventoryStruct();
    }

    // (5) ���â ���� ����
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
//    // �ʱ� ���� �ʿ� �� ���⿡ �ۼ�
//
//     // 1. �κ��丮 ������Ʈ ���� (�÷��̾� ������ ���� ����)
//    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
//    {
//        StorageInventoryComponent = PlayerCharacter->InventoryComponent;
//    }
//    else
//    {
//        // �÷��̾ ������ ��¥ �κ��丮 ������Ʈ ����
//        StorageInventoryComponent = NewObject<UInventoryComponent>(this);
//        StorageInventoryComponent->RegisterComponent(); // ������Ʈ�� ���
//        StorageInventoryComponent->Capacity = 20; // ���� �뷮 ����
//        StorageInventoryComponent->InventoryItemsStruct.SetNum(StorageInventoryComponent->Capacity);
//    }
//
//
//    if (InventoryWidgetClass)
//    {
//        InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
//        if (InventoryWidgetInstance)
//        {
//            InventoryWidgetInstance->InventoryRef = PlayerCharacter->GetInventoryComponent(); // �÷��̾� �κ��丮 ����
//            InventoryWidgetInstance->RefreshInventoryStruct();
//            InventoryWidgetInstance->AddToViewport(1);
//            InventoryWidgetInstance->SetPositionInViewport(FVector2D(100, 300), false); // ���ϴ� ��ġ�� ����
//        }
//    }
//
//    if (StorageWidgetClass)
//    {
//        StorageInventoryComponent = NewObject<UInventoryComponent>(this); // ���丮���� �ӽ� �κ��丮 ������Ʈ ����
//        if (StorageInventoryComponent)
//        {
//            StorageInventoryComponent->Capacity = 30; // ����: â��� 30ĭ
//            StorageInventoryComponent->RegisterComponent();
//
//            StorageWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), StorageWidgetClass);
//            if (StorageWidgetInstance)
//            {
//                StorageWidgetInstance->InventoryRef = StorageInventoryComponent;
//                StorageWidgetInstance->bIsChestInventory = true;
//                StorageWidgetInstance->RefreshInventoryStruct();
//                StorageWidgetInstance->AddToViewport(1);
//                StorageWidgetInstance->SetPositionInViewport(FVector2D(800, 300), false); // ���ϴ� ��ġ�� ����
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
//            } // �÷��̾� ��� ����
//            EquipmentWidgetInstance->RefreshEquipmentSlots();
//            EquipmentWidgetInstance->AddToViewport(1);
//            EquipmentWidgetInstance->SetPositionInViewport(FVector2D(500, 300), false); // ���ϴ� ��ġ�� ����
//        }
//    }
//
//}

void ULobbyWidget::OnStartGameClicked()
{
    //UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));

    // ���� �غ� ó��
    // ��: �÷��̾� ���� ����, ������ �˸� ��

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

   

    // ���� â ����
    if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
    if (EquipmentWidgetInstance) EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

    if (!ShopWidgetInstance && ShopWidgetClass)
    {
        ShopWidgetInstance = CreateWidget<UShopWidget>(GetWorld(), ShopWidgetClass);

         
        if (ShopWidgetInstance)
        {
            // ���� ������ ����
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