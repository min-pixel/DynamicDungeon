#include "RageEnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include "EnemyCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "HitInterface.h"
#include "MyDCharacter.h"
#include "InventoryWidget.h"
#include "ItemDataD.h"
#include "Net/UnrealNetwork.h"
#include "TreasureGlowEffect.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "AIController.h"

ARageEnemyCharacter::ARageEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // === 레이지 에너미 전용 메시 설정 ===
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> RageEnemyMeshAsset(TEXT("/Game/panda_samurai/Meshes/base_mesh_M_Q/SKM_base_mesh_helmet_1.SKM_base_mesh_helmet_1"));
    if (RageEnemyMeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(RageEnemyMeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        GetMesh()->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
    }

    // === 레이지 에너미 전용 애니메이션 블루프린트 ===
    static ConstructorHelpers::FClassFinder<UAnimInstance> RageAnimBPClass(TEXT("/Game/panda_samurai/Demo/Mannequin/Animations/anim_ThirdPerson_AnimBP.anim_ThirdPerson_AnimBP_C"));
    if (RageAnimBPClass.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(RageAnimBPClass.Class);
    }

    // === FSM 컴포넌트 ===
    fsm = CreateDefaultSubobject<UEnemyFSMComponent>(TEXT("FSM"));

    // === AI 컨트롤러 설정 ===
    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // === 이동 설정 ===
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    // === 레이지 에너미 전용 공격 몽타주 ===
    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageAsset(TEXT("/Game/panda_samurai/animation/animation_Manny_Quinn_skeleton/monster/anim_monster_attack_3_Montage.anim_monster_attack_3_Montage"));
    if (AttackMontageAsset.Succeeded())
    {
        AttackMontage = AttackMontageAsset.Object;
    }

    // 점프 공격 몽타주 설정
    
    static ConstructorHelpers::FObjectFinder<UAnimMontage> JumpAttackMontageAsset(TEXT("/Game/panda_samurai/animation/animation_Manny_Quinn_skeleton/monster/anim_monster_Jumping_attack_Montage.anim_monster_Jumping_attack_Montage"));
    if (JumpAttackMontageAsset.Succeeded())
    {
        JumpAttackMontage = JumpAttackMontageAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UAnimMontage> RageEnterMontageAsset(TEXT("/Game/panda_samurai/animation/animation_Manny_Quinn_skeleton/monster/anim_monster_rage_Montage.anim_monster_rage_Montage"));
    if (RageEnterMontageAsset.Succeeded())
    {
        RageEnterMontage = RageEnterMontageAsset.Object;
    }
    

    // === 인벤토리 시스템 ===
    EnemyInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("EnemyInventory"));

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(500.f));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &ARageEnemyCharacter::OnOverlapBegin);
    InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ARageEnemyCharacter::OnOverlapEnd);

    LootInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("LootInventory"));
    LootInventory->SetIsReplicated(true);

    static ConstructorHelpers::FClassFinder<UInventoryWidget> WidgetBPClass(TEXT("/Game/BP/UI/InventoryWidget.InventoryWidget_C"));
    if (WidgetBPClass.Succeeded())
    {
        InventoryWidgetClass = WidgetBPClass.Class;
    }

 

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FoundChestMesh(TEXT("/Game/BP/object/Chest_Low.Chest_Low"));
    if (FoundChestMesh.Succeeded())
    {
        ChestMeshAsset = FoundChestMesh.Object;
    }

    // 나이아가라 이펙트 로드
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> GlowEffect(TEXT("/Game/PixieDustTrail/FX/NS_PixieDustTrail.NS_PixieDustTrail"));
    if (GlowEffect.Succeeded())
    {
        TreasureGlowEffectAsset = GlowEffect.Object;
    }


    bReplicates = true;
    SetReplicatingMovement(true);
    bAlwaysRelevant = true;

}

void ARageEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    // 원래 이동 속도 저장
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        OriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
    }

    if (HasAuthority())
    {
        GenerateRandomLoot();
    }
}

void ARageEnemyCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 매 프레임마다 레이지 모드 체크
    if (!bIsStunned && !bIsPlayingRageMontage)
    {
        CheckRageMode();
        
    }
}

void ARageEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ARageEnemyCharacter::PlayAttackMontage()
{
    
   

    if (!AttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance()) return;

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

    // 공격 시작 시 이동 제한 설정
    bIsAttacking = true;

    //CharacterMovement 비활성화
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement(); // 이동 완전 비활성화
        MoveComp->MaxWalkSpeed = 0.0f;
    }

    

    // 레이지 모드에서는 더 빠른 공격
    float PlayRate = bIsInRageMode ? 2.0f : 1.0f;

    // 섹션 이름을 컴보 인덱스로 결정
    FName SelectedSection = (AttackComboIndex == 0) ? FName("Attack1") : FName("Attack2");

    AnimInstance->Montage_Play(AttackMontage, PlayRate);
    AnimInstance->Montage_JumpToSection(SelectedSection, AttackMontage);

    // 몽타주 끝날 때 콜백 설정
    FOnMontageEnded MontageEndedDelegate;
    MontageEndedDelegate.BindUObject(this, &ARageEnemyCharacter::OnAttackMontageEnded);
    AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);

    UE_LOG(LogTemp, Warning, TEXT("RageEnemy Playing Attack Section: %s (Rate: %.1f)"),
        *SelectedSection.ToString(), PlayRate);

    // 컴보 인덱스 변경 (다음에 반대 섹션)
    AttackComboIndex = (AttackComboIndex + 1) % 2;
}

// 공격 몽타주 종료 시 호출
void ARageEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == AttackMontage)
    {
        // 공격 종료 시 이동 재활성화
        bIsAttacking = false;

        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking); // 이동 재활성화
            MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;
        }

        UE_LOG(LogTemp, Log, TEXT("Attack montage ended, movement re-enabled"));
    }
}


void ARageEnemyCharacter::TraceAttack()
{
    if (!GetMesh()) return;

    FVector CurrentStart = GetMesh()->GetSocketLocation(FName("AttackStart"));
    FVector CurrentEnd = GetMesh()->GetSocketLocation(FName("AttackEnd"));

    TArray<FHitResult> HitResults;

    TArray<FHitResult> StartHitResults;
    TArray<FHitResult> EndHitResults;
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
        StartHitResults,
        true
    );

    // End 소켓 궤적 트레이스
    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        LastEndLocation,
        CurrentEnd,
        30.0f,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoredActors,
        EDrawDebugTrace::None,
        EndHitResults,
        true
    );

    // === StartHitResults 판정 ===
    for (const FHitResult& Hit : StartHitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {

            // 적끼리는 공격하지 않도록 체크
            if (Cast<AEnemyCharacter>(HitActor) || Cast<ARageEnemyCharacter>(HitActor))
            {
                continue; // 모든 적 타입 스킵
            }

            if (!HitActors.Contains(HitActor))
            {
                HitActors.Add(HitActor);
                float Damage = bIsInRageMode ? (10.0f * RageDamageMultiplier) : 10.0f;
                IHitInterface::Execute_GetHit(HitActor, Hit, this, Damage);
            }
        }
    }

    // === EndHitResults 판정 ===
    for (const FHitResult& Hit : EndHitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {

            // 적끼리는 공격하지 않도록 체크
            if (Cast<AEnemyCharacter>(HitActor) || Cast<ARageEnemyCharacter>(HitActor))
            {
                continue; // 모든 적 타입 스킵
            }

            if (!HitActors.Contains(HitActor)) // 중복 방지(여기서도 꼭!)
            {
                HitActors.Add(HitActor);
                float Damage = bIsInRageMode ? (10.0f * RageDamageMultiplier) : 10.0f;
                IHitInterface::Execute_GetHit(HitActor, Hit, this, Damage);
            }
        }
    }

    LastStartLocation = CurrentStart;
    LastEndLocation = CurrentEnd;
}

void ARageEnemyCharacter::StartTrace()
{
    if (!GetMesh()) return;

    LastStartLocation = GetMesh()->GetSocketLocation(FName("AttackStart"));
    LastEndLocation = GetMesh()->GetSocketLocation(FName("AttackEnd"));
    HitActors.Empty();
}

void ARageEnemyCharacter::GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage)
{
    if (bIsStunned) return;

    CurrentHealth -= Damage;
    UE_LOG(LogTemp, Log, TEXT("RageEnemy took damage: %f | Current HP: %f"), Damage, CurrentHealth);

    if (HasAuthority() && InstigatorActor)
    {
        if (AMyDCharacter* PlayerPawn = Cast<AMyDCharacter>(InstigatorActor))
        {
            PlayerPawn->ClientPlayHitSoundAtLocation(GetActorLocation());
        }
    }

    // 레이지 모드 체크
    CheckRageMode();



    // 레이지 모드에서는 스턴 저항 (스턴 시간 단축)
    if (bIsInRageMode)
    {
        bIsStunned = true;
        GetCharacterMovement()->StopMovementImmediately();

        if (AAIController* AI = Cast<AAIController>(GetController()))
        {
            AI->StopMovement();
        }

        // 짧은 스턴 시간
        GetWorldTimerManager().SetTimer(StunTimerHandle, [this]()
            {
                bIsStunned = false;
            }, 0.075f, false); // 기본의 절반 시간
    }
    else
    {
        // 일반 스턴 처리
        HandleStun();
    }

    //if (CurrentHealth <= 0)
    //{
    //    // 레이지 모드 종료
    //    if (bIsInRageMode)
    //    {
    //        ExitRageMode();
    //    }

    //    if (fsm)
    //    {
    //        fsm->SetState(EEnemyState::Dead);
    //    }

    //    // 사망 처리
    //    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    //    GetMesh()->SetSimulatePhysics(true);
    //    SetActorEnableCollision(true);
    //    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    //    Tags.Add(FName("Chest"));
    //}


     // === 멀티플레이 사망 처리 ===
    if (CurrentHealth <= 0)
    {
        if (!bIsDead)
        {
            bIsDead = true;
            OnRep_Dead();
            MulticastActivateRagdoll();
        }
    }
}

void ARageEnemyCharacter::OnRep_Dead()
{
    if (fsm)
    {
        fsm->SetState(EEnemyState::Dead);
    }

    //// 레이지 모드 종료
    if (bIsInRageMode)
    {
        ExitRageMode();
    }
}

void ARageEnemyCharacter::MulticastActivateRagdoll_Implementation()
{
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    SetActorEnableCollision(true);
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Tags.Add(FName("Chest"));

    bIsTransformedToChest = true;

    // 일정 시간 후 보물상자 메시로 변경
    FTimerHandle ReplaceMeshTimer;
    GetWorldTimerManager().SetTimer(ReplaceMeshTimer, this, &ARageEnemyCharacter::OnRep_TransformToChest, 2.0f, false);
}

void ARageEnemyCharacter::ReplaceMeshWithChest()
{
    if (!IsValid(this) || !ChestMeshAsset) return;

    // 기존 스켈레탈 메시 숨김
    GetMesh()->SetVisibility(false, true);

    // 보물 상자 메시 생성 및 부착
    if (!ChestStaticMesh)
    {
        ChestStaticMesh = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), TEXT("ChestStaticMesh"));
        if (ChestStaticMesh)
        {
            ChestStaticMesh->RegisterComponent();
            ChestStaticMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);

            ChestStaticMesh->SetStaticMesh(ChestMeshAsset);
            ChestStaticMesh->SetCollisionProfileName(TEXT("BlockAll"));
            ChestStaticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
            ChestStaticMesh->SetVisibility(true);
            
            if (TreasureGlowEffectAsset)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Owner = this;

                ATreasureGlowEffect* GlowEffect = GetWorld()->SpawnActor<ATreasureGlowEffect>(
                    ATreasureGlowEffect::StaticClass(),
                    GetActorLocation(),
                    FRotator::ZeroRotator,
                    SpawnParams
                );

                if (GlowEffect)
                {
                    GlowEffect->InitEffect(this, TreasureGlowEffectAsset, -80.0f);
                }
            }

        }
    }
    else
    {
        ChestStaticMesh->SetVisibility(true);
    }
}


void ARageEnemyCharacter::OnRep_TransformToChest()
{
    ReplaceMeshWithChest();
}


void ARageEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARageEnemyCharacter, bIsDead);
    DOREPLIFETIME(ARageEnemyCharacter, LootInventory);
    DOREPLIFETIME(ARageEnemyCharacter, bIsTransformedToChest);
}

void ARageEnemyCharacter::ApplyDebuff_Implementation(EDebuffType DebuffType, float Value, float Duration)
{
    UE_LOG(LogTemp, Warning, TEXT("[RageEnemyCharacter] ApplyDebuff: %s, Value: %.2f, Duration: %.2f"),
        *UEnum::GetValueAsString(DebuffType), Value, Duration);

    switch (DebuffType)
    {
    case EDebuffType::Slow:
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            float CurrentSpeed = MoveComp->MaxWalkSpeed;
            MoveComp->MaxWalkSpeed *= (1.f - Value);
            UE_LOG(LogTemp, Warning, TEXT("RageEnemy slowed to %.2f for %.2f seconds"), MoveComp->MaxWalkSpeed, Duration);

            // 일정 시간 후 속도 복원
            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(TimerHandle, [MoveComp, CurrentSpeed]()
                {
                    MoveComp->MaxWalkSpeed = CurrentSpeed;
                    UE_LOG(LogTemp, Warning, TEXT("RageEnemy slow debuff ended. Speed restored to %.2f"), CurrentSpeed);
                }, Duration, false);
        }
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown debuff type."));
        break;
    }
}

void ARageEnemyCharacter::HandleStun()
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
        }, 0.45f, false);
}

void ARageEnemyCharacter::PlayHitShake()
{
    FVector OriginalLocation = GetMesh()->GetRelativeLocation();

    for (int i = 0; i < 3; ++i)
    {
        float Delay = i * 0.15f;
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this, OriginalLocation, i]()
            {
                FVector ShakeOffset = FVector(0.0f, (i % 2 == 0 ? -10.0f : 10.0f), 0.0f);
                GetMesh()->SetRelativeLocation(OriginalLocation + ShakeOffset, false, nullptr, ETeleportType::TeleportPhysics);
            }), Delay, false);
    }

    // 원래 위치 복원
    GetWorldTimerManager().SetTimerForNextTick([this, OriginalLocation]() {
        FTimerHandle RestoreHandle;
        GetWorldTimerManager().SetTimer(RestoreHandle, [this, OriginalLocation]() {
            GetMesh()->SetRelativeLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
            }, 0.48f, false);
        });
}

// === 인벤토리 시스템 (기본 에너미와 동일) ===
void ARageEnemyCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor))
    {
        Player->OverlappedActor = this;
        if (CurrentHealth <= 0.0f)
        {
            GetMesh()->SetRenderCustomDepth(true);
        }
    }
}

void ARageEnemyCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor))
    {
        Player->OverlappedActor = nullptr;
        GetMesh()->SetRenderCustomDepth(false);

        if (LootInventoryWidgetInstance &&
            LootInventory->OwningWidgetInstance == LootInventoryWidgetInstance &&
            Player->InventoryWidgetInstance &&
            Player->InventoryWidgetInstance->IsInViewport())
        {
            LootInventory->OwningWidgetInstance = nullptr;
            LootInventoryWidgetInstance->RemoveFromParent();
            LootInventoryWidgetInstance = nullptr;
            UE_LOG(LogTemp, Log, TEXT("Closed enemy loot UI"));

            // 이 플레이어의 인벤토리도 닫기
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

void ARageEnemyCharacter::OpenLootUI(AMyDCharacter* InteractingPlayer)
{
    if (!LootInventory || !InventoryWidgetClass) return;

    if (LootInventoryWidgetInstance && LootInventoryWidgetInstance->IsInViewport())
        return;

    LootInventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
    if (LootInventoryWidgetInstance)
    {
        LootInventory->OwningWidgetInstance = LootInventoryWidgetInstance;
        LootInventoryWidgetInstance->InventoryRef = LootInventory;
        LootInventoryWidgetInstance->bIsChestInventory = true;
        LootInventoryWidgetInstance->SlotWidgetClass = InteractingPlayer->InventoryWidgetInstance->SlotWidgetClass;
        LootInventoryWidgetInstance->RefreshInventoryStruct();
        LootInventoryWidgetInstance->AddToViewport(2);
        LootInventoryWidgetInstance->SetPositionInViewport(FVector2D(800, 0), false);

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

void ARageEnemyCharacter::GenerateRandomLoot()
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

    // 레이지 에너미는 더 많은 아이템 드랍
    int32 ItemCount = FMath::RandRange(5, 12);

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
        NewItemData.Price = DefaultItem->Price; // 기본 가격 사용

        // 등급별 가격 조정
        if (DefaultItem->IsA(AArmor::StaticClass()))
        {
            int32 GradeRoll = FMath::RandRange(0, 99);
            if (GradeRoll < 60)
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::C);
                NewItemData.Price = DefaultItem->Price; // C등급: 기본 가격
            }
            else if (GradeRoll < 90)
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::B);
                NewItemData.Price = DefaultItem->Price * 2; // B등급: 2배
            }
            else
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::A);
                NewItemData.Price = DefaultItem->Price * 3; // A등급: 3배
            }
        }
        else if (DefaultItem->IsA(AWeapon::StaticClass()))
        {
            int32 GradeRoll = FMath::RandRange(0, 99);
            if (GradeRoll < 60)
            {
                NewItemData.Grade = static_cast<uint8>(EWeaponGrade::C);
                NewItemData.Price = DefaultItem->Price;
            }
            else if (GradeRoll < 90)
            {
                NewItemData.Grade = static_cast<uint8>(EWeaponGrade::B);
                NewItemData.Price = DefaultItem->Price * 2;
            }
            else
            {
                NewItemData.Grade = static_cast<uint8>(EWeaponGrade::A);
                NewItemData.Price = DefaultItem->Price * 3;
            }
        }
        else
        {
            // 무기/방어구가 아닌 경우 기본 가격 사용
            NewItemData.Price = DefaultItem->Price;
        }


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

// === 레이지 모드 시스템 ===
void ARageEnemyCharacter::CheckRageMode()
{
    float HealthPercent = CurrentHealth / MaxHealth;

    // HP가 30% 이하이고 아직 레이지 모드가 아니라면
    if (HealthPercent <= RageThreshold && !bIsInRageMode)
    {
        EnterRageMode();
    }
}

void ARageEnemyCharacter::EnterRageMode()
{
    if (bIsInRageMode) return;

    bIsInRageMode = true;
    bIsPlayingRageMontage = true;
    
    UE_LOG(LogTemp, Warning, TEXT("RageEnemy entered RAGE MODE!"));

    // === 움직임 완전 정지 ===
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        //MoveComp->DisableMovement(); // 움직임 완전 비활성화
        MoveComp->MaxWalkSpeed = 0.0f;
    }

    
 
   

    // 메시 색상 변경으로 레이지 모드 표시 (빨간색 틴트)
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVectorParameterValueOnMaterials(FName("TintColor"), FVector(1.0f, 0.3f, 0.3f));
    }

    // 레이지 모드 애니메이션 재생
    if (RageEnterMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        AnimInstance->Montage_Play(RageEnterMontage, 1.0f);
        
        // 몽타주 종료 시 콜백 설정
        FOnMontageEnded RageMontageEndedDelegate;
        RageMontageEndedDelegate.BindUObject(this, &ARageEnemyCharacter::OnRageMontageEnded);
        AnimInstance->Montage_SetEndDelegate(RageMontageEndedDelegate, RageEnterMontage);

    }
}

// 레이지 몽타주 종료 시 호출되는 함수
void ARageEnemyCharacter::OnRageMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == RageEnterMontage)
    {
        bIsPlayingRageMontage = false;

        // 움직임 재활성화
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking);
            // 이동 속도 증가
           
                MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * RageSpeedMultiplier;
           
        }

        

        UE_LOG(LogTemp, Warning, TEXT("Rage montage ended, movement and AI re-enabled"));
    }
}

// FSM이나 AI에서 움직임을 허용하기 전에 체크하는 함수들도 수정 필요
bool ARageEnemyCharacter::CanMove() const
{
    return !bIsStunned && !bIsAttacking && !bIsPlayingRageMontage;
}

void ARageEnemyCharacter::ExitRageMode()
{
    if (!bIsInRageMode) return;

    bIsInRageMode = false;
    UE_LOG(LogTemp, Warning, TEXT("RageEnemy exited rage mode"));

    // 이동 속도 원래대로
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;
    }

    // 메시 색상 원래대로
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVectorParameterValueOnMaterials(FName("TintColor"), FVector(1.0f, 1.0f, 1.0f));
    }
}

// === 점프 공격 시스템 ===
void ARageEnemyCharacter::TryJumpAttack()
{
    if (!bCanJumpAttack || bIsStunned || !bIsInRageMode) return;

    // 플레이어와의 거리 체크
    AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!Player) return;

    float DistanceToPlayer = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

    // 사거리 내에 있고 레이지 모드일 때만 점프 공격
    if (DistanceToPlayer <= JumpAttackRange)
    {
        PerformJumpAttack();
    }
}

void ARageEnemyCharacter::PerformJumpAttack()
{
    if (!JumpAttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance()) return;

    UE_LOG(LogTemp, Warning, TEXT("RageEnemy performing jump attack!"));

    // 점프 공격 쿨다운 시작
    bCanJumpAttack = false;
    GetWorldTimerManager().SetTimer(JumpAttackCooldownTimer, this,
        &ARageEnemyCharacter::OnJumpAttackCooldownEnd, JumpAttackCooldown, false);

    // 점프 공격 애니메이션 재생
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    AnimInstance->Montage_Play(JumpAttackMontage, 1.2f);

    // 플레이어 방향으로 점프
    AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (Player)
    {
        FVector JumpDirection = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        FVector JumpVelocity = JumpDirection * 600.0f + FVector(0, 0, 400.0f);
        LaunchCharacter(JumpVelocity, true, true);

        // 착지 후 데미지 처리를 위한 타이머 (애니메이션 시간에 맞춰 조정)
        FTimerHandle JumpDamageTimer;
        GetWorldTimerManager().SetTimer(JumpDamageTimer, [this, Player]()
            {
                // 착지 지점 주변의 적들에게 데미지
                TArray<FHitResult> HitResults;
                TArray<AActor*> IgnoredActors;
                IgnoredActors.Add(this);

                UKismetSystemLibrary::SphereTraceMulti(
                    GetWorld(),
                    GetActorLocation(),
                    GetActorLocation() + FVector(0, 0, -50),
                    150.0f, // 착지 공격 범위
                    UEngineTypes::ConvertToTraceType(ECC_Pawn),
                    false,
                    IgnoredActors,
                    EDrawDebugTrace::ForOneFrame,
                    HitResults,
                    true
                );

                for (const FHitResult& Hit : HitResults)
                {
                    AActor* HitActor = Hit.GetActor();
                    if (HitActor && HitActor->Implements<UHitInterface>())
                    {
                        IHitInterface::Execute_GetHit(HitActor, Hit, this, JumpAttackDamage);
                    }
                }
            }, 1.0f, false); // 1초 후 착지 데미지
    }
}

void ARageEnemyCharacter::OnJumpAttackCooldownEnd()
{
    bCanJumpAttack = true;
    UE_LOG(LogTemp, Log, TEXT("RageEnemy jump attack cooldown ended"));
}

void ARageEnemyCharacter::PlayRageAttackMontage()
{
    // 레이지 모드 전용 공격 패턴이 필요하다면 여기에 구현
    // 현재는 기본 공격을 더 빠르게 재생하는 방식으로 구현됨
    PlayAttackMontage();
}