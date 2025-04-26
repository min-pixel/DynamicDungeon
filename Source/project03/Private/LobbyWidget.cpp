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
        InventoryComponentRef->Capacity = 50;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // (2) 창고 인벤토리 컴포넌트 생성 (항상 새로 만든다)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 100; // 창고는 더 크게
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

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
    UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));

    // 게임 준비 처리
    // 예: 플레이어 상태 변경, 서버에 알림 등
    UWorld* World = GetWorld();
    if (World)
    {
        FString TargetLevel = TEXT("/Game/DynamicDugeon"); // 디렉터리+레벨명 (확장자 없음)
        FString Options = FString(TEXT("/Script/project03.DynamicDungeonModeBase"));
        // 안전하게 월드 재로딩 준비
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

    // 상점 UI 열기 등의 처리
}