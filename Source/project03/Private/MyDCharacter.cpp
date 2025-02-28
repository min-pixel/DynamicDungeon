// Fill out your copyright notice in the Description page of Project Settings.


#include "MyDCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"  // �ִϸ��̼� ���� Ŭ���� �߰�

// �⺻ ������
AMyDCharacter::AMyDCharacter()
{
	// �� ������ Tick�� Ȱ��ȭ
	PrimaryActorTick.bCanEverTick = true;



	AutoPossessPlayer = EAutoReceiveInput::Player0;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement =true;

	//��������(SprintArm) ���� (�޽��� ī�޶� ���������� ��ġ�ϱ� ����)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f; // 1��Ī�̹Ƿ� �Ÿ� ����
	SpringArm->bUsePawnControlRotation = false; // ī�޶� ȸ���� ������ ���� ����

	// ĳ���� �޽� ����
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(SpringArm);

	// �޽� �ε� (ConstructorHelpers ���)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'"));
	if (MeshAsset.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MeshAsset.Object);
	}


	// �ִϸ��̼� �������Ʈ �ε�
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	if (AnimBP.Succeeded())
	{
		UE_LOG(LogTemp, Log, TEXT("Animation Blueprint Loaded Successfully: %s"), *AnimBP.Class->GetName());
		CharacterMesh->SetAnimInstanceClass(AnimBP.Class);
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load Animation Blueprint!"));
	}


	//**�޽� ���� ���� (Z�� 90�� ȸ��)**
	CharacterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // �޽� ���⸸ ����
	//**�޽� ��ġ ���� (���� �ٴڿ� �Ĺ����� �ʵ���)**
	CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));  // ��Ʈ���� �Ʒ��� ��ġ

	//1��Ī ī�޶� ���� (���������� �ƴ� ��Ʈ�� ���� ����)
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);  // ��Ʈ�� ���� ���� (�޽� ���� X)
	FirstPersonCameraComponent->bUsePawnControlRotation = true;  // ��Ʈ�ѷ� ȸ�� ����

	//**ī�޶� ��ġ �� ���� ����**
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-100.0f, 0.0f, 80.0f));  // �Ӹ� ��ġ , ������ 20.0f, 0.0f, 50.0f, �׽�Ʈ�� -100.0f, 0.0f, 80.0f
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // ���� ����

	//��ü ����� (1��Ī���� ������ �ʰ�)
	//CharacterMesh->SetOwnerNoSee(true);

	GetCharacterMovement()->bOrientRotationToMovement = true; // �̵� ������ �ٶ󺸰� ��
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �ȱ� �ӵ�
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f; // ���� ����


	

}

// ���� ���� �� ȣ��
void AMyDCharacter::BeginPlay()
{
	Super::BeginPlay();
	// �ִϸ��̼� �������Ʈ ���� ����
	UClass* AnimClass = LoadObject<UClass>(nullptr, TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	if (AnimClass)
	{
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded ABP_Manny via LoadObject"));
		CharacterMesh->SetAnimInstanceClass(AnimClass);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load ABP_Manny via LoadObject"));
	}

}

// �� ������ ȣ��
void AMyDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// �̵� �Լ� (W/S)
void AMyDCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		//��Ʈ�ѷ�(ī�޶�)�� ȸ�� ������ ������
		FRotator ControlRotation = GetControlRotation();
		FVector ForwardDirection = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);

		UE_LOG(LogTemp, Log, TEXT("MoveForward called with value: %f"), Value);
		AddMovementInput(ForwardDirection, Value);
	}
}
// �̵� �Լ� (A/D)
void AMyDCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		//��Ʈ�ѷ�(ī�޶�)�� ȸ�� ������ ������
		FRotator ControlRotation = GetControlRotation();
		
		FVector RightDirection = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);
	

		UE_LOG(LogTemp, Log, TEXT("MoveRight called with value: %f"), Value);
		AddMovementInput(RightDirection, Value);

		
		
	}
}

// �Է� ���ε� ����
void AMyDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Log, TEXT("SetupPlayerInputComponent called!"));

	// �̵� �Է� ���ε� (WASD)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::MoveRight);

	//���콺 �Է� ���ε� (ī�޶� ȸ��)
	PlayerInputComponent->BindAxis("Turn", this, &AMyDCharacter::AddControllerYawInput);  // �¿� ȸ��
	PlayerInputComponent->BindAxis("LookUp", this, &AMyDCharacter::AddControllerPitchInput);  // ���� ȸ��

}


