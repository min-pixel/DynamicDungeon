// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/Button.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"



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
        InventoryComponentRef->Capacity = 50;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // (2) â�� �κ��丮 ������Ʈ ���� (�׻� ���� �����)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 100; // â��� �� ũ��
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

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
    UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));

    // ���� �غ� ó��
    // ��: �÷��̾� ���� ����, ������ �˸� ��
    UWorld* World = GetWorld();
    if (World)
    {
        FString TargetLevel = TEXT("/Game/DynamicDugeon"); // ���͸�+������ (Ȯ���� ����)
        FString Options = FString(TEXT("/Script/project03.DynamicDungeonModeBase"));
        // �����ϰ� ���� ��ε� �غ�
       /* if (World->IsPendingKillPending())
        {
            World->ClearFlags(RF_PendingKill);
        }*/
        World->CleanupWorld();
        //UGameplayStatics::OpenLevel(World, FName(*TargetLevel)); ///Script/CoreUObject.Class'' , true, FString("?game=/Script/project03.DynamicDungeonModeBase")
        UGameplayStatics::OpenLevel(this, FName("DynamicDugeon"), true, FString("?game=/Script/project03.DynamicDungeonModeBase"));
    }
}

void ULobbyWidget::OnGoToShopClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Go to Shop button clicked"));

    // ���� UI ���� ���� ó��
}