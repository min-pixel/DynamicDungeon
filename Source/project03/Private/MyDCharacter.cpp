// Fill out your copyright notice in the Description page of Project Settings.


#include "MyDCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DynamicDungeonInstance.h"
#include "Animation/AnimInstance.h"  // �ִϸ��̼� ���� Ŭ���� �߰�
#include "Components/BoxComponent.h"  // �ݸ��� �ڽ� �߰�
#include "Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "UCharacterHUDWidget.h"
#include "Blueprint/UserWidget.h"

// �⺻ ������
AMyDCharacter::AMyDCharacter()
{
	// �� ������ Tick�� Ȱ��ȭ
	PrimaryActorTick.bCanEverTick = true;



	AutoPossessPlayer = EAutoReceiveInput::Player0;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement =true;


	//�⺻ �Ӽ� ����
	MaxHealth = 100.0f;
	Health = MaxHealth;
	Agility = 1.0f; // �⺻ ��ø�� (�̵� �ӵ� ����)
	MaxKnowledge = 100.0f;
	Knowledge = MaxKnowledge;

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


	// ��ȣ�ۿ� ������ ���� �ڽ� �ݸ��� �߰�
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(50.0f, 50.0f, 100.0f)); // �ݸ��� ũ�� ����
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapBegin);
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapEnd);

	// HUDWidgetClass�� �������Ʈ ��ο��� �ε�
	static ConstructorHelpers::FClassFinder<UUCharacterHUDWidget> WidgetBPClass(TEXT("/Game/BP/UI/CharacterHUDWidget_BP.CharacterHUDWidget_BP_C"));

	if (WidgetBPClass.Succeeded())
	{
		HUDWidgetClass = WidgetBPClass.Class;
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded CharacterHUDWidget_BP."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load CharacterHUDWidget_BP! Check the path."));
	}

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
	// Agility ���� ���� �ʱ� �̵� �ӵ� ����
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
	
	
	if (HUDWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("HUDWidgetClass is valid, attempting to create widget..."));

		HUDWidget = CreateWidget<UUCharacterHUDWidget>(GetWorld(), HUDWidgetClass);
		if (HUDWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("Widget successfully created, adding to viewport..."));
			HUDWidget->AddToViewport(10);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create widget!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HUDWidgetClass is NULL!"));
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

// �޸��� ����
void AMyDCharacter::StartSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed * Agility;
}

// �޸��� ����
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
}

// �Է� ���ε� ����
void AMyDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Log, TEXT("SetupPlayerInputComponent called!"));

	// �̵� �Է� ���ε� (WASD)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::MoveRight);

	// �޸��� (Shift Ű)
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMyDCharacter::StartSprinting);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMyDCharacter::StopSprinting);

	// ���� �Է� ���ε�
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyDCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMyDCharacter::StopJump);


	//���콺 �Է� ���ε� (ī�޶� ȸ��)
	PlayerInputComponent->BindAxis("Turn", this, &AMyDCharacter::AddControllerYawInput);  // �¿� ȸ��
	PlayerInputComponent->BindAxis("LookUp", this, &AMyDCharacter::AddControllerPitchInput);  // ���� ȸ��
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMyDCharacter::StartInteraction);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMyDCharacter::StopInteraction);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMyDCharacter::PickupWeapon);

	PlayerInputComponent->BindAction("DropWeapon", IE_Pressed, this, &AMyDCharacter::DropWeapon);
}

void AMyDCharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		OverlappedActor = OtherActor;
		UE_LOG(LogTemp, Log, TEXT("Overlapped with: %s"), *OtherActor->GetName());
	}
}

void AMyDCharacter::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == OverlappedActor)
	{
		OverlappedActor = nullptr;

		// ������ ���� �� NewGameInstance ���� �ʱ�ȭ
		UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
		if (GameInstance)
		{
			GameInstance->itemEAt = false;
			GameInstance->OpenDoor = false;
			GameInstance->WeaponEAt = false;
			UE_LOG(LogTemp, Log, TEXT("Overlap Ended - Reset GameInstance variables"));
		}

		UE_LOG(LogTemp, Log, TEXT("Overlap Ended with: %s"), *OtherActor->GetName());
	}
}


void AMyDCharacter::StartInteraction()
{
	// ���� �ν��Ͻ��� ������
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

	if (GameInstance)
	{
		// R Ű�� ������ �� ���� ��� true
		GameInstance->itemEAt = true;
		GameInstance->OpenDoor = true;
		GameInstance->WeaponEAt = true;

		UE_LOG(LogTemp, Log, TEXT("StartInteraction() : itemEAt = true, OpenDoor = true"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartInteraction(): GameInstance not found!"));
	}
}

void AMyDCharacter::StopInteraction()
{
	// ���� �ν��Ͻ��� ������
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

	if (GameInstance)
	{
		// R Ű�� ���� �� ���� ��� false
		GameInstance->itemEAt = false;
		GameInstance->OpenDoor = false;
		GameInstance->WeaponEAt = false;

		UE_LOG(LogTemp, Log, TEXT("StopInteraction(): itemEAt = false, OpenDoor = false"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StopInteraction(): GameInstance not found!"));
	}
}

// ���� �ݱ� �Լ� (R Ű �Է� �� ����)
void AMyDCharacter::PickupWeapon()
{
	if (OverlappingWeapon)
	{
		// ���� ���Ⱑ �ִٸ� ������ (���� ����)
		if (EquippedWeapon)
		{
			EquippedWeapon->Destroy();
		}

		// ���ο� ���� ����
		EquippedWeapon = OverlappingWeapon;
		OverlappingWeapon = nullptr;

		// �� ������ �ִ��� Ȯ��
		if (CharacterMesh->DoesSocketExist(FName("hand_r")))
		{
			UE_LOG(LogTemp, Log, TEXT("Socket hand_r exists! Attaching weapon."));

			// �� ���� ��ġ ���
			FTransform HandSocketTransform = CharacterMesh->GetSocketTransform(FName("hand_r"));
			UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());

			// ���� ����
			FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
			EquippedWeapon->AttachToComponent(CharacterMesh, AttachRules, FName("hand_r"));

			// ���� �߷� �� ���� �ùķ��̼� ��Ȱ��ȭ
			if (EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->WeaponMesh->SetSimulatePhysics(false);  //���� ��Ȱ��ȭ
				EquippedWeapon->WeaponMesh->SetEnableGravity(false);    //�߷� ��Ȱ��ȭ
			}

			// ���� ũ�� ����
			EquippedWeapon->SetActorScale3D(FVector(0.25f, 0.25f, 1.0f));

			// ���� �浹 ��Ȱ��ȭ �� ���� ����
			EquippedWeapon->SetActorEnableCollision(false);
			EquippedWeapon->SetActorHiddenInGame(false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Socket hand_r NOT found!"));
		}
	}
}

// ���� �������� �Լ� (G Ű)
void AMyDCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		UE_LOG(LogTemp, Log, TEXT("Dropping weapon: %s"), *EquippedWeapon->GetName());

		// ���� ����
		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// ���� �浹 Ȱ��ȭ
		EquippedWeapon->SetActorEnableCollision(true);

		// ���� �߷� �� ���� �ùķ��̼� Ȱ��ȭ
		if (EquippedWeapon->WeaponMesh)
		{
			EquippedWeapon->WeaponMesh->SetSimulatePhysics(true);  //���� Ȱ��ȭ
			EquippedWeapon->WeaponMesh->SetEnableGravity(true);    //�߷� Ȱ��ȭ
		}

		// ���⸦ ĳ���� ���ʿ� ����
		FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 100.0f; // ĳ���� �� 100cm ��ġ
		EquippedWeapon->SetActorLocation(DropLocation);

		// ���� �ʱ�ȭ
		EquippedWeapon = nullptr;
	}
}


void AMyDCharacter::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);
		HUDWidget->UpdateMana(Knowledge, MaxKnowledge);
	}
}

// ���� ����
void AMyDCharacter::StartJump()
{
	Jump();  // ACharacter�� �⺻ ���� ��� ȣ��
}

// ���� ���߱�
void AMyDCharacter::StopJump()
{
	StopJumping();  // ���� ���ߴ� �Լ� ȣ��
}