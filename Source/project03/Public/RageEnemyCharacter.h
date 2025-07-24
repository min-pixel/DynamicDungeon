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

	// === �⺻ FSM ������Ʈ ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSMComponent")
	class UEnemyFSMComponent* fsm;

	// === �⺻ ���� �ý��� ===
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

	// === ü�� �ý��� ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 150.0f; // �⺻ ���ʹ̺��� ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Dead)
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_Dead();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// === ��Ƽ�÷��� ���� ���� �߰� ===
	UPROPERTY(Replicated)
	bool bIsTransformedToChest = false;

	UPROPERTY()
	class UStaticMeshComponent* ChestStaticMesh;

	UPROPERTY()
	class UStaticMesh* ChestMeshAsset;

	// === ��Ƽ�÷��� �Լ� �߰� ===
	UFUNCTION(NetMulticast, Reliable)
	void MulticastActivateRagdoll();

	UFUNCTION()
	void OnRep_TransformToChest();

	void ReplaceMeshWithChest();

	UPROPERTY()
	class UNiagaraSystem* TreasureGlowEffectAsset;

	// === �ǰ� ó�� ===
	virtual void GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage) override;
	virtual void ApplyDebuff_Implementation(EDebuffType DebuffType, float Value, float Duration) override;

	void HandleStun();
	FTimerHandle StunTimerHandle;
	bool bIsStunned = false;
	void PlayHitShake();

	// === �κ��丮 �ý��� ===
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

	// === ������ ��� ���� ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageThreshold = 0.3f; // HP 30% ���Ͽ��� ������ ���

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageSpeedMultiplier = 1.8f; // ������ ��� �� �ӵ� ����

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rage System")
	float RageDamageMultiplier = 1.5f; // ������ ��� �� ������ ����

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rage System")
	bool bIsInRageMode = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsPlayingRageMontage = false;

	// === ���� ���� ���� ===
	UPROPERTY(EditDefaultsOnly, Category = "Jump Attack")
	UAnimMontage* JumpAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackRange = 800.0f; // ���� ���� ��Ÿ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackDamage = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump Attack")
	float JumpAttackCooldown = 8.0f; // ��ٿ� �ð�

	bool bCanJumpAttack = true;
	FTimerHandle JumpAttackCooldownTimer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// === ������ ��� �Լ��� ===
	void CheckRageMode();
	void EnterRageMode();
	void ExitRageMode();

	// === ���� ���� �Լ��� ===
	void TryJumpAttack();
	void PerformJumpAttack();
	void OnJumpAttackCooldownEnd();

	void PlayRageAttackMontage(); // ������ ��� ���� ���� ����

	void OnRageMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	bool CanMove() const;

private:
	float OriginalMaxWalkSpeed; // ���� �ӵ� �����
};
