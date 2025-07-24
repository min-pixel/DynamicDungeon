// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitInterface.h"
#include "GameFramework/Character.h"
#include "InventoryComponent.h"
#include "InventoryWidget.h"
#include "TreasureGlowEffect.h"
#include "Components/BoxComponent.h"
#include "RageEnemyCharacter.generated.h"

UCLASS()
class PROJECT03_API ARageEnemyCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	ARageEnemyCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// === 기본 FSM 컴포넌트 ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSMComponent")
	class UEnemyFSMComponent* fsm;

	// === 기본 공격 시스템 ===
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* RageEnterMontage;

	void PlayAttackMontage();
	void TraceAttack();
	void StartTrace();

	int32 AttackComboIndex = 0;
	FVector LastStartLocation;
	FVector LastEndLocation;
	TArray<AActor*> HitActors;

	// === 체력 시스템 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 150.0f; // 기본 에너미보다 높게

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_Dead();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// === 멀티플레이 관련 변수 추가 ===
	UPROPERTY(Replicated)
	bool bIsTransformedToChest = false;

	UPROPERTY()
	class UStaticMeshComponent* ChestStaticMesh;

	UPROPERTY()
	class UStaticMesh* ChestMeshAsset;

	// === 멀티플레이 함수 추가 ===
	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivateRagdoll();

	UFUNCTION()
	void OnRep_TransformToChest();

	void ReplaceMeshWithChest();

	UPROPERTY()
	class UNiagaraSystem* TreasureGlowEffectAsset;

	// === 피격 처리 ===
	virtual void GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage) override;
	virtual void ApplyDebuff_Implementation(EDebuffType DebuffType, float Value, float Duration) override;

	void HandleStun();
	FTimerHandle StunTimerHandle;
	bool bIsStunned = false;
	void PlayHitShake();

	// === 인벤토리 시스템 ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UInventoryComponent* EnemyInventory;

	UPROPERTY(VisibleAnywhere)
	UBoxComponent* InteractionBox;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OpenLootUI(class AMyDCharacter* InteractingPlayer);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Loot")
	UInventoryComponent* LootInventory;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	UInventoryWidget* LootInventoryWidgetInstance;

	void GenerateRandomLoot();
	TArray<TSubclassOf<AItem>> PossibleItems;

	// === 레이지 모드 관련 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageThreshold = 0.3f; // HP 30% 이하에서 레이지 모드

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageSpeedMultiplier = 1.8f; // 레이지 모드 시 속도 증가

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageDamageMultiplier = 1.5f; // 레이지 모드 시 데미지 증가

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rage System")
	bool bIsInRageMode = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsPlayingRageMontage = false;

	// === 점프 공격 관련 ===
	UPROPERTY(EditDefaultsOnly, Category = "Jump Attack")
	UAnimMontage* JumpAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackRange = 800.0f; // 점프 공격 사거리

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackDamage = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackCooldown = 8.0f; // 쿨다운 시간

	bool bCanJumpAttack = true;
	FTimerHandle JumpAttackCooldownTimer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// === 레이지 모드 함수들 ===
	void CheckRageMode();
	void EnterRageMode();
	void ExitRageMode();

	// === 점프 공격 함수들 ===
	void TryJumpAttack();
	void PerformJumpAttack();
	void OnJumpAttackCooldownEnd();

	void PlayRageAttackMontage(); // 레이지 모드 전용 공격 패턴

	void OnRageMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	bool CanMove() const;

private:
	float OriginalMaxWalkSpeed; // 원래 속도 저장용
};
