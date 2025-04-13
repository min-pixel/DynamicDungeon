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
#include "GreatWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "UCharacterHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Item.h"
#include "InventoryWidget.h"
#include "TreasureChest.h"
#include "EnemyCharacter.h"
#include "Animation/AnimBlueprintGeneratedClass.h"

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
	// �����ڿ��� ���׹̳� �� �ʱ�ȭ
	MaxStamina = 100.0f;
	Stamina = MaxStamina;

	//��������(SprintArm) ���� (�޽��� ī�޶� ���������� ��ġ�ϱ� ����)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f; // 1��Ī�̹Ƿ� �Ÿ� ����
	SpringArm->bUsePawnControlRotation = false; // ī�޶� ȸ���� ������ ���� ����

	// ĳ���� �޽� ����
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(SpringArm);

	// �浹 ����: ���׵� ��ȯ�� ���� �⺻ ����
	CharacterMesh->SetCollisionProfileName(TEXT("CharacterMesh")); // ���߿� Ragdoll�� �ٲ� �� �ֵ���
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);


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

	// ���ָ� �޺� ���� ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage> PunchMontage(TEXT("/Game/BP/character/Retarget/RTA_Punching_Anim_mixamo_com_Montage1.RTA_Punching_Anim_mixamo_com_Montage1"));
	if (PunchMontage.Succeeded())
	{
		UnarmedAttackMontage = PunchMontage.Object;
		UE_LOG(LogTemp, Log, TEXT("Unarmed Attack Montage Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Unarmed Attack Montage!"));
	}

	// ���� ���� �޺� ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage> SwordMontage(TEXT("/Game/BP/character/Retarget/RTA_Stable_Sword_Outward_Slash_Anim_mixamo_com_Montage.RTA_Stable_Sword_Outward_Slash_Anim_mixamo_com_Montage"));
	if (SwordMontage.Succeeded())
	{
		WeaponAttackMontage = SwordMontage.Object;
		UE_LOG(LogTemp, Log, TEXT("Weapon Attack Montage Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Weapon Attack Montage!"));
	}


	//  ���� ���� ���� �޺� ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage> GreatSwordMontage(TEXT("/Game/BP/character/Retarget/RTA_Great_Sword_Slash_Anim_mixamo_com_Montage.RTA_Great_Sword_Slash_Anim_mixamo_com_Montage"));
	if (GreatSwordMontage.Succeeded())
	{
		GreatWeaponMontage = GreatSwordMontage.Object;
		UE_LOG(LogTemp, Log, TEXT("Weapon Attack Montage Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Weapon Attack Montage!"));
	}

	//  �ܰ� ���� ���� �޺� ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage>DaggerMontage(TEXT("/Game/BP/character/Retarget/RTA_Stabbing_Anim_mixamo_com_Montage.RTA_Stabbing_Anim_mixamo_com_Montage"));
	if (DaggerMontage.Succeeded())
	{
		DaggerWeaponMontage = DaggerMontage.Object;
		UE_LOG(LogTemp, Log, TEXT("Weapon Attack Montage Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Weapon Attack Montage!"));
	}


	// ������ ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage> RollMontageAsset(TEXT("/Game/BP/character/Retarget/RTA_Sprinting_Forward_Roll_Anim_mixamo_com_Montage.RTA_Sprinting_Forward_Roll_Anim_mixamo_com_Montage"));
	if (RollMontageAsset.Succeeded())
	{
		RollMontage = RollMontageAsset.Object;
		UE_LOG(LogTemp, Log, TEXT("Roll Montage Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Roll Montage!"));
	}

	// �ʱ� ������ ���� ����
	bIsRolling = false;

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

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	static ConstructorHelpers::FClassFinder<UInventoryWidget> InventoryWidgetBPClass(TEXT("/Game/BP/UI/InventoryWidget.InventoryWidget_C"));
	if (InventoryWidgetBPClass.Succeeded())
	{
		InventoryWidgetClass = InventoryWidgetBPClass.Class;
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded InventoryWidget_BP."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load InventoryWidget_BP!"));
	}

	static ConstructorHelpers::FClassFinder<UEquipmentWidget> EquipmentWidgetBPClass(TEXT("/Game/BP/UI/EquipmentWidget.EquipmentWidget_C"));
	if (EquipmentWidgetBPClass.Succeeded())
	{
		EquipmentWidgetClass = EquipmentWidgetBPClass.Class;
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded EquipmentWidget_BP."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load EquipmentWidget_BP!"));
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> CombinedWidgetBP(TEXT("/Game/BP/UI/CombinedInventoryWidget.CombinedInventoryWidget_C"));
	if (CombinedWidgetBP.Succeeded())
	{
		CombinedInventoryWidgetClass = CombinedWidgetBP.Class;
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
			HUDWidget->AddToViewport(1);
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

	// ���� ������ �׻�!
	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->InventoryRef = InventoryComponent;
			InventoryWidgetInstance->RefreshInventoryStruct();

			if (InventoryComponent)
			{
				InventoryComponent->TryAddItemByClass(AGreatWeapon::StaticClass());

				InventoryWidgetInstance->RefreshInventoryStruct(); // �ٽ� ����
			}

		}
	}

	if (EquipmentWidgetClass)
	{
		EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);
		EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;
	}


	/*if (InventoryComponent)
	{
		AItem* GreatWeaponItem = GetWorld()->SpawnActor<AGreatWeapon>(AGreatWeapon::StaticClass(), GetActorLocation() + FVector(200, 0, 0), FRotator::ZeroRotator);
		if (GreatWeaponItem)
		{
			InventoryComponent->TryAddItem(GreatWeaponItem);
			UE_LOG(LogTemp, Log, TEXT("GreatWeaponItem added to inventory: %s"), *GreatWeaponItem->GetName());

			if (InventoryWidgetInstance)
			{
				InventoryWidgetInstance->RefreshInventory();
			}
		}
	}*/

}

// �� ������ ȣ��
void AMyDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ������ ���� ī�޶� �ε巴�� �̵�
	//moothCameraFollow();

}

// �̵� �Լ� (W/S)
void AMyDCharacter::MoveForward(float Value)
{

	if (bIsRolling)
	{
		// ������ ������ ���̳� ���� ���� �̵� ���
		float DotProductF = FVector::DotProduct(GetActorForwardVector(), StoredRollDirection);
		if (FMath::Abs(DotProductF) < 0.8f) return; // ��/�� ������ �ƴ϶�� �̵� ����
	}
	

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

	if (bIsRolling)
	{
		// ������ ������ �¿��� ���� �̵� ���
		float DotProductR = FVector::DotProduct(GetActorRightVector(), StoredRollDirection);
		if (FMath::Abs(DotProductR) < 0.8f) return; // ��/�� ������ �ƴ϶�� �̵� ����
	}
	

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
	GetWorldTimerManager().SetTimer(TimerHandle_SprintDrain, this, &AMyDCharacter::SprintStaminaDrain, 0.1f, true);
}

// �޸��� ����
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
	GetWorldTimerManager().ClearTimer(TimerHandle_SprintDrain);
	ManageStaminaRegen();  //�޸��� ������ ���׹̳� ȸ�� ��Ű��
}

void AMyDCharacter::SprintStaminaDrain()
{
	if (Stamina > 0)
	{
		ReduceStamina(SprintStaminaCost);
	}
	else
	{
		StopSprinting();
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

	// ���� �Է��� �����ϱ� ���� �Լ� (������ ���� ��꿡 ���)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::UpdateMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::UpdateMoveRight);

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

	// ���� �Է� �߰� (���콺 ��Ŭ��)
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AMyDCharacter::PlayAttackAnimation);

	PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &AMyDCharacter::PlayRollAnimation);

	PlayerInputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AMyDCharacter::ToggleInventoryUI);
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

		//�������� ���� ���� �߰�
		if (OverlappedActor && OverlappedActor->ActorHasTag("Chest"))
		{
			ATreasureChest* Chest = Cast<ATreasureChest>(OverlappedActor);
			if (Chest)
			{
				Chest->OpenChestUI(this); // �÷��̾ �����Ͽ� ���� UI ����
				UE_LOG(LogTemp, Log, TEXT("Opened chest UI"));
			}
			// �� ĳ���� ���� ��ȣ�ۿ�
			AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(OverlappedActor);
			if (Enemy)
			{
				Enemy->OpenLootUI(this);
				UE_LOG(LogTemp, Log, TEXT("Opened enemy loot UI"));
				return;
			}
		}
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
		// ���� ��ȣ�ۿ� ���� ���Ͱ� ������ Ȯ��
		if (OverlappedActor && OverlappedActor->ActorHasTag("Door"))
		{
			UE_LOG(LogTemp, Log, TEXT("Player is interacting with a door, skipping weapon pickup."));
			return; // ���� ��ȣ�ۿ� ���̸� ���� ���� ����
		}

		// ���� ���⸦ ������ �ʰ� �ٴڿ� ���������� ����
		if (EquippedWeapon)
		{
			DropWeapon();
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

			// ���� ��ġ ���ſ� �ʱ�ȭ
			if (EquippedWeapon && EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->LastStartLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackStart"));
				EquippedWeapon->LastEndLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackEnd"));
				EquippedWeapon->SetOwner(this);
			}

			// ���� �߷� �� ���� �ùķ��̼� ��Ȱ��ȭ
			if (EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->WeaponMesh->SetSimulatePhysics(false);  //���� ��Ȱ��ȭ
				EquippedWeapon->WeaponMesh->SetEnableGravity(false);    //�߷� ��Ȱ��ȭ
			}

			// ���� �浹 ��Ȱ��ȭ �� ���� ����
			EquippedWeapon->SetActorEnableCollision(false);
			EquippedWeapon->SetActorHiddenInGame(false);

			EquippedWeapon->ApplyWeaponStats(this);

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

		EquippedWeapon->RemoveWeaponStats(this);

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

void AMyDCharacter::EquipWeaponFromClass(TSubclassOf<AItem> WeaponClass)
{
	if (!WeaponClass) {
		return;
	}
	// ���� ���� ����
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	// �� ���� ����
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	// �� ���� ��ġ ���
	FTransform HandSocketTransform = CharacterMesh->GetSocketTransform(FName("hand_r"));
	UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());

	AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	EquippedWeapon = SpawnedWeapon;

	if (CharacterMesh && CharacterMesh->DoesSocketExist("hand_r"))
	{
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeapon->AttachToComponent(CharacterMesh, AttachRules, FName("hand_r"));

		// �ʼ� �ʱ�ȭ�� �߰�
		if (EquippedWeapon->WeaponMesh)
		{
			EquippedWeapon->WeaponMesh->SetSimulatePhysics(false);
			EquippedWeapon->WeaponMesh->SetEnableGravity(false);
			EquippedWeapon->SetActorEnableCollision(false);
			EquippedWeapon->SetActorHiddenInGame(false);

			EquippedWeapon->LastStartLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackStart"));
			EquippedWeapon->LastEndLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackEnd"));
		}

		EquippedWeapon->SetOwner(this);
		EquippedWeapon->ApplyWeaponStats(this);

		UE_LOG(LogTemp, Log, TEXT("Weapon equipped from class: %s"), *EquippedWeapon->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterMesh or hand_r socket missing"));
	}
}

void AMyDCharacter::UnequipWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
		UE_LOG(LogTemp, Log, TEXT("Weapon unequipped."));
	}
}


void AMyDCharacter::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);
		HUDWidget->UpdateMana(Knowledge, MaxKnowledge);
		HUDWidget->UpdateStamina(Stamina, MaxStamina);
		
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

void AMyDCharacter::PlayAttackAnimation()
{
	if (bIsAttacking)  // ���� ���̸� �Է� ����
	{
		UE_LOG(LogTemp, Log, TEXT("Already attacking! Ignoring input."));
		return;
	}

	if (bIsAttacking || bIsRolling)  // ������ ���̸� ���� �Է� ����
	{
		UE_LOG(LogTemp, Log, TEXT("Already attacking or rolling! Ignoring input."));
		return;
	}

	if (Stamina <= 0)  // ���¹̳��� 0�̸� ���� �Ұ�
	{
		ReduceStamina(0.0f);
		return;
	}

	// ���� ���� ����
	bIsAttacking = true;

	ReduceStamina(AttackStaminaCost);
	

	// ����� ��Ÿ�� ���� (���� ���� ���ο� ���� �ٸ��� ����)
	UAnimMontage* SelectedMontage = nullptr;

	if (EquippedWeapon)
	{
		switch (EquippedWeapon->WeaponType)
		{
		case EWeaponType::GreatWeapon:
			SelectedMontage = GreatWeaponMontage; // �̰� ������ �̸� �����־� ��
			break;
		case EWeaponType::Dagger:
			SelectedMontage = DaggerWeaponMontage;
			break;
		case EWeaponType::Staff:
			//SelectedMontage = StaffMontage;
			break;
		default:
			SelectedMontage = WeaponAttackMontage; // �⺻��
			break;
		}
	}
	else
	{
		SelectedMontage = UnarmedAttackMontage;
	}
	if (!SelectedMontage || !CharacterMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Error, TEXT("Attack montage or anim instance is missing!"));
		bIsAttacking = false;
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

	// **���� �޺� ���� ���� ����� ���� ����**
	FName SelectedSection = (AttackComboIndex == 0) ? FName("Combo1") : FName("Combo2");

	// Ư�� ���Կ����� ���� (UpperBody ���Կ��� ���)
	FName UpperBodySlot = FName("UpperBody");

	// UpperBody ���Կ��� ����ǵ��� ����
	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(SelectedMontage); 
	if (MontageInstance) 
	{
		MontageInstance->Montage->SlotAnimTracks[0].SlotName = UpperBodySlot; 
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StartTrace(); 
	}

	// �ִϸ��̼� ���� (���õ� ���� ó������ ���)
	AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	AnimInstance->Montage_JumpToSection(SelectedSection, SelectedMontage);
	
	UE_LOG(LogTemp, Log, TEXT("Playing Attack Montage Section: %s"), *SelectedSection.ToString());

	// **���� ������ ���� ��������**
	float SectionDuration = SelectedMontage->GetSectionLength(SelectedMontage->GetSectionIndex(SelectedSection));

	// **Ÿ�̸� ����: ��Ȯ�� ���� ���� �� ���� ���� �ʱ�ȭ**
	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, SectionDuration, false);

	// **���� �Է� �� �ٸ� ���� ����ǵ��� ���� (�޺� ����)**
	AttackComboIndex = (AttackComboIndex == 0) ? 1 : 0;
}


void AMyDCharacter::ResetAttack()
{
	// ���� ���� �ʱ�ȭ
	bIsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("Attack Ended, Resetting Attack State"));

	ResetHitActors();
}


void AMyDCharacter::PlayRollAnimation()
{
	if (bIsRolling || !RollMontage) return;

	if (Stamina <= 0)  // ���¹̳��� 0�̸� ���� �Ұ�
	{
		ReduceStamina(0.0f);
		return;
	}

	bIsRolling = true;

	// ���� �̵� ���� ���
	FVector RollDirection;
	FName SelectedSection = "RollF"; // �⺻�� (�ձ�����)

	ReduceStamina(RollStaminaCost);

	if (FMath::Abs(MoveForwardValue) > 0.1f || FMath::Abs(MoveRightValue) > 0.1f)
	{
		// ����Ű �Է��� ���� ���: �ش� �������� ������
		FRotator ControlRotation = GetControlRotation();
		FVector ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
		FVector RightVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);

		RollDirection = ForwardVector * MoveForwardValue + RightVector * MoveRightValue;
		RollDirection.Normalize();

		// **�Է� ���⿡ ���� ������ ���� ����**
		if (MoveForwardValue > 0.1f)
		{
			SelectedSection = "RollF";  // �ձ�����
		}
		else if (MoveForwardValue < -0.1f)
		{
			SelectedSection = "RollB";  // �ڱ�����
		}
		else if (MoveRightValue > 0.1f)
		{
			SelectedSection = "RollR";  // ������ ������
		}
		else if (MoveRightValue < -0.1f)
		{
			SelectedSection = "RollL";  // ���� ������
		}
	}
	else
	{
		// ����Ű �Է��� ���� ���: �⺻������ �ձ�����
		RollDirection = GetActorForwardVector();
	}

	// RollDirection�� ��� ������ ���� (�̵� �Լ����� ���)
	StoredRollDirection = RollDirection;

	// **������ �������� ĳ���� ȸ�� (ī�޶�� ����)**
	if (RollDirection.SizeSquared() > 0)
	{
		FRotator NewRotation = RollDirection.Rotation();
		SetActorRotation(NewRotation);
	}

	// **�ִϸ��̼� ���� (���õ� �������� ����)**
	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(RollMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(SelectedSection, RollMontage);
		UE_LOG(LogTemp, Log, TEXT("Playing Roll Montage Section: %s"), *SelectedSection.ToString());
	}

	// **������ �̵� ����**
	ApplyRollMovement(RollDirection);

	// **������ �� ���� ���� ����**
	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetRoll, RollDuration, false);
}

void AMyDCharacter::ResetRoll()
{
	bIsRolling = false;
	bIsAttacking = false;  // �����Ⱑ ������ ���� ���µ� �ʱ�ȭ
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}


void AMyDCharacter::ApplyRollMovement(FVector RollDirection)
{

	if (RollDirection.SizeSquared() > 0)
	{
		RollDirection.Z = 0.0f;
		RollDirection.Normalize();

		FVector RollImpulse = RollDirection * RollSpeed;

		// ���� ��� �̵�
		GetCharacterMovement()->Launch(RollImpulse);

		// ������ ���� �߷� ���� ���� (�ʿ��ϸ� �ּ� ����)
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

void AMyDCharacter::UpdateMoveForward(float Value)
{
	MoveForwardValue = Value;
}

void AMyDCharacter::UpdateMoveRight(float Value)
{
	MoveRightValue = Value;
}

void AMyDCharacter::ReduceStamina(float StaminaCost)
{
	//���¹̳� ����
	Stamina -= StaminaCost;
	Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);

	// UI ������Ʈ
	UpdateHUD();

	// ȸ�� �ߴ� & ����� ���
	GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
	ManageStaminaRegen();
}

void AMyDCharacter::ManageStaminaRegen()
{
	//�޸��� ���̸� ȸ�� ���� (��������)
	if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->MaxWalkSpeed > WalkSpeed)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
		return;
	}

	// 3�ʰ� �ൿ�� ������ ȸ�� ����
	if (!GetWorldTimerManager().IsTimerActive(TimerHandle_StaminaRegenDelay))
	{
		GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegenDelay, this, &AMyDCharacter::StartStaminaRegen, 3.0f, false);
	}
}

// ���¹̳� ���� ȸ�� �Լ�
void AMyDCharacter::StartStaminaRegen()
{
	if (!bIsAttacking && !bIsRolling && GetCharacterMovement()->MaxWalkSpeed == WalkSpeed)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegen, this, &AMyDCharacter::RegenerateStamina, 0.5f, true);
	}
}

void AMyDCharacter::RegenerateStamina() // ���¹̳� ȸ�� ����
{

	if (Stamina < MaxStamina)
	{
		Stamina += StaminaRegenRate;
		Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);

		//UI ������Ʈ �ݿ�
		HUDWidget->UpdateStamina(Stamina, MaxStamina);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
	}
}

void AMyDCharacter::GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage)
{

	Health -= Damage;
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);

	UpdateHUD();

	if (Health <= 0)
	{

		// ���� Ȱ��ȭ
		DoRagDoll();

		// ��Ʈ�ѷ� ��Ȱ��ȭ (�Է� ����)
		AController* PlayerController = GetController();
		if (PlayerController)
		{
			PlayerController->UnPossess();
		}

		// �ִϸ��̼� ����
		GetMesh()->bPauseAnims = true;
		GetMesh()->bNoSkeletonUpdate = true;

		// �̵� ���߱�
		GetCharacterMovement()->DisableMovement();
		return;

	}

}

void AMyDCharacter::DoRagDoll() 
{

	CharacterMesh->SetSimulatePhysics(true);
	CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
}

void AMyDCharacter::ResetHitActors()
{
	HitActors.Empty(); //�ǰ� ��� �ʱ�ȭ
}

void AMyDCharacter::ToggleInventoryUI()
{
	if (!InventoryWidgetInstance || !EquipmentWidgetInstance) {
		UE_LOG(LogTemp, Log, TEXT("nononononEQ"));
		return;
	}


	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (bIsInventoryVisible)
	{
		InventoryWidgetInstance->RemoveFromParent();
		EquipmentWidgetInstance->RemoveFromParent();
		bIsInventoryVisible = false;

		// ���콺 Ŀ�� ��Ȱ��ȭ + ���� �Է����� ��ȯ
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else
	{
		InventoryWidgetInstance->AddToViewport(10);
		InventoryWidgetInstance->SetPositionInViewport(FVector2D(0, 0), false);
		InventoryWidgetInstance->RefreshInventoryStruct();

		EquipmentWidgetInstance->AddToViewport(10); // �κ��丮���� ���� ���� ����
		EquipmentWidgetInstance->SetPositionInViewport(FVector2D(100, 0), false);
		EquipmentWidgetInstance->RefreshEquipmentSlots(); // ���߿� �Լ����� ���� ���� �ݿ��ϰ� ���� �� ����

		//if (CombinedInventoryWidgetClass)
		//{
		//	CombinedInventoryWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), CombinedInventoryWidgetClass);
		//	if (CombinedInventoryWidgetInstance)
		//	{
		//		CombinedInventoryWidgetInstance->AddToViewport(10); // ZOrder ������
		//		//CombinedInventoryWidgetInstance->SetVisibility(ESlateVisibility::Hidden); // ó���� ���ܵ�
		//		InventoryWidgetInstance->RefreshInventoryStruct();
		//		EquipmentWidgetInstance->RefreshEquipmentSlots();
		//		UE_LOG(LogTemp, Log, TEXT("wrwrwrwrrEQ"));
		//	}
		//}

		bIsInventoryVisible = true;

		// ���콺 Ŀ�� ǥ�� + UI �Է� ���� ��ȯ
		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // ���콺 �̵� ���� ����
		//InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());  // UI�� ��Ŀ��
		InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());
		PC->SetInputMode(InputMode);
	}
}

bool AMyDCharacter::IsDead() const
{
	return Health <= 0.0f;
}