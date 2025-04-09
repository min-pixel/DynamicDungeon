// Fill out your copyright notice in the Description page of Project Settings.

#include "MyDCharacter.h"
#include "EnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include <Kismet/GameplayStatics.h>
#include "EngineUtils.h"
#include "DrawDebugHelpers.h" // (디버그 선 그리기 용도)
#include "Engine/World.h"
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

    currentTime += DeltaTime;

    switch (CurrentState)
    {
    case EEnemyState::Idle:
        HandleIdleState();
        break;
    case EEnemyState::Roaming:
        //HandleRoamingState();
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
	// ...
}

void UEnemyFSMComponent::HandleIdleState()
{
    FVector ToPlayer;
    float Distance;

    if (CanSeePlayer(ToPlayer, Distance))
    {
        SetState(EEnemyState::Chasing);
        currentTime = 0;
        return;
    }

    if (currentTime > idleDelayTime)
    {
        SetState(EEnemyState::Idle);
        currentTime = 0;
    }
}

void UEnemyFSMComponent::HandleChasingState()
{
    FVector ToPlayer;
    float Distance;

    if (!CanSeePlayer(ToPlayer, Distance))
    {
        SetState(EEnemyState::Idle);  // 시야에서 놓침
        currentTime = 0;
        return;
    }

    if (Distance < AttackRange)
    {
        SetState(EEnemyState::Attacking);
        currentTime = 0;
        return;
    }

    if (me)
    {
        UE_LOG(LogTemp, Warning, TEXT("findfindfind"));
        me->AddMovementInput(ToPlayer.GetSafeNormal());  // 시야 체크에서 이미 얻은 방향 벡터 재활용
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
            DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.2f);

            if (bHit && Hit.GetActor() != Player)
            {
                // 뭔가 사이에 있으면 무시 (시야가 가려졌음)
                UE_LOG(LogTemp, Warning, TEXT("NO EYE: %s"), *GetNameSafe(Hit.GetActor()));
                continue;
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

    float Distance = FVector::Dist(me->GetActorLocation(), target->GetActorLocation());
    
    if (Distance > AttackRange)
    {
        SetState(EEnemyState::Chasing);
        return;
    }

    if (currentTime >= AttackCooldown)
    {
        // 공격 애니메이션 (우선 로그만)
        UE_LOG(LogTemp, Log, TEXT("Enemy attacks player!"));


        currentTime = 0;
    }
}

void UEnemyFSMComponent::SetState(EEnemyState NewState)
{
    if (CurrentState == EEnemyState::Dead) return; // 죽은 상태면 변경 불가
    CurrentState = NewState;
    currentTime = 0; // 상태 바뀔 때 타이머 초기화
}

//void UEnemyFSMComponent::HandleRoamingState()
//{
//    // 나중에 랜덤 워크 구현 예정
//}