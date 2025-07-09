// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyFSMComponent.h"
#include "MyDCharacter.h"
#include "EnemyCharacter.h"
#include <Kismet/GameplayStatics.h>
#include "EngineUtils.h"
#include "DrawDebugHelpers.h" // (����� �� �׸��� �뵵)
#include "Engine/World.h"
#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "RageEnemyCharacter.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UEnemyFSMComponent::UEnemyFSMComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}


// Called when the game starts
void UEnemyFSMComponent::BeginPlay()
{
	Super::BeginPlay();

    auto actor = UGameplayStatics::GetActorOfClass(GetWorld(), AMyDCharacter::StaticClass());
    target = Cast<AMyDCharacter>(actor);

    me = GetOwner();
	
}


// Called every frame
void UEnemyFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    

    if (me && IsEnemyStunned()) return;

    currentTime += DeltaTime;

    switch (CurrentState)
    {
    case EEnemyState::Idle:
        HandleIdleState();
        break;
    case EEnemyState::Roaming:
        HandleRoamingState();
        break;
    case EEnemyState::Chasing:
        HandleChasingState();
        break;
    case EEnemyState::Attacking:
        HandleAttackingState();
        break;
    case EEnemyState::Dead:
        // ��� ���¸� �ƹ� �͵� �� ��
        break;
    }

    // EnemyFSMComponent.cpp �Ǵ� EnemyCharacter.cpp���� Tick ����
    FVector Start = me->GetActorLocation() + FVector(0, 0, 50); // �� ��ġ ����
    FVector Forward = me->GetActorForwardVector();

    // �þ� �� �ð�ȭ
    float HalfSightAngle = SightAngle * 0.5f;
    float SightLength = SightRadius;

    // ���� �þ� �� ���� ���
    FVector LeftDir = Forward.RotateAngleAxis(-HalfSightAngle, FVector::UpVector);
    FVector RightDir = Forward.RotateAngleAxis(HalfSightAngle, FVector::UpVector);

    /*DrawDebugLine(GetWorld(), Start, Start + LeftDir * SightLength, FColor::Green, false, 0.1f);
    DrawDebugLine(GetWorld(), Start, Start + RightDir * SightLength, FColor::Green, false, 0.1f);
    DrawDebugLine(GetWorld(), Start, Start + Forward * SightLength, FColor::Yellow, false, 0.1f);*/

	// ...
}

void UEnemyFSMComponent::HandleIdleState()
{
    FVector ToPlayer;
    float Distance;

    SightAngle = 180.0f;

    if (CanSeePlayer(ToPlayer, Distance))
    {
        SetState(EEnemyState::Chasing);
        currentTime = 0;
        return;
    }

    if (currentTime > idleDelayTime)
    {
        SetState(EEnemyState::Roaming);
        currentTime = 0;
    }
}

void UEnemyFSMComponent::HandleChasingState()
{
    // ������ �� ������ ����
    if (!CanEnemyMove())
    {
        return;
    }

    FVector ToPlayer;
    float Distance;

    SightAngle = 360.0f;

    if (!IsValid(target) || target->IsDead())
    {
        SetState(EEnemyState::Roaming);
        target = nullptr;
        return;
    }

    if (!CanSeePlayer(ToPlayer, Distance))
    {
        TimeSinceLastSeen += GetWorld()->GetDeltaSeconds();
        if (TimeSinceLastSeen > ChaseMemoryTime)
        {
            SetState(EEnemyState::Idle);
            return;
        }

        // �þ߿� �� ���� �� ������ ��ġ�� �̵�
        if (CanEnemyMove())  // �� �� �� üũ
        {
            if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetEnemyController()))
            {
                AI->MoveToLocation(LastKnownPlayerLocation, 5.0f);
            }
        }
        return;
    }

    // ���� �� ��ġ ����
    LastKnownPlayerLocation = target->GetActorLocation();
    TimeSinceLastSeen = 0;

    // ���� ������ ����
    if (Distance < AttackRange)
    {
        SetState(EEnemyState::Attacking);
        return;
    }

    // �̵�
    if (CanEnemyMove())  // �̵� �� üũ
    {
        if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetEnemyController()))
        {
            AI->MoveToActor(target, 5.0f);
        }
    }
}

bool UEnemyFSMComponent::CanSeePlayer(FVector& OutToPlayer, float& OutDistance)
{
    if (!me) return false;

    AMyDCharacter* ClosestPlayer = nullptr;
    float ClosestDistance = SightRadius;
    FVector ClosestDirection;

    for (TActorIterator<AMyDCharacter> It(GetWorld()); It; ++It)
    {
        AMyDCharacter* Player = *It;
        if (!Player || Player->IsPendingKillPending()) continue;

        if (Player->IsDead()) continue;

        FVector ToPlayer = Player->GetActorLocation() - me->GetActorLocation();
        float Distance = ToPlayer.Size();
        if (Distance > SightRadius) continue;

        FVector Forward = me->GetActorForwardVector();
        FVector ToPlayerDir = ToPlayer.GetSafeNormal();

        float Dot = FVector::DotProduct(Forward, ToPlayerDir);
        float Angle = FMath::Acos(Dot) * (180.0f / PI);

        if (Angle <= SightAngle * 0.5f)
        {
            // ����Ʈ���̽��� �þ� ���� üũ �߰�
            FHitResult Hit;
            FVector Start = me->GetActorLocation() + FVector(0, 0, 50); // �� ���� ����
            FVector End = Player->GetActorLocation() + FVector(0, 0, 50);

            bool bHit = GetWorld()->LineTraceSingleByChannel(
                Hit,
                Start,
                End,
                ECC_Visibility
            );

            // ����� �� (����)
            //DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.2f);

            if (bHit && Hit.GetActor() != Player)
            {
                // ���� ���̿� ������ ���� (�þ߰� ��������)
                //UE_LOG(LogTemp, Warning, TEXT("NO EYE: %s"), *GetNameSafe(Hit.GetActor()));
                if (Distance > 300.0f) 
                {
                    continue;
                }
            }

            // �� �÷��̾ �� ������ ����
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestPlayer = Player;
                ClosestDirection = ToPlayer;
            }
        }
    }

    if (ClosestPlayer)
    {
        target = ClosestPlayer;
        //LastKnownPlayerLocation = ClosestPlayer->GetActorLocation();
        OutToPlayer = ClosestDirection;
        OutDistance = ClosestDistance;
        return true;
    }

    target = nullptr;
    return false;
}


void UEnemyFSMComponent::HandleAttackingState()
{
    if (!me || !target) return;

    if (!IsValid(target) || target->IsDead())
    {
        SetState(EEnemyState::Roaming);
        target = nullptr;
        return;
    }

    SightAngle = 360.0f;

    float Distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
    
    if (Distance > AttackRange)
    {
        SetState(EEnemyState::Chasing);
        return;
    }

    if (currentTime >= AttackCooldown)
    {
        if (me)
        {
            CallPlayAttackMontage();  // �� ĳ���� Ŭ�������� ���� ���
            UE_LOG(LogTemp, Log, TEXT("Enemy attacks player!"));
        }

        currentTime = 0;
    }
}

void UEnemyFSMComponent::SetState(EEnemyState NewState)
{
    if (CurrentState == EEnemyState::Dead) return; // ���� ���¸� ���� �Ұ�
    CurrentState = NewState;


    currentTime = 0; // ���� �ٲ� �� Ÿ�̸� �ʱ�ȭ

    if (NewState == EEnemyState::Attacking)
    {
        currentTime = AttackCooldown; // ��Ÿ�� �Ѱܼ� �ٷ� �ߵ�
    }
   
}

void UEnemyFSMComponent::HandleRoamingState()
{
    if (!me) return;

    // ������ �� ������ ����
    if (!CanEnemyMove())
    {
        return;
    }

    SightAngle = 180.0f;

    FVector ToPlayer;
    float Distance;
    if (CanSeePlayer(ToPlayer, Distance))
    {
        SetState(EEnemyState::Chasing);
        return;
    }

    AEnemyAIController* AI = Cast<AEnemyAIController>(GetEnemyController());
    if (!AI) return;

    EPathFollowingStatus::Type MoveStatus = AI->GetMoveStatus();
    if (MoveStatus == EPathFollowingStatus::Moving)
    {
        return;
    }

    // ���ο� ������ ���� (������ �� ���� ����)
    if (CanEnemyMove())
    {
        UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        if (NavSys)
        {
            FVector Origin = me->GetActorLocation();
            FNavLocation RandomLocation;

            for (int i = 0; i < 10; ++i)
            {
                float Radius = FMath::FRandRange(3000.f, 4000.f);
                if (NavSys->GetRandomReachablePointInRadius(Origin, Radius, RandomLocation))
                {
                    float ActualDist = FVector::Dist(Origin, RandomLocation.Location);
                    if (ActualDist >= 3000.f && ActualDist <= 4000.f)
                    {
                        AI->MoveToLocation(RandomLocation.Location, 5.0f);
                        break;
                    }
                }
            }
        }
    }

    currentTime = 0;
}

// ���� �Լ��� ����
void UEnemyFSMComponent::CallPlayAttackMontage()
{
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(me))
    {
        Enemy->PlayAttackMontage();
    }
    else if (ARageEnemyCharacter* RageEnemy = Cast<ARageEnemyCharacter>(me))
    {
        RageEnemy->PlayAttackMontage();
    }
}

bool UEnemyFSMComponent::IsEnemyStunned() const
{
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(me))
    {
        return Enemy->bIsStunned;
    }
    else if (ARageEnemyCharacter* RageEnemy = Cast<ARageEnemyCharacter>(me))
    {
        return RageEnemy->bIsStunned; // RageEnemy�� �ִ� ���� ���������� �ٲ��ּ���
    }
    return false;
}

AController* UEnemyFSMComponent::GetEnemyController() const
{
    if (APawn* Pawn = Cast<APawn>(me))
    {
        return Pawn->GetController();
    }
    return nullptr;
}

APawn* UEnemyFSMComponent::GetEnemyPawn() const
{
    return Cast<APawn>(me);
}


// ������ �� �ִ��� üũ�ϴ� ���� �Լ�
bool UEnemyFSMComponent::CanEnemyMove() const
{
    // �⺻ ���׹� üũ
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(me))
    {
        return !Enemy->bIsStunned;  // ���� ���°� �ƴ� ���� ������
    }
    // ������ ���׹� üũ
    else if (ARageEnemyCharacter* RageEnemy = Cast<ARageEnemyCharacter>(me))
    {
        // ������ ���׹̴� �� ���� ���� üũ
        return RageEnemy->CanMove();  // RageEnemy�� CanMove() �Լ� Ȱ��
    }

    return true;  // �⺻��
}

