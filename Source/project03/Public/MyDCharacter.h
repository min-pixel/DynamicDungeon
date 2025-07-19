// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"  // 애니메이션 인스턴스 관련 클래스 추가
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

//(클래스 선언 전)
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
	// 기본 생성자
	AMyDCharacter();

protected:
	// 게임 시작 시 호출
	virtual void BeginPlay() override;

public:
	// 매 프레임 호출
	virtual void Tick(float DeltaTime) override;

	// 입력 바인딩 설정
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SetOverlappingWeapon(AWeapon* Weapon) { OverlappingWeapon = Weapon; }
	AWeapon* GetOverlappingWeapon() const { return OverlappingWeapon; }

	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	void PickupWeapon();
	void DropWeapon();
	virtual void PossessedBy(AController* NewController) override;



	// === 모듈러 메쉬 컴포넌트들 ===
	/** Body (기본 스켈레탈 메쉬) */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* BodyMesh;

	/** Face 모듈 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* FaceMesh;

	/** Legs 모듈 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* LegsMeshmetha;

	/** Torso 모듈 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* TorsoMesh;

	/** Feet 모듈 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* FeetMesh;

	UPROPERTY()
	USkeletalMesh* DefaultTorsoMesh;

	UPROPERTY()
	USkeletalMesh* DefaultLegsMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsInvulnerable = false;



	//서버
	UFUNCTION(Server, Reliable)
	void ServerTeleportToWFCRegen();

	UFUNCTION(Server, Reliable)
	void Server_TryOpenDoor(AActor* DoorActor);

	UFUNCTION(Server, Reliable)
	void Server_TryEatStatue(AActor* StatueActor);

	// 클라이언트가 호출 -> 서버에서 실행
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestWFCRegen(AWFCRegenerator* RegenActor);

	UFUNCTION(Server, Reliable)
	void ServerTeleportToNearestChest();

	UFUNCTION()
	void TeleportToNearestPlayer();

	UFUNCTION(Server, Reliable)
	void ServerTeleportToNearestPlayer();

	// 서버에게 요청 -> 모든 클라이언트에서 재생성 연출 실행
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlayWFCRegenEffects();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayWFCRegenEffects();

	UFUNCTION(Server, Reliable)
	void ServerPerformTraceAttack();

	// 서버 함수 선언
	UFUNCTION(Server, Reliable)
	void ServerRequestPlayAttackMontage();

	// 멀티캐스트 함수 선언
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
	// 마법 시스템 RPC 함수들
	// =========================

	// 클라이언트 -> 서버: 마법 시전 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCastSpell(int32 SpellIndex, FVector TargetLocation = FVector::ZeroVector, FRotator TargetRotation = FRotator::ZeroRotator);

	// 서버 -> 모든 클라이언트: 파이어볼 생성
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnFireball(FVector SpawnLocation, FRotator SpawnRotation, float Damage, float Speed);

	// 서버 -> 모든 클라이언트: 힐링 이펙트
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHealEffect(FVector Location);  // 매개변수 간소화

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayCurseEffect(FVector StartLocation, FVector EndLocation, AActor* TargetActor);

	// 서버 -> 모든 클라이언트: 마법 시전 애니메이션
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySpellCastAnimation();

	// 서버에서 실행되는 마법 검증 및 처리
	UFUNCTION(BlueprintCallable)
	void ExecuteSpellOnServer(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation);

	// 마법별 실행 함수들 (서버에서 호출)
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
	
	// 디버프 상태 관리
	UPROPERTY(Replicated)
	bool bIsSlowed = false;

	UPROPERTY(Replicated)
	float OriginalWalkSpeed = 600.0f;  // 원래 속도 저장

	// 디버프 해제 함수
	UFUNCTION()
	void RemoveSlowDebuff();

	void ExecuteRoll(float ForwardValue, float RightValue);


	// 무기 정보
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	FItemData EquippedWeaponData;

	// 방어구 정보 (Map 또는 배열)
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

	// 모든 플레이어가 끝났는지 확인
	UFUNCTION(Server, Reliable)
	void ServerCheckAllPlayersFinished();

	UFUNCTION(Server, Reliable)
	void ServerApplyLobbyStats(float NewMaxHealth, float NewMaxStamina, float NewMaxKnowledge, int32 NewGold);


	//맵뷰
	UPROPERTY()
	ACameraActor* OverheadCameraActor;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<ACameraActor> OverheadCameraClass;
	

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Health, BlueprintReadWrite, Category = "Player Stats")
	float Health = 100; // 체력

	float PreviousHealth = -1.0f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxHealth = 100; // 최대 체력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Agility = 50; // 민첩성 (이동 속도와 연결)

	UPROPERTY(Replicated ,EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Knowledge = 100; // 지식 (마나량)

	UFUNCTION()
	void OnRep_Knowledge();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxKnowledge = 100; // 최대 마나량

	//스테미나 관련 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina = 100;  // 현재 스테미나

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxStamina = 100;  // 최대 스테미나

	// 스태미나 소모량 변수 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float SprintStaminaCost = 5.0f;   // 달리기 스태미나 소모량 (1초당)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float RollStaminaCost = 30.0f;    // 구르기 스태미나 소모량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float AttackStaminaCost = 20.0f;  // 공격 시 스태미나 소모량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float StaminaRegenRate = 10.0f;

	// 재화 (골드)
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Currency")
	int32 Gold;

	UPROPERTY()
	UGoldWidget* GoldWidgetInstance;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UGoldWidget> GoldWidgetClass;

	UFUNCTION(BlueprintCallable)
	void UpdateGoldUI();

	//핫키 인덱스 기반 호출 함수
	void UseHotkey(int32 Index);



	//개별 바인딩용 래퍼 함수들
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



	// 기본 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 600.0f;

	// 달리기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 1200.0f;

	/** HUD 위젯 클래스 */
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUCharacterHUDWidget> HUDWidgetClass;

	/** HUD 위젯 */
	UPROPERTY()
	UUCharacterHUDWidget* HUDWidget;

	virtual void GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage);

	virtual void ApplyDebuff_Implementation(EDebuffType DebuffType, float Magnitude, float Duration);

	FTimerHandle DebuffRecoveryTimerHandle;


	/** 체력 & 마나 UI 업데이트 */
	void UpdateHUD();

	/** 공격 애니메이션 실행 */
	void PlayAttackAnimation();

	// 구르기 시 적용할 속도
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollStrength = 1000.0f;  // (적절한 값으로 조정 가능)

	// 구르기 실행 함수 (수정)
	void PlayRollAnimation();

	// 구르기 이동 처리
	void ApplyRollMovement(FVector RollDirection);
	
	// 구르기 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* RollMontage;

	// 구르기 속도 관련 변수
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollSpeed = 1500.0f;  // 적절한 값으로 조절

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float RollDuration = 0.7f;  // 구르기 지속 시간

	void DoRagDoll();

	
	bool bIsRolling;

	// 구르기 상태 초기화 함수
	void ResetRoll();

	// 입력된 방향 저장
	float MoveForwardValue = 0.0f;
	float MoveRightValue = 0.0f;
	FVector StoredRollDirection; // 구르기 시작할 때 저장된 방향

	// 이동 입력 업데이트 함수
	void UpdateMoveForward(float Forward);
	void UpdateMoveRight(float Right);

	TArray<AActor*> HitActors; //공격 중 피격된 액터 목록

	void ResetHitActors(); //공격 종료 시 초기화


	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UInventoryWidget> InventoryWidgetClass;

	UPROPERTY()
	UInventoryWidget* InventoryWidgetInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	bool bIsInventoryVisible = false;

	void ToggleInventoryUI();

	// 장비창 위젯 클래스 (블루프린트에서 연결할 변수)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UEquipmentWidget> EquipmentWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> CombinedInventoryWidgetClass;

	UUserWidget* CombinedInventoryWidgetInstance;


	// 실제 생성된 인스턴스
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
	TMap<int32, class AArmor*> EquippedArmors;  // 슬롯 인덱스 → 장착된 갑옷

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

	// 인벤토리 컴포넌트 가져오기
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	// 장비창 위젯 가져오기 (여기는 장비 "컴포넌트"가 없으니, 장비창 위젯을 넘겨주는 걸로 한다)
	FORCEINLINE UEquipmentWidget* GetEquipmentWidget() const { return EquipmentWidgetInstance; }

	UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
	TSubclassOf<UUserWidget> WFCWarningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
	TSubclassOf<UUserWidget> WFCDoneWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	TSubclassOf<UUserWidget> EscapeWarningWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Escape")
	TSubclassOf<UUserWidget> EscapeDoneWidgetClass;


	// Escape 진행 상태
	bool bIsEscapeCountdownActive = false;
	AActor* PendingEscapeActor = nullptr;

	FTimerHandle TimerHandle_DelayedEscapeFade;
	FTimerHandle TimerHandle_DelayedEscapeFinal;

	FTimerHandle TimerHandle_EscapeProgressUpdate;

	float CurrentEscapeTime = 0.0f;
	float MaxEscapeTime = 5.0f;

	void UpdateEscapeProgressBar();



	// Escape 함수
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



	// 횃불 액터 포인터 저장
	UPROPERTY()
	AActor* AttachedTorch = nullptr;

	// 현재 횃불이 보이는 상태인지 여부
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

	/** 카메라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FirstPersonCameraComponent;


	void PlayMagicMontage();



private:
	/** 캐릭터의 스켈레탈 메쉬 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USkeletalMeshComponent* CharacterMesh;

	

	/** 스프링암 (카메라와 메쉬의 독립적인 배치를 위해 사용) */
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* SpringArm;

	/** 오버랩 감지용 콜리전 박스 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	class UBoxComponent* InteractionBox;

	// 이동 입력 함수
	void MoveForward(float Value);
	void MoveRight(float Value);

	// 달리기 관련 함수
	void StartSprinting();
	void StopSprinting();
	

	


	// 점프 기능 추가
	void StartJump();
	void StopJump();

	// 오버랩 이벤트
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// R키 입력 시 상호작용
	void StartInteraction();
	void StopInteraction();

	

	UPROPERTY()
	AWeapon* OverlappingWeapon;

	UPROPERTY()
	AWeapon* EquippedWeapon;

	// 현재 공격 가능한 상태인지 (공격 중이 아닐 때만 새로운 공격 입력 가능)
	bool bIsAttacking = false;

	// 콤보 가능 여부 (애니메이션 도중 특정 타이밍에만 가능)
	bool bCanCombo = false;

	// 현재 콤보 인덱스
	int32 AttackComboIndex = 0;

	// 공격 애니메이션 몽타주
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


	// 공격 종료 및 초기화 함수
	void ResetAttack();


	FTimerHandle TimerHandle_Combo; // 콤보 타이밍 활성화 타이머
	FTimerHandle TimerHandle_Reset; // 공격 종료 타이머

	FTimerHandle TimerHandle_ComboReset;
	FTimerHandle ComboTimerHandle;
	FTimerHandle TimerHandle_SprintDrain;
	FTimerHandle TimerHandle_StaminaRegen;
	FTimerHandle TimerHandle_StaminaRegenDelay;
	
};
