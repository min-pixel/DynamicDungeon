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
#include "MyDCharacter.generated.h"

//**���⿡ �߰�!** (Ŭ���� ���� ��)
UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None UMETA(DisplayName = "None"),
	Unarmed UMETA(DisplayName = "Unarmed Attack"),
	Weapon UMETA(DisplayName = "Weapon Attack")
};



UCLASS()
class PROJECT03_API AMyDCharacter : public ACharacter
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


	/** ü�� & ���� UI ������Ʈ */
	void UpdateHUD();

	/** ���� �ִϸ��̼� ���� */
	void PlayAttackAnimation();
	//�ִϸ��̼� ����
	void ResetToIdleAnimation();

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

	// ���� �������� ����
	UPROPERTY()
	AActor* OverlappedActor;

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

	// �ָ� ���� �ִϸ��̼� �迭
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimSequence*> UnarmedAttackAnimations;

	// ���� ���� �ִϸ��̼� �迭
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TArray<UAnimSequence*> WeaponAttackAnimations;

	// �޺� Ÿ�̹��� Ȱ��ȭ�ϴ� �Լ�
	void EnableCombo();

	// ���� ���� �� �ʱ�ȭ �Լ�
	void ResetAttack();
	void ForceResetCombo();

	FTimerHandle TimerHandle_Combo; // �޺� Ÿ�̹� Ȱ��ȭ Ÿ�̸�
	FTimerHandle TimerHandle_Reset; // ���� ���� Ÿ�̸�

	FTimerHandle TimerHandle_ForceReset;

};
