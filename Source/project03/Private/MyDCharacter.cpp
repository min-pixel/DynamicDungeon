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
#include "Armor.h"
#include "Hat.h"
#include "Mask.h"
#include "RobeTop.h"
#include "RobeBottom.h"
#include "StaminaPotion.h"
#include "ManaPotion.h"
#include "HealSpell.h"
#include "CurseSpell.h"
#include "GoldWidget.h"
#include "InventoryWidget.h"
#include "PlayerCharacterData.h"
#include "TreasureChest.h"
#include "EnemyCharacter.h"
#include "Components/Image.h"
#include "NiagaraFunctionLibrary.h"
#include "RageEnemyCharacter.h"
#include "Potion.h"
#include "FireballSpell.h"
#include "Camera/CameraShakeBase.h"
#include "Net/UnrealNetwork.h"
#include "WFCRegenerator.h"
#include "Components/ProgressBar.h"
#include "Animation/AnimBlueprintGeneratedClass.h"

// �⺻ ������
AMyDCharacter::AMyDCharacter()
{
	// �� ������ Tick�� Ȱ��ȭ
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AutoPossessPlayer = EAutoReceiveInput::Disabled;

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
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'"));  //  ,,,,/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'
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


	//���� ���� ��Ÿ�� �ε�
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontageObj(TEXT("/Game/BP/character/Retarget/RTA_Magic_Heal_Anim_mixamo_com_Montage.RTA_Magic_Heal_Anim_mixamo_com_Montage"));
	if (MontageObj.Succeeded())
	{
		SpellCastMontage = MontageObj.Object;
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

	// ����
	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh"));
	ChestMesh->SetupAttachment(GetMesh());
	ChestMesh->SetLeaderPoseComponent(CharacterMesh);
	ChestMesh->SetRelativeLocation(FVector(0.0f, 0.f, -90.f));
	ChestMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	ChestMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	//ChestMesh->SetRelativeLocation(FVector(12.5f, 0.f, -95.f));  // ���ϴ� ��ġ�� ����
	//ChestMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	//ChestMesh->SetRelativeScale3D(FVector(1.1f, 1.5f, 1.0f));
	ChestMesh->SetVisibility(true);

	// ����
	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh"));
	LegsMesh->SetupAttachment(GetMesh());
	LegsMesh->SetLeaderPoseComponent(CharacterMesh);
	LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
	LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	LegsMesh->SetVisibility(false);

	// ����
	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
	HelmetMesh->SetupAttachment(CharacterMesh, TEXT("hellmet")); 
	
	HelmetMesh->SetVisibility(false);

	//idle ���� �߰� (��ü)
	/*static ConstructorHelpers::FObjectFinder<UAnimMontage> PoseMontageAsset(TEXT("/Game/BP/character/Retarget/RTA_Male_Sitting_Pose_Anim_mixamo_com_Montage.RTA_Male_Sitting_Pose_Anim_mixamo_com_Montage"));
	if (PoseMontageAsset.Succeeded())
	{
		PoseMontage = PoseMontageAsset.Object;
	}*/

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
	FirstPersonCameraComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 50.0f));  // �Ӹ� ��ġ , ������ 20.0f, 0.0f, 50.0f, �׽�Ʈ�� -100.0f, 0.0f, 80.0f
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
	//if (!HasAnyFlags(RF_ClassDefaultObject))
	//{
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);  // or ECC_GameTraceChannel1 �� ����� ä�� ���� ����

		// �̷��� ��� ä�ο� ���� ������ �ϰ�
		InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
		InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapBegin);
		InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapEnd);
	//}

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

	

	//Tags.Add(FName("Player"));

	static ConstructorHelpers::FClassFinder<UUserWidget> WFCWarnBP(TEXT("/Game/BP/UI/WFCWarningWidget.WFCWarningWidget_C"));
	if (WFCWarnBP.Succeeded())
	{
		WFCWarningWidgetClass = WFCWarnBP.Class;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load WFCWarningWidget blueprint!"));
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> WFCDoneBP(TEXT("/Game/BP/UI/WFCDoneWidget.WFCDoneWidget_C")); //Game/BP/UI/WFCDoneWidget.WFCDoneWidget
	if (WFCDoneBP.Succeeded())
	{
		WFCDoneWidgetClass = WFCDoneBP.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> EscapeWarningBP(TEXT("/Game/BP/UI/EscapeWarningWidget.EscapeWarningWidget_C"));
	if (EscapeWarningBP.Succeeded())
	{
		EscapeWarningWidgetClass = EscapeWarningBP.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> EscapeDoneBP(TEXT("/Game/BP/UI/EscapeDoneWidget.EscapeDoneWidget_C"));
	if (EscapeDoneBP.Succeeded())
	{
		EscapeDoneWidgetClass = EscapeDoneBP.Class;
	}

	static ConstructorHelpers::FClassFinder<UGoldWidget> GoldWidgetBPClass(TEXT("/Game/BP/UI/goldWidget_BP.goldWidget_BP_C"));
	if (GoldWidgetBPClass.Succeeded())
	{
		GoldWidgetClass = GoldWidgetBPClass.Class;
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded GoldWidget_BP."));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load GoldWidget_BP! Check the path."));
	}


	static ConstructorHelpers::FClassFinder<ACameraActor> CameraBPClass(TEXT("/Game/BP/BP_OverheadCamera.BP_OverheadCamera_C"));
	if (CameraBPClass.Succeeded())
	{
		OverheadCameraClass = CameraBPClass.Class;
	}

}

// ���� ���� �� ȣ��
void AMyDCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!CharacterMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("CharacterMesh is null!"));
	}
	if (!FirstPersonCameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("FirstPersonCameraComponent is null!"));
	}
	
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

	//if (IsLocallyControlled())
	//{
	//	// ���� ������ �׻�!
	//	if (InventoryWidgetClass)
	//	{
	//		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
	//		if (InventoryWidgetInstance)
	//		{
	//			InventoryWidgetInstance->InventoryRef = InventoryComponent;
	//			InventoryWidgetInstance->RefreshInventoryStruct();

	//			//if (InventoryComponent)
	//			//{
	//			//	InventoryComponent->TryAddItemByClass(AGreatWeapon::StaticClass());

	//			//	InventoryWidgetInstance->RefreshInventoryStruct(); // �ٽ� ����
	//			//}

	//		}
	//	}

	//	if (EquipmentWidgetClass)
	//	{
	//		EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);

	//		if (EquipmentWidgetClass)
	//		{
	//			EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);

	//			EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;
	//		}
	//	}
	//}
	


	if (OverheadCameraClass)
	{
		OverheadCameraActor = GetWorld()->SpawnActor<ACameraActor>(OverheadCameraClass, FVector(15000, 16000, 50500), FRotator(-90, 0, 0));
		if (OverheadCameraActor)
		{
			UE_LOG(LogTemp, Log, TEXT("OverheadCameraActor Spawned: %s"), *OverheadCameraActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("OverheadCameraActor Spawn Failed"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OverheadCameraClass is null!"));
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

	//UBlueprint* LightBP = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, TEXT("/Game/BP/light.light")));
	//if (LightBP && LightBP->GeneratedClass)
	//{
	//	AttachedTorch = GetWorld()->SpawnActor<AActor>(LightBP->GeneratedClass);

	//	if (AttachedTorch && CharacterMesh && CharacterMesh->DoesSocketExist("hand_l"))
	//	{
	//		AttachedTorch->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_l"));

	//		// ȸ�� ���� (�ʿ��ϴٸ� ���� ����)
	//		FRotator DesiredRotation = FRotator(0.0f, 0.0f, 180.0f);
	//		AttachedTorch->SetActorRelativeRotation(DesiredRotation);

	//		// ó���� ������ ����
	//		AttachedTorch->SetActorHiddenInGame(true);
	//		bTorchVisible = false;

	//		UE_LOG(LogTemp, Log, TEXT("Torch attached and hidden on start."));
	//	}
	//}

	FSoftObjectPath TorchClassPath(TEXT("/Game/BP/light.light_C"));  // �� "_C" ���� ��!
	UClass* TorchClass = Cast<UClass>(TorchClassPath.TryLoad());

	if (TorchClass)
	{
		AttachedTorch = GetWorld()->SpawnActor<AActor>(TorchClass);

		if (AttachedTorch && CharacterMesh && CharacterMesh->DoesSocketExist("hand_l"))
		{
			AttachedTorch->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_l"));
			AttachedTorch->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 180.0f));
			AttachedTorch->SetActorHiddenInGame(true);
			bTorchVisible = false;

			UE_LOG(LogTemp, Log, TEXT("Torch attached and hidden on start."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Torch class at runtime!"));
	}


	if (WFCWarningWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("WFCWarningWidgetInstance Created Successfully"));
		WFCWarningWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WFCWarningWidgetClass);
		WFCWarningWidgetInstance->AddToViewport();
		WFCWarningWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}
	if (WFCDoneWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("WFCWarningWidgetInstance Created Successfully"));
		WFCDoneWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WFCDoneWidgetClass);
		WFCDoneWidgetInstance->AddToViewport();
		WFCDoneWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (EscapeWarningWidgetClass)
	{
		EscapeWarningWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), EscapeWarningWidgetClass);
		EscapeWarningWidgetInstance->AddToViewport();
		EscapeWarningWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (EscapeDoneWidgetClass)
	{
		EscapeDoneWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), EscapeDoneWidgetClass);
		EscapeDoneWidgetInstance->AddToViewport();
		EscapeDoneWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (GoldWidgetClass)
	{
		GoldWidgetInstance = CreateWidget<UGoldWidget>(GetWorld(), GoldWidgetClass);
		if (GoldWidgetInstance)
		{
			GoldWidgetInstance->AddToViewport();
			GoldWidgetInstance->UpdateGoldAmount(Gold);
			//GoldWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}



	//UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	//if (GameInstance)
	//{
	//	ApplyCharacterData(GameInstance->CurrentCharacterData);

	//	// �κ��丮 ����
	//	if (InventoryComponent)
	//	{
	//		InventoryComponent->InventoryItemsStruct = GameInstance->SavedInventoryItems;
	//	}

	//	// ���â ����
	//	if (EquipmentWidgetInstance)
	//	{
	//		EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
	//	}

	//	//��� ����
	//	if (GameInstance)
	//	{
	//		Gold = GameInstance->LobbyGold;
	//		GoldWidgetInstance->UpdateGoldAmount(Gold);
	//	}

	//}

	
		UFireballSpell* Fireball = NewObject<UFireballSpell>(this);
		SpellSet.Add(Fireball); // �ε��� 0��: ���̾� ��

		UHealSpell* Heal = NewObject<UHealSpell>(this);
		SpellSet.Add(Heal); // �ε��� 1��: �� ����

		UCurseSpell* Curse = NewObject<UCurseSpell>(this);
		SpellSet.Add(Curse);

	
	

	if (!CachedDirectionalLight)
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			CachedDirectionalLight = *It;
			break; // �ϳ��� ����Ѵٰ� ����
		}

		if (CachedDirectionalLight)
		{
			UE_LOG(LogTemp, Log, TEXT("Directional Light cached: %s"), *CachedDirectionalLight->GetName());
		}
	}


}



void AMyDCharacter::Restart()
{
	Super::Restart();

	if (IsLocallyControlled())
	{

		if (HUDWidgetClass)
		{
			// ���� HUD ���� (�ִٸ�)
			if (HUDWidget)
			{
				HUDWidget->RemoveFromParent();
				HUDWidget = nullptr;
			}

			// ���� ����
			HUDWidget = CreateWidget<UUCharacterHUDWidget>(GetWorld(), HUDWidgetClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport(1);
				UE_LOG(LogTemp, Warning, TEXT("Restart: HUD Widget created and added to viewport"));
			}
		}

		if (InventoryWidgetClass)
		{
			InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
			if (InventoryWidgetInstance)
			{
				InventoryWidgetInstance->InventoryRef = InventoryComponent;
				InventoryComponent->OwningWidgetInstance = InventoryWidgetInstance;
				InventoryWidgetInstance->RefreshInventoryStruct();

			}
		}

		if (EquipmentWidgetClass)
		{
			EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);
			if (EquipmentWidgetInstance)
			{
				EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;
			}
		}
	}
}

void AMyDCharacter::SetupInventoryAndEquipmentUI()
{
	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->InventoryRef = InventoryComponent;
			InventoryComponent->OwningWidgetInstance = InventoryWidgetInstance;
			InventoryWidgetInstance->RefreshInventoryStruct();
		}
	}

	if (EquipmentWidgetClass)
	{
		EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);
		if (EquipmentWidgetInstance)
		{
			EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;
		}
	}
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
	if (bIsSlowed)
	{
		ApplySpeedMultiplier();
	}
}

// �޸��� ����
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
	GetWorldTimerManager().ClearTimer(TimerHandle_SprintDrain);
	ManageStaminaRegen();  //�޸��� ������ ���׹̳� ȸ�� ��Ű��

	if (bIsSlowed)
	{
		ApplySpeedMultiplier();
	}
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

	//���� ��� �߰� (QŰ)
	PlayerInputComponent->BindAction("CastSpell1", IE_Pressed, this, &AMyDCharacter::CastSpell1);

	//���� ��� �߰� (EŰ)
	PlayerInputComponent->BindAction("CastSpell2", IE_Pressed, this, &AMyDCharacter::CastSpell2);
	
		PlayerInputComponent->BindAction("ToggleMapView", IE_Pressed, this, &AMyDCharacter::ToggleMapView);
	
	//fŰ
	PlayerInputComponent->BindAction("ToggleTorch", IE_Pressed, this, &AMyDCharacter::ToggleTorch);

	//m
	PlayerInputComponent->BindAction("TeleportToRegen", IE_Pressed, this, &AMyDCharacter::TeleportToWFCRegen);

	//n
	PlayerInputComponent->BindAction("TeleportToEscape", IE_Pressed, this, &AMyDCharacter::TeleportToEscapeObject);

	//b
	PlayerInputComponent->BindAction("TeleportToChest", IE_Pressed, this, &AMyDCharacter::TeleportToNearestChest);

	//v
	PlayerInputComponent->BindAction("TeleportToEnemy", IE_Pressed, this, &AMyDCharacter::TeleportToNearestEnemy);

	//y
	PlayerInputComponent->BindAction("TeleportToNearestPlayer", IE_Pressed, this, &AMyDCharacter::TeleportToNearestPlayer);

	// ��Ű �Է� ���ε� (��: 1~5 Ű)

	PlayerInputComponent->BindAction("UseHotkey1", IE_Pressed, this, &AMyDCharacter::UseHotkey1);
	PlayerInputComponent->BindAction("UseHotkey2", IE_Pressed, this, &AMyDCharacter::UseHotkey2);
	PlayerInputComponent->BindAction("UseHotkey3", IE_Pressed, this, &AMyDCharacter::UseHotkey3);
	PlayerInputComponent->BindAction("UseHotkey4", IE_Pressed, this, &AMyDCharacter::UseHotkey4);
	PlayerInputComponent->BindAction("UseHotkey5", IE_Pressed, this, &AMyDCharacter::UseHotkey5);
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

	UE_LOG(LogTemp, Warning, TEXT("OnOverlapEnd: Resetting OverlappedActor"));
	// ���� ����: �׳� ������ �������� OverlappedActor�� �ʱ�ȭ
	OverlappedActor = nullptr;

	// GameInstance ���� �ʱ�ȭ
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->itemEAt = false;
		GameInstance->OpenDoor = false;
		GameInstance->WeaponEAt = false;
		UE_LOG(LogTemp, Log, TEXT("Overlap Ended - Reset GameInstance variables"));
	}

	if (OtherActor)
	{
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
			ARageEnemyCharacter* RageEnemy = Cast<ARageEnemyCharacter>(OverlappedActor);
			if (RageEnemy)
			{
				RageEnemy->OpenLootUI(this);
				UE_LOG(LogTemp, Log, TEXT("Opened enemy loot UI"));
				return;
			}
			


		}

		if (OverlappedActor && OverlappedActor->ActorHasTag("WFCRegen") && !bIsWFCCountdownActive)
		{
			PendingRegenActor = OverlappedActor;
			bIsWFCCountdownActive = true;

			//��� UI ǥ�� (UMG�� ǥ���ϴ� ��Ŀ� ���� ���� �ʿ�)
			UE_LOG(LogTemp, Warning, TEXT("55555se"));

			// ������ �ƴ� �׻� ���� ȣ��
			if (HasAuthority())
			{
				MulticastPlayWFCRegenEffects();
			}
			else
			{
				ServerPlayWFCRegenEffects();
			}
			return;

			
		}

		if (OverlappedActor && OverlappedActor->ActorHasTag("Escape") && !bIsEscapeCountdownActive) 
		{
			PendingEscapeActor = OverlappedActor;
			bIsEscapeCountdownActive = true;

			TriggerEscapeSequence();

			return;
		}

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("StartInteraction(): GameInstance not found!"));
	}

	if (OverlappedActor && OverlappedActor->ActorHasTag("Door"))
	{
		Server_TryOpenDoor(OverlappedActor);
	}
	
	if (OverlappedActor && OverlappedActor->ActorHasTag("Statue"))
	{
		Server_TryEatStatue(OverlappedActor);
	}

}


void AMyDCharacter::Server_TryOpenDoor_Implementation(AActor* DoorActor)
{
	// DoorActor�� Door BP�� ĳ����
	if (DoorActor)
	{
		// ��: Door BP�� Server_RequestOpenDoor��� �Լ��� �ִٰ� ����
		DoorActor->CallFunctionByNameWithArguments(TEXT("Server_RequestOpenDoor"), *GLog, nullptr, true);
	}
}

void AMyDCharacter::Server_TryEatStatue_Implementation(AActor* StatueActor)
{
	
	if (StatueActor)
	{
		
		StatueActor->CallFunctionByNameWithArguments(TEXT("ServerRequestEatStatue"), *GLog, nullptr, true);
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

void AMyDCharacter::EquipWeaponFromClass(TSubclassOf<AItem> WeaponClass, EWeaponGrade Grade)
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
	FTransform HandSocketTransform = CharacterMesh->GetSocketTransform(FName("middle_01_r"));
	UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());

	AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (SpawnedWeapon) {
		SpawnedWeapon->WeaponGrade = Grade;  
		SpawnedWeapon->ApplyGradeEffects(this);     
	}

	EquippedWeapon = SpawnedWeapon;

	if (CharacterMesh && CharacterMesh->DoesSocketExist("middle_01_r"))
	{
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeapon->AttachToComponent(CharacterMesh, AttachRules, FName("middle_01_r"));

		if (EquippedWeapon->WeaponMesh && EquippedWeapon->WeaponMesh->DoesSocketExist("GripWeapon"))
		{
			FTransform GripTransform = EquippedWeapon->WeaponMesh->GetSocketTransform(FName("GripWeapon"), RTS_Component);

			// ���� ��ġ�� �� ���Ͽ� ������ WeaponMesh ��ü�� �ݴ�� �̵�
			FVector OffsetLocation = -GripTransform.GetLocation();
			FRotator OffsetRotation = (-GripTransform.GetRotation()).Rotator();

			EquippedWeapon->WeaponMesh->SetRelativeLocation(OffsetLocation);
			EquippedWeapon->WeaponMesh->SetRelativeRotation(OffsetRotation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GripWeapon ������ StaticMesh�� ����"));
		}


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


void AMyDCharacter::EquipArmorMesh(int32 SlotIndex, USkeletalMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor)
{
	if (!NewMesh) return;

	UE_LOG(LogTemp, Warning, TEXT("EquipArmorMesh called with SlotIndex = %d, Mesh = %s"),
		SlotIndex, *NewMesh->GetName());

	// --- �⺻ ��Ƽ���� �������̵� ���� ���� ����
	auto ResetToDefaultMaterials = [&](USkeletalMeshComponent* MeshComp) {
		if (!MeshComp) return;
		int32 NumMats = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			// nullptr�� �ѱ�� ���¿� ������ ��Ƽ����� �ڵ� �����˴ϴ�
			MeshComp->SetMaterial(i, nullptr);
		}
	};

	switch (SlotIndex)
	{
	case EQUIP_SLOT_CHEST:




		if (ChestMesh)
		{
			ChestMesh->SetMasterPoseComponent(CharacterMesh);
			ChestMesh->SetSkeletalMesh(NewMesh);
			ChestMesh->SetVisibility(true);
			// �ʿ� �� ��� ��ġ ����
			ChestMesh->SetRelativeLocation(FVector(0.0f, 0.f, -90.f));
			ChestMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
			if (Armor && Armor->IsA(ARobeTop::StaticClass()))
			{
				ChestMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
				// ��Ƽ���� ���� (���Ǵ� ��� ���� �ݺ�)
				if (SlotIndex == EQUIP_SLOT_CHEST && ChestMesh)
				{
					int32 NumMaterials = ChestMesh->GetNumMaterials();

					for (int32 i = 0; i < NumMaterials; ++i)
					{
						if (Grade == EArmorGrade::B && SilverMat)
						{
							ChestMesh->SetMaterial(i, SilverMat);
						}
						else if (Grade == EArmorGrade::A && GoldMat)
						{
							ChestMesh->SetMaterial(i, GoldMat);
						}
					}
				}

				
			}
			else
			{
				ChestMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			}

			ResetToDefaultMaterials(ChestMesh);

			// ��Ƽ���� ����
			if (Grade == EArmorGrade::B && SilverMat)
			{
				ChestMesh->SetMaterial(0, SilverMat);
			}
			else if (Grade == EArmorGrade::A && GoldMat)
			{
				ChestMesh->SetMaterial(0, GoldMat);
			}

		}
		break;

	case EQUIP_SLOT_LEG:
		if (LegsMesh)
		{
			//LegsMesh->SetMasterPoseComponent(CharacterMesh);
			LegsMesh->SetSkeletalMesh(NewMesh);
			LegsMesh->SetVisibility(true);
			// ���� ��Ʈ ��ġ�� ĳ���ͺ��� �ʹ� ���� ���� �� ���� �ʿ�
			LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
			LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
			if (Armor && Armor->IsA(ARobeBottom::StaticClass()))
			{
				LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.25f, 1.0f));
			}
			else
			{
				LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			}
			//LegsMesh->SetLeaderPoseComponent(CharacterMesh);

			ResetToDefaultMaterials(LegsMesh);

			// ��Ƽ���� ����
			if (Grade == EArmorGrade::B && SilverMat)
			{
				LegsMesh->SetMaterial(0, SilverMat);
			}
			else if (Grade == EArmorGrade::A && GoldMat)
			{
				LegsMesh->SetMaterial(0, GoldMat);
			}


			UE_LOG(LogTemp, Warning, TEXT("EquipArmorMesh called with2222 SlotIndex = %d, Mesh = %s"),
				SlotIndex, *NewMesh->GetName());
		}
		break;
	}





}


void AMyDCharacter::EquipArmorFromClass(int32 SlotIndex, TSubclassOf<AItem> ArmorClass, uint8 Grade)
{
	if (!ArmorClass) return;

	// ���� ���� ������ ����
	UnequipArmorAtSlot(SlotIndex);

	// ��� ���� ����
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AArmor* Armor = GetWorld()->SpawnActor<AArmor>(ArmorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Armor) return;

	Armor->ArmorGrade = static_cast<EArmorGrade>(Grade);

	// �޽ø� ĳ���� ������Ʈ�� ����

	// ��� ������ StaticMesh ó��
	if (SlotIndex == EQUIP_SLOT_HELMET)
	{
		
			// �Ϲ� ���(StaticMesh) ó��
			UStaticMesh* HelmetMeshAsset = Armor->HelmetStaticMesh;
			if (HelmetMeshAsset)
			{
				EquipHelmetMesh(HelmetMeshAsset, Armor->ArmorGrade, Armor->SilverMaterial, Armor->GoldMaterial, Armor);
			}
		
	}
	else {
		USkeletalMesh* EquippedMesh = Armor->ArmorVisualMesh->GetSkeletalMeshAsset();
		if (EquippedMesh)
		{
			EquipArmorMesh(SlotIndex, EquippedMesh, Armor->ArmorGrade, Armor->SilverMaterial, Armor->GoldMaterial, Armor);
		}
	}
	

	UE_LOG(LogTemp, Warning, TEXT("[ARMOR EQUIP] Slot %d �� %s (Grade: %s)"),
		SlotIndex,
		*Armor->GetName(),
		*UEnum::GetValueAsString(Armor->ArmorGrade));

	// ���� �ݿ� �� ��� ��� ���
	EquippedArmors.Add(SlotIndex, Armor);
	Armor->ApplyArmorStats(this);

	// Armor ���ʹ� �ð������� �ʿ� ����
	Armor->SetActorHiddenInGame(true);
	Armor->SetActorEnableCollision(false);
}

void AMyDCharacter::EquipHelmetMesh(UStaticMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor)
{
	if (!NewMesh || !HelmetMesh) return;

	HelmetMesh->SetStaticMesh(NewMesh);
	HelmetMesh->SetVisibility(true);

	// �⺻ ũ��
	FVector DesiredScale = FVector(1.f);

	// ���ڸ� ũ�� ���̱�
	if (Armor && Armor->IsA<AHat>() || Armor->IsA<AMask>())
	{
		DesiredScale = FVector(0.1f); // ���ϴ� ũ��� ����
		UE_LOG(LogTemp, Log, TEXT("Helmet: AHat detected, scaled down"));
	}

	HelmetMesh->SetRelativeScale3D(DesiredScale);

	
	{
		int32 NumMats = HelmetMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			// nullptr �� �����ϸ� ���¿� ������ �⺻ ��Ƽ����� �ڵ� �����˴ϴ�
			HelmetMesh->SetMaterial(i, nullptr);
		}
	}


	// ��Ƽ���� ���� ����
	switch (Grade)
	{
	case EArmorGrade::B:
		if (SilverMat)
		{
			HelmetMesh->SetMaterial(0, SilverMat);
			UE_LOG(LogTemp, Log, TEXT("Helmet: Silver Material applied"));
		}
		break;

	case EArmorGrade::A:
		if (GoldMat)
		{
			HelmetMesh->SetMaterial(0, GoldMat);
			UE_LOG(LogTemp, Log, TEXT("Helmet: Gold Material applied"));
		}
		break;

	default:
		UE_LOG(LogTemp, Log, TEXT("Helmet: Default (C grade) Material retained"));
		break;
	}


	HelmetMesh->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	HelmetMesh->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	//HelmetMesh->SetRelativeScale3D(FVector(1.f));


	

}


void AMyDCharacter::UnequipArmorAtSlot(int32 SlotIndex)
{
	if (EquippedArmors.Contains(SlotIndex))
	{
		AArmor* Armor = EquippedArmors[SlotIndex];
		if (Armor)
		{
			Armor->RemoveArmorStats(this);
			Armor->Destroy();
		}
		// �ð������ε� ����
		switch (SlotIndex)
		{
		case EQUIP_SLOT_CHEST:
			if (ChestMesh)
			{
				ChestMesh->SetSkeletalMesh(nullptr);
				ChestMesh->SetVisibility(false);
			}
			break;

		case EQUIP_SLOT_LEG:
			if (LegsMesh)
			{
				LegsMesh->SetSkeletalMesh(nullptr);
				LegsMesh->SetVisibility(false);
			}
			break;

		case EQUIP_SLOT_HELMET:
			if (HelmetMesh)
			{
				HelmetMesh->SetStaticMesh(nullptr);
				HelmetMesh->SetVisibility(false);
			}
			break;
		}


		EquippedArmors.Remove(SlotIndex);
	}
}

void AMyDCharacter::ServerRequestEquipWeapon_Implementation(const FItemData& WeaponData)
{
	EquippedWeaponData = WeaponData;
	EquipWeaponFromClass(WeaponData.ItemClass, static_cast<EWeaponGrade>(WeaponData.Grade));
}

void AMyDCharacter::ServerRequestEquipArmor_Implementation(const FItemData& ArmorData, int32 SlotIndex)
{
	FEquippedArmorData NewEntry;
	NewEntry.SlotIndex = SlotIndex;
	NewEntry.ArmorData = ArmorData;

	// ���� ���� ������ ����
	bool bFound = false;
	for (FEquippedArmorData& Entry : EquippedArmorsData)
	{
		if (Entry.SlotIndex == SlotIndex)
		{
			Entry.ArmorData = ArmorData;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		EquippedArmorsData.Add(NewEntry);
	}

	EquipArmorFromClass(SlotIndex, ArmorData.ItemClass, ArmorData.Grade);
}

void AMyDCharacter::OnRep_EquippedWeapon()
{
	if (EquippedWeaponData.ItemClass)
	{
		EquipWeaponFromClass(EquippedWeaponData.ItemClass, static_cast<EWeaponGrade>(EquippedWeaponData.Grade));
	}
	else
	{
		UnequipWeapon();
	}
}

void AMyDCharacter::OnRep_EquippedArmors()
{
	UE_LOG(LogTemp, Log, TEXT("Client: OnRep_EquippedArmors called"));

	// ���� ������ ���Ե� Ȯ��
	TArray<int32> CurrentSlots;
	for (auto& Pair : EquippedArmors)
	{
		CurrentSlots.Add(Pair.Key);
	}

	// ���ο� �����Ϳ� �ִ� ���Ե� Ȯ��
	TArray<int32> NewSlots;
	for (const FEquippedArmorData& ArmorEntry : EquippedArmorsData)
	{
		NewSlots.Add(ArmorEntry.SlotIndex);
	}

	// ���ŵ� ���Ե� ����
	for (int32 CurrentSlot : CurrentSlots)
	{
		if (!NewSlots.Contains(CurrentSlot))
		{
			UnequipArmorAtSlot(CurrentSlot);
			UE_LOG(LogTemp, Log, TEXT("Client: Unequipped armor at slot %d"), CurrentSlot);
		}
	}

	// ���ο� ���� ����
	for (const FEquippedArmorData& ArmorEntry : EquippedArmorsData)
	{
		if (!CurrentSlots.Contains(ArmorEntry.SlotIndex))
		{
			EquipArmorFromClass(ArmorEntry.SlotIndex, ArmorEntry.ArmorData.ItemClass, ArmorEntry.ArmorData.Grade);
			UE_LOG(LogTemp, Log, TEXT("Client: Equipped armor at slot %d"), ArmorEntry.SlotIndex);
		}
	}
}

void AMyDCharacter::ServerRequestUnequipWeapon_Implementation()
{
	EquippedWeaponData = FItemData(); // �⺻��
	//ForceNetUpdate();

	if (IsLocallyControlled())
	{
		UnequipWeapon();
	}

	OnRep_EquippedWeapon();
}

void AMyDCharacter::ServerRequestUnequipArmor_Implementation(int32 SlotIndex)
{
	// �����Ϳ��� ����
	for (int32 i = 0; i < EquippedArmorsData.Num(); ++i)
	{
		if (EquippedArmorsData[i].SlotIndex == SlotIndex)
		{
			EquippedArmorsData.RemoveAt(i);
			break;
		}
	}

	// �������� ���� ���� (���� ����)
	UnequipArmorAtSlot(SlotIndex);

	// ��Ʈ��ũ ������Ʈ ����
	ForceNetUpdate();

	UE_LOG(LogTemp, Log, TEXT("Server: Unequipped armor at slot %d"), SlotIndex);
}


void AMyDCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyDCharacter, EquippedWeaponData);
	DOREPLIFETIME(AMyDCharacter, EquippedArmorsData);
	DOREPLIFETIME(AMyDCharacter, Health);
	DOREPLIFETIME(AMyDCharacter, bIsSlowed);
	DOREPLIFETIME(AMyDCharacter, OriginalWalkSpeed);
	DOREPLIFETIME(AMyDCharacter, SpeedMultiplier);
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

void AMyDCharacter::OnRep_Health()
{
	if (GetLocalRole() == ROLE_AutonomousProxy && HUDWidget)
	{
		// �񱳴� ���⼭!
		if (PreviousHealth < 0.0f || Health < PreviousHealth)
		{
			// ���� ���� ���� ����
			float NormalizedHealth = Health / MaxHealth;
			float OverlayAlpha = FMath::Clamp(1.0f - NormalizedHealth, 0.0f, 0.6f);

			HUDWidget->Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, OverlayAlpha));
			HUDWidget->Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			HUDWidget->StartHitOverlayFadeOut();
		}

		// HUD�� �ܼ��� ���� ü�� ǥ��
		HUDWidget->UpdateHealth(Health, MaxHealth);

		PreviousHealth = Health;
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

void AMyDCharacter::PlayMagicMontage()
{
	if (!SpellCastMontage || !CharacterMesh || !CharacterMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Error, TEXT("SpellCastMontage or AnimInstance missing!"));
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

	// ��Ÿ�� ���
	float Duration = AnimInstance->Montage_Play(SpellCastMontage, 1.0f);
}

void AMyDCharacter::PlayAttackAnimation()
{
	//if (bIsAttacking)  // ���� ���̸� �Է� ����
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Already attacking! Ignoring input."));
	//	return;
	//}

	//if (bIsAttacking || bIsRolling)  // ������ ���̸� ���� �Է� ����
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Already attacking or rolling! Ignoring input."));
	//	return;
	//}

	//if (Stamina <= 0)  // ���¹̳��� 0�̸� ���� �Ұ�
	//{
	//	ReduceStamina(0.0f);
	//	return;
	//}

	//// ���� ���� ����
	//bIsAttacking = true;

	//ReduceStamina(AttackStaminaCost);
	//

	//// ����� ��Ÿ�� ���� (���� ���� ���ο� ���� �ٸ��� ����)
	//UAnimMontage* SelectedMontage = nullptr;

	//if (EquippedWeapon)
	//{
	//	switch (EquippedWeapon->WeaponType)
	//	{
	//	case EWeaponType::GreatWeapon:
	//		SelectedMontage = GreatWeaponMontage; // �̰� ������ �̸� �����־� ��
	//		break;
	//	case EWeaponType::Dagger:
	//		SelectedMontage = DaggerWeaponMontage;
	//		break;
	//	case EWeaponType::Staff:
	//		//SelectedMontage = StaffMontage;
	//		break;
	//	default:
	//		SelectedMontage = WeaponAttackMontage; // �⺻��
	//		break;
	//	}
	//}
	//else
	//{
	//	SelectedMontage = UnarmedAttackMontage;
	//}
	//if (!SelectedMontage || !CharacterMesh->GetAnimInstance())
	//{
	//	UE_LOG(LogTemp, Error, TEXT("Attack montage or anim instance is missing!"));
	//	bIsAttacking = false;
	//	return;
	//}

	//UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

	//// **���� �޺� ���� ���� ����� ���� ����**
	//FName SelectedSection = (AttackComboIndex == 0) ? FName("Combo1") : FName("Combo2");

	//// Ư�� ���Կ����� ���� (UpperBody ���Կ��� ���)
	//FName UpperBodySlot = FName("UpperBody");

	//// UpperBody ���Կ��� ����ǵ��� ����
	//FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(SelectedMontage); 
	//if (MontageInstance) 
	//{
	//	MontageInstance->Montage->SlotAnimTracks[0].SlotName = UpperBodySlot; 
	//}

	//if (EquippedWeapon)
	//{
	//	EquippedWeapon->StartTrace(); 
	//}

	//// �ִϸ��̼� ���� (���õ� ���� ó������ ���)
	//AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	//AnimInstance->Montage_JumpToSection(SelectedSection, SelectedMontage);
	//
	//UE_LOG(LogTemp, Log, TEXT("Playing Attack Montage Section: %s"), *SelectedSection.ToString());

	//// **���� ������ ���� ��������**
	//float SectionDuration = SelectedMontage->GetSectionLength(SelectedMontage->GetSectionIndex(SelectedSection));

	//// **Ÿ�̸� ����: ��Ȯ�� ���� ���� �� ���� ���� �ʱ�ȭ**
	//GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, SectionDuration, false);

	//// **���� �Է� �� �ٸ� ���� ����ǵ��� ���� (�޺� ����)**
	//AttackComboIndex = (AttackComboIndex == 0) ? 1 : 0;

	if (HasAuthority())
	{
		MulticastPlayAttackMontage();
	}
	else
	{
		ServerRequestPlayAttackMontage();
	}
}

void AMyDCharacter::MulticastPlayAttackMontage_Implementation()
{
	PlayAttackAnimation_Internal();
}


void AMyDCharacter::PlayAttackAnimation_Internal()
{
	if (bIsAttacking || bIsRolling)
	{
		UE_LOG(LogTemp, Log, TEXT("Already attacking or rolling! Ignoring input."));
		return;
	}

	if (Stamina <= 0)
	{
		ReduceStamina(0.0f);
		return;
	}

	bIsAttacking = true;
	ReduceStamina(AttackStaminaCost);

	UAnimMontage* SelectedMontage = nullptr;

	if (EquippedWeapon)
	{
		switch (EquippedWeapon->WeaponType)
		{
		case EWeaponType::GreatWeapon:
			SelectedMontage = GreatWeaponMontage;
			break;
		case EWeaponType::Dagger:
			SelectedMontage = DaggerWeaponMontage;
			break;
		case EWeaponType::Staff:
			//SelectedMontage = StaffMontage;
			break;
		default:
			SelectedMontage = WeaponAttackMontage;
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
	FName SelectedSection = (AttackComboIndex == 0) ? FName("Combo1") : FName("Combo2");

	if (EquippedWeapon)
	{
		EquippedWeapon->StartTrace();
	}

	AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	AnimInstance->Montage_JumpToSection(SelectedSection, SelectedMontage);

	UE_LOG(LogTemp, Log, TEXT("Playing Attack Montage Section: %s"), *SelectedSection.ToString());

	float SectionDuration = SelectedMontage->GetSectionLength(SelectedMontage->GetSectionIndex(SelectedSection));
	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, SectionDuration, false);

	AttackComboIndex = (AttackComboIndex == 0) ? 1 : 0;
}


void AMyDCharacter::ResetAttack()
{
	// ���� ���� �ʱ�ȭ
	bIsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("Attack Ended, Resetting Attack State"));

	ResetHitActors();
}

void AMyDCharacter::ServerPerformTraceAttack_Implementation()
{
	// HitActors�� �������� �ʱ�ȭ
	HitActors.Empty();

	if (EquippedWeapon)
	{
		EquippedWeapon->TraceAttack();
	}
	else
	{
		FVector Start = CharacterMesh->GetSocketLocation(FName("hand_r"));
		FVector End = Start + GetActorForwardVector() * 33.0f;

		TArray<FHitResult> HitResults;
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		UKismetSystemLibrary::SphereTraceMulti(
			GetWorld(),
			Start,
			End,
			20.0f, // Radius
			UEngineTypes::ConvertToTraceType(ECC_Pawn),
			false,
			IgnoredActors,
			EDrawDebugTrace::None,
			HitResults,
			true
		);

		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->Implements<UHitInterface>())
			{
				if (!HitActors.Contains(HitActor))
				{
					HitActors.Add(HitActor);
					IHitInterface::Execute_GetHit(HitActor, Hit, this, 30.0f);
				}
			}
		}
	}
}

void AMyDCharacter::ServerRequestPlayAttackMontage_Implementation()
{
	MulticastPlayAttackMontage();
}



void AMyDCharacter::PlayRollAnimation()
{
	//if (bIsRolling || !RollMontage) return;

	//if (Stamina <= 0)  // ���¹̳��� 0�̸� ���� �Ұ�
	//{
	//	ReduceStamina(0.0f);
	//	return;
	//}

	//bIsRolling = true;

	//// ���� �̵� ���� ���
	//FVector RollDirection;
	//FName SelectedSection = "RollF"; // �⺻�� (�ձ�����)

	//ReduceStamina(RollStaminaCost);

	//if (FMath::Abs(MoveForwardValue) > 0.1f || FMath::Abs(MoveRightValue) > 0.1f)
	//{
	//	// ����Ű �Է��� ���� ���: �ش� �������� ������
	//	FRotator ControlRotation = GetControlRotation();
	//	FVector ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
	//	FVector RightVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);

	//	RollDirection = ForwardVector * MoveForwardValue + RightVector * MoveRightValue;
	//	RollDirection.Normalize();

	//	// **�Է� ���⿡ ���� ������ ���� ����**
	//	if (MoveForwardValue > 0.1f)
	//	{
	//		SelectedSection = "RollF";  // �ձ�����
	//	}
	//	else if (MoveForwardValue < -0.1f)
	//	{
	//		SelectedSection = "RollB";  // �ڱ�����
	//	}
	//	else if (MoveRightValue > 0.1f)
	//	{
	//		SelectedSection = "RollR";  // ������ ������
	//	}
	//	else if (MoveRightValue < -0.1f)
	//	{
	//		SelectedSection = "RollL";  // ���� ������
	//	}
	//}
	//else
	//{
	//	// ����Ű �Է��� ���� ���: �⺻������ �ձ�����
	//	RollDirection = GetActorForwardVector();
	//}

	//// RollDirection�� ��� ������ ���� (�̵� �Լ����� ���)
	//StoredRollDirection = RollDirection;

	//// **������ �������� ĳ���� ȸ�� (ī�޶�� ����)**
	//if (RollDirection.SizeSquared() > 0)
	//{
	//	FRotator NewRotation = RollDirection.Rotation();
	//	SetActorRotation(NewRotation);
	//}

	//// **�ִϸ��̼� ���� (���õ� �������� ����)**
	//UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	//if (AnimInstance)
	//{
	//	AnimInstance->Montage_Play(RollMontage, 1.0f);
	//	AnimInstance->Montage_JumpToSection(SelectedSection, RollMontage);
	//	UE_LOG(LogTemp, Log, TEXT("Playing Roll Montage Section: %s"), *SelectedSection.ToString());
	//}

	//// **������ �̵� ����**
	//ApplyRollMovement(RollDirection);

	//// **������ �� ���� ���� ����**
	//GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetRoll, RollDuration, false);
	if (HasAuthority())
	{
		MulticastPlayRoll(MoveForwardValue, MoveRightValue);
	}
	else
	{
		ServerRequestRoll(MoveForwardValue, MoveRightValue);
	}

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

void AMyDCharacter::ServerRequestRoll_Implementation(float ForwardValue, float RightValue)
{
	MulticastPlayRoll(ForwardValue, RightValue);
}

void AMyDCharacter::MulticastPlayRoll_Implementation(float ForwardValue, float RightValue)
{
	ExecuteRoll(ForwardValue, RightValue);
}

void AMyDCharacter::ExecuteRoll(float ForwardValue, float RightValue)
{
	if (bIsRolling || !RollMontage) return;

	if (Stamina <= 0)
	{
		ReduceStamina(0.0f);
		return;
	}

	bIsRolling = true;

	FVector RollDirection;
	FName SelectedSection = "RollF";

	ReduceStamina(RollStaminaCost);

	if (FMath::Abs(ForwardValue) > 0.1f || FMath::Abs(RightValue) > 0.1f)
	{
		FRotator ControlRotation = GetControlRotation();
		FVector ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
		FVector RightVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);

		RollDirection = ForwardVector * ForwardValue + RightVector * RightValue;
		RollDirection.Normalize();

		if (ForwardValue > 0.1f)
		{
			SelectedSection = "RollF";
		}
		else if (ForwardValue < -0.1f)
		{
			SelectedSection = "RollB";
		}
		else if (RightValue > 0.1f)
		{
			SelectedSection = "RollR";
		}
		else if (RightValue < -0.1f)
		{
			SelectedSection = "RollL";
		}
	}
	else
	{
		RollDirection = GetActorForwardVector();
	}

	StoredRollDirection = RollDirection;

	if (RollDirection.SizeSquared() > 0)
	{
		FRotator NewRotation = RollDirection.Rotation();
		SetActorRotation(NewRotation);
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(RollMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(SelectedSection, RollMontage);
		UE_LOG(LogTemp, Log, TEXT("Playing Roll Montage Section: %s"), *SelectedSection.ToString());
	}

	ApplyRollMovement(RollDirection);

	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetRoll, RollDuration, false);
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

	//UpdateHUD();

	// ���� ���� HUD ����
	if (IsLocallyControlled() && HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);

		float NormalizedHealth = Health / MaxHealth;
		float OverlayAlpha = FMath::Clamp(1.0f - NormalizedHealth, 0.0f, 0.6f);

		HUDWidget->Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, OverlayAlpha));
		HUDWidget->Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HUDWidget->StartHitOverlayFadeOut();
	}

	// Ŭ��鿡�� RepNotify�� ���� �ڵ� ����
	OnRep_Health();

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

		Die();
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

//void AMyDCharacter::TriggerDelayedWFC()
//{
//	//ī�޶� ����ũ ȿ�� �Ǵ� ȭ�� ���̵� �߰� ����
//	UE_LOG(LogTemp, Warning, TEXT("rereereere"));
//
//	if (PendingRegenActor)
//	{
//		AWFCRegenerator* Regen = Cast<AWFCRegenerator>(PendingRegenActor);
//		if (!Regen && PendingRegenActor->GetOwner())
//		{
//			Regen = Cast<AWFCRegenerator>(PendingRegenActor->GetOwner());
//		}
//
//		if (Regen)
//		{
//			PlayWFCRegenCameraShake();
//			Regen->GenerateWFCAtLocation();
//			UE_LOG(LogTemp, Log, TEXT("ggggggggg"));
//		}
//		else
//		{
//			UE_LOG(LogTemp, Error, TEXT("hhhhhhh"));
//		}
//	}
//
//	// ����
//	bIsWFCCountdownActive = false;
//	PendingRegenActor = nullptr;
//
//	// UI ����� �� �߰� ����
//}

void AMyDCharacter::PlayWFCRegenCameraShake()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->PlayerCameraManager)
	{
		TSubclassOf<UCameraShakeBase> ShakeClass = LoadClass<UCameraShakeBase>(nullptr, TEXT("/Game/BP/BP_WFCShake.BP_WFCShake_C")); // �������Ʈ ���

		if (ShakeClass)
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass);
			UE_LOG(LogTemp, Log, TEXT("Camera Shake Played"));
		}
	}
}

void AMyDCharacter::TriggerDelayedWFC()
{
	ShowWFCFadeAndRegenSequence(); // ���� �Լ� ȣ��
}

void AMyDCharacter::ShowWFCFadeAndRegenSequence()
{
	if (WFCWarningWidgetInstance)
	{
		if (!WFCWarningWidgetInstance->IsInViewport())
		{
			WFCWarningWidgetInstance->AddToViewport(20);
		}
		WFCWarningWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WFCWarningWidgetInstance is NULL!"));
	}

	// 5�� �Ŀ� ���� �� ����� ����
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFade, this, &AMyDCharacter::FadeAndRegenWFC, 2.0f, false);
}

void AMyDCharacter::FadeAndRegenWFC()
{
	if (WFCWarningWidgetInstance)
	{
		WFCWarningWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (WFCDoneWidgetInstance)
	{
		if (!WFCDoneWidgetInstance->IsInViewport())
		{
			WFCDoneWidgetInstance->AddToViewport(21);
		}
		WFCDoneWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WFCDoneWidgetInstance is NULL in FadeAndRegenWFC"));
	}

	// ����� ���� �÷��̾ ������ ��Ÿ�� ��ó�� �ִ��� Ȯ��
	bool bPlayerInFixedRoom = IsPlayerInFixedRoomTile();

	if (bPlayerInFixedRoom)
	{
		// �÷��̾� �߷� ��Ȱ��ȭ
		ServerSetPlayerGravity(0.0f);
		TeleportToWFCRegen();
	}


	// 0.5�� �Ŀ� ����� ���� (������ ���� ���̵���)
	GetWorldTimerManager().SetTimer(TimerHandle_StartWFC, [this]() 
		{
			// UI ���� Ȯ�� �� ����
			
				ExecuteWFCNow();
			
		}, 0.5f, false);

	// 5�� �� ���� ����� (���� ���)
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFinal, [this]()
		{
			if (WFCDoneWidgetInstance)
			{
				WFCDoneWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			}
		}, 5.0f, false);

	//ExecuteWFCNow();
	//GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFinal, this, &AMyDCharacter::ExecuteWFCNow, 5.0f, false);
}

void AMyDCharacter::ServerSetPlayerGravity_Implementation(float GravityScale)
{
	if (!HasAuthority()) return;

	// �������� �߷� ����
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->GravityScale = GravityScale;
	}

	// ��� Ŭ���̾�Ʈ�� ����
	MulticastSetPlayerGravity(GravityScale);
}

bool AMyDCharacter::ServerSetPlayerGravity_Validate(float GravityScale)
{
	return true;
}

void AMyDCharacter::MulticastSetPlayerGravity_Implementation(float GravityScale)
{
	// �ڽ��� ĳ���͸� ������Ʈ
	if (!IsLocallyControlled()) return;

	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->GravityScale = GravityScale;
	}
}

void AMyDCharacter::ExecuteWFCNow()
{
	if (PendingRegenActor) {
		if (AWFCRegenerator* Regen = Cast<AWFCRegenerator>(PendingRegenActor))
		{
			// ������� �ٷ� ����
			if (Regen->HasAuthority())
			{
				Regen->GenerateWFCAtLocation();
			}
			// Ŭ���̾�Ʈ��� �������� ��û
			else
			{
				ServerRequestWFCRegen(Regen);
			}
		}
	}

	/*if (WFCDoneWidgetInstance)
	{
		WFCDoneWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}*/

	bIsWFCCountdownActive = false;
	PendingRegenActor = nullptr;

}

bool AMyDCharacter::ServerRequestWFCRegen_Validate(AWFCRegenerator* RegenActor)
{
	// �ʿ��ϸ� ��ȿ�� �˻� �߰�
	return true;
}

void AMyDCharacter::ServerRequestWFCRegen_Implementation(AWFCRegenerator* RegenActor)
{
	// ���������� ����
	if (!HasAuthority() || !IsValid(RegenActor)) return;

	// ���� �����ʷ����� ���� ����
	RegenActor->GenerateWFCAtLocation();
}

// RPC: Ŭ���̾�Ʈ->����
bool AMyDCharacter::ServerPlayWFCRegenEffects_Validate()
{
	return true;
}

void AMyDCharacter::ServerPlayWFCRegenEffects_Implementation()
{
	// ���������� Multicast ȣ��
    if (HasAuthority())
    {
        MulticastPlayWFCRegenEffects();
    }
}

// RPC: �������� Ŭ���̾�Ʈ
void AMyDCharacter::MulticastPlayWFCRegenEffects_Implementation()
{
	// ��� �ν��Ͻ����� ����
	PlayWFCRegenCameraShake();
	TriggerDelayedWFC();
}

bool AMyDCharacter::ServerCastSpell_Validate(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation)
{
	// ��ȿ�� �˻�
	return SpellSet.IsValidIndex(SpellIndex) && SpellSet[SpellIndex] != nullptr;
}

void AMyDCharacter::ServerCastSpell_Implementation(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation)
{
	if (!HasAuthority()) return;

	// �������� �ٽ� �ѹ� ����
	if (!SpellSet.IsValidIndex(SpellIndex) || !SpellSet[SpellIndex]) return;
	if (!SpellSet[SpellIndex]->CanActivate(this)) return;

	// ���ҽ� �Ҹ�
	Knowledge -= SpellSet[SpellIndex]->ManaCost;
	Stamina -= SpellSet[SpellIndex]->StaminaCost;
	UpdateHUD();

	// ���� ���� �ִϸ��̼� ��� (��� Ŭ���̾�Ʈ)
	MulticastPlaySpellCastAnimation();

	// ������ ����
	ExecuteSpellOnServer(SpellIndex, TargetLocation, TargetRotation);
}

void AMyDCharacter::ExecuteSpellOnServer(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation)
{
	switch (SpellIndex)
	{
	case 0: // Fireball
		ExecuteFireballSpell(TargetLocation, TargetRotation);
		break;

	case 1: // Heal
		ExecuteHealSpell();
		//MulticastPlayHealEffect(GetActorLocation());
		break;

	case 2: // Curse
		ExecuteCurseSpell(TargetLocation);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown spell index: %d"), SpellIndex);
		break;
	}
}

void AMyDCharacter::ExecuteFireballSpell(FVector TargetLocation, FRotator TargetRotation)
{
	UFireballSpell* FireballSpell = Cast<UFireballSpell>(SpellSet[0]);
	if (!FireballSpell) return;

	// �������� ����ü ����
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	ASpellProjectile* Fireball = GetWorld()->SpawnActor<ASpellProjectile>(
		FireballSpell->FireballProjectileClass,
		TargetLocation,
		TargetRotation,
		SpawnParams
	);

	if (Fireball)
	{
		FVector LaunchDirection = TargetRotation.Vector();
		Fireball->Init(this, FireballSpell->Damage);
		Fireball->LaunchInDirection(LaunchDirection * FireballSpell->FireballSpeed);

		// ��� Ŭ���̾�Ʈ���� ���̾ ����Ʈ ���
		MulticastSpawnFireball(TargetLocation, TargetRotation, FireballSpell->Damage, FireballSpell->FireballSpeed);
	}
}

void AMyDCharacter::ExecuteHealSpell()
{
	UHealSpell* HealSpell = Cast<UHealSpell>(SpellSet[1]);
	if (!HealSpell) return;

	// �������� ���� ���� ó��
	HealPlayer(HealSpell->HealAmount);

	// ��� Ŭ���̾�Ʈ���� ���� ����Ʈ ���
	MulticastPlayHealEffect(GetActorLocation());

	//UE_LOG(LogTemp, Log, TEXT("Server: Heal spell executed"));
}

void AMyDCharacter::ExecuteCurseSpell(FVector TargetLocation)
{
	UCurseSpell* CurseSpell = Cast<UCurseSpell>(SpellSet[2]);
	if (!CurseSpell) return;

	// �������� ���� Ʈ���̽� ����
	FVector Start = FirstPersonCameraComponent->GetComponentLocation() +
		FirstPersonCameraComponent->GetForwardVector() * 30.0f;
	FVector End = TargetLocation;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);  // �ڽ��� ����

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	UE_LOG(LogTemp, Warning, TEXT("Server: Curse spell line trace from %s to %s"),
		*Start.ToString(), *End.ToString());

	AActor* HitActor = nullptr;
	if (bHit)
	{
		HitActor = Hit.GetActor();
		UE_LOG(LogTemp, Warning, TEXT("Server: Curse hit actor: %s"),
			HitActor ? *HitActor->GetName() : TEXT("None"));

		if (HitActor && HitActor->Implements<UHitInterface>())
		{
			// �������� ����� ����
			IHitInterface::Execute_ApplyDebuff(HitActor, EDebuffType::Slow,
				CurseSpell->SlowAmount, CurseSpell->Duration);

			UE_LOG(LogTemp, Warning, TEXT("Server: Applied curse debuff to %s"), *HitActor->GetName());
		}
	}

	// ��� Ŭ���̾�Ʈ���� ���� ����Ʈ ��� (��Ʈ�� ��ġ��)
	FVector EffectLocation = bHit ? Hit.Location : End;
	MulticastPlayCurseEffect(Start, EffectLocation, HitActor);
}

// =========================
// Multicast RPC ������
// =========================

void AMyDCharacter::MulticastPlaySpellCastAnimation_Implementation()
{
	if (SpellCastMontage && CharacterMesh && CharacterMesh->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
		AnimInstance->Montage_Play(SpellCastMontage, 1.0f);
	}
}

void AMyDCharacter::MulticastSpawnFireball_Implementation(FVector SpawnLocation, FRotator SpawnRotation, float Damage, float Speed)
{
	// ���� �÷��̾ �ƴ� ��쿡�� ����Ʈ ��� (�ߺ� ����)
	if (IsLocallyControlled()) return;

	// ���̾ ����Ʈ�� ��� (���� ����ü�� �������� �̹� ������)
	UE_LOG(LogTemp, Log, TEXT("Fireball effect played on client"));
}

void AMyDCharacter::MulticastPlayHealEffect_Implementation(FVector Location)
{
	UE_LOG(LogTemp, Log, TEXT("MulticastPlayHealEffect: Playing heal effect at %s"), *Location.ToString());

	UHealSpell* HealSpell = Cast<UHealSpell>(SpellSet[1]);
	if (!HealSpell || !HealSpell->HealEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("HealSpell or HealEffect is null!"));
		return;
	}

	// OrbitEffectActor ���� (���� ���� ����)
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AOrbitEffectActor* OrbitActor = GetWorld()->SpawnActor<AOrbitEffectActor>(
		AOrbitEffectActor::StaticClass(),
		Location,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (OrbitActor)
	{
		// ���� HealSpell.cpp�� �������� ���
		OrbitActor->InitOrbit(
			this,                                    // Center Actor
			HealSpell->HealEffect,                  // Effect
			100.f,                                  // Radius
			5.f,                                    // Duration (����: ������ 2.f����)
			1080.f,                                 // Speed
			FLinearColor(0.4f, 1.f, 0.2f),        // Color (���)
			5.f                                     // Sprite Size
		);

		UE_LOG(LogTemp, Log, TEXT("Heal OrbitEffectActor created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Heal OrbitEffectActor"));
	}
}

void AMyDCharacter::MulticastPlayCurseEffect_Implementation(FVector StartLocation, FVector EndLocation, AActor* TargetActor)
{
	UE_LOG(LogTemp, Log, TEXT("MulticastPlayCurseEffect: Playing curse effect from %s to %s"),
		*StartLocation.ToString(), *EndLocation.ToString());

	UCurseSpell* CurseSpell = Cast<UCurseSpell>(SpellSet[2]);
	if (!CurseSpell || !CurseSpell->CurseEffect)
	{
		UE_LOG(LogTemp, Error, TEXT("CurseSpell or CurseEffect is null!"));
		return;
	}

	// ����� ���� �׸���
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Purple, false, 3.0f, 0, 3.0f);

	// Ÿ���� ������ Ÿ�� ��ġ��, ������ EndLocation�� ����Ʈ ����
	FVector EffectLocation = TargetActor ? TargetActor->GetActorLocation() : EndLocation;

	// OrbitEffectActor ���� (���� ���� ����)
	if (TargetActor) // Ÿ���� ���� ���� OrbitEffect ����
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		AOrbitEffectActor* OrbitActor = GetWorld()->SpawnActor<AOrbitEffectActor>(
			AOrbitEffectActor::StaticClass(),
			EffectLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (OrbitActor)
		{
			// ���� CurseSpell.cpp�� �������� ���
			OrbitActor->InitOrbit(
				TargetActor,                        // Center Actor
				CurseSpell->CurseEffect,           // Effect
				100.f,                             // Radius
				10.f,                               // Duration (���� ������)
				1080.f,                            // Speed
				FLinearColor(0.8f, 0.5f, 1.0f),  // Color (�����)
				3.f                                // Sprite Size
			);

			UE_LOG(LogTemp, Log, TEXT("Curse OrbitEffectActor created successfully on target: %s"),
				*TargetActor->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Curse OrbitEffectActor"));
		}
	}
	else
	{
		// Ÿ���� ������ �ܼ��� ��ġ�� ����Ʈ�� ����
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			CurseSpell->CurseEffect,
			EffectLocation,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::None
		);

		UE_LOG(LogTemp, Log, TEXT("Curse Niagara effect spawned at location (no target)"));
	}
}

bool AMyDCharacter ::IsPlayerInFixedRoomTile()
{
	// �˻� �ݰ� ����
	float SearchRadius = 690.0f;

	// ����� �ð�ȭ: �÷��̾� �ֺ��� ��ü �׸���
	DrawDebugSphere(
		GetWorld(),
		GetActorLocation(),
		SearchRadius,
		32,                    // ���׸�Ʈ (�ε巯�� ����)
		FColor::Green,         // ����
		true,                 // ���� ���� (true�� ����)
		5.0f,                  // ���� �ð� (��)
		0,                     // ���� �켱 ����
		1.0f                   // �β�
	);

	// �÷��̾� �ֺ����� WFCRegen �±װ� �ִ� ����� ������Ʈ �˻�
	TArray<AActor*> AllRegenObjects;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("WFCRegen"), AllRegenObjects);

	FVector PlayerLocation = GetActorLocation();

	for (AActor* RegenActor : AllRegenObjects)
	{
		if (IsValid(RegenActor))
		{
			float DistanceSquared = FVector::DistSquared(PlayerLocation, RegenActor->GetActorLocation());
			if (DistanceSquared <= FMath::Square(SearchRadius))
			{
				float Distance = FMath::Sqrt(DistanceSquared);
				UE_LOG(LogTemp, Warning, TEXT("Player is near WFCRegen object at distance: %.2f"), Distance);
				return true;
			}
		}
	}

	return false;
}




void AMyDCharacter::TriggerEscapeSequence()
{
	if (EscapeWarningWidgetInstance)
	{
		if (!EscapeWarningWidgetInstance->IsInViewport())
		{
			EscapeWarningWidgetInstance->AddToViewport(20);
		}
		EscapeWarningWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EscapeWarningWidgetInstance is NULL!"));
	}

	CurrentEscapeTime = 0.0f;

	GetWorldTimerManager().SetTimer(TimerHandle_DelayedEscapeFade, this, &AMyDCharacter::FadeAndEscape, 5.0f, false);
	GetWorldTimerManager().SetTimer(TimerHandle_EscapeProgressUpdate, this, &AMyDCharacter::UpdateEscapeProgressBar, 0.05f, true);
}

void AMyDCharacter::FadeAndEscape()
{
	if (EscapeWarningWidgetInstance)
	{
		EscapeWarningWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (EscapeDoneWidgetInstance)
	{
		if (!EscapeDoneWidgetInstance->IsInViewport())
		{
			EscapeDoneWidgetInstance->AddToViewport(21);
		}
		EscapeDoneWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("EscapeDoneWidgetInstance is NULL in FadeAndEscape"));
	}

	GetWorldTimerManager().SetTimer(TimerHandle_DelayedEscapeFinal, this, &AMyDCharacter::ExecuteEscape, 0.5f, false);
}

void AMyDCharacter::ExecuteEscape()
{

	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (GameInstance && InventoryComponent)
	{
		GameInstance->SavedInventoryItems = InventoryComponent->InventoryItemsStruct;
		GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
	}

	UGameplayStatics::OpenLevel(this, FName("LobbyMap")); // �κ�� �̵�

	bIsEscapeCountdownActive = false;
	PendingEscapeActor = nullptr;
}

void AMyDCharacter::UpdateEscapeProgressBar()
{
	CurrentEscapeTime += 0.05f; // Ÿ�̸� �ֱ⸸ŭ �߰�

	float ProgressPercent = FMath::Clamp(CurrentEscapeTime / MaxEscapeTime, 0.0f, 1.0f);

	if (EscapeWarningWidgetInstance)
	{
		UProgressBar* EscapeProgressBar = Cast<UProgressBar>(EscapeWarningWidgetInstance->GetWidgetFromName(TEXT("EscapeProgressBar")));
		if (EscapeProgressBar)
		{
			EscapeProgressBar->SetPercent(ProgressPercent);
		}
	}

	if (ProgressPercent >= 1.0f)
	{
		// 100% ä������ Progress ������Ʈ Ÿ�̸Ӵ� ���߱�
		GetWorldTimerManager().ClearTimer(TimerHandle_EscapeProgressUpdate);
	}
}


void AMyDCharacter::ApplyCharacterData(const FPlayerCharacterData& Data)
{
	PlayerClass = Data.PlayerClass;
	MaxHealth = Data.MaxHealth;
	MaxStamina = Data.MaxStamina;
	MaxKnowledge = Data.MaxKnowledge;

	Health = MaxHealth;
	Stamina = MaxStamina;
	Knowledge = MaxKnowledge;

	// ��� ����
	for (const FItemData& EquipItem : Data.EquippedItems)
	{
		if (EquipItem.ItemClass && EquipItem.ItemType == EItemType::Weapon)
		{
			EquipWeaponFromClass(EquipItem.ItemClass, static_cast<EWeaponGrade>(EquipItem.Grade));

			break;
		}
	}

	// �κ��丮 ����
	if (InventoryComponent)
	{
		InventoryComponent->InventoryItemsStruct.SetNum(InventoryComponent->Capacity);
		for (int32 i = 0; i < Data.InventoryItems.Num(); ++i)
		{
			if (i < InventoryComponent->InventoryItemsStruct.Num())
			{
				InventoryComponent->InventoryItemsStruct[i] = Data.InventoryItems[i];
			}
		}
	}

	// ���â UI���� �ݿ�
	if (EquipmentWidgetInstance)
	{
		EquipmentWidgetInstance->RestoreEquipmentFromData(Data.EquippedItems);
	}
}

void AMyDCharacter::TryCastSpell(int32 Index)
{
	if (PlayerClass != EPlayerClass::Mage) return;

	if (SpellSet.IsValidIndex(Index) && SpellSet[Index]->CanActivate(this))
	{

		// ���� ���� ��Ÿ�� ���
		if (SpellCastMontage)
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to play montage: %s"), *SpellCastMontage->GetName()); 
			PlayMagicMontage();
		}

		SpellSet[Index]->ActivateSpell(this);
	}
}

void AMyDCharacter::CastSpell1()
{
	TryCastSpellMultiplayer(0);      // 0 - ���̾, 1 - ��, 2 - ����.
}

void AMyDCharacter::CastSpell2()
{
	TryCastSpellMultiplayer(2);
}

void AMyDCharacter::TryCastSpellMultiplayer(int32 SpellIndex)
{
	if (PlayerClass != EPlayerClass::Mage) return;

	// ��ȿ�� �˻�
	if (!SpellSet.IsValidIndex(SpellIndex) || !SpellSet[SpellIndex]) return;

	// ������ ���� ���� (0.1��)
	

	// ���ҽ� Ȯ��
	if (!SpellSet[SpellIndex]->CanActivate(this)) return;

	
	

	
	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = GetControlRotation();

	if (SpellIndex == 0) // Fireball
	{
		TargetLocation = GetActorLocation() + GetActorForwardVector() * 200.0f + FVector(0, 0, 50.0f);
	}
	else if (SpellIndex == 1) // Heal
	{
		TargetLocation = GetActorLocation(); // Heal�� �ڱ� ��ġ
		UE_LOG(LogTemp, Warning, TEXT("Client: Requesting Heal spell at %s"), *TargetLocation.ToString());
	}
	else if (SpellIndex == 2) // Curse
	{
		FVector Start = FirstPersonCameraComponent->GetComponentLocation() +
			FirstPersonCameraComponent->GetForwardVector() * 30.0f;
		FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 1000.0f;
		TargetLocation = End;
	}

	// ������ ���� ���� ��û
	ServerCastSpell(SpellIndex, TargetLocation, TargetRotation);
}


void AMyDCharacter::ToggleMapView()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (!bIsInOverheadView)
	{
		if (OverheadCameraActor)
		{
			PC->SetViewTargetWithBlend(OverheadCameraActor, 0.5f);
			bIsInOverheadView = true;
		}
	}
	else
	{
		PC->SetViewTargetWithBlend(this, 0.5f);
		bIsInOverheadView = false;
	}
}


//void AMyDCharacter::ToggleMapView()
//{
//
//
//
//
//
//	if (!FirstPersonCameraComponent) return;
//
//	APlayerController* PC = Cast<APlayerController>(GetController());
//
//	if (!bIsInOverheadView)
//	{
//		// ���� ī�޶� ��ġ ���� (ȸ�� ����)
//		DefaultCameraLocation = FirstPersonCameraComponent->GetComponentLocation();
//		DefaultCameraRotation = FirstPersonCameraComponent->GetComponentRotation();
//
//		// �� ��ü ���� ���� ����
//		FVector OverheadLocation = FVector(15000.0f, 16000.0f, 50500.0f);
//		FRotator OverheadRotation = FRotator(-90.0f, 0.0f, 0.0f); // ��¥�� �������� �����ٺ�
//
//		FirstPersonCameraComponent->SetWorldLocation(OverheadLocation);
//		FirstPersonCameraComponent->SetWorldRotation(OverheadRotation);
//
//		if (PC)
//		{
//			PC->SetControlRotation(OverheadRotation); // �� �߿�: ��Ʈ�ѷ� ȸ���� ����
//			PC->SetIgnoreLookInput(true);              // ���콺 ȸ�� ����
//		}
//
//		
//
//		if (CachedDirectionalLight)
//		{
//			CachedDirectionalLight->SetActorHiddenInGame(false);
//			CachedDirectionalLight->SetEnabled(true);
//		}
//
//		bUseControllerRotationYaw = false;
//		bIsInOverheadView = true;
//	}
//	else
//	{
//		// ���� �������� ����
//		FirstPersonCameraComponent->SetWorldLocation(DefaultCameraLocation);
//		FirstPersonCameraComponent->SetWorldRotation(DefaultCameraRotation);
//
//		if (PC)
//		{
//			PC->SetControlRotation(DefaultCameraRotation); // �� �߿�: ������ ���� ���� ����
//			PC->SetIgnoreLookInput(false);
//		}
//
//		if (CachedDirectionalLight)
//		{
//			CachedDirectionalLight->SetActorHiddenInGame(true);
//			CachedDirectionalLight->SetEnabled(false);
//		}
//
//		bUseControllerRotationYaw = true;
//		bIsInOverheadView = false;
//	}
//}





void AMyDCharacter::ToggleTorch()
{
	if (AttachedTorch)
	{
		if (!AttachedTorch) return;

		bTorchVisible = !bTorchVisible;
		AttachedTorch->SetActorHiddenInGame(!bTorchVisible);

		//UE_LOG(LogTemp, Log, TEXT("Torch is now %s"), bTorchVisible ? TEXT("visible") : TEXT("hidden"));
	}
}

void AMyDCharacter::Die()
{
	//// �κ��丮 ����
	//if (InventoryComponent)
	//{
	//	InventoryComponent->ClearInventory(); // �ʰ� ������ �ʱ�ȭ �Լ��� ��ü
	//}

	// ���� �̵� (��: LobbyMap�̶�� �̸��� ������ ��ȯ)
	UGameplayStatics::OpenLevel(this, FName("LobbyMap"));

	// �ʿ�� �α�
	UE_LOG(LogTemp, Warning, TEXT("Player died. Returning to lobby..."));
}

void AMyDCharacter::ServerTeleportToWFCRegen_Implementation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> FoundRegens;
	UGameplayStatics::GetAllActorsOfClass(World, AWFCRegenerator::StaticClass(), FoundRegens);

	if (FoundRegens.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No AWFCRegenerator found in level"));
		return;
	}

	// ���� ����� WFCRegen ã��
	AActor* Closest = nullptr;
	float MinDistSqr = TNumericLimits<float>::Max();
	FVector MyLocation = GetActorLocation();

	for (AActor* Regen : FoundRegens)
	{
		float DistSqr = FVector::DistSquared(MyLocation, Regen->GetActorLocation());
		if (DistSqr < MinDistSqr)
		{
			MinDistSqr = DistSqr;
			Closest = Regen;
		}
	}

	if (Closest)
	{
		FVector TargetLocation = Closest->GetActorLocation() + FVector(0, 0, 100); // �ణ ���� ����ֱ�
		SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("Teleported to WFCRegenerator at %s"), *TargetLocation.ToString());
	}
}

void AMyDCharacter::TeleportToWFCRegen()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	// Ŭ���̾�Ʈ���� ���� RPC ȣ��
	ServerTeleportToWFCRegen();
}

void AMyDCharacter::TeleportToEscapeObject()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> EscapeActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Escape"), EscapeActors);

	if (EscapeActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No escape objects found!"));
		return;
	}

	// ���� ����� Escape ������Ʈ ã��
	AActor* ClosestEscape = nullptr;
	float MinDistanceSq = FLT_MAX;
	FVector MyLocation = GetActorLocation();

	for (AActor* EscapeActor : EscapeActors)
	{
		float DistSq = FVector::DistSquared(MyLocation, EscapeActor->GetActorLocation());
		if (DistSq < MinDistanceSq)
		{
			MinDistanceSq = DistSq;
			ClosestEscape = EscapeActor;
		}
	}

	if (ClosestEscape)
	{
		SetActorLocation(ClosestEscape->GetActorLocation() + FVector(0, 0, 100)); // ��¦ ���� ����� �̵�
		UE_LOG(LogTemp, Log, TEXT("Teleported to escape object: %s"), *ClosestEscape->GetName());
	}
}

void AMyDCharacter::HealPlayer(int32 Amount)
{
	Health = FMath::Clamp(Health + Amount, 0, MaxHealth);
	UpdateHUD();
}

void AMyDCharacter::UseHotkey(int32 Index)
{

	const int32 EquipSlotIndex = 4 + Index;
	FItemData& Item = EquipmentWidgetInstance->EquipmentSlots[EquipSlotIndex]; 


	UE_LOG(LogTemp, Warning, TEXT("gpt bbbbbyu"));

	if (!Item.ItemClass) return;

	AItem* DefaultItem = Item.ItemClass->GetDefaultObject<AItem>();
	if (!DefaultItem) return;

	if (Item.ItemType == EItemType::Consumable)
	{
		DefaultItem->Use(this);  // AScrollItem�̸� ��ų �ߵ���

		// ���� ó��
		Item.Count--;
		if (Item.Count <= 0)
		{
			EquipmentWidgetInstance->ClearSlot(EquipSlotIndex);
		}
		EquipmentWidgetInstance->RefreshEquipmentSlots();

		if (HUDWidget)
		{
			HUDWidget->UpdateHotkeySlot(Index, EquipmentWidgetInstance->EquipmentSlots[EquipSlotIndex]);
		}
		return;
	}


	if (Item.ItemClass && Item.ItemType != EItemType::Potion)
	{
		Item.ItemType = EItemType::Potion;
		UE_LOG(LogTemp, Warning, TEXT("ItemType hhhhh: Potion"));
	}

	if (Item.ItemType == EItemType::Potion && Item.ItemClass)
	{
		//AItem* DefaultItem = Item.ItemClass->GetDefaultObject<AItem>();
		const EPotionEffectType EffectType = Item.PotionEffect;

		switch (EffectType)
		{
		case EPotionEffectType::Health:
		{
			APotion* Potion = Cast<APotion>(DefaultItem);
			if (Potion) HealPlayer(Potion->GetHealAmount());
			break;
		}
		case EPotionEffectType::Mana:
		{
			AManaPotion* ManaPotion = Cast<AManaPotion>(DefaultItem);
			if (ManaPotion)
			{
				Knowledge += ManaPotion->ManaAmount;
				Knowledge = FMath::Clamp(Knowledge, 0.0f, MaxKnowledge);
				UpdateHUD();
			}
			break;
		}
		case EPotionEffectType::Stamina:
		{
			AStaminaPotion* StaminaPotion = Cast<AStaminaPotion>(DefaultItem);
			if (StaminaPotion)
			{
				Stamina += StaminaPotion->StaminaAmount;
				Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);
				UpdateHUD(); 
			}
			break;
		}
		default:
			UE_LOG(LogTemp, Warning, TEXT("Unknown potion effect type."));
			break;
		}

		// ���� ó��
		Item.Count--;
		if (Item.Count <= 0)
		{
			EquipmentWidgetInstance->ClearSlot(EquipSlotIndex);
		}
		EquipmentWidgetInstance->RefreshEquipmentSlots();
		if (HUDWidget)
		{
			HUDWidget->UpdateHotkeySlot(Index, EquipmentWidgetInstance->EquipmentSlots[EquipSlotIndex]);
		}
	}
}

void AMyDCharacter::UseHotkey1() { UseHotkey(0); }
void AMyDCharacter::UseHotkey2() { UseHotkey(1); }
void AMyDCharacter::UseHotkey3() { UseHotkey(2); }
void AMyDCharacter::UseHotkey4() { UseHotkey(3); }
void AMyDCharacter::UseHotkey5() { UseHotkey(4); }



void AMyDCharacter::TeleportToNearestPlayer()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	ServerTeleportToNearestPlayer();
}

void AMyDCharacter::ServerTeleportToNearestPlayer_Implementation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsOfClass(World, AMyDCharacter::StaticClass(), FoundPlayers);

	if (FoundPlayers.Num() <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("No other players found!"));
		return;
	}

	AMyDCharacter* ClosestPlayer = nullptr;
	float MinDistSqr = TNumericLimits<float>::Max();
	FVector MyLocation = GetActorLocation();

	for (AActor* Actor : FoundPlayers)
	{
		AMyDCharacter* OtherPlayer = Cast<AMyDCharacter>(Actor);
		if (!OtherPlayer || OtherPlayer == this) continue;

		float DistSqr = FVector::DistSquared(MyLocation, OtherPlayer->GetActorLocation());
		if (DistSqr < MinDistSqr)
		{
			MinDistSqr = DistSqr;
			ClosestPlayer = OtherPlayer;
		}
	}

	if (ClosestPlayer)
	{
		FVector TargetLocation = ClosestPlayer->GetActorLocation() + FVector(0, 0, 100);
		SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("Teleported to nearest player at %s"), *TargetLocation.ToString());
	}
}



void AMyDCharacter::TeleportToNearestChest()
{
	if (!IsLocallyControlled())
	{
		return;
	}
	ServerTeleportToNearestChest();
}

void AMyDCharacter::ServerTeleportToNearestChest_Implementation()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> FoundChests;
	UGameplayStatics::GetAllActorsOfClass(World, ATreasureChest::StaticClass(), FoundChests);

	if (FoundChests.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No treasure chests found!"));
		return;
	}

	AActor* ClosestChest = nullptr;
	float MinDistSqr = TNumericLimits<float>::Max();
	FVector MyLocation = GetActorLocation();

	for (AActor* Chest : FoundChests)
	{
		float DistSqr = FVector::DistSquared(MyLocation, Chest->GetActorLocation());
		if (DistSqr < MinDistSqr)
		{
			MinDistSqr = DistSqr;
			ClosestChest = Chest;
		}
	}

	if (ClosestChest)
	{
		FVector TargetLocation = ClosestChest->GetActorLocation() + FVector(0, 0, 100);
		SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
		UE_LOG(LogTemp, Log, TEXT("Teleported to chest at %s"), *TargetLocation.ToString());
	}
}

void AMyDCharacter::ApplyDebuff_Implementation(EDebuffType DebuffType, float Magnitude, float Duration)
{
	if (DebuffType == EDebuffType::Slow)
	{
		UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (!Movement) return;

		// �̹� ���ο� ���¶�� ���� Ÿ�̸� Ŭ����
		if (bIsSlowed)
		{
			GetWorldTimerManager().ClearTimer(DebuffRecoveryTimerHandle);
		}

		// ���� �ӵ� ���� (ó�� ����� ����)
		if (!bIsSlowed)
		{
			OriginalWalkSpeed = Movement->MaxWalkSpeed;
		}

		bIsSlowed = true;

		// �ӵ� ���� ���� (���� �ӵ� �������� �ʰ� ������ ���)
		SpeedMultiplier = FMath::Clamp(1.0f - Magnitude, 0.1f, 1.0f);

		// ���� �ӵ��� ���� ����
		ApplySpeedMultiplier();

		//// �ӵ� ���� ����
		//const float SlowFactor = FMath::Clamp(1.0f - Magnitude, 0.1f, 1.0f);
		//Movement->MaxWalkSpeed = OriginalWalkSpeed * SlowFactor;

		UE_LOG(LogTemp, Warning, TEXT("Applied slow debuff: %f -> %f for %f seconds"),
			OriginalWalkSpeed, Movement->MaxWalkSpeed, Duration);

		// ����� ������ Ÿ�̸� ����
		GetWorldTimerManager().SetTimer(
			DebuffRecoveryTimerHandle,
			this,
			&AMyDCharacter::RemoveSlowDebuff,
			Duration,
			false
		);
	}
}



void AMyDCharacter::RemoveSlowDebuff()
{
	if (!bIsSlowed) return;

	bIsSlowed = false;
	SpeedMultiplier = 1.0f;  // ���� ������ ����

	// ���� �ӵ��� ���� ���� ����
	ApplySpeedMultiplier();

	UE_LOG(LogTemp, Warning, TEXT("Removed slow debuff: SpeedMultiplier restored to 1.0"));
	GetWorldTimerManager().ClearTimer(DebuffRecoveryTimerHandle);
}

void AMyDCharacter::ApplySpeedMultiplier()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;

	float CurrentBaseSpeed = Movement->MaxWalkSpeed;

	// ���� �ӵ��� SprintSpeed �������� WalkSpeed �������� �Ǵ�
	float NormalizedSpeed = CurrentBaseSpeed / (WalkSpeed * Agility);

	if (NormalizedSpeed > 1.5f)  // �޸��� ���·� ����
	{
		// �޸��� �ӵ��� ���� ����
		Movement->MaxWalkSpeed = (SprintSpeed * Agility) * SpeedMultiplier;
	}
	else  // �ȱ� ����
	{
		// �ȱ� �ӵ��� ���� ����
		Movement->MaxWalkSpeed = (WalkSpeed * Agility) * SpeedMultiplier;
	}

	UE_LOG(LogTemp, Log, TEXT("Applied SpeedMultiplier %f: %f -> %f"),
		SpeedMultiplier, CurrentBaseSpeed, Movement->MaxWalkSpeed);
}

void AMyDCharacter::TeleportToNearestEnemy()
{
	FVector MyLocation = GetActorLocation();
	float NearestDistance = TNumericLimits<float>::Max();
	ARageEnemyCharacter* NearestEnemy = nullptr;
	//AEnemyCharacter* NearestEnemy = nullptr;

	for (TActorIterator<ARageEnemyCharacter> It(GetWorld()); It; ++It)
	{
		ARageEnemyCharacter* Enemy = *It;
		if (!Enemy) continue;

		float Dist = FVector::Dist(MyLocation, Enemy->GetActorLocation());
		if (Dist < NearestDistance)
		{
			NearestDistance = Dist;
			NearestEnemy = Enemy;
		}
	}

	/*for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyCharacter* Enemy = *It;
		if (!Enemy) continue;

		float Dist = FVector::Dist(MyLocation, Enemy->GetActorLocation());
		if (Dist < NearestDistance)
		{
			NearestDistance = Dist;
			NearestEnemy = Enemy;
		}
	}*/

	if (NearestEnemy)
	{
		FVector TargetLocation = NearestEnemy->GetActorLocation() + FVector(0, 0, 200); // ��¦ ��
		SetActorLocation(TargetLocation);
		UE_LOG(LogTemp, Warning, TEXT("Teleported to enemy: %s"), *NearestEnemy->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No enemies found to teleport to."));
	}
}

void AMyDCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsLocallyControlled())
	{
		EnableInput(Cast<APlayerController>(NewController));

	}
}
