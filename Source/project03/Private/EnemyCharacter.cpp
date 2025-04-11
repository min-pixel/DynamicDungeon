// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnemyAIController.h"
#include "HitInterface.h"
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

}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
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
        EDrawDebugTrace::ForDuration,
        HitResults,
        true
    );

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {
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