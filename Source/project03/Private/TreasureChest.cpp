// Fill out your copyright notice in the Description page of Project Settings.


#include "TreasureChest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MyDCharacter.h"
#include "Weapon.h"
#include "ScrollItem.h"
#include "Armor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ATreasureChest::ATreasureChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
    RootComponent = ChestMesh;



    static ConstructorHelpers::FObjectFinder<UStaticMesh> ChestMeshAsset(TEXT("/Game/BP/object/Chest_Low.Chest_Low"));
    if (ChestMeshAsset.Succeeded())
    {
        ChestMesh->SetStaticMesh(ChestMeshAsset.Object);
    }

    //ChestInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("ChestInventory"));

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &ATreasureChest::OnOverlapBegin);
    InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ATreasureChest::OnOverlapEnd);

    Tags.Add(FName("Chest"));

    static ConstructorHelpers::FClassFinder<UInventoryWidget> WidgetBPClass(TEXT("/Game/BP/UI/InventoryWidget.InventoryWidget_C"));
    if (WidgetBPClass.Succeeded())
    {
        InventoryWidgetClass = WidgetBPClass.Class;
    }

    bReplicates = true;
    bAlwaysRelevant = true;
    // ChestInventory 복제 설정
    ChestInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("ChestInventory"));
    ChestInventory->SetIsReplicated(true);

}

// Called when the game starts or when spawned
void ATreasureChest::BeginPlay()
{
	Super::BeginPlay();

    if (PossibleItems.Num() == 0)
    {
        for (TObjectIterator<UClass> It; It; ++It)
        {
            if (It->IsChildOf(AItem::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract) && *It != AItem::StaticClass())
            {
                PossibleItems.Add(*It);
            }
        }
    }

    // 서버에서만 아이템 생성
    if (HasAuthority() && !bIsPlayerDeathChest)
    {
        GenerateRandomItems();
    }
	
    if (!InventoryWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidgetClass is null!"));
    }

}

// Called every frame
void ATreasureChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATreasureChest::GenerateRandomItems()
{
    int32 ItemCount = FMath::RandRange(5, 20);

    for (int32 i = 0; i < ItemCount; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, PossibleItems.Num() - 1);
        TSubclassOf<AItem> SelectedClass = PossibleItems[RandomIndex];

        FItemData NewItemData;
        NewItemData.ItemClass = SelectedClass;

        // 아이콘과 이름은 클래스에서 미리 가져오기
        if (SelectedClass)
        {
            AItem* DefaultItem = SelectedClass->GetDefaultObject<AItem>();
            NewItemData.ItemIcon = DefaultItem->ItemIcon;
            NewItemData.ItemName = DefaultItem->ItemName;
            NewItemData.ItemType = DefaultItem->ItemType;
            NewItemData.bIsStackable = DefaultItem->bIsStackable;
            NewItemData.MaxStack = DefaultItem->MaxStack;
            NewItemData.Count = 1;
            NewItemData.Price = DefaultItem->Price;




            // 스크롤이면 스킬 인덱스 랜덤 지정
            if (DefaultItem->IsA(AScrollItem::StaticClass()))
            {
                NewItemData.SkillIndex = FMath::RandRange(0, 2);
                //FString SpellName = (NewItemData.SkillIndex == 0) ? TEXT("Fireball") : TEXT("Heal");
                //NewItemData.ItemName = FText::FromString(FString::Printf(TEXT("Scroll of %s"), *SpellName));
            }


            // 등급을 무작위로 설정
            if (DefaultItem->IsA(AArmor::StaticClass()))
            {
                int32 GradeRoll = FMath::RandRange(0, 99);
                if (GradeRoll < 5)
                {
                    NewItemData.Grade = static_cast<uint8>(EArmorGrade::C);
                    NewItemData.Price = DefaultItem->Price; // C등급 가격 
                }
                else if (GradeRoll < 50)
                {
                    NewItemData.Grade = static_cast<uint8>(EArmorGrade::B);
                    NewItemData.Price = DefaultItem->Price * 2; // : B등급 가격 
                }
                else
                {
                    NewItemData.Grade = static_cast<uint8>(EArmorGrade::A);
                    NewItemData.Price = DefaultItem->Price * 3; //A등급 가격 
                }
            }
            else if (DefaultItem->IsA(AWeapon::StaticClass()))
            {
                int32 GradeRoll = FMath::RandRange(0, 99);
                if (GradeRoll < 60)
                {
                    NewItemData.Grade = static_cast<uint8>(EWeaponGrade::C);
                    NewItemData.Price = DefaultItem->Price; //C등급 가격 
                }
                else if (GradeRoll < 90)
                {
                    NewItemData.Grade = static_cast<uint8>(EWeaponGrade::B);
                    NewItemData.Price = DefaultItem->Price * 2; // B등급 가격 
                }
                else
                {
                    NewItemData.Grade = static_cast<uint8>(EWeaponGrade::A);
                    NewItemData.Price = DefaultItem->Price * 3; //  A등급 가격 
                }
            }
            // 기타 아이템은 기본 가격 유지 
            else
            {
                NewItemData.Price = DefaultItem->Price;
            }


        }

       //ChestInventory->TryAddItemByClass(PossibleItems[RandomIndex]); // 구조체 기반으로 추가
        for (int32 j = 0; j < ChestInventory->InventoryItemsStruct.Num(); ++j)
        {
            if (ChestInventory->InventoryItemsStruct[j].ItemClass == nullptr)
            {
                ChestInventory->InventoryItemsStruct[j] = NewItemData;
                break;
            }
        }
       //ChestInventory->TryAddItemStruct(NewItemData);
    }
}

void ATreasureChest::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player)
    {
        UE_LOG(LogTemp, Log, TEXT("Player overlapped with treasure chest!"));

        Player->OverlappedActor = this;

        ChestMesh->SetRenderCustomDepth(true);
    }
}

void ATreasureChest::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player)
    {
        UE_LOG(LogTemp, Log, TEXT("Player exited overlap with treasure chest."));

        CloseChestUI(Player);

        ChestMesh->SetRenderCustomDepth(false);
    }
}

//void ATreasureChest::OpenChestUI(AMyDCharacter* InteractingPlayer)
//{
//    if (!ChestInventory || !InventoryWidgetClass) return;
//
//    if (ChestInventoryWidgetInstance && ChestInventoryWidgetInstance->IsInViewport())
//    {
//        UE_LOG(LogTemp, Warning, TEXT("Chest UI already open, skipping duplicate creation"));
//        return;
//    }
//
//    ChestInventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
//    if (ChestInventoryWidgetInstance)
//    {
//        ChestInventoryWidgetInstance->InventoryRef = ChestInventory;
//        ChestInventoryWidgetInstance->bIsChestInventory = true;
//        ChestInventoryWidgetInstance->SlotWidgetClass = InteractingPlayer->InventoryWidgetInstance->SlotWidgetClass;
//        ChestInventoryWidgetInstance->RefreshInventoryStruct();
//
//        ChestInventoryWidgetInstance->AddToViewport(2);
//        ChestInventoryWidgetInstance->SetPositionInViewport(FVector2D(800, 0), false);
//
//        UE_LOG(LogTemp, Log, TEXT("ChestInventoryWidget added to viewport"));
//
//        // 플레이어 인벤토리도 같이 띄우고 드래그 앤 드롭 되게 만들기
//        if (InteractingPlayer && InteractingPlayer->InventoryWidgetInstance)
//        {
//            InteractingPlayer->InventoryWidgetInstance->AddToViewport(2);
//            InteractingPlayer->InventoryWidgetInstance->SetPositionInViewport(FVector2D(0, 0), false);
//
//            // 마우스 설정
//            APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController());
//            if (PC)
//            {
//                PC->bShowMouseCursor = true;
//                FInputModeGameAndUI Mode;
//                Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
//                PC->SetInputMode(Mode);
//            }
//        }
//    }
//}

void ATreasureChest::OpenChestUI(AMyDCharacter* InteractingPlayer)
{
    
    UE_LOG(LogTemp, Error, TEXT("=== CHEST ACCESS - Player: %s, CurrentUser: %s ==="), *InteractingPlayer->GetName(), CurrentUser ? *CurrentUser->GetName() : TEXT("NULL"));

    // 서버에서만 체크
    if (HasAuthority())
    {
        if (CurrentUser && CurrentUser != InteractingPlayer)
        {
            UE_LOG(LogTemp, Warning, TEXT("Access denied"));
            return;
        }
        CurrentUser = InteractingPlayer;
    }
    
    if (!ChestInventory || !InventoryWidgetClass) return;

    if (ChestInventoryWidgetInstance && ChestInventoryWidgetInstance->IsInViewport())
    {
        UE_LOG(LogTemp, Warning, TEXT("Chest UI already open, skipping duplicate creation"));
        return;
    }

    


    ChestInventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
    if (ChestInventoryWidgetInstance)
    {
        // 서버 or 클라 모두에서 OwningWidgetInstance 지정
        ChestInventory->OwningWidgetInstance = ChestInventoryWidgetInstance;

        ChestInventoryWidgetInstance->InventoryRef = ChestInventory;
        ChestInventoryWidgetInstance->bIsChestInventory = true;
        ChestInventoryWidgetInstance->SlotWidgetClass = InteractingPlayer->InventoryWidgetInstance->SlotWidgetClass;

        ChestInventoryWidgetInstance->RefreshInventoryStruct();

        ChestInventoryWidgetInstance->AddToViewport(2);
        ChestInventoryWidgetInstance->SetPositionInViewport(FVector2D(800, 0), false);

        UE_LOG(LogTemp, Log, TEXT("ChestInventoryWidget added to viewport"));

        // 플레이어 인벤토리도 표시
        if (InteractingPlayer && InteractingPlayer->InventoryWidgetInstance)
        {
            InteractingPlayer->InventoryWidgetInstance->AddToViewport(2);
            InteractingPlayer->InventoryWidgetInstance->SetPositionInViewport(FVector2D(0, 0), false);

            APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController());
            if (PC)
            {
                PC->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                PC->SetInputMode(Mode);
            }
        }
    }
}

void ATreasureChest::CloseChestUI(AMyDCharacter* InteractingPlayer)
{

    // 사용자 해제
    if (CurrentUser == InteractingPlayer)
    {
        CurrentUser = nullptr;
    }

    if (ChestInventoryWidgetInstance)
    {
        ChestInventoryWidgetInstance->RemoveFromParent();
        ChestInventoryWidgetInstance = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Closed Chest Inventory UI"));
    }

    if (InteractingPlayer && InteractingPlayer->InventoryWidgetInstance)
    {
        InteractingPlayer->InventoryWidgetInstance->RemoveFromParent();

        APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController());
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }

        UE_LOG(LogTemp, Log, TEXT("Closed Player Inventory UI"));
    }
}

void ATreasureChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATreasureChest, CurrentUser);
}