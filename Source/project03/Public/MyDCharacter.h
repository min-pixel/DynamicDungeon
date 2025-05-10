// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"  // �ִϸ��̼� �ν��Ͻ� ���� Ŭ���� �߰�
#include "Components/BoxComponent.h"
#include "DynamicDungeonInstance.h"
#include "Weapon.h"
#include "Animation/AnimSequence.h"
#include "HitInterface.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "USpellBase.h"
#include "InventoryComponent.h"
#include "EquipmentWidget.h" 
#include "MyDCharacter.generated.h"

//(Ŭ���� ���� ��)
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None UMETA(DisplayName = "None"),
	Unarmed UMETA(DisplayName = "Unarmed Attack"),
	Weapon UMETA(DisplayName = "Weapon Attack")
};



UCLASS()
class PROJECT03_API AMyDCharacter : public ACharacter, public IHitInterface
{
	GENERATED_BODY()

public:
	// �⺻ ������
	AMyDCharacter();

protected:
	// ���� ���� �� ȣ��
	virtual void BeginPlay() override;

public:
	// �� ������ ȣ��
	virtual void Tick(float DeltaTime) override;

	// �Է� ���ε� ����
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SetOverlappingWeapon(AWeapon* Weapon) { OverlappingWeapon = Weapon; }
	AWeapon* GetOverlappingWeapon() const { return OverlappingWeapon; }

	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	void PickupWeapon();
	void DropWeapon();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Health; // ü��

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxHealth; // �ִ� ü��

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Agility; // ��ø�� (�̵� �ӵ��� ����)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Knowledge; // ���� (������)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxKnowledge; // �ִ� ������

	//���׹̳� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina;  // ���� ���׹̳�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxStamina;  // �ִ� ���׹̳�

	// ���¹̳� �Ҹ� ���� �߰�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float SprintStaminaCost = 5.0f;   // �޸��� ���¹̳� �Ҹ� (1�ʴ�)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float RollStaminaCost = 30.0f;    // ������ ���¹̳� �Ҹ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float AttackStaminaCost = 20.0f;  // ���� �� ���¹̳� �Ҹ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float StaminaRegenRate = 10.0f;

	UFUNCTION(BlueprintCallable, Category = "State")
	bool IsDead() const;

	void SprintStaminaDrain();

	void ReduceStamina(float StaminaCost);

	void ManageStaminaRegen();
	void StartStaminaRegen();
	void RegenerateStamina();

	// �⺻ �̵� �ӵ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.0f;

	// �޸��� �ӵ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 1200.0f;

	/** HUD ���� Ŭ���� */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUCharacterHUDWidget> HUDWidgetClass;

	/** HUD ���� */
	UPROPERTY()
	UUCharacterHUDWidget* HUDWidget;

	virtual void GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage);


	/** ü�� & ���� UI ������Ʈ */
	void UpdateHUD();

	/** ���� �ִϸ��̼� ���� */
	void PlayAttackAnimation();

	// ������ �� ������ �ӵ�
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollStrength = 1000.0f;  // (������ ������ ���� ����)

	// ������ ���� �Լ� (����)
	void PlayRollAnimation();

	// ������ �̵� ó��
	void ApplyRollMovement(FVector RollDirection);
	
	// ������ �ִϸ��̼� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* RollMontage;

	// ������ �ӵ� ���� ����
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollSpeed = 1500.0f;  // ������ ������ ����

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollDuration = 0.7f;  // ������ ���� �ð�

	void DoRagDoll();

	
	bool bIsRolling;

	// ������ ���� �ʱ�ȭ �Լ�
	void ResetRoll();

	// �Էµ� ���� ����
	float MoveForwardValue = 0.0f;
	float MoveRightValue = 0.0f;
	FVector StoredRollDirection; // ������ ������ �� ����� ����

	// �̵� �Է� ������Ʈ �Լ�
	void UpdateMoveForward(float Forward);
	void UpdateMoveRight(float Right);

	TArray<AActor*> HitActors; //���� �� �ǰݵ� ���� ���

	void ResetHitActors(); //���� ���� �� �ʱ�ȭ


	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UInventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	UInventoryWidget* InventoryWidgetInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	bool bIsInventoryVisible = false;

	void ToggleInventoryUI();

	// ���â ���� Ŭ���� (�������Ʈ���� ������ ����)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UEquipmentWidget> EquipmentWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> CombinedInventoryWidgetClass;

	UUserWidget* CombinedInventoryWidgetInstance;


	// ���� ������ �ν��Ͻ�
	UPROPERTY()
	UEquipmentWidget* EquipmentWidgetInstance;

	UFUNCTION(BlueprintCallable)
	void EquipWeaponFromClass(TSubclassOf<class AItem> WeaponClass);

	UFUNCTION(BlueprintCallable)
	void UnequipWeapon();

	UPROPERTY()
	AActor* OverlappedActor;

	FTimerHandle TimerHandle_DelayedWFC;
	AActor* PendingRegenActor = nullptr;
	bool bIsWFCCountdownActive = false;

	void TriggerDelayedWFC();

	void PlayWFCRegenCameraShake();

	void ShowWFCFadeAndRegenSequence();

	void FadeAndRegenWFC();

	void ExecuteWFCNow();

	// �κ��丮 ������Ʈ ��������
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// ���â ���� �������� (����� ��� "������Ʈ"�� ������, ���â ������ �Ѱ��ִ� �ɷ� �Ѵ�)
	FORCEINLINE UEquipmentWidget* GetEquipmentWidget() const { return EquipmentWidgetInstance; }

	UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
	TSubclassOf<UUserWidget> WFCWarningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
	TSubclassOf<UUserWidget> WFCDoneWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	TSubclassOf<UUserWidget> EscapeWarningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	TSubclassOf<UUserWidget> EscapeDoneWidgetClass;


	// Escape ���� ����
	bool bIsEscapeCountdownActive = false;
	AActor* PendingEscapeActor = nullptr;

	FTimerHandle TimerHandle_DelayedEscapeFade;
	FTimerHandle TimerHandle_DelayedEscapeFinal;

	FTimerHandle TimerHandle_EscapeProgressUpdate;

	float CurrentEscapeTime = 0.0f;
	float MaxEscapeTime = 5.0f;

	void UpdateEscapeProgressBar();

	// Escape �Լ�
	void TriggerEscapeSequence();
	void FadeAndEscape();
	void ExecuteEscape();

	void ApplyCharacterData(const FPlayerCharacterData& Data);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	EPlayerClass PlayerClass;

	UPROPERTY()
	TArray<UUSpellBase*> SpellSet;

	void TryCastSpell(int32 Index);

	void CastSpell1();

	// ȶ�� ���� ������ ����
	UPROPERTY()
	AActor* AttachedTorch = nullptr;

	// ���� ȶ���� ���̴� �������� ����
	bool bTorchVisible = false;

	void ToggleTorch();

	void Die();

	bool bIsInOverheadView = false;
	FVector DefaultCameraLocation;
	FRotator DefaultCameraRotation;

	void ToggleMapView();

	ADirectionalLight* CachedDirectionalLight = nullptr;

	UUserWidget* WFCWarningWidgetInstance;
	UUserWidget* WFCDoneWidgetInstance;

	UUserWidget* EscapeWarningWidgetInstance;
	UUserWidget* EscapeDoneWidgetInstance;

	FTimerHandle TimerHandle_DelayedWFCFade;
	FTimerHandle TimerHandle_DelayedWFCFinal;



private:
	/** ĳ������ ���̷�Ż �޽� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USkeletalMeshComponent* CharacterMesh;

	/** ī�޶� ������Ʈ */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FirstPersonCameraComponent;

	/** �������� (ī�޶�� �޽��� �������� ��ġ�� ���� ���) */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* SpringArm;

	/** ������ ������ �ݸ��� �ڽ� */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	class UBoxComponent* InteractionBox;

	// �̵� �Է� �Լ�
	void MoveForward(float Value);
	void MoveRight(float Value);

	// �޸��� ���� �Լ�
	void StartSprinting();
	void StopSprinting();
	

	


	// ���� ��� �߰�
	void StartJump();
	void StopJump();

	// ������ �̺�Ʈ
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// RŰ �Է� �� ��ȣ�ۿ�
	void StartInteraction();
	void StopInteraction();

	

	UPROPERTY()
	AWeapon* OverlappingWeapon;

	UPROPERTY()
	AWeapon* EquippedWeapon;

	// ���� ���� ������ �������� (���� ���� �ƴ� ���� ���ο� ���� �Է� ����)
	bool bIsAttacking = false;

	// �޺� ���� ���� (�ִϸ��̼� ���� Ư�� Ÿ�ֿ̹��� ����)
	bool bCanCombo = false;

	// ���� �޺� �ε���
	int32 AttackComboIndex = 0;

	// ���� �ִϸ��̼� ��Ÿ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* UnarmedAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* WeaponAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* GreatWeaponMontage;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* DaggerWeaponMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* PoseMontage;

	// ���� ���� �� �ʱ�ȭ �Լ�
	void ResetAttack();


	FTimerHandle TimerHandle_Combo; // �޺� Ÿ�̹� Ȱ��ȭ Ÿ�̸�
	FTimerHandle TimerHandle_Reset; // ���� ���� Ÿ�̸�

	FTimerHandle TimerHandle_ComboReset;
	FTimerHandle ComboTimerHandle;
	FTimerHandle TimerHandle_SprintDrain;
	FTimerHandle TimerHandle_StaminaRegen;
	FTimerHandle TimerHandle_StaminaRegenDelay;
	
};
