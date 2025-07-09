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
    // 움직일 수 없으면 리턴
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

        // 시야에 안 보일 때 마지막 위치로 이동
        if (CanEnemyMove())  // 한 번 더 체크
        {
            if (AEnemyAIController* AI = Cast<AEnemyAIController>(GetEnemyController()))
            {
                AI->MoveToLocation(LastKnownPlayerLocation, 5.0f);
            }
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
    if (CanEnemyMove())  // 이동 전 체크
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
                //UE_LOG(LogTemp, Warning, TEXT("NO EYE: %s"), *GetNameSafe(Hit.GetActor()));
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
            CallPlayAttackMontage();  // 적 캐릭터 클래스에서 직접 재생
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

    // 움직일 수 없으면 리턴
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

    // 새로운 목적지 생성 (움직일 수 있을 때만)
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

// 헬퍼 함수들 구현
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
        return RageEnemy->bIsStunned; // RageEnemy에 있는 실제 변수명으로 바꿔주세요
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


// 움직일 수 있는지 체크하는 범용 함수
bool UEnemyFSMComponent::CanEnemyMove() const
{
    // 기본 에네미 체크
    if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(me))
    {
        return !Enemy->bIsStunned;  // 스턴 상태가 아닐 때만 움직임
    }
    // 레이지 에네미 체크
    else if (ARageEnemyCharacter* RageEnemy = Cast<ARageEnemyCharacter>(me))
    {
        // 레이지 에네미는 더 많은 조건 체크
        return RageEnemy->CanMove();  // RageEnemy의 CanMove() 함수 활용
    }

    return true;  // 기본값
}

