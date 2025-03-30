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
#include "InventoryComponent.h"
#include "MyDCharacter.generated.h"

//(클래스 선언 전)
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Health; // 체력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxHealth; // 최대 체력

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Agility; // 민첩성 (이동 속도와 연결)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Knowledge; // 지식 (마나량)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxKnowledge; // 최대 마나량

	//스테미나 관련 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float Stamina;  // 현재 스테미나

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float MaxStamina;  // 최대 스테미나

	// 스태미나 소모량 변수 추가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float SprintStaminaCost = 5.0f;   // 달리기 스태미나 소모량 (1초당)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float RollStaminaCost = 30.0f;    // 구르기 스태미나 소모량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float AttackStaminaCost = 20.0f;  // 공격 시 스태미나 소모량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")
	float StaminaRegenRate = 10.0f;

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


private:
	/** 캐릭터의 스켈레탈 메쉬 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USkeletalMeshComponent* CharacterMesh;

	/** 카메라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UCameraComponent* FirstPersonCameraComponent;

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

	// 현재 오버랩된 액터
	UPROPERTY()
	AActor* OverlappedActor;

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
	UAnimMontage* DaggerWeaponMontage;

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
