// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include "RageEnemyCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "HitInterface.h"
#include "MyDCharacter.h"
#include "InventoryWidget.h"
#include "ItemDataD.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TreasureGlowEffect.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

   

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMeshAsset(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog"));
    if (EnemyMeshAsset.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(EnemyMeshAsset.Object);
        // �޽� ��ġ ���� (�⺻������ ĸ�� ������Ʈ�� �߾ӿ� ��ġ�ϱ� ������ �Ʒ��� ������)
        GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));

        // �޽� ȸ�� ���� (�޽ð� Y���� �������� �� ��� -90�� ȸ��)
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


    GetCharacterMovement()->bOrientRotationToMovement = true; // �̵� ������ �ٶ󺸰�
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
    
    LootInventory->SetIsReplicated(true);
    
    static ConstructorHelpers::FClassFinder<UInventoryWidget> WidgetBPClass(TEXT("/Game/BP/UI/InventoryWidget.InventoryWidget_C"));
    if (WidgetBPClass.Succeeded())
    {
        InventoryWidgetClass = WidgetBPClass.Class;
    }
    
    static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathMontageAsset(TEXT("/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Death_front_Montage.Death_front_Montage"));
    if (DeathMontageAsset.Succeeded())
    {
        DeathMontage = DeathMontageAsset.Object;
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

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
    CurrentHealth = MaxHealth;

    if (HasAuthority())
    {
       GenerateRandomLoot();
    }
   
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

    // ���� �̸��� �޺� �ε����� ����
    FName SelectedSection = (AttackComboIndex == 0) ? FName("Attack1") : FName("Attack2");


    // ��Ÿ�� ��� �� ���� ����
    AnimInstance->Montage_Play(AttackMontage, 1.0f);
    AnimInstance->Montage_JumpToSection(SelectedSection, AttackMontage);

    UE_LOG(LogTemp, Warning, TEXT("Enemy Playing Attack Section: %s"), *SelectedSection.ToString());

    // �޺� �ε��� ���� (������ �ݴ� ����)
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

            // �������� �������� �ʵ��� üũ
            if (Cast<AEnemyCharacter>(HitActor) || Cast<ARageEnemyCharacter>(HitActor))
            {
                continue; // ��� �� Ÿ�� ��ŵ
            }

            if (!HitActors.Contains(HitActor)) // �ߺ� ����
            {
                HitActors.Add(HitActor);
                IHitInterface::Execute_GetHit(HitActor, Hit, this, 20.0f); // ���� ������ ���� �����Ӱ� ����
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

    HitActors.Empty(); // �ߺ� ������ �ʱ�ȭ
}


void AEnemyCharacter::GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage)
{
    if (bIsStunned) return;

    CurrentHealth -= Damage;
    UE_LOG(LogTemp, Log, TEXT("Enemy took damage: %f | Current HP: %f"), Damage, CurrentHealth);

    AMyDCharacter* PlayerPawn = Cast<AMyDCharacter>(InstigatorActor);
    if (HasAuthority() && PlayerPawn)
    {
        // ���� �� �ش� �÷��̾� Ŭ���̾�Ʈ������ ���� ���
        PlayerPawn->ClientPlayHitSoundAtLocation(GetActorLocation());
    }

    HandleStun(); // ��� ����

    //if (CurrentHealth <= 0)
    //{
    //    if (fsm)
    //    {
    //        fsm->SetState(EEnemyState::Dead); // FSM ���¸� Dead�� ����
    //    }

    //    // ��� ó��
    //    
    //    //SetActorEnableCollision(false);
    //    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    //    GetMesh()->SetSimulatePhysics(true);
    //    //GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

    //    SetActorEnableCollision(true);
    //    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    //    Tags.Add(FName("Chest"));
    //    
    //}

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


void AEnemyCharacter::OnRep_Dead()
{
    if (fsm)
    {
        fsm->SetState(EEnemyState::Dead);
    }

   /* if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        AnimInstance->Montage_Play(DeathMontage);
    }*/
    
}

void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AEnemyCharacter, bIsDead);
    DOREPLIFETIME(AEnemyCharacter, LootInventory);
    DOREPLIFETIME(AEnemyCharacter, bIsTransformedToChest);
}

void AEnemyCharacter::MulticastActivateRagdoll_Implementation()
{

    /*if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        AnimInstance->Montage_Play(DeathMontage);
    }*/

    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    SetActorEnableCollision(true);
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Tags.Add(FName("Chest"));

    bIsTransformedToChest = true;
    

    // ���� �ð� �� �������� �޽��� ����
    FTimerHandle ReplaceMeshTimer;
    GetWorldTimerManager().SetTimer(ReplaceMeshTimer, this, &AEnemyCharacter::OnRep_TransformToChest, 2.0f, false);
}

void AEnemyCharacter::ReplaceMeshWithChest()
{
    if (!IsValid(this) || !ChestMeshAsset) return;

    // �� ���̷�Ż �޽� ����
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

void AEnemyCharacter::OnRep_TransformToChest()
{
    ReplaceMeshWithChest();
}


void AEnemyCharacter::ApplyDebuff_Implementation(EDebuffType DebuffType, float Value, float Duration)
{
    UE_LOG(LogTemp, Warning, TEXT("[EnemyCharacter] ApplyDebuff: %s, Value: %.2f, Duration: %.2f"),
        *UEnum::GetValueAsString(DebuffType), Value, Duration);

    switch (DebuffType)
    {
    case EDebuffType::Slow:
        if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
        {
            float OriginalSpeed = MoveComp->MaxWalkSpeed;
            MoveComp->MaxWalkSpeed *= (1.f - Value); // ��: 0.3 �̸� 70% �ӵ�
            UE_LOG(LogTemp, Warning, TEXT("Enemy slowed to %.2f for %.2f seconds"), MoveComp->MaxWalkSpeed, Duration);

            // ���� �ð� �� �ӵ� ����
            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(TimerHandle, [MoveComp, OriginalSpeed]()
                {
                    MoveComp->MaxWalkSpeed = OriginalSpeed;
                    UE_LOG(LogTemp, Warning, TEXT("Enemy slow debuff ended. Speed restored to %.2f"), OriginalSpeed);
                }, Duration, false);
        }
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown debuff type."));
        break;
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

        }, 0.45f, false);
}

void AEnemyCharacter::PlayHitShake()
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
            if (!IsValid(this) || !GetMesh()) return;
            GetMesh()->SetRelativeLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
            }, 0.48f, false);
        });
}

void AEnemyCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

void AEnemyCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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
            //UE_LOG(LogTemp, Log, TEXT("Closed enemy loot UI"));

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

void AEnemyCharacter::OpenLootUI(AMyDCharacter* InteractingPlayer)
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

        // �÷��̾� �κ��丮 ���� ����
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

    int32 ItemCount = FMath::RandRange(3, 10); // ������ ������ ����

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

        // ���� ����ü�� �迭�� ����
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