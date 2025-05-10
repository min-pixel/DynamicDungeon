// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "HitInterface.h"
#include "MyDCharacter.h"
#include "InventoryWidget.h"
#include "ItemDataD.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMeshAsset(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog"));
    if (EnemyMeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(EnemyMeshAsset.Object);
        // 메시 위치 조정 (기본적으로 캡슐 컴포넌트의 중앙에 위치하기 때문에 아래로 내려줌)
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));

        // 메시 회전 조정 (메시가 Y축을 전방으로 볼 경우 -90도 회전)
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -120.0f, 0.0f));

    }

    static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBPClass(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sevarog_AnimBlueprint.Sevarog_AnimBlueprint_C"));
    if (AnimBPClass.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);
    }

	fsm = CreateDefaultSubobject<UEnemyFSMComponent>(TEXT("FSM"));

    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;


    GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향을 바라보게
    bUseControllerRotationYaw = false;

    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageAsset(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/EnemtAt.EnemtAt"));
    if (AttackMontageAsset.Succeeded())
    {
        AttackMontage = AttackMontageAsset.Object;
    }

    EnemyInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("EnemyInventory"));

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(500.f));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemyCharacter::OnOverlapBegin);
    InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AEnemyCharacter::OnOverlapEnd);

    LootInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("LootInventory"));

    static ConstructorHelpers::FClassFinder<UInventoryWidget> WidgetBPClass(TEXT("/Game/BP/UI/InventoryWidget.InventoryWidget_C"));
    if (WidgetBPClass.Succeeded())
    {
        InventoryWidgetClass = WidgetBPClass.Class;
    }
    
 
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
    CurrentHealth = MaxHealth;


    GenerateRandomLoot();
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::PlayAttackMontage()
{
    if (!AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance()) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 섹션 이름을 콤보 인덱스로 결정
    FName SelectedSection = (AttackComboIndex == 0) ? FName("Attack1") : FName("Attack2");


    // 몽타주 재생 및 섹션 점프
    AnimInstance->Montage_Play(AttackMontage, 1.0f);
    AnimInstance->Montage_JumpToSection(SelectedSection, AttackMontage);

    UE_LOG(LogTemp, Warning, TEXT("Enemy Playing Attack Section: %s"), *SelectedSection.ToString());

    // 콤보 인덱스 변경 (다음에 반대 섹션)
    AttackComboIndex = (AttackComboIndex + 1) % 2;
}

void AEnemyCharacter::TraceAttack()
{
    if (!GetMesh()) return;

    FVector CurrentStart = GetMesh()->GetSocketLocation(FName("AttackStart"));
    FVector CurrentEnd = GetMesh()->GetSocketLocation(FName("AttackEnd"));

    TArray<FHitResult> HitResults;
    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(this);

    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        LastStartLocation,
        CurrentStart,
        30.0f,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoredActors,
        EDrawDebugTrace::None,
        HitResults,
        true
    );

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {
            if (!HitActors.Contains(HitActor)) // 중복 방지
            {
                HitActors.Add(HitActor);
                IHitInterface::Execute_GetHit(HitActor, Hit, this, 20.0f); // 적의 데미지 값은 자유롭게 조정
            }
        }
    }

    LastStartLocation = CurrentStart;
    LastEndLocation = CurrentEnd;
}

void AEnemyCharacter::StartTrace()
{
    if (!GetMesh()) return;

    LastStartLocation = GetMesh()->GetSocketLocation(FName("AttackStart"));
    LastEndLocation = GetMesh()->GetSocketLocation(FName("AttackEnd"));

    HitActors.Empty(); // 중복 방지용 초기화
}


void AEnemyCharacter::GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage)
{
    if (bIsStunned) return;

    CurrentHealth -= Damage;
    UE_LOG(LogTemp, Log, TEXT("Enemy took damage: %f | Current HP: %f"), Damage, CurrentHealth);

    HandleStun(); // 잠깐 경직

    if (CurrentHealth <= 0)
    {
        if (fsm)
        {
            fsm->SetState(EEnemyState::Dead); // FSM 상태를 Dead로 설정
        }

        // 사망 처리
        
        //SetActorEnableCollision(false);
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
        GetMesh()->SetSimulatePhysics(true);
        //GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

        SetActorEnableCollision(true);
        InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Tags.Add(FName("Chest"));
        
    }
}

void AEnemyCharacter::HandleStun()
{
    bIsStunned = true;

    GetCharacterMovement()->StopMovementImmediately();

    if (AAIController* AI = Cast<AAIController>(GetController()))
    {
        AI->StopMovement();
    }

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && AnimInstance->Montage_IsPlaying(AttackMontage))
    {
        AnimInstance->Montage_Pause(AttackMontage);

        FVector KnockbackDirection = FVector(0, 0, 1.0f);
        LaunchCharacter(KnockbackDirection * 100.0f, true, false);

        if (!GetMesh()->IsSimulatingPhysics()) {
            PlayHitShake();
        }
    }

    GetWorldTimerManager().SetTimer(StunTimerHandle, [this, AnimInstance]()
        {
            bIsStunned = false;

            if (AnimInstance && AnimInstance->GetCurrentActiveMontage() == AttackMontage)
            {
                AnimInstance->Montage_Resume(AttackMontage);
            }

        }, 0.15f, false);
}

void AEnemyCharacter::PlayHitShake()
{

    FVector OriginalLocation = GetMesh()->GetRelativeLocation();

    for (int i = 0; i < 3; ++i)
    {
        float Delay = i * 0.05f;
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this, OriginalLocation, i]()
            {
                FVector ShakeOffset = FVector(0.0f, (i % 2 == 0 ? -3.0f : 3.0f), 0.0f);
                GetMesh()->SetRelativeLocation(OriginalLocation + ShakeOffset, false, nullptr, ETeleportType::TeleportPhysics);
            }), Delay, false);
    }

    // 원래 위치 복원
    GetWorldTimerManager().SetTimerForNextTick([this, OriginalLocation]() {
        FTimerHandle RestoreHandle;
        GetWorldTimerManager().SetTimer(RestoreHandle, [this, OriginalLocation]() {
            GetMesh()->SetRelativeLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
            }, 0.16f, false);
        });
}

void AEnemyCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor))
    {
        Player->OverlappedActor = this; // 기존 상호작용 로직 호환
        if (CurrentHealth <= 0.0f)
        {
            GetMesh()->SetRenderCustomDepth(true);
        }
    }
}

void AEnemyCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor))
    {
        Player->OverlappedActor = nullptr;
        GetMesh()->SetRenderCustomDepth(false);

        if (LootInventoryWidgetInstance)
        {
            LootInventoryWidgetInstance->RemoveFromParent();
            LootInventoryWidgetInstance = nullptr;
            UE_LOG(LogTemp, Log, TEXT("Closed enemy loot UI"));
        }

        if (Player->InventoryWidgetInstance)
        {
            Player->InventoryWidgetInstance->RemoveFromParent();

            APlayerController* PC = Cast<APlayerController>(Player->GetController());
            if (PC)
            {
                PC->bShowMouseCursor = false;
                PC->SetInputMode(FInputModeGameOnly());
            }
        }

    }
}

void AEnemyCharacter::OpenLootUI(AMyDCharacter* InteractingPlayer)
{
    if (!LootInventory || !InventoryWidgetClass) return;

    if (LootInventoryWidgetInstance && LootInventoryWidgetInstance->IsInViewport())
        return;

    LootInventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
    if (LootInventoryWidgetInstance)
    {
        LootInventoryWidgetInstance->InventoryRef = LootInventory;
        LootInventoryWidgetInstance->bIsChestInventory = true;
        LootInventoryWidgetInstance->SlotWidgetClass = InteractingPlayer->InventoryWidgetInstance->SlotWidgetClass;
        LootInventoryWidgetInstance->RefreshInventoryStruct();
        LootInventoryWidgetInstance->AddToViewport(2);
        LootInventoryWidgetInstance->SetPositionInViewport(FVector2D(800, 0), false);

        // 플레이어 인벤토리 같이 열기
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

void AEnemyCharacter::GenerateRandomLoot()
{
    if (PossibleItems.Num() == 0)
    {
        for (TObjectIterator<UClass> It; It; ++It)
        {
            if (It->IsChildOf(AItem::StaticClass()) &&
                !It->HasAnyClassFlags(CLASS_Abstract) &&
                *It != AItem::StaticClass())
            {
                PossibleItems.Add(*It);
            }
        }
    }

    int32 ItemCount = FMath::RandRange(3, 10); // 생성할 아이템 개수

    for (int32 i = 0; i < ItemCount; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, PossibleItems.Num() - 1);
        TSubclassOf<AItem> SelectedClass = PossibleItems[RandomIndex];

        if (!SelectedClass) continue;

        AItem* DefaultItem = SelectedClass->GetDefaultObject<AItem>();
        if (!DefaultItem) continue;

        FItemData NewItemData;
        NewItemData.ItemClass = SelectedClass;
        NewItemData.ItemIcon = DefaultItem->ItemIcon;
        NewItemData.ItemName = DefaultItem->ItemName;
        NewItemData.ItemType = DefaultItem->ItemType;
        NewItemData.bIsStackable = DefaultItem->bIsStackable;
        NewItemData.MaxStack = DefaultItem->MaxStack;
        NewItemData.Count = 1;

        // 직접 구조체를 배열에 넣음
        for (int32 j = 0; j < LootInventory->InventoryItemsStruct.Num(); ++j)
        {
            if (LootInventory->InventoryItemsStruct[j].ItemClass == nullptr)
            {
                LootInventory->InventoryItemsStruct[j] = NewItemData;
                break;
            }
        }
    }
}