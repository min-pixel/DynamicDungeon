// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MyDCharacter.generated.h"

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


	// �̵� �Է� �Լ�
	void MoveForward(float Value);
	void MoveRight(float Value);

};