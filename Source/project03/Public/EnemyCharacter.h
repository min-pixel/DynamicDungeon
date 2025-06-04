// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HitInterface.h"
#include "GameFramework/Character.h"
#include "InventoryComponent.h"
#include "InventoryWidget.h"
#include "Components/BoxComponent.h"
#include "EnemyCharacter.generated.h"


UCLASS()
class PROJECT03_API AEnemyCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FSMComponent")
	class UEnemyFSMComponent* fsm;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	void PlayAttackMontage();

	void TraceAttack();

	void StartTrace();

	int32 AttackComboIndex = 0;

	FVector LastStartLocation;
	FVector LastEndLocation;
	TArray<AActor*> HitActors;

	// 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float CurrentHealth;

	// 피격 처리
	virtual void GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage);

	virtual void ApplyDebuff_Implementation(EDebuffType DebuffType, float Value, float Duration) override;

	// 피격 후 경직 처리
	void HandleStun();

	FTimerHandle StunTimerHandle;
	bool bIsStunned = false;

	void PlayHitShake();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	UInventoryComponent* LootInventory;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UInventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	UInventoryWidget* LootInventoryWidgetInstance;

	void GenerateRandomLoot();


	TArray<TSubclassOf<AItem>> PossibleItems;

};
