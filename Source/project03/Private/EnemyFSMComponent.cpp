// Fill out your copyright notice in the Description page of Project Settings.

#include "MyDCharacter.h"
#include "EnemyCharacter.h"
#include "EnemyFSMComponent.h"
#include <Kismet/GameplayStatics.h>
#include "EngineUtils.h"
#include "DrawDebugHelpers.h" // (����� �� �׸��� �뵵)
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
        // ��� ���¸� �ƹ� �͵� �� ��
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
        SetState(EEnemyState::Idle);  // �þ߿��� ��ħ
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
        me->AddMovementInput(ToPlayer.GetSafeNormal());  // �þ� üũ���� �̹� ���� ���� ���� ��Ȱ��
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
            DrawDebugLine(GetWorld(), Start, End, FColor::Yellow, false, 0.2f);

            if (bHit && Hit.GetActor() != Player)
            {
                // ���� ���̿� ������ ���� (�þ߰� ��������)
                UE_LOG(LogTemp, Warning, TEXT("NO EYE: %s"), *GetNameSafe(Hit.GetActor()));
                continue;
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
        // ���� �ִϸ��̼� (�켱 �α׸�)
        UE_LOG(LogTemp, Log, TEXT("Enemy attacks player!"));


        currentTime = 0;
    }
}

void UEnemyFSMComponent::SetState(EEnemyState NewState)
{
    if (CurrentState == EEnemyState::Dead) return; // ���� ���¸� ���� �Ұ�
    CurrentState = NewState;
    currentTime = 0; // ���� �ٲ� �� Ÿ�̸� �ʱ�ȭ
}

//void UEnemyFSMComponent::HandleRoamingState()
//{
//    // ���߿� ���� ��ũ ���� ����
//}