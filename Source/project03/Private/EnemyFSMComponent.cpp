// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyFSMComponent.h"
#include "MyDCharacter.h"
#include "EnemyCharacter.h"
#include <Kismet/GameplayStatics.h>
#include "EngineUtils.h"
#include "DrawDebugHelpers.h" // (디버그 선 그리기 용도)
#include "Engine/World.h"
#include "EnemyAIController.h"
#include "EnemyCharacter.h"
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

    me = Cast<AEnemyCharacter>(GetOwner());
	// ...
	
}


// Called every frame
void UEnemyFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (me && me->bIsStunned) return;

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
        // 사망 상태면 아무 것도 안 함
        break;
    }

    // EnemyFSMComponent.cpp 또는 EnemyCharacter.cpp에서 Tick 내에
    FVector Start = me->GetActorLocation() + FVector(0, 0, 50); // 눈 위치 보정
    FVector Forward = me->GetActorForwardVector();

    // 시야 각 시각화
    float HalfSightAngle = SightAngle * 0.5f;
    float SightLength = SightRadius;

    // 양쪽 시야 끝 방향 계산
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

        // 시야에 안 보일 땐 마지막 위치로 이동
        if (AEnemyAIController* AI = Cast<AEnemyAIController>(me->GetController()))
        {
            AI->MoveToLocation(LastKnownPlayerLocation, 5.0f);
        }

        return;
    }

    // 보임 → 위치 갱신
    LastKnownPlayerLocation = target->GetActorLocation();
    TimeSinceLastSeen = 0;

    // 공격 범위면 공격
    if (Distance < AttackRange)
    {
        SetState(EEnemyState::Attacking);
        return;
    }

    // 이동
    if (AEnemyAIController* AI = Cast<AEnemyAIController>(me->GetController()))
    {
        AI->MoveToActor(target, 5.0f);
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
            // 라인트레이스로 시야 막힘 체크 추가
            FHitResult Hit;
            FVector Start = me->GetActorLocation() + FVector(0, 0, 50); // 눈 높이 보정
            FVector End = Player->GetActorLocation() + FVector(0, 0, 50);

            bool bHit = GetWorld()->LineTraceSingleByChannel(
                Hit,
                Start,
                End,
                ECC_Visibility
            );

            // 디버그 선 (선택)
            //DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.2f);

            if (bHit && Hit.GetActor() != Player)
            {
                // 뭔가 사이에 있으면 무시 (시야가 가려졌음)
                UE_LOG(LogTemp, Warning, TEXT("NO EYE: %s"), *GetNameSafe(Hit.GetActor()));
                if (Distance > 300.0f) 
                {
                    continue;
                }
            }

            // 이 플레이어가 더 가까우면 갱신
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
            me->PlayAttackMontage();  // 적 캐릭터 클래스에서 직접 재생
            UE_LOG(LogTemp, Log, TEXT("Enemy attacks player!"));
        }

        currentTime = 0;
    }
}

void UEnemyFSMComponent::SetState(EEnemyState NewState)
{
    if (CurrentState == EEnemyState::Dead) return; // 죽은 상태면 변경 불가
    CurrentState = NewState;


    currentTime = 0; // 상태 바뀔 때 타이머 초기화

    if (NewState == EEnemyState::Attacking)
    {
        currentTime = AttackCooldown; // 쿨타임 넘겨서 바로 발동
    }
   
}

void UEnemyFSMComponent::HandleRoamingState()
{
    if (!me) return;

    SightAngle = 180.0f;

    FVector ToPlayer;
    float Distance;
    if (CanSeePlayer(ToPlayer, Distance))
    {
        SetState(EEnemyState::Chasing);
        return;
    }

    AEnemyAIController* AI = Cast<AEnemyAIController>(me->GetController());
    if (!AI) return;

    EPathFollowingStatus::Type MoveStatus = AI->GetMoveStatus();
    if (MoveStatus == EPathFollowingStatus::Moving)
    {
        // 이동 중이면 새로운 목적지 생성하지 않음
        return;
    }

    // 도달했거나 멈췄다면 새로운 목적지 생성
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if (NavSys)
    {
        FVector Origin = me->GetActorLocation();
        FNavLocation RandomLocation;

        // 목적지까지 거리 제한: 최소 3000 ~ 최대 4000
        for (int i = 0; i < 10; ++i) // 최대 10번 시도
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

    currentTime = 0;
}