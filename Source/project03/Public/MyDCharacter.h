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
#include "Armor.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "USpellBase.h"
#include "WFCRegenerator.h"
#include "GoldWidget.h"
#include "OrbitEffectActor.h"
#include "InventoryComponent.h"
#include "EquipmentWidget.h" 
#include "Engine/SceneCapture2D.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MyDCharacter.generated.h"

//(Ŭ���� ���� ��)
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None UMETA(DisplayName = "None"),
	Unarmed UMETA(DisplayName = "Unarmed Attack"),
	Weapon UMETA(DisplayName = "Weapon Attack")
};

USTRUCT(BlueprintType)
struct FEquippedArmorData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 SlotIndex;

	UPROPERTY()
	FItemData ArmorData;
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
	virtual void PossessedBy(AController* NewController) override;



	// === ��ⷯ �޽� ������Ʈ�� ===
	/** Body (�⺻ ���̷�Ż �޽�) */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* BodyMesh;

	/** Face ��� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* FaceMesh;

	/** Legs ��� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* LegsMeshmetha;

	/** Torso ��� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* TorsoMesh;

	/** Feet ��� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* FeetMesh;

	UPROPERTY()
	USkeletalMesh* DefaultTorsoMesh;

	UPROPERTY()
	USkeletalMesh* DefaultLegsMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInvulnerable = false;



	//����
	UFUNCTION(Server, Reliable)
	void ServerTeleportToWFCRegen();

	UFUNCTION(Server, Reliable)
	void Server_TryOpenDoor(AActor* DoorActor);

	UFUNCTION(Server, Reliable)
	void Server_TryEatStatue(AActor* StatueActor);

	// Ŭ���̾�Ʈ�� ȣ�� -> �������� ����
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestWFCRegen(AWFCRegenerator* RegenActor);

	UFUNCTION(Server, Reliable)
	void ServerTeleportToNearestChest();

	UFUNCTION()
	void TeleportToNearestPlayer();

	UFUNCTION(Server, Reliable)
	void ServerTeleportToNearestPlayer();

	// �������� ��û -> ��� Ŭ���̾�Ʈ���� ����� ���� ����
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlayWFCRegenEffects();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayWFCRegenEffects();

	UFUNCTION(Server, Reliable)
	void ServerPerformTraceAttack();

	// ���� �Լ� ����
	UFUNCTION(Server, Reliable)
	void ServerRequestPlayAttackMontage();

	// ��Ƽĳ��Ʈ �Լ� ����
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAttackMontage();

	void PlayAttackAnimation_Internal();

	UFUNCTION(Server, Reliable)
	void ServerRequestRoll(float ForwardValue, float RightValue);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayRoll(float ForwardValue, float RightValue);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetPlayerGravity(float GravityScale);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPlayerGravity(float GravityScale);

	UPROPERTY()
	UUserWidget* DelayedWarningWidget;



	// =========================
	// ���� �ý��� RPC �Լ���
	// =========================

	// Ŭ���̾�Ʈ -> ����: ���� ���� ��û
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCastSpell(int32 SpellIndex, FVector TargetLocation = FVector::ZeroVector, FRotator TargetRotation = FRotator::ZeroRotator);

	// ���� -> ��� Ŭ���̾�Ʈ: ���̾ ����
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnFireball(FVector SpawnLocation, FRotator SpawnRotation, float Damage, float Speed);

	// ���� -> ��� Ŭ���̾�Ʈ: ���� ����Ʈ
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHealEffect(FVector Location);  // �Ű����� ����ȭ

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayCurseEffect(FVector StartLocation, FVector EndLocation, AActor* TargetActor);

	// ���� -> ��� Ŭ���̾�Ʈ: ���� ���� �ִϸ��̼�
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySpellCastAnimation();

	// �������� ����Ǵ� ���� ���� �� ó��
	UFUNCTION(BlueprintCallable)
	void ExecuteSpellOnServer(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation);

	// ������ ���� �Լ��� (�������� ȣ��)
	void ExecuteFireballSpell(FVector TargetLocation, FRotator TargetRotation);
	void ExecuteHealSpell();
	void ExecuteCurseSpell(FVector TargetLocation);

	UUSpellBase* TempScrollSpell;

	UPROPERTY(Replicated)
	float SpeedMultiplier = 1.0f;

	void ApplySpeedMultiplier();

	UFUNCTION()
	void TryCastSpellMultiplayer(int32 SpellIndex);

	/*UFUNCTION()
	void ResetSpellCasting() { bCanCastSpell = true; }*/

	bool bCanCastSpell = true;
	
	// ����� ���� ����
	UPROPERTY(Replicated)
	bool bIsSlowed = false;

	UPROPERTY(Replicated)
	float OriginalWalkSpeed = 600.0f;  // ���� �ӵ� ����

	// ����� ���� �Լ�
	UFUNCTION()
	void RemoveSlowDebuff();

	void ExecuteRoll(float ForwardValue, float RightValue);


	// ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	FItemData EquippedWeaponData;

	// �� ���� (Map �Ǵ� �迭)
	UPROPERTY(ReplicatedUsing = OnRep_EquippedArmors)
	TArray<FEquippedArmorData> EquippedArmorsData;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_EquippedArmors();

	UFUNCTION(Server, Reliable)
	void ServerRequestEquipWeapon(const FItemData& WeaponData);

	UFUNCTION(Server, Reliable)
	void ServerRequestEquipArmor(const FItemData& ArmorData, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerRequestUnequipWeapon();

	UFUNCTION(Server, Reliable)
	void ServerRequestUnequipArmor(int32 SlotIndex);

	virtual void Restart() override;
	void SetupInventoryAndEquipmentUI();

	UFUNCTION(BlueprintCallable)
	void RestoreDataFromLobby();

	UFUNCTION(Server, Reliable)
	void ServerHostTravel();

	FTimerHandle TimerHandle_ServerTravel;

	UFUNCTION(Server, Reliable)
	void ServerHandleDeath();

	UFUNCTION(Server, Reliable)
	void ServerTeleportToEscapeObject();
	


	UFUNCTION(Server, Reliable)
	void ServerHandleEscape();

	UFUNCTION(Client, Reliable)
	void ClientEnterSpectatorMode();

	// ��� �÷��̾ �������� Ȯ��
	UFUNCTION(Server, Reliable)
	void ServerCheckAllPlayersFinished();

	UFUNCTION(Server, Reliable)
	void ServerApplyLobbyStats(float NewMaxHealth, float NewMaxStamina, float NewMaxKnowledge, int32 NewGold);


	//�ʺ�
	UPROPERTY()
	ACameraActor* OverheadCameraActor;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<ACameraActor> OverheadCameraClass;
	

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Health, BlueprintReadWrite, Category = "Player Stats")
	float Health = 100; // ü��

	float PreviousHealth = -1.0f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxHealth = 100; // �ִ� ü��

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Agility = 50; // ��ø�� (�̵� �ӵ��� ����)

	UPROPERTY(Replicated ,EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Knowledge = 100; // ���� (������)

	UFUNCTION()
	void OnRep_Knowledge();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxKnowledge = 100; // �ִ� ������

	//���׹̳� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina = 100;  // ���� ���׹̳�

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxStamina = 100;  // �ִ� ���׹̳�

	// ���¹̳� �Ҹ� ���� �߰�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float SprintStaminaCost = 5.0f;   // �޸��� ���¹̳� �Ҹ� (1�ʴ�)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float RollStaminaCost = 30.0f;    // ������ ���¹̳� �Ҹ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float AttackStaminaCost = 20.0f;  // ���� �� ���¹̳� �Ҹ�

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float StaminaRegenRate = 10.0f;

	// ��ȭ (���)
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Currency")
	int32 Gold;

	UPROPERTY()
	UGoldWidget* GoldWidgetInstance;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UGoldWidget> GoldWidgetClass;

	UFUNCTION(BlueprintCallable)
	void UpdateGoldUI();

	//��Ű �ε��� ��� ȣ�� �Լ�
	void UseHotkey(int32 Index);



	//���� ���ε��� ���� �Լ���
	UFUNCTION()
	void UseHotkey1();

	UFUNCTION()
	void UseHotkey2();

	UFUNCTION()
	void UseHotkey3();

	UFUNCTION()
	void UseHotkey4();

	UFUNCTION()
	void UseHotkey5();

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

	virtual void ApplyDebuff_Implementation(EDebuffType DebuffType, float Magnitude, float Duration);

	FTimerHandle DebuffRecoveryTimerHandle;


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
	void EquipWeaponFromClass(TSubclassOf<class AItem> WeaponClass, EWeaponGrade Grade);

	UFUNCTION(BlueprintCallable)
	void UnequipWeapon();

	UFUNCTION(BlueprintCallable)
	void EquipArmorFromClass(int32 SlotIndex, TSubclassOf<AItem> ArmorClass, uint8 Grade);

	UFUNCTION(BlueprintCallable)
	void UnequipArmorAtSlot(int32 SlotIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Armor")
	UStaticMeshComponent* HelmetMesh;

	void EquipHelmetMesh(UStaticMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor);

	UPROPERTY()
	TMap<int32, class AArmor*> EquippedArmors;  // ���� �ε��� �� ������ ����

	bool IsPlayerInFixedRoomTile();



	UPROPERTY()
	AActor* OverlappedActor;
	FTimerHandle TimerHandle_StartWFC;
	FTimerHandle TimerHandle_DelayedWFC;
	AActor* PendingRegenActor = nullptr;
	bool bIsWFCCountdownActive = false;

	void TriggerDelayedWFC();

	void PlayWFCRegenCameraShake();

	void ShowWFCFadeAndRegenSequence();

	void FadeAndRegenWFC();

	void ExecuteWFCNow();

	UFUNCTION()
	void TeleportToWFCRegen();

	UFUNCTION(BlueprintCallable)
	void TeleportToEscapeObject();

	UFUNCTION()
	void TeleportToNearestChest();

	UFUNCTION()
	void TeleportToNearestEnemy();

	void HealPlayer(int32 Amount);

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

	void CastSpell2();



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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ChestMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* LegsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* HatMesh;

	void EquipArmorMesh(int32 SlotIndex, USkeletalMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor);

	ADirectionalLight* CachedDirectionalLight = nullptr;

	UUserWidget* WFCWarningWidgetInstance;
	UUserWidget* WFCDoneWidgetInstance;

	UUserWidget* EscapeWarningWidgetInstance;
	UUserWidget* EscapeDoneWidgetInstance;

	FTimerHandle TimerHandle_DelayedWFCFade;
	FTimerHandle TimerHandle_DelayedWFCFinal;

	/** ī�޶� ������Ʈ */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FirstPersonCameraComponent;


	void PlayMagicMontage();



private:
	/** ĳ������ ���̷�Ż �޽� */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USkeletalMeshComponent* CharacterMesh;

	

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
	UAnimMontage* SpellCastMontage;

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
