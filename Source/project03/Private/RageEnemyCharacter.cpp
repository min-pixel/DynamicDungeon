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

    // === ������ ���ʹ� ���� �޽� ���� ===
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> RageEnemyMeshAsset(TEXT("/Game/panda_samurai/Meshes/base_mesh_M_Q/SKM_base_mesh_helmet_1.SKM_base_mesh_helmet_1"));
    if (RageEnemyMeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(RageEnemyMeshAsset.Object);
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
        GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
        GetMesh()->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
    }

    // === ������ ���ʹ� ���� �ִϸ��̼� �������Ʈ ===
    static ConstructorHelpers::FClassFinder<UAnimInstance> RageAnimBPClass(TEXT("/Game/panda_samurai/Demo/Mannequin/Animations/anim_ThirdPerson_AnimBP.anim_ThirdPerson_AnimBP_C"));
    if (RageAnimBPClass.Succeeded())
    {
        GetMesh()->SetAnimInstanceClass(RageAnimBPClass.Class);
    }

    // === FSM ������Ʈ ===
    fsm = CreateDefaultSubobject<UEnemyFSMComponent>(TEXT("FSM"));

    // === AI ��Ʈ�ѷ� ���� ===
    AIControllerClass = AEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // === �̵� ���� ===
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    // === ������ ���ʹ� ���� ���� ��Ÿ�� ===
    static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageAsset(TEXT("/Game/panda_samurai/animation/animation_Manny_Quinn_skeleton/monster/anim_monster_attack_3_Montage.anim_monster_attack_3_Montage"));
    if (AttackMontageAsset.Succeeded())
    {
        AttackMontage = AttackMontageAsset.Object;
    }

    // ���� ���� ��Ÿ�� ����
    
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
    

    // === �κ��丮 �ý��� ===
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

    // ���̾ư��� ����Ʈ �ε�
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

    // ���� �̵� �ӵ� ����
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

    // �� �����Ӹ��� ������ ��� üũ
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

    // ���� ���� �� �̵� ���� ����
    bIsAttacking = true;

    //CharacterMovement ��Ȱ��ȭ
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement(); // �̵� ���� ��Ȱ��ȭ
        MoveComp->MaxWalkSpeed = 0.0f;
    }

    

    // ������ ��忡���� �� ���� ����
    float PlayRate = bIsInRageMode ? 2.0f : 1.0f;

    // ���� �̸��� �ĺ� �ε����� ����
    FName SelectedSection = (AttackComboIndex == 0) ? FName("Attack1") : FName("Attack2");

    AnimInstance->Montage_Play(AttackMontage, PlayRate);
    AnimInstance->Montage_JumpToSection(SelectedSection, AttackMontage);

    // ��Ÿ�� ���� �� �ݹ� ����
    FOnMontageEnded MontageEndedDelegate;
    MontageEndedDelegate.BindUObject(this, &ARageEnemyCharacter::OnAttackMontageEnded);
    AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);

    UE_LOG(LogTemp, Warning, TEXT("RageEnemy Playing Attack Section: %s (Rate: %.1f)"),
        *SelectedSection.ToString(), PlayRate);

    // �ĺ� �ε��� ���� (������ �ݴ� ����)
    AttackComboIndex = (AttackComboIndex + 1) % 2;
}

// ���� ��Ÿ�� ���� �� ȣ��
void ARageEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == AttackMontage)
    {
        // ���� ���� �� �̵� ��Ȱ��ȭ
        bIsAttacking = false;

        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking); // �̵� ��Ȱ��ȭ
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

    // End ���� ���� Ʈ���̽�
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

    // === StartHitResults ���� ===
    for (const FHitResult& Hit : StartHitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {

            // �������� �������� �ʵ��� üũ
            if (Cast<AEnemyCharacter>(HitActor) || Cast<ARageEnemyCharacter>(HitActor))
            {
                continue; // ��� �� Ÿ�� ��ŵ
            }

            if (!HitActors.Contains(HitActor))
            {
                HitActors.Add(HitActor);
                float Damage = bIsInRageMode ? (10.0f * RageDamageMultiplier) : 10.0f;
                IHitInterface::Execute_GetHit(HitActor, Hit, this, Damage);
            }
        }
    }

    // === EndHitResults ���� ===
    for (const FHitResult& Hit : EndHitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {

            // �������� �������� �ʵ��� üũ
            if (Cast<AEnemyCharacter>(HitActor) || Cast<ARageEnemyCharacter>(HitActor))
            {
                continue; // ��� �� Ÿ�� ��ŵ
            }

            if (!HitActors.Contains(HitActor)) // �ߺ� ����(���⼭�� ��!)
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

    // ������ ��� üũ
    CheckRageMode();



    // ������ ��忡���� ���� ���� (���� �ð� ����)
    if (bIsInRageMode)
    {
        bIsStunned = true;
        GetCharacterMovement()->StopMovementImmediately();

        if (AAIController* AI = Cast<AAIController>(GetController()))
        {
            AI->StopMovement();
        }

        // ª�� ���� �ð�
        GetWorldTimerManager().SetTimer(StunTimerHandle, [this]()
            {
                bIsStunned = false;
            }, 0.075f, false); // �⺻�� ���� �ð�
    }
    else
    {
        // �Ϲ� ���� ó��
        HandleStun();
    }

    //if (CurrentHealth <= 0)
    //{
    //    // ������ ��� ����
    //    if (bIsInRageMode)
    //    {
    //        ExitRageMode();
    //    }

    //    if (fsm)
    //    {
    //        fsm->SetState(EEnemyState::Dead);
    //    }

    //    // ��� ó��
    //    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    //    GetMesh()->SetSimulatePhysics(true);
    //    SetActorEnableCollision(true);
    //    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    //    Tags.Add(FName("Chest"));
    //}


     // === ��Ƽ�÷��� ��� ó�� ===
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

    //// ������ ��� ����
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

    // ���� �ð� �� �������� �޽÷� ����
    FTimerHandle ReplaceMeshTimer;
    GetWorldTimerManager().SetTimer(ReplaceMeshTimer, this, &ARageEnemyCharacter::OnRep_TransformToChest, 2.0f, false);
}

void ARageEnemyCharacter::ReplaceMeshWithChest()
{
    if (!IsValid(this) || !ChestMeshAsset) return;

    // ���� ���̷�Ż �޽� ����
    GetMesh()->SetVisibility(false, true);

    // ���� ���� �޽� ���� �� ����
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

            // ���� �ð� �� �ӵ� ����
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

    // ���� ��ġ ����
    GetWorldTimerManager().SetTimerForNextTick([this, OriginalLocation]() {
        FTimerHandle RestoreHandle;
        GetWorldTimerManager().SetTimer(RestoreHandle, [this, OriginalLocation]() {
            GetMesh()->SetRelativeLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
            }, 0.48f, false);
        });
}

// === �κ��丮 �ý��� (�⺻ ���ʹ̿� ����) ===
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

            // �� �÷��̾��� �κ��丮�� �ݱ�
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

    // ������ ���ʹ̴� �� ���� ������ ���
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
        NewItemData.Price = DefaultItem->Price; // �⺻ ���� ���

        // ��޺� ���� ����
        if (DefaultItem->IsA(AArmor::StaticClass()))
        {
            int32 GradeRoll = FMath::RandRange(0, 99);
            if (GradeRoll < 60)
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::C);
                NewItemData.Price = DefaultItem->Price; // C���: �⺻ ����
            }
            else if (GradeRoll < 90)
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::B);
                NewItemData.Price = DefaultItem->Price * 2; // B���: 2��
            }
            else
            {
                NewItemData.Grade = static_cast<uint8>(EArmorGrade::A);
                NewItemData.Price = DefaultItem->Price * 3; // A���: 3��
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
            // ����/���� �ƴ� ��� �⺻ ���� ���
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

// === ������ ��� �ý��� ===
void ARageEnemyCharacter::CheckRageMode()
{
    float HealthPercent = CurrentHealth / MaxHealth;

    // HP�� 30% �����̰� ���� ������ ��尡 �ƴ϶��
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

    // === ������ ���� ���� ===
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        //MoveComp->DisableMovement(); // ������ ���� ��Ȱ��ȭ
        MoveComp->MaxWalkSpeed = 0.0f;
    }

    
 
   

    // �޽� ���� �������� ������ ��� ǥ�� (������ ƾƮ)
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVectorParameterValueOnMaterials(FName("TintColor"), FVector(1.0f, 0.3f, 0.3f));
    }

    // ������ ��� �ִϸ��̼� ���
    if (RageEnterMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        AnimInstance->Montage_Play(RageEnterMontage, 1.0f);
        
        // ��Ÿ�� ���� �� �ݹ� ����
        FOnMontageEnded RageMontageEndedDelegate;
        RageMontageEndedDelegate.BindUObject(this, &ARageEnemyCharacter::OnRageMontageEnded);
        AnimInstance->Montage_SetEndDelegate(RageMontageEndedDelegate, RageEnterMontage);

    }
}

// ������ ��Ÿ�� ���� �� ȣ��Ǵ� �Լ�
void ARageEnemyCharacter::OnRageMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == RageEnterMontage)
    {
        bIsPlayingRageMontage = false;

        // ������ ��Ȱ��ȭ
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            MoveComp->SetMovementMode(MOVE_Walking);
            // �̵� �ӵ� ����
           
                MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed * RageSpeedMultiplier;
           
        }

        

        UE_LOG(LogTemp, Warning, TEXT("Rage montage ended, movement and AI re-enabled"));
    }
}

// FSM�̳� AI���� �������� ����ϱ� ���� üũ�ϴ� �Լ��鵵 ���� �ʿ�
bool ARageEnemyCharacter::CanMove() const
{
    return !bIsStunned && !bIsAttacking && !bIsPlayingRageMontage;
}

void ARageEnemyCharacter::ExitRageMode()
{
    if (!bIsInRageMode) return;

    bIsInRageMode = false;
    UE_LOG(LogTemp, Warning, TEXT("RageEnemy exited rage mode"));

    // �̵� �ӵ� �������
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = OriginalMaxWalkSpeed;
    }

    // �޽� ���� �������
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        MeshComp->SetVectorParameterValueOnMaterials(FName("TintColor"), FVector(1.0f, 1.0f, 1.0f));
    }
}

// === ���� ���� �ý��� ===
void ARageEnemyCharacter::TryJumpAttack()
{
    if (!bCanJumpAttack || bIsStunned || !bIsInRageMode) return;

    // �÷��̾���� �Ÿ� üũ
    AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (!Player) return;

    float DistanceToPlayer = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

    // ��Ÿ� ���� �ְ� ������ ����� ���� ���� ����
    if (DistanceToPlayer <= JumpAttackRange)
    {
        PerformJumpAttack();
    }
}

void ARageEnemyCharacter::PerformJumpAttack()
{
    if (!JumpAttackMontage || !GetMesh() || !GetMesh()->GetAnimInstance()) return;

    UE_LOG(LogTemp, Warning, TEXT("RageEnemy performing jump attack!"));

    // ���� ���� ��ٿ� ����
    bCanJumpAttack = false;
    GetWorldTimerManager().SetTimer(JumpAttackCooldownTimer, this,
        &ARageEnemyCharacter::OnJumpAttackCooldownEnd, JumpAttackCooldown, false);

    // ���� ���� �ִϸ��̼� ���
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    AnimInstance->Montage_Play(JumpAttackMontage, 1.2f);

    // �÷��̾� �������� ����
    AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    if (Player)
    {
        FVector JumpDirection = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        FVector JumpVelocity = JumpDirection * 600.0f + FVector(0, 0, 400.0f);
        LaunchCharacter(JumpVelocity, true, true);

        // ���� �� ������ ó���� ���� Ÿ�̸� (�ִϸ��̼� �ð��� ���� ����)
        FTimerHandle JumpDamageTimer;
        GetWorldTimerManager().SetTimer(JumpDamageTimer, [this, Player]()
            {
                // ���� ���� �ֺ��� ���鿡�� ������
                TArray<FHitResult> HitResults;
                TArray<AActor*> IgnoredActors;
                IgnoredActors.Add(this);

                UKismetSystemLibrary::SphereTraceMulti(
                    GetWorld(),
                    GetActorLocation(),
                    GetActorLocation() + FVector(0, 0, -50),
                    150.0f, // ���� ���� ����
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
            }, 1.0f, false); // 1�� �� ���� ������
    }
}

void ARageEnemyCharacter::OnJumpAttackCooldownEnd()
{
    bCanJumpAttack = true;
    UE_LOG(LogTemp, Log, TEXT("RageEnemy jump attack cooldown ended"));
}

void ARageEnemyCharacter::PlayRageAttackMontage()
{
    // ������ ��� ���� ���� ������ �ʿ��ϴٸ� ���⿡ ����
    // ����� �⺻ ������ �� ������ ����ϴ� ������� ������
    PlayAttackMontage();
}