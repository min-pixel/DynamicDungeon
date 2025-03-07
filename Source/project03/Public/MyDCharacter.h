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
#include "MyDCharacter.generated.h"

UCLASS()
class PROJECT03_API AMyDCharacter : public ACharacter
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

	/** 체력 & 마나 UI 업데이트 */
	void UpdateHUD();

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

};
