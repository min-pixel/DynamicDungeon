// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyFSMComponent.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Roaming     UMETA(DisplayName = "Roaming"),
	Chasing     UMETA(DisplayName = "Chasing"),
	Attacking   UMETA(DisplayName = "Attacking"),
	Dead        UMETA(DisplayName = "Dead")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT03_API UEnemyFSMComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEnemyFSMComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSM")
	EEnemyState CurrentState = EEnemyState::Idle;

	void SetState(EEnemyState NewState);

	UPROPERTY(EditDefaultsOnly, Category = "FSM")
	float idleDelayTime = 2;

	float currentTime = 0;

	UPROPERTY(VisibleAnywhere, Category = "FSM")
	class AMyDCharacter* target;

	UPROPERTY(VisibleAnywhere, Category = "FSM")
	class AActor* me;


	UPROPERTY(EditDefaultsOnly, Category = "FSM")
	float moveDelayTime = 1;

	UPROPERTY(EditAnywhere, Category = "Sight")
	float SightRadius = 2000.0f; // 감지 거리

	UPROPERTY(EditAnywhere, Category = "Sight")
	float SightAngle = 180.0f;

	bool CanSeePlayer(FVector& OutToPlayer, float& OutDistance);

	UPROPERTY(EditAnywhere, Category = "FSM")
	float AttackRange = 150.f;

	UPROPERTY(EditAnywhere, Category = "FSM")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, Category = "FSM|Sight")
	float ChaseMemoryTime = 15.0f;

	float TimeSinceLastSeen = 0.0f;
	bool bCurrentlySeeingPlayer = false;

	UPROPERTY()
	FVector LastKnownPlayerLocation;

private:
	void HandleIdleState();
	void HandleRoamingState();
	void HandleChasingState();
	void HandleAttackingState();
	//void HandleDeadState();

	// 헬퍼 함수들 추가
	void CallPlayAttackMontage();
	bool IsEnemyStunned() const;
	AController* GetEnemyController() const;
	APawn* GetEnemyPawn() const;
};
