// Fill out your copyright notice in the Description page of Project Settings.


#include "MyDCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DynamicDungeonInstance.h"
#include "DynamicDungeonModeBase.h"
#include "Animation/AnimInstance.h"  // 애니메이션 관련 클래스 추가
#include "Components/BoxComponent.h"  // 콜리전 박스 추가
#include "Weapon.h"
#include "GreatWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "UCharacterHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Item.h"
#include "Armor.h"
#include "Hat.h"
#include "Mask.h"
#include "GameFramework/PlayerState.h"
#include "RobeTop.h"
#include "RobeBottom.h"
#include "StaminaPotion.h"
#include "ManaPotion.h"
#include "HealSpell.h"
#include "CurseSpell.h"
#include "GoldWidget.h"
#include "InventoryWidget.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
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

// 기본 생성자
AMyDCharacter::AMyDCharacter()
{
	// 매 프레임 Tick을 활성화
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement =true;


	//기본 속성 설정
	MaxHealth = 100.0f;
	Health = MaxHealth;
	Agility = 1.0f; // 기본 민첩성 (이동 속도 배율)
	MaxKnowledge = 100.0f;
	Knowledge = MaxKnowledge;
	// 생성자에서 스테미나 값 초기화
	MaxStamina = 100.0f;
	Stamina = MaxStamina;

	////스프링암(SprintArm) 생성 (메쉬와 카메라를 독립적으로 배치하기 위해)
	//SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	//SpringArm->SetupAttachment(RootComponent);
	//SpringArm->TargetArmLength = 0.0f; // 1인칭이므로 거리 없음
	//SpringArm->bUsePawnControlRotation = false; // 카메라 회전에 영향을 주지 않음

	
// 캐릭터 메쉬 생성
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(RootComponent);

	// 충돌 설정: 래그돌 전환을 위한 기본 설정
	CharacterMesh->SetCollisionProfileName(TEXT("CharacterMesh")); // 나중에 Ragdoll로 바꿀 수 있도록
	
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	

	// 메쉬 로드 (ConstructorHelpers 사용)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'"));  //  ,,,,/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'
	if (MeshAsset.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MeshAsset.Object);
	}

	/*static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/MetaHumans/Trey/Male/Tall/NormalWeight/Body/m_tal_nrw_body.m_tal_nrw_body"));
	if (MeshAsset.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MeshAsset.Object);
	}*/

	//// --- 1) BodyMesh 생성 ---
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(CharacterMesh);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyAsset(
		TEXT("/Game/MetaHumans/Trey/Male/Tall/NormalWeight/Body/m_tal_nrw_body.m_tal_nrw_body")
	);
	if (BodyAsset.Succeeded())
	{
		BodyMesh->SetSkeletalMesh(BodyAsset.Object);
	}
	BodyMesh->SetLeaderPoseComponent(CharacterMesh, /*bForceUpdate=*/true, /*bInFollowerShouldTickPose=*/true);

	CharacterMesh->SetRelativeLocation(FVector(0.f, 0.f, -98.35042f));
	CharacterMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	CharacterMesh->SetRelativeScale3D(FVector(1.f));
	CharacterMesh->SetVisibility(false);

	// --- 2) FaceMesh 생성 & 동기화 ---
	FaceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMesh"));
	FaceMesh->SetupAttachment(CharacterMesh);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FaceAsset(
		TEXT("/Game/MetaHumans/Trey/Face/Trey_FaceMesh.Trey_FaceMesh")
	);
	if (FaceAsset.Succeeded())
	{
		FaceMesh->SetSkeletalMesh(FaceAsset.Object);
	}
	// CharacterMesh의 애니메이션을 FaceMesh가 따라가도록 설정
	FaceMesh->SetLeaderPoseComponent(CharacterMesh, /*bForceUpdate=*/true, /*bInFollowerShouldTickPose=*/true);

	// --- 3) LegsMeshmetha 생성 & 동기화 ---
	LegsMeshmetha = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMeshmetha"));
	LegsMeshmetha->SetupAttachment(CharacterMesh);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> LegsAsset(
		TEXT("/Game/MetaHumans/Common/Male/Tall/NormalWeight/Bottoms/Cargopants/m_tal_nrw_btm_cargopants.m_tal_nrw_btm_cargopants")
	);
	if (LegsAsset.Succeeded())
	{
		LegsMeshmetha->SetSkeletalMesh(LegsAsset.Object);
	}
	LegsMeshmetha->SetLeaderPoseComponent(CharacterMesh, true, true);



	// --- 4) TorsoMesh 생성 & 동기화 ---
	TorsoMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TorsoMesh"));
	TorsoMesh->SetupAttachment(CharacterMesh);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> TorsoAsset(
		TEXT("/Game/MetaHumans/Common/Male/Tall/NormalWeight/Tops/Crewneckt/m_tal_nrw_top_crewneckt_nrm.m_tal_nrw_top_crewneckt_nrm")
	);
	if (TorsoAsset.Succeeded())
	{
		TorsoMesh->SetSkeletalMesh(TorsoAsset.Object);
	}
	TorsoMesh->SetLeaderPoseComponent(CharacterMesh, true, true);

	// --- 5) FeetMesh 생성 & 동기화 ---
	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh"));
	FeetMesh->SetupAttachment(CharacterMesh);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FeetAsset(
		TEXT("/Game/MetaHumans/Common/Male/Tall/NormalWeight/Shoes/CasualSneakers/m_tal_nrw_shs_casualsneakers.m_tal_nrw_shs_casualsneakers")
	);
	if (FeetAsset.Succeeded())
	{
		FeetMesh->SetSkeletalMesh(FeetAsset.Object);
	}
	FeetMesh->SetLeaderPoseComponent(CharacterMesh, true, true);


	// 애니메이션 블루프린트 로드
	/*static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	if (AnimBP.Succeeded())
	{
		UE_LOG(LogTemp, Log, TEXT("Animation Blueprint Loaded Successfully: %s"), *AnimBP.Class->GetName());
		CharacterMesh->SetAnimInstanceClass(AnimBP.Class);
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load Animation Blueprint!"));
	}*/

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/BP/character/Retarget/RTA_ABP_Manny.RTA_ABP_Manny_C"));
	if (AnimBP.Succeeded())
	{
		UE_LOG(LogTemp, Log, TEXT("Animation Blueprint Loaded Successfully: %s"), *AnimBP.Class->GetName());
		CharacterMesh->SetAnimInstanceClass(AnimBP.Class);
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("Failed to load Animation Blueprint!"));
	}


	// 맨주먹 콤보 공격 몽타주 로드
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

	// 무기 공격 콤보 몽타주 로드
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


	//마법 시전 몽타주 로드
	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontageObj(TEXT("/Game/BP/character/Retarget/RTA_Magic_Heal_Anim_mixamo_com_Montage.RTA_Magic_Heal_Anim_mixamo_com_Montage"));
	if (MontageObj.Succeeded())
	{
		SpellCastMontage = MontageObj.Object;
	}

	//  대형 무기 공격 콤보 몽타주 로드
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

	//  단검 무기 공격 콤보 몽타주 로드
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


	// 구르기 몽타주 로드
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


	// 상의
	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh"));
	ChestMesh->SetupAttachment(CharacterMesh);
	ChestMesh->SetMasterPoseComponent(CharacterMesh);
	//ChestMesh->SetLeaderPoseComponent(CharacterMesh, /*bForceTickThisFrame=*/true, /*bFollowerShouldTickPose=*/true);
	// --- 3) LeaderPoseComponent 설정 (TorsoMesh와 동일) ---
	//ChestMesh->SetLeaderPoseComponent(CharacterMesh,
	//	/*bForceTickThisFrame=*/ true,
	//	/*bFollowerShouldTickPose=*/ true);

	// --- 4) Transform, Visibility 설정 ---
	ChestMesh->SetVisibility(true);
	//ChestMesh->bUseBoundsFromMasterPoseComponent = true;

	// 하의
	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh"));
	LegsMesh->SetupAttachment(CharacterMesh);

	LegsMesh->SetMasterPoseComponent(nullptr); // 먼저 해제
	LegsMesh->SetMasterPoseComponent(CharacterMesh); // 다시 설정
		

	//LegsMesh->SetLeaderPoseComponent(CharacterMesh, /*bForceTickThisFrame=*/true, /*bFollowerShouldTickPose=*/true);
	//LegsMesh->SetMasterPoseComponent(CharacterMesh);
	/*LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
	LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));*/
	//LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	LegsMesh->SetVisibility(true);
	LegsMesh->bNoSkeletonUpdate = false;
	LegsMesh->bUpdateJointsFromAnimation = true;

	// 투구
	HelmetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelmetMesh"));
	HelmetMesh->SetupAttachment(CharacterMesh, TEXT("hellmet")); 
	
	HelmetMesh->SetVisibility(false);

	//idle 포즈 추가 (상체)
	/*static ConstructorHelpers::FObjectFinder<UAnimMontage> PoseMontageAsset(TEXT("/Game/BP/character/Retarget/RTA_Male_Sitting_Pose_Anim_mixamo_com_Montage.RTA_Male_Sitting_Pose_Anim_mixamo_com_Montage"));
	if (PoseMontageAsset.Succeeded())
	{
		PoseMontage = PoseMontageAsset.Object;
	}*/

	// 초기 구르기 상태 설정
	bIsRolling = false;

	//**메쉬 방향 조정 (Z축 90도 회전)**
	CharacterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));  // 메쉬 방향만 조정
	//**메쉬 위치 조정 (몸이 바닥에 파묻히지 않도록)**
	CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));  // 루트보다 아래로 배치

	//1인칭 카메라 설정 (스프링암이 아닌 루트에 직접 부착)
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);  // 루트에 직접 부착 (메쉬 영향 X)
	FirstPersonCameraComponent->bUsePawnControlRotation = true;  // 컨트롤러 회전 적용

	//**카메라 위치 및 방향 유지**
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-100.0f, 0.0f, 80.0f));  // 머리 위치 , 원래값 20.0f, 0.0f, 50.0f, 테스트값 -100.0f, 0.0f, 80.0f
	FirstPersonCameraComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f)); // 정면 유지

	//몸체 숨기기 (1인칭에서 보이지 않게)
	//CharacterMesh->SetOwnerNoSee(true);

	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향을 바라보게 함
	GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 걷기 속도
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f; // 감속 설정


	// 상호작용 감지를 위한 박스 콜리전 추가
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(RootComponent);
	InteractionBox->SetBoxExtent(FVector(50.0f, 50.0f, 100.0f)); // 콜리전 크기 설정
	//if (!HasAnyFlags(RF_ClassDefaultObject))
	//{
		InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);  // or ECC_GameTraceChannel1 등 사용자 채널 설정 가능

		// 이렇게 모든 채널에 대해 오버랩 하게
		InteractionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
		InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapBegin);
		InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapEnd);
	//}

	// HUDWidgetClass를 블루프린트 경로에서 로드
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

// 게임 시작 시 호출
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
	
	//if (ChestMesh && CharacterMesh)
	//{
	//	// ChestMesh가 매 Tick마다 CharacterMesh의 Pose를 복사하게 만듭니다.
	//	ChestMesh->SetLeaderPoseComponent(CharacterMesh, /*bForceTickThisFrame=*/true, /*bFollowerShouldTickPose=*/true);
	//}

	//if (LegsMesh && CharacterMesh)
	//{
	//	LegsMesh->SetLeaderPoseComponent(CharacterMesh, true, true);
	//}

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
	//	// 위젯 생성은 항상!
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

	//			//	InventoryWidgetInstance->RefreshInventoryStruct(); // 다시 갱신
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

	//		// 회전 적용 (필요하다면 수정 가능)
	//		FRotator DesiredRotation = FRotator(0.0f, 0.0f, 180.0f);
	//		AttachedTorch->SetActorRelativeRotation(DesiredRotation);

	//		// 처음엔 숨겨진 상태
	//		AttachedTorch->SetActorHiddenInGame(true);
	//		bTorchVisible = false;

	//		UE_LOG(LogTemp, Log, TEXT("Torch attached and hidden on start."));
	//	}
	//}

	FSoftObjectPath TorchClassPath(TEXT("/Game/BP/light.light_C"));  // 꼭 "_C" 붙일 것!
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

	if (IsLocallyControlled() && GoldWidgetClass)
	{
		// 기존 위젯이 있으면 제거
		if (GoldWidgetInstance)
		{
			GoldWidgetInstance->RemoveFromParent();
			GoldWidgetInstance = nullptr;
			UE_LOG(LogTemp, Warning, TEXT("Removed existing Gold Widget"));
		}

		// 새로 생성
		GoldWidgetInstance = CreateWidget<UGoldWidget>(GetWorld(), GoldWidgetClass);
		if (GoldWidgetInstance)
		{
			GoldWidgetInstance->AddToViewport();
			GoldWidgetInstance->UpdateGoldAmount(Gold);
			UE_LOG(LogTemp, Warning, TEXT("Gold Widget created successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Gold Widget"));
		}
	}
	


	//UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	//if (GameInstance)
	//{
	//	ApplyCharacterData(GameInstance->CurrentCharacterData);

	//	// 인벤토리 복원
	//	if (InventoryComponent)
	//	{
	//		InventoryComponent->InventoryItemsStruct = GameInstance->SavedInventoryItems;
	//	}

	//	// 장비창 복원
	//	if (EquipmentWidgetInstance)
	//	{
	//		EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
	//	}

	//	//골드 복원
	//	if (GameInstance)
	//	{
	//		Gold = GameInstance->LobbyGold;
	//		GoldWidgetInstance->UpdateGoldAmount(Gold);
	//	}

	//}

	
		UFireballSpell* Fireball = NewObject<UFireballSpell>(this);
		SpellSet.Add(Fireball); // 인덱스 0번: 파이어 볼

		UHealSpell* Heal = NewObject<UHealSpell>(this);
		SpellSet.Add(Heal); // 인덱스 1번: 힐 마법

		UCurseSpell* Curse = NewObject<UCurseSpell>(this);
		SpellSet.Add(Curse);

	
	

	if (!CachedDirectionalLight)
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			CachedDirectionalLight = *It;
			break; // 하나만 사용한다고 가정
		}

		if (CachedDirectionalLight)
		{
			UE_LOG(LogTemp, Log, TEXT("Directional Light cached: %s"), *CachedDirectionalLight->GetName());
		}
	}

	RestoreDataFromLobby();

}



void AMyDCharacter::Restart()
{
	Super::Restart();

	if (IsLocallyControlled())
	{

		if (HUDWidgetClass)
		{
			// 기존 HUD 제거 (있다면)
			if (HUDWidget)
			{
				HUDWidget->RemoveFromParent();
				HUDWidget = nullptr;
			}

			// 새로 생성
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

		if (GoldWidgetClass)
		{
			// 기존 위젯이 있으면 제거
			if (GoldWidgetInstance)
			{
				GoldWidgetInstance->RemoveFromParent();
				GoldWidgetInstance = nullptr;
				UE_LOG(LogTemp, Warning, TEXT("Removed existing Gold Widget"));
			}

			// 새로 생성
			GoldWidgetInstance = CreateWidget<UGoldWidget>(GetWorld(), GoldWidgetClass);
			if (GoldWidgetInstance)
			{
				GoldWidgetInstance->AddToViewport();
				GoldWidgetInstance->UpdateGoldAmount(Gold);
				UE_LOG(LogTemp, Warning, TEXT("Gold Widget created successfully"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create Gold Widget"));
			}
		}

	}

	RestoreDataFromLobby();

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


// 매 프레임 호출
void AMyDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 구르는 동안 카메라 부드럽게 이동
	//moothCameraFollow();
	static float AccumulatedTime = 0.0f;
	AccumulatedTime += DeltaTime;

	if (AccumulatedTime >= 0.2f)
	{
		if (GetMesh())
		{
			GetMesh()->ResetAllBodiesSimulatePhysics();
		}

		// Armor도 동시에
		if (LegsMesh)
		{
			LegsMesh->ResetAllBodiesSimulatePhysics();
		}

		AccumulatedTime = 0.0f;
	}
}

// 이동 함수 (W/S)
void AMyDCharacter::MoveForward(float Value)
{

	if (bIsRolling)
	{
		// 구르는 방향이 앞이나 뒤일 때만 이동 허용
		float DotProductF = FVector::DotProduct(GetActorForwardVector(), StoredRollDirection);
		if (FMath::Abs(DotProductF) < 0.8f) return; // 앞/뒤 방향이 아니라면 이동 차단
	}
	

	if (Value != 0.0f)
	{
		//컨트롤러(카메라)의 회전 방향을 가져옴
		FRotator ControlRotation = GetControlRotation();
		FVector ForwardDirection = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);

		UE_LOG(LogTemp, Log, TEXT("MoveForward called with value: %f"), Value);
		AddMovementInput(ForwardDirection, Value);
	}
}
// 이동 함수 (A/D)
void AMyDCharacter::MoveRight(float Value)
{

	if (bIsRolling)
	{
		// 구르는 방향이 좌우일 때만 이동 허용
		float DotProductR = FVector::DotProduct(GetActorRightVector(), StoredRollDirection);
		if (FMath::Abs(DotProductR) < 0.8f) return; // 좌/우 방향이 아니라면 이동 차단
	}
	

	if (Value != 0.0f)
	{
		//컨트롤러(카메라)의 회전 방향을 가져옴
		FRotator ControlRotation = GetControlRotation();
		
		FVector RightDirection = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);
	

		UE_LOG(LogTemp, Log, TEXT("MoveRight called with value: %f"), Value);
		AddMovementInput(RightDirection, Value);

		
		
	}
}

// 달리기 시작
void AMyDCharacter::StartSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed * Agility;
	GetWorldTimerManager().SetTimer(TimerHandle_SprintDrain, this, &AMyDCharacter::SprintStaminaDrain, 0.1f, true);
	if (bIsSlowed)
	{
		ApplySpeedMultiplier();
	}
}

// 달리기 중지
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
	GetWorldTimerManager().ClearTimer(TimerHandle_SprintDrain);
	ManageStaminaRegen();  //달리기 끝나고 스테미나 회복 시키기

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
// 입력 바인딩 설정
void AMyDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Log, TEXT("SetupPlayerInputComponent called!"));

	// 이동 입력 바인딩 (WASD)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::MoveRight);

	// 방향 입력을 저장하기 위한 함수 (구르기 방향 계산에 사용)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::UpdateMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::UpdateMoveRight);

	// 달리기 (Shift 키)
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMyDCharacter::StartSprinting);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMyDCharacter::StopSprinting);

	// 점프 입력 바인딩
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMyDCharacter::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMyDCharacter::StopJump);


	//마우스 입력 바인딩 (카메라 회전)
	PlayerInputComponent->BindAxis("Turn", this, &AMyDCharacter::AddControllerYawInput);  // 좌우 회전
	PlayerInputComponent->BindAxis("LookUp", this, &AMyDCharacter::AddControllerPitchInput);  // 상하 회전
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMyDCharacter::StartInteraction);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMyDCharacter::StopInteraction);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMyDCharacter::PickupWeapon);

	PlayerInputComponent->BindAction("DropWeapon", IE_Pressed, this, &AMyDCharacter::DropWeapon);

	// 공격 입력 추가 (마우스 좌클릭)
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AMyDCharacter::PlayAttackAnimation);

	PlayerInputComponent->BindAction("Roll", IE_Pressed, this, &AMyDCharacter::PlayRollAnimation);

	PlayerInputComponent->BindAction("ToggleInventory", IE_Pressed, this, &AMyDCharacter::ToggleInventoryUI);

	//스펠 사용 추가 (Q키)
	PlayerInputComponent->BindAction("CastSpell1", IE_Pressed, this, &AMyDCharacter::CastSpell1);

	//스펠 사용 추가 (E키)
	PlayerInputComponent->BindAction("CastSpell2", IE_Pressed, this, &AMyDCharacter::CastSpell2);
	
		PlayerInputComponent->BindAction("ToggleMapView", IE_Pressed, this, &AMyDCharacter::ToggleMapView);
	
	//f키
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

	// 핫키 입력 바인딩 (예: 1~5 키)

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
	// 조건 제거: 그냥 오버랩 끝났으면 OverlappedActor를 초기화
	OverlappedActor = nullptr;

	// GameInstance 변수 초기화
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
	// 게임 인스턴스를 가져옴
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

	if (GameInstance)
	{
		// R 키를 누르면 두 변수 모두 true
		GameInstance->itemEAt = true;
		GameInstance->OpenDoor = true;
		GameInstance->WeaponEAt = true;

		

		UE_LOG(LogTemp, Log, TEXT("StartInteraction() : itemEAt = true, OpenDoor = true"));

		//보물상자 열기 로직 추가
		if (OverlappedActor && OverlappedActor->ActorHasTag("Chest"))
		{
			ATreasureChest* Chest = Cast<ATreasureChest>(OverlappedActor);


			if (Chest)
			{
				Chest->OpenChestUI(this); // 플레이어를 전달하여 양쪽 UI 열기
				UE_LOG(LogTemp, Log, TEXT("Opened chest UI"));
			}
			// 적 캐릭터 루팅 상호작용
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

			//경고 UI 표시 (UMG로 표시하는 방식에 따라 구현 필요)
			UE_LOG(LogTemp, Warning, TEXT("55555se"));

			// 로컬이 아닌 항상 서버 호출
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
	// DoorActor를 Door BP로 캐스팅
	if (DoorActor)
	{
		// 예: Door BP에 Server_RequestOpenDoor라는 함수가 있다고 가정
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
	// 게임 인스턴스를 가져옴
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

	if (GameInstance)
	{
		// R 키를 떼면 두 변수 모두 false
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




// 무기 줍기 함수 (R 키 입력 시 실행)
void AMyDCharacter::PickupWeapon()
{
	if (OverlappingWeapon)
	{
		// 현재 상호작용 중인 액터가 문인지 확인
		if (OverlappedActor && OverlappedActor->ActorHasTag("Door"))
		{
			UE_LOG(LogTemp, Log, TEXT("Player is interacting with a door, skipping weapon pickup."));
			return; // 문과 상호작용 중이면 무기 줍지 않음
		}

		// 기존 무기를 버리지 않고 바닥에 내려놓도록 변경
		if (EquippedWeapon)
		{
			DropWeapon();
		}

		// 새로운 무기 장착
		EquippedWeapon = OverlappingWeapon;
		OverlappingWeapon = nullptr;

		// 손 소켓이 있는지 확인
		if (CharacterMesh->DoesSocketExist(FName("hand_r")))
		{
			UE_LOG(LogTemp, Log, TEXT("Socket hand_r exists! Attaching weapon."));

			// 손 소켓 위치 출력
			FTransform HandSocketTransform = CharacterMesh->GetSocketTransform(FName("hand_r"));
			UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());

			// 무기 부착
			FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
			EquippedWeapon->AttachToComponent(CharacterMesh, AttachRules, FName("hand_r"));

			// 무기 위치 갱신용 초기화
			if (EquippedWeapon && EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->LastStartLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackStart"));
				EquippedWeapon->LastEndLocation = EquippedWeapon->WeaponMesh->GetSocketLocation(FName("AttackEnd"));
				EquippedWeapon->SetOwner(this);
			}

			// 무기 중력 및 물리 시뮬레이션 비활성화
			if (EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->WeaponMesh->SetSimulatePhysics(false);  //물리 비활성화
				EquippedWeapon->WeaponMesh->SetEnableGravity(false);    //중력 비활성화
			}

			// 무기 충돌 비활성화 및 숨김 해제
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

// 무기 내려놓기 함수 (G 키)
void AMyDCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		UE_LOG(LogTemp, Log, TEXT("Dropping weapon: %s"), *EquippedWeapon->GetName());

		EquippedWeapon->RemoveWeaponStats(this);

		// 부착 해제
		EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 무기 충돌 활성화
		EquippedWeapon->SetActorEnableCollision(true);

		// 무기 중력 및 물리 시뮬레이션 활성화
		if (EquippedWeapon->WeaponMesh)
		{
			EquippedWeapon->WeaponMesh->SetSimulatePhysics(true);  //물리 활성화
			EquippedWeapon->WeaponMesh->SetEnableGravity(true);    //중력 활성화
		}

		// 무기를 캐릭터 앞쪽에 놓기
		FVector DropLocation = GetActorLocation() + GetActorForwardVector() * 100.0f; // 캐릭터 앞 100cm 위치
		EquippedWeapon->SetActorLocation(DropLocation);

		// 무기 초기화
		EquippedWeapon = nullptr;
	}
}

void AMyDCharacter::EquipWeaponFromClass(TSubclassOf<AItem> WeaponClass, EWeaponGrade Grade)
{
	if (!WeaponClass) {
		return;
	}
	// 기존 무기 제거
	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	// 새 무기 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	// 손 소켓 위치 출력
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

			// 소켓 위치가 손 소켓에 오도록 WeaponMesh 전체를 반대로 이동
			FVector OffsetLocation = -GripTransform.GetLocation();
			FRotator OffsetRotation = (-GripTransform.GetRotation()).Rotator();

			EquippedWeapon->WeaponMesh->SetRelativeLocation(OffsetLocation);
			EquippedWeapon->WeaponMesh->SetRelativeRotation(OffsetRotation);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GripWeapon 소켓이 StaticMesh에 없음"));
		}


		// 필수 초기화들 추가
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

	// --- 기본 머티리얼 오버라이드 제거 헬퍼 람다
	auto ResetToDefaultMaterials = [&](USkeletalMeshComponent* MeshComp) {
		if (!MeshComp) return;
		int32 NumMats = MeshComp->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			// nullptr을 넘기면 에셋에 설정된 머티리얼로 자동 복구됩니다
			MeshComp->SetMaterial(i, nullptr);
		}
	};

	switch (SlotIndex)
	{
	case EQUIP_SLOT_CHEST:




		if (ChestMesh)
		{
			//ChestMesh->SetMasterPoseComponent(CharacterMesh);
			ChestMesh->SetSkeletalMesh(NewMesh);
			ChestMesh->SetVisibility(true);
			// 필요 시 상대 위치 조정
			/*ChestMesh->SetRelativeLocation(FVector(0.0f, 0.f, -90.f));
			ChestMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));*/
			if (Armor && Armor->IsA(ARobeTop::StaticClass()))
			{
				ChestMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
				// 머티리얼 적용 (상의는 모든 슬롯 반복)
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
			ChestMesh->SetLeaderPoseComponent(CharacterMesh, true, true);
			ResetToDefaultMaterials(ChestMesh);

			// 머티리얼 적용
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

			LegsMesh->SetMasterPoseComponent(nullptr); // 먼저 해제
			LegsMesh->SetMasterPoseComponent(CharacterMesh); // 다시 설정
			// 애니메이션 인스턴스 동기화 강제
			if (CharacterMesh->GetAnimInstance())
			{
				LegsMesh->SetAnimInstanceClass(nullptr); // 먼저 클리어
				LegsMesh->bNoSkeletonUpdate = false;
				LegsMesh->bUpdateJointsFromAnimation = true;
				LegsMesh->RefreshBoneTransforms();
			}
			LegsMesh->SetVisibility(true);
			// 하의 루트 위치가 캐릭터보다 너무 위에 있음 → 보정 필요
			/*LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
			LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));*/
			if (Armor && Armor->IsA(ARobeBottom::StaticClass()))
			{
				LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			}
			else
			{
				LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
			}
			//LegsMesh->SetLeaderPoseComponent(CharacterMesh, true, true);

			ResetToDefaultMaterials(LegsMesh);

			// 머티리얼 적용
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

////2025 - 07 - 15
////void AMyDCharacter::EquipArmorMesh(int32 SlotIndex, USkeletalMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor)
////{
////	if (!NewMesh) return;
////
////	UE_LOG(LogTemp, Warning, TEXT("EquipArmorMesh called with SlotIndex = %d, Mesh = %s"),
////		SlotIndex, *NewMesh->GetName());
////
////	// 기본 머티리얼 오버라이드 제거 함수
////	auto ResetToDefaultMaterials = [&](USkeletalMeshComponent* MeshComp) {
////		if (!MeshComp) return;
////		int32 NumMats = MeshComp->GetNumMaterials();
////		for (int32 i = 0; i < NumMats; ++i)
////		{
////			MeshComp->SetMaterial(i, nullptr);
////		}
////		};
////
////	switch (SlotIndex)
////	{
////	case EQUIP_SLOT_CHEST:
////		if (ChestMesh)
////		{
////			// 갑옷 메쉬 설정
////			ChestMesh->SetMasterPoseComponent(CharacterMesh);
////			ChestMesh->SetSkeletalMesh(NewMesh);
////			ChestMesh->SetVisibility(true);
////			ChestMesh->SetRelativeLocation(FVector(0.0f, 0.f, -90.f));
////			ChestMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
////
////			// 상의 장착 시 Body와 Torso만 숨기기
////			if (CharacterMesh) CharacterMesh->SetVisibility(false);  // Body 메쉬 숨기기
////			if (TorsoMesh) TorsoMesh->SetVisibility(false);         // Torso 메쉬 숨기기
////
////			// 하의는 그대로 보이게 유지
////			// LegsMeshmetha와 FeetMesh는 건드리지 않음
////
////			// 갑옷 머티리얼 적용
////			ResetToDefaultMaterials(ChestMesh);
////			if (Grade == EArmorGrade::B && SilverMat)
////			{
////				ChestMesh->SetMaterial(0, SilverMat);
////			}
////			else if (Grade == EArmorGrade::A && GoldMat)
////			{
////				ChestMesh->SetMaterial(0, GoldMat);
////			}
////
////			// 로브인 경우 스케일 조정
////			if (Armor && Armor->IsA(ARobeTop::StaticClass()))
////			{
////				ChestMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
////			}
////		}
////		break;
////
////	case EQUIP_SLOT_LEG:
////		if (LegsMesh)
////		{
////			// 갑옷 메쉬 설정
////			LegsMesh->SetSkeletalMesh(NewMesh);
////			LegsMesh->SetVisibility(true);
////			LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
////			LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
////
////			// 하의 장착 시 LegsMeshmetha와 FeetMesh만 숨기기
////			if (LegsMeshmetha) LegsMeshmetha->SetVisibility(false);  // 기본 바지 숨기기
////			if (FeetMesh) FeetMesh->SetVisibility(false);            // 기본 신발 숨기기
////
////			// 상체는 그대로 보이게 유지
////			// CharacterMesh(Body)와 TorsoMesh는 건드리지 않음
////
////			// 갑옷 머티리얼 적용
////			ResetToDefaultMaterials(LegsMesh);
////			if (Grade == EArmorGrade::B && SilverMat)
////			{
////				LegsMesh->SetMaterial(0, SilverMat);
////			}
////			else if (Grade == EArmorGrade::A && GoldMat)
////			{
////				LegsMesh->SetMaterial(0, GoldMat);
////			}
////
////			// 로브인 경우 스케일 조정
////			if (Armor && Armor->IsA(ARobeBottom::StaticClass()))
////			{
////				LegsMesh->SetRelativeScale3D(FVector(1.0f, 1.25f, 1.0f));
////			}
////		}
////		break;
////	}
////}
//
////2025 - 07 - 15 버전 2
//void AMyDCharacter::EquipArmorMesh(int32 SlotIndex, USkeletalMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor)
//{
//	if (!NewMesh) return;
//
//	UE_LOG(LogTemp, Warning, TEXT("EquipArmorMesh called with SlotIndex = %d, Mesh = %s"),
//		SlotIndex, *NewMesh->GetName());
//
//	// --- 기본 머티리얼 오버라이드 제거 헬퍼 람다
//	auto ResetToDefaultMaterials = [&](USkeletalMeshComponent* MeshComp) {
//		if (!MeshComp) return;
//		int32 NumMats = MeshComp->GetNumMaterials();
//		for (int32 i = 0; i < NumMats; ++i)
//		{
//			MeshComp->SetMaterial(i, nullptr);
//		}
//		};
//
//	switch (SlotIndex)
//	{
//	case EQUIP_SLOT_CHEST:
//		if (TorsoMesh)  // <-- ChestMesh 대신 TorsoMesh 사용
//		{
//			// 1) 새 메시 적용
//			TorsoMesh->SetSkeletalMesh(NewMesh);
//			TorsoMesh->SetVisibility(true);
//
//			// 2) 위치/회전/스케일 보정 (원래 ChestMesh 값에서 조금 바꿀 수도 있음)
//			/*TorsoMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.f));
//			TorsoMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
//			TorsoMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));*/
//
//			// 3) Bone Transform 복사
//			TorsoMesh->SetLeaderPoseComponent(CharacterMesh, true, true);
//			//TorsoMesh->SetMasterPoseComponent(CharacterMesh, true);
//			// 4) 머티리얼 리셋 → 등급별 머티리얼 적용
//			ResetToDefaultMaterials(TorsoMesh);
//			int32 NumMaterials = TorsoMesh->GetNumMaterials();
//			for (int32 i = 0; i < NumMaterials; ++i)
//			{
//				if (Grade == EArmorGrade::B && SilverMat)
//					TorsoMesh->SetMaterial(i, SilverMat);
//				else if (Grade == EArmorGrade::A && GoldMat)
//					TorsoMesh->SetMaterial(i, GoldMat);
//			}
//		}
//		break;
//
//	case EQUIP_SLOT_LEG:
//		if (LegsMesh)
//		{
//			// 기존 로직 유지
//			LegsMesh->SetSkeletalMesh(NewMesh);
//			LegsMesh->SetVisibility(true);
//			LegsMesh->SetRelativeLocation(FVector(1.65f, 0.0f, -90.f));
//			LegsMesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
//			LegsMesh->SetRelativeScale3D(Armor && Armor->IsA(ARobeBottom::StaticClass())
//				? FVector(1.0f, 1.25f, 1.0f)
//				: FVector(1.0f, 1.0f, 1.0f));
//			LegsMesh->SetLeaderPoseComponent(CharacterMesh, true, true);
//			ResetToDefaultMaterials(LegsMesh);
//			if (Grade == EArmorGrade::B && SilverMat)
//				LegsMesh->SetMaterial(0, SilverMat);
//			else if (Grade == EArmorGrade::A && GoldMat)
//				LegsMesh->SetMaterial(0, GoldMat);
//
//			UE_LOG(LogTemp, Warning, TEXT("EquipArmorMesh SlotIndex=%d, Mesh=%s"), SlotIndex, *NewMesh->GetName());
//		}
//		break;
//	}
//}



void AMyDCharacter::EquipArmorFromClass(int32 SlotIndex, TSubclassOf<AItem> ArmorClass, uint8 Grade)
{
	if (!ArmorClass) return;

	// 기존 장착 아이템 제거
	UnequipArmorAtSlot(SlotIndex);

	// 장비 액터 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AArmor* Armor = GetWorld()->SpawnActor<AArmor>(ArmorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!Armor) return;

	Armor->ArmorGrade = static_cast<EArmorGrade>(Grade);

	// 메시만 캐릭터 컴포넌트에 설정

	// 헬멧 슬롯은 StaticMesh 처리
	if (SlotIndex == EQUIP_SLOT_HELMET)
	{
		
			// 일반 헬멧(StaticMesh) 처리
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
	

	UE_LOG(LogTemp, Warning, TEXT("[ARMOR EQUIP] Slot %d → %s (Grade: %s)"),
		SlotIndex,
		*Armor->GetName(),
		*UEnum::GetValueAsString(Armor->ArmorGrade));

	// 스탯 반영 및 장비 목록 등록
	EquippedArmors.Add(SlotIndex, Armor);
	Armor->ApplyArmorStats(this);

	// Armor 액터는 시각적으로 필요 없음
	Armor->SetActorHiddenInGame(true);
	Armor->SetActorEnableCollision(false);
}

void AMyDCharacter::EquipHelmetMesh(UStaticMesh* NewMesh, EArmorGrade Grade, UMaterialInterface* SilverMat, UMaterialInterface* GoldMat, AArmor* Armor)
{
	if (!NewMesh || !HelmetMesh) return;

	HelmetMesh->SetStaticMesh(NewMesh);
	HelmetMesh->SetVisibility(true);

	// 기본 크기
	FVector DesiredScale = FVector(1.f);

	// 모자면 크기 줄이기
	if (Armor && Armor->IsA<AHat>() || Armor->IsA<AMask>())
	{
		DesiredScale = FVector(0.1f); // 원하는 크기로 조절
		UE_LOG(LogTemp, Log, TEXT("Helmet: AHat detected, scaled down"));
	}

	HelmetMesh->SetRelativeScale3D(DesiredScale);

	
	{
		int32 NumMats = HelmetMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			// nullptr 로 설정하면 에셋에 설정된 기본 머티리얼로 자동 복구됩니다
			HelmetMesh->SetMaterial(i, nullptr);
		}
	}


	// 머티리얼 직접 적용
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
		// 시각적으로도 제거
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

//2025 - 07 -15
//void AMyDCharacter::UnequipArmorAtSlot(int32 SlotIndex)
//{
//	if (EquippedArmors.Contains(SlotIndex))
//	{
//		AArmor* Armor = EquippedArmors[SlotIndex];
//		if (Armor)
//		{
//			Armor->RemoveArmorStats(this);
//			Armor->Destroy();
//		}
//
//		// 시각적으로도 제거
//		switch (SlotIndex)
//		{
//		case EQUIP_SLOT_CHEST:
//			if (ChestMesh)
//			{
//				ChestMesh->SetSkeletalMesh(nullptr);
//				ChestMesh->SetVisibility(false);
//			}
//			// 상의 해제 시 Body와 Torso만 다시 보이기
//			if (CharacterMesh) CharacterMesh->SetVisibility(true);  // Body 메쉬 복구
//			if (TorsoMesh) TorsoMesh->SetVisibility(true);         // Torso 메쉬 복구
//			break;
//
//		case EQUIP_SLOT_LEG:
//			if (LegsMesh)
//			{
//				LegsMesh->SetSkeletalMesh(nullptr);
//				LegsMesh->SetVisibility(false);
//			}
//			// 하의 해제 시 LegsMeshmetha와 FeetMesh만 다시 보이기
//			if (LegsMeshmetha) LegsMeshmetha->SetVisibility(true);  // 기본 바지 복구
//			if (FeetMesh) FeetMesh->SetVisibility(true);            // 기본 신발 복구
//			break;
//
//		case EQUIP_SLOT_HELMET:
//			if (HelmetMesh)
//			{
//				HelmetMesh->SetStaticMesh(nullptr);
//				HelmetMesh->SetVisibility(false);
//			}
//			// 투구는 다른 메쉬에 영향 없음
//			break;
//		}
//
//		EquippedArmors.Remove(SlotIndex);
//	}
//}


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

	// 기존 슬롯 데이터 갱신
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

	// 현재 장착된 슬롯들 확인
	TArray<int32> CurrentSlots;
	for (auto& Pair : EquippedArmors)
	{
		CurrentSlots.Add(Pair.Key);
	}

	// 새로운 데이터에 있는 슬롯들 확인
	TArray<int32> NewSlots;
	for (const FEquippedArmorData& ArmorEntry : EquippedArmorsData)
	{
		NewSlots.Add(ArmorEntry.SlotIndex);
	}

	// 제거된 슬롯들 해제
	for (int32 CurrentSlot : CurrentSlots)
	{
		if (!NewSlots.Contains(CurrentSlot))
		{
			UnequipArmorAtSlot(CurrentSlot);
			UE_LOG(LogTemp, Log, TEXT("Client: Unequipped armor at slot %d"), CurrentSlot);
		}
	}

	// 새로운 장비들 장착
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
	EquippedWeaponData = FItemData(); // 기본값
	//ForceNetUpdate();

	if (IsLocallyControlled())
	{
		UnequipWeapon();
	}

	OnRep_EquippedWeapon();
}

void AMyDCharacter::ServerRequestUnequipArmor_Implementation(int32 SlotIndex)
{
	// 데이터에서 제거
	for (int32 i = 0; i < EquippedArmorsData.Num(); ++i)
	{
		if (EquippedArmorsData[i].SlotIndex == SlotIndex)
		{
			EquippedArmorsData.RemoveAt(i);
			break;
		}
	}

	// 서버에서 직접 해제 (조건 제거)
	UnequipArmorAtSlot(SlotIndex);

	// 네트워크 업데이트 강제
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
	DOREPLIFETIME(AMyDCharacter, Gold);

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
		// 비교는 여기서!
		if (PreviousHealth < 0.0f || Health < PreviousHealth)
		{
			// 빨간 연출 직접 실행
			float NormalizedHealth = Health / MaxHealth;
			float OverlayAlpha = FMath::Clamp(1.0f - NormalizedHealth, 0.0f, 0.6f);

			HUDWidget->Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, OverlayAlpha));
			HUDWidget->Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			HUDWidget->StartHitOverlayFadeOut();
		}

		// HUD에 단순히 현재 체력 표시
		HUDWidget->UpdateHealth(Health, MaxHealth);

		PreviousHealth = Health;
	}
}


// 점프 시작
void AMyDCharacter::StartJump()
{
	Jump();  // ACharacter의 기본 점프 기능 호출
}

// 점프 멈추기
void AMyDCharacter::StopJump()
{
	StopJumping();  // 점프 멈추는 함수 호출
}

void AMyDCharacter::PlayMagicMontage()
{
	if (!SpellCastMontage || !CharacterMesh || !CharacterMesh->GetAnimInstance())
	{
		UE_LOG(LogTemp, Error, TEXT("SpellCastMontage or AnimInstance missing!"));
		return;
	}

	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();

	// 몽타주 재생
	float Duration = AnimInstance->Montage_Play(SpellCastMontage, 1.0f);
}

void AMyDCharacter::PlayAttackAnimation()
{
	//if (bIsAttacking)  // 공격 중이면 입력 무시
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Already attacking! Ignoring input."));
	//	return;
	//}

	//if (bIsAttacking || bIsRolling)  // 구르기 중이면 공격 입력 무시
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Already attacking or rolling! Ignoring input."));
	//	return;
	//}

	//if (Stamina <= 0)  // 스태미나가 0이면 공격 불가
	//{
	//	ReduceStamina(0.0f);
	//	return;
	//}

	//// 공격 상태 설정
	//bIsAttacking = true;

	//ReduceStamina(AttackStaminaCost);
	//

	//// 사용할 몽타주 결정 (무기 장착 여부에 따라 다르게 설정)
	//UAnimMontage* SelectedMontage = nullptr;

	//if (EquippedWeapon)
	//{
	//	switch (EquippedWeapon->WeaponType)
	//	{
	//	case EWeaponType::GreatWeapon:
	//		SelectedMontage = GreatWeaponMontage; // 이건 변수로 미리 만들어둬야 함
	//		break;
	//	case EWeaponType::Dagger:
	//		SelectedMontage = DaggerWeaponMontage;
	//		break;
	//	case EWeaponType::Staff:
	//		//SelectedMontage = StaffMontage;
	//		break;
	//	default:
	//		SelectedMontage = WeaponAttackMontage; // 기본값
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

	//// **현재 콤보 수에 따라 재생할 섹션 선택**
	//FName SelectedSection = (AttackComboIndex == 0) ? FName("Combo1") : FName("Combo2");

	//// 특정 슬롯에서만 실행 (UpperBody 슬롯에서 재생)
	//FName UpperBodySlot = FName("UpperBody");

	//// UpperBody 슬롯에서 실행되도록 설정
	//FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(SelectedMontage); 
	//if (MontageInstance) 
	//{
	//	MontageInstance->Montage->SlotAnimTracks[0].SlotName = UpperBodySlot; 
	//}

	//if (EquippedWeapon)
	//{
	//	EquippedWeapon->StartTrace(); 
	//}

	//// 애니메이션 실행 (선택된 섹션 처음부터 재생)
	//AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	//AnimInstance->Montage_JumpToSection(SelectedSection, SelectedMontage);
	//
	//UE_LOG(LogTemp, Log, TEXT("Playing Attack Montage Section: %s"), *SelectedSection.ToString());

	//// **현재 섹션의 길이 가져오기**
	//float SectionDuration = SelectedMontage->GetSectionLength(SelectedMontage->GetSectionIndex(SelectedSection));

	//// **타이머 설정: 정확한 섹션 종료 후 공격 상태 초기화**
	//GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, SectionDuration, false);

	//// **다음 입력 시 다른 섹션 실행되도록 설정 (콤보 증가)**
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
	// 공격 상태 초기화
	bIsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("Attack Ended, Resetting Attack State"));

	ResetHitActors();
}

void AMyDCharacter::ServerPerformTraceAttack_Implementation()
{
	// HitActors를 서버에서 초기화
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

	//if (Stamina <= 0)  // 스태미나가 0이면 공격 불가
	//{
	//	ReduceStamina(0.0f);
	//	return;
	//}

	//bIsRolling = true;

	//// 현재 이동 방향 계산
	//FVector RollDirection;
	//FName SelectedSection = "RollF"; // 기본값 (앞구르기)

	//ReduceStamina(RollStaminaCost);

	//if (FMath::Abs(MoveForwardValue) > 0.1f || FMath::Abs(MoveRightValue) > 0.1f)
	//{
	//	// 방향키 입력이 있을 경우: 해당 방향으로 구르기
	//	FRotator ControlRotation = GetControlRotation();
	//	FVector ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
	//	FVector RightVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);

	//	RollDirection = ForwardVector * MoveForwardValue + RightVector * MoveRightValue;
	//	RollDirection.Normalize();

	//	// **입력 방향에 따라 적절한 섹션 선택**
	//	if (MoveForwardValue > 0.1f)
	//	{
	//		SelectedSection = "RollF";  // 앞구르기
	//	}
	//	else if (MoveForwardValue < -0.1f)
	//	{
	//		SelectedSection = "RollB";  // 뒤구르기
	//	}
	//	else if (MoveRightValue > 0.1f)
	//	{
	//		SelectedSection = "RollR";  // 오른쪽 구르기
	//	}
	//	else if (MoveRightValue < -0.1f)
	//	{
	//		SelectedSection = "RollL";  // 왼쪽 구르기
	//	}
	//}
	//else
	//{
	//	// 방향키 입력이 없을 경우: 기본적으로 앞구르기
	//	RollDirection = GetActorForwardVector();
	//}

	//// RollDirection을 멤버 변수로 저장 (이동 함수에서 사용)
	//StoredRollDirection = RollDirection;

	//// **구르기 방향으로 캐릭터 회전 (카메라는 고정)**
	//if (RollDirection.SizeSquared() > 0)
	//{
	//	FRotator NewRotation = RollDirection.Rotation();
	//	SetActorRotation(NewRotation);
	//}

	//// **애니메이션 실행 (선택된 섹션으로 점프)**
	//UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	//if (AnimInstance)
	//{
	//	AnimInstance->Montage_Play(RollMontage, 1.0f);
	//	AnimInstance->Montage_JumpToSection(SelectedSection, RollMontage);
	//	UE_LOG(LogTemp, Log, TEXT("Playing Roll Montage Section: %s"), *SelectedSection.ToString());
	//}

	//// **구르기 이동 적용**
	//ApplyRollMovement(RollDirection);

	//// **구르기 후 원래 상태 복구**
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
	bIsInvulnerable = false;

	// 구르기 시작 시 충돌 비활성화
	//SetActorEnableCollision(true);

	bIsAttacking = false;  // 구르기가 끝나면 공격 상태도 초기화
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
}


void AMyDCharacter::ApplyRollMovement(FVector RollDirection)
{

	if (RollDirection.SizeSquared() > 0)
	{
		RollDirection.Z = 0.0f;
		RollDirection.Normalize();

		FVector RollImpulse = RollDirection * RollSpeed;

		// 물리 기반 이동
		GetCharacterMovement()->Launch(RollImpulse);

		// 구르는 동안 중력 적용 가능 (필요하면 주석 해제)
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
	bIsInvulnerable = true;
	// 구르기 시작 시 충돌 비활성화
	//SetActorEnableCollision(false);

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
	//스태미나 감소
	Stamina -= StaminaCost;
	Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);

	// UI 업데이트
	UpdateHUD();

	// 회복 중단 & 재생성 대기
	GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
	ManageStaminaRegen();
}

void AMyDCharacter::ManageStaminaRegen()
{
	//달리기 중이면 회복 금지 (삭제예정)
	if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->MaxWalkSpeed > WalkSpeed)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
		return;
	}

	// 3초간 행동이 없으면 회복 시작
	if (!GetWorldTimerManager().IsTimerActive(TimerHandle_StaminaRegenDelay))
	{
		GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegenDelay, this, &AMyDCharacter::StartStaminaRegen, 3.0f, false);
	}
}

// 스태미나 실제 회복 함수
void AMyDCharacter::StartStaminaRegen()
{
	if (!bIsAttacking && !bIsRolling && GetCharacterMovement()->MaxWalkSpeed == WalkSpeed)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_StaminaRegen, this, &AMyDCharacter::RegenerateStamina, 0.5f, true);
	}
}

void AMyDCharacter::RegenerateStamina() // 스태미나 회복 진행
{

	if (Stamina < MaxStamina)
	{
		Stamina += StaminaRegenRate;
		Stamina = FMath::Clamp(Stamina, 0.0f, MaxStamina);

		//UI 업데이트 반영
		HUDWidget->UpdateStamina(Stamina, MaxStamina);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_StaminaRegen);
	}
}

void AMyDCharacter::GetHit_Implementation(const FHitResult& HitResult, AActor* InstigatorActor, float Damage)
{

	// 무적 상태면 데미지 무시
	if (bIsInvulnerable)
	{
		UE_LOG(LogTemp, Log, TEXT("Damage ignored - Player is invulnerable"));
		return;
	}

	Health -= Damage;
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);

	//UpdateHUD();

	// 서버 전용 HUD 갱신
	if (IsLocallyControlled() && HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);

		float NormalizedHealth = Health / MaxHealth;
		float OverlayAlpha = FMath::Clamp(1.0f - NormalizedHealth, 0.0f, 0.6f);

		HUDWidget->Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, OverlayAlpha));
		HUDWidget->Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HUDWidget->StartHitOverlayFadeOut();
	}

	// 클라들에는 RepNotify를 통해 자동 전파
	OnRep_Health();

	if (Health <= 0)
	{
		DoRagDoll();
		if (IsLocallyControlled())
		{
			ExecuteEscape();       // 인벤토리·플래그
		}
		if (HasAuthority())
		{
			ServerHandleEscape();  // 서버 RPC · 파괴 타이머
		}
	}

}

void AMyDCharacter::ServerHandleDeath_Implementation()
{
	if (!HasAuthority()) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	UE_LOG(LogTemp, Warning, TEXT("ServerHandleDeath - Player %s died"), *PC->GetPlayerState<APlayerState>()->GetPlayerName());

	// 데이터 저장
	//UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	//if (GameInstance)
	//{
	//	// 인벤토리, 장비 등 저장
	//	//SaveDataToGameInstance();

	//}

	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->bIsReturningFromGame = true;
		if (!GameInstance->CurrentCharacterData.PlayerName.IsEmpty())
		{
			GameInstance->bHasValidCharacterData = true;
		}
	}
	// 관전자 모드로 전환
	PC->UnPossess();
	PC->ChangeState(NAME_Spectating);
	PC->ClientGotoState(NAME_Spectating);

	// 클라이언트에게 관전자 UI 표시
	ClientEnterSpectatorMode();

	// 캐릭터 제거 (일정 시간 후)
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(DestroyTimer, [this]()
		{
			if (IsValid(this))
			{
				Destroy();
			}
		}, 3.0f, false);

	// GameMode에서 체크하도록 변경
	ADynamicDungeonModeBase* GameMode = Cast<ADynamicDungeonModeBase>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		// 타이머로 딜레이 후 체크
		FTimerHandle CheckTimer;
		GetWorld()->GetTimerManager().SetTimer(CheckTimer, [GameMode]()
			{
				if (IsValid(GameMode))
				{
					GameMode->CheckAllPlayersStatus();
				}
			}, 1.0f, false);
	}
}

void AMyDCharacter::ClientEnterSpectatorMode_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("ClientEnterSpectatorMode - Entering spectator mode"));

	/*if (IsLocallyControlled())
	{*/
		UDynamicDungeonInstance* GI = Cast<UDynamicDungeonInstance>(GetGameInstance());
		if (GI)
		{
			GI->bIsReturningFromGame = true;
			if (!GI->CurrentCharacterData.PlayerName.IsEmpty())
				GI->bHasValidCharacterData = true;
		}
	//}

	// 게임 UI 숨기기
	if (HUDWidget) HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	if (InventoryWidgetInstance) InventoryWidgetInstance->RemoveFromParent();
	if (EquipmentWidgetInstance) EquipmentWidgetInstance->RemoveFromParent();

	// 관전자 UI 표시
	APlayerController* PC = GetController<APlayerController>();
	if (PC)
	{



		// 간단한 관전자 메시지
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow,
				TEXT("You are now spectating. Waiting for other players to finish..."));
		}

		
	}
}



void AMyDCharacter::DoRagDoll() 
{

	CharacterMesh->SetSimulatePhysics(true);
	CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
}

void AMyDCharacter::ResetHitActors()
{
	HitActors.Empty(); //피격 목록 초기화
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

		// 마우스 커서 비활성화 + 게임 입력으로 전환
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	else
	{
		InventoryWidgetInstance->AddToViewport(10);
		InventoryWidgetInstance->SetPositionInViewport(FVector2D(0, 0), false);
		InventoryWidgetInstance->RefreshInventoryStruct();

		EquipmentWidgetInstance->AddToViewport(10); // 인벤토리보다 위일 수도 있음
		EquipmentWidgetInstance->SetPositionInViewport(FVector2D(100, 0), false);
		EquipmentWidgetInstance->RefreshEquipmentSlots(); // 나중에 함수에서 슬롯 정보 반영하게 만들 수 있음

		//if (CombinedInventoryWidgetClass)
		//{
		//	CombinedInventoryWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), CombinedInventoryWidgetClass);
		//	if (CombinedInventoryWidgetInstance)
		//	{
		//		CombinedInventoryWidgetInstance->AddToViewport(10); // ZOrder 적당히
		//		//CombinedInventoryWidgetInstance->SetVisibility(ESlateVisibility::Hidden); // 처음엔 숨겨둠
		//		InventoryWidgetInstance->RefreshInventoryStruct();
		//		EquipmentWidgetInstance->RefreshEquipmentSlots();
		//		UE_LOG(LogTemp, Log, TEXT("wrwrwrwrrEQ"));
		//	}
		//}

		bIsInventoryVisible = true;

		// 마우스 커서 표시 + UI 입력 모드로 전환
		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // 마우스 이동 제한 없음
		//InputMode.SetWidgetToFocus(InventoryWidgetInstance->TakeWidget());  // UI에 포커스
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
//	//카메라 쉐이크 효과 또는 화면 페이드 추가 예정
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
//	// 리셋
//	bIsWFCCountdownActive = false;
//	PendingRegenActor = nullptr;
//
//	// UI 숨기기 등 추가 가능
//}

void AMyDCharacter::PlayWFCRegenCameraShake()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && PC->PlayerCameraManager)
	{
		TSubclassOf<UCameraShakeBase> ShakeClass = LoadClass<UCameraShakeBase>(nullptr, TEXT("/Game/BP/BP_WFCShake.BP_WFCShake_C")); // 블루프린트 경로

		if (ShakeClass)
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass);
			UE_LOG(LogTemp, Log, TEXT("Camera Shake Played"));
		}
	}
}

void AMyDCharacter::TriggerDelayedWFC()
{
	ShowWFCFadeAndRegenSequence(); // 연출 함수 호출
}

void AMyDCharacter::ShowWFCFadeAndRegenSequence()
{

	// 유효성 검사 강화
	if (!IsValid(this) || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowWFCFadeAndRegenSequence - Invalid state"));
		return;
	}

	// 관전자가 아닌 경우만 처리
	APlayerController* PC = GetController<APlayerController>();
	if (!PC || PC->GetStateName() == NAME_Spectating)
	{
		return;
	}

	// WFCWarningWidgetInstance 안전 검사 및 생성
	if (!IsValid(WFCWarningWidgetInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("WFCWarningWidgetInstance is invalid, attempting to recreate"));

		// 위젯 클래스가 설정되어 있다면 다시 생성 시도
		if (WFCWarningWidgetClass)
		{
			WFCWarningWidgetInstance = CreateWidget<UUserWidget>(PC, WFCWarningWidgetClass);
			if (!IsValid(WFCWarningWidgetInstance))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create WFCWarningWidgetInstance"));
				return; // 위젯 생성 실패시 함수 종료
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WFCWarningWidgetClass is not set"));
			return; // 위젯 클래스가 없으면 함수 종료
		}
	}

	
	if (IsValid(WFCWarningWidgetInstance))
	{
		if (!WFCWarningWidgetInstance->IsInViewport())
		{
			WFCWarningWidgetInstance->AddToViewport(20);
		}
		WFCWarningWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		// 5초 후에 암전 및 재생성 진행
		GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFade, this, &AMyDCharacter::FadeAndRegenWFC, 2.0f, false);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WFCWarningWidgetInstance is still invalid after creation attempt"));
	}

	// 5초 후에 암전 및 재생성 진행
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

	// 재생성 전에 플레이어가 고정된 방타일 근처에 있는지 확인
	bool bPlayerInFixedRoom = IsPlayerInFixedRoomTile();

	if (bPlayerInFixedRoom)
	{
		// 플레이어 중력 비활성화
		ServerSetPlayerGravity(0.0f);
		TeleportToWFCRegen();
	}


	// 0.5초 후에 재생성 시작 (연출이 먼저 보이도록)
	GetWorldTimerManager().SetTimer(TimerHandle_StartWFC, [this]() 
		{
			// UI 상태 확인 후 실행
			
				ExecuteWFCNow();
			
		}, 0.5f, false);

	// 5초 후 위젯 숨기기 (람다 사용)
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

	// 서버에서 중력 설정
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->GravityScale = GravityScale;
	}

	// 모든 클라이언트에 전파
	MulticastSetPlayerGravity(GravityScale);
}

bool AMyDCharacter::ServerSetPlayerGravity_Validate(float GravityScale)
{
	return true;
}

void AMyDCharacter::MulticastSetPlayerGravity_Implementation(float GravityScale)
{
	// 자신의 캐릭터만 업데이트
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
			// 서버라면 바로 실행
			if (Regen->HasAuthority())
			{
				Regen->GenerateWFCAtLocation();
			}
			// 클라이언트라면 서버에게 요청
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
	// 필요하면 유효성 검사 추가
	return true;
}

void AMyDCharacter::ServerRequestWFCRegen_Implementation(AWFCRegenerator* RegenActor)
{
	// 서버에서만 실행
	if (!HasAuthority() || !IsValid(RegenActor)) return;

	// 실제 리제너레이터 로직 실행
	RegenActor->GenerateWFCAtLocation();
}

// RPC: 클라이언트->서버
bool AMyDCharacter::ServerPlayWFCRegenEffects_Validate()
{
	return true;
}

void AMyDCharacter::ServerPlayWFCRegenEffects_Implementation()
{
	// 서버에서만 Multicast 호출
    if (HasAuthority())
    {
        MulticastPlayWFCRegenEffects();
    }
}

// RPC: 서버→모든 클라이언트
void AMyDCharacter::MulticastPlayWFCRegenEffects_Implementation()
{
	// 모든 인스턴스에서 실행
	PlayWFCRegenCameraShake();
	TriggerDelayedWFC();
}

bool AMyDCharacter::ServerCastSpell_Validate(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation)
{
	// 유효성 검사
	return SpellSet.IsValidIndex(SpellIndex) && SpellSet[SpellIndex] != nullptr;
}

void AMyDCharacter::ServerCastSpell_Implementation(int32 SpellIndex, FVector TargetLocation, FRotator TargetRotation)
{
	if (!HasAuthority()) return;

	// 서버에서 다시 한번 검증
	if (!SpellSet.IsValidIndex(SpellIndex) || !SpellSet[SpellIndex]) return;
	if (!SpellSet[SpellIndex]->CanActivate(this)) return;

	// 리소스 소모
	Knowledge -= SpellSet[SpellIndex]->ManaCost;
	Stamina -= SpellSet[SpellIndex]->StaminaCost;
	UpdateHUD();

	// 마법 시전 애니메이션 재생 (모든 클라이언트)
	MulticastPlaySpellCastAnimation();

	// 마법별 실행
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

	// 서버에서 투사체 생성
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

		// 모든 클라이언트에서 파이어볼 이펙트 재생
		MulticastSpawnFireball(TargetLocation, TargetRotation, FireballSpell->Damage, FireballSpell->FireballSpeed);
	}
}

void AMyDCharacter::ExecuteHealSpell()
{
	UHealSpell* HealSpell = Cast<UHealSpell>(SpellSet[1]);
	if (!HealSpell) return;

	// 서버에서 실제 힐링 처리
	HealPlayer(HealSpell->HealAmount);

	// 모든 클라이언트에서 힐링 이펙트 재생
	MulticastPlayHealEffect(GetActorLocation());

	//UE_LOG(LogTemp, Log, TEXT("Server: Heal spell executed"));
}

void AMyDCharacter::ExecuteCurseSpell(FVector TargetLocation)
{
	UCurseSpell* CurseSpell = Cast<UCurseSpell>(SpellSet[2]);
	if (!CurseSpell) return;

	// 서버에서 라인 트레이스 실행
	FVector Start = FirstPersonCameraComponent->GetComponentLocation() +
		FirstPersonCameraComponent->GetForwardVector() * 30.0f;
	FVector End = TargetLocation;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);  // 자신은 제외

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
			// 서버에서 디버프 적용
			IHitInterface::Execute_ApplyDebuff(HitActor, EDebuffType::Slow,
				CurseSpell->SlowAmount, CurseSpell->Duration);

			UE_LOG(LogTemp, Warning, TEXT("Server: Applied curse debuff to %s"), *HitActor->GetName());
		}
	}

	// 모든 클라이언트에서 저주 이펙트 재생 (히트한 위치로)
	FVector EffectLocation = bHit ? Hit.Location : End;
	MulticastPlayCurseEffect(Start, EffectLocation, HitActor);
}

// =========================
// Multicast RPC 구현부
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
	// 로컬 플레이어가 아닌 경우에만 이펙트 재생 (중복 방지)
	if (IsLocallyControlled()) return;

	// 파이어볼 이펙트만 재생 (실제 투사체는 서버에서 이미 생성됨)
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

	// OrbitEffectActor 생성 (원래 로직 복원)
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
		// 원래 HealSpell.cpp의 설정값들 사용
		OrbitActor->InitOrbit(
			this,                                    // Center Actor
			HealSpell->HealEffect,                  // Effect
			100.f,                                  // Radius
			5.f,                                    // Duration (수정: 원래는 2.f였음)
			1080.f,                                 // Speed
			FLinearColor(0.4f, 1.f, 0.2f),        // Color (녹색)
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

	// 디버그 라인 그리기
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Purple, false, 3.0f, 0, 3.0f);

	// 타겟이 있으면 타겟 위치에, 없으면 EndLocation에 이펙트 생성
	FVector EffectLocation = TargetActor ? TargetActor->GetActorLocation() : EndLocation;

	// OrbitEffectActor 생성 (원래 로직 복원)
	if (TargetActor) // 타겟이 있을 때만 OrbitEffect 생성
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
			// 원래 CurseSpell.cpp의 설정값들 사용
			OrbitActor->InitOrbit(
				TargetActor,                        // Center Actor
				CurseSpell->CurseEffect,           // Effect
				100.f,                             // Radius
				10.f,                               // Duration (원래 설정값)
				1080.f,                            // Speed
				FLinearColor(0.8f, 0.5f, 1.0f),  // Color (보라색)
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
		// 타겟이 없으면 단순히 위치에 이펙트만 스폰
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
	// 검색 반경 설정
	float SearchRadius = 690.0f;

	// 디버그 시각화: 플레이어 주변에 구체 그리기
	DrawDebugSphere(
		GetWorld(),
		GetActorLocation(),
		SearchRadius,
		32,                    // 세그먼트 (부드러운 정도)
		FColor::Green,         // 색상
		true,                 // 지속 여부 (true면 무한)
		5.0f,                  // 지속 시간 (초)
		0,                     // 깊이 우선 순위
		1.0f                   // 두께
	);

	// 플레이어 주변에서 WFCRegen 태그가 있는 재생성 오브젝트 검색
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

	// 로컬 플레이어인지 확인
	if (IsLocallyControlled())
	{




		UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
		if (GameInstance)
		{
			// 1. 장비창의 모든 아이템을 인벤토리/창고로 이동
			if (EquipmentWidgetInstance && InventoryComponent)
			{
				TArray<FItemData> EquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();

				for (int32 i = 0; i < EquipmentItems.Num(); ++i)
				{
					if (EquipmentItems[i].ItemClass)
					{
						bool bAddedToInventory = false;

						// 인벤토리에 빈 슬롯 찾기
						for (int32 j = 0; j < InventoryComponent->InventoryItemsStruct.Num(); ++j)
						{
							if (!InventoryComponent->InventoryItemsStruct[j].ItemClass)
							{
								InventoryComponent->InventoryItemsStruct[j] = EquipmentItems[i];
								bAddedToInventory = true;
								UE_LOG(LogTemp, Warning, TEXT("Moved %s from equipment slot %d to inventory slot %d"),
									*EquipmentItems[i].ItemName, i, j);

								EquipmentWidgetInstance->ClearSlot(i);
								break;
							}
						}

						// 인벤토리가 꽉 찼다면 창고로
						if (!bAddedToInventory)
						{
							if (GameInstance->SavedStorageItems.Num() == 0)
							{
								GameInstance->SavedStorageItems.SetNum(50);
							}

							for (int32 k = 0; k < GameInstance->SavedStorageItems.Num(); ++k)
							{
								if (!GameInstance->SavedStorageItems[k].ItemClass)
								{
									GameInstance->SavedStorageItems[k] = EquipmentItems[i];
									UE_LOG(LogTemp, Warning, TEXT("Moved %s from equipment slot %d to storage slot %d"),
										*EquipmentItems[i].ItemName, i, k);

									EquipmentWidgetInstance->ClearSlot(i);
									break;
								}
							}
						}
					}
				}
			}

			// 2. 현재 인벤토리 저장
			if (InventoryComponent)
			{
				GameInstance->SavedInventoryItems = InventoryComponent->InventoryItemsStruct;
			}

			// 3. 수정된 장비창 저장
			if (EquipmentWidgetInstance)
			{
				GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
			}

			//// 4. 탈출 플래그 설정
			GameInstance->bIsReturningFromGame = true;
			if (!GameInstance->CurrentCharacterData.PlayerName.IsEmpty())
			{
				GameInstance->bHasValidCharacterData = true;
				UE_LOG(LogTemp, Warning,
					TEXT("[DEBUG ExecuteEscape] bIsReturningFromGame=%d, bHasValidCharacterData=%d"),
					GameInstance->bIsReturningFromGame, GameInstance->bHasValidCharacterData);
			}
		}
	}

	// 5. Seamless Travel 사용
	//APlayerController* PC = Cast<APlayerController>(GetController());
	//if (PC && IsLocallyControlled())
	//{
	//	// Seamless Travel - TRAVEL_Relative 사용
	//	PC->ClientTravel(TEXT("/Game/Maps/LobbyMap"), ETravelType::TRAVEL_Relative);
	//}

	// 서버에 탈출 요청
	ServerHandleEscape();

	bIsEscapeCountdownActive = false;
	PendingEscapeActor = nullptr;
}

void AMyDCharacter::ServerHandleEscape_Implementation()
{
	if (!HasAuthority()) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	UE_LOG(LogTemp, Warning, TEXT("ServerHandleEscape - Player %s escaped"), *PC->GetPlayerState<APlayerState>()->GetPlayerName());

	// 데이터 저장
	//SaveDataToGameInstance();
	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (GameInstance)
	{
		GameInstance->bIsReturningFromGame = true;
		if (!GameInstance->CurrentCharacterData.PlayerName.IsEmpty())
		{
			GameInstance->bHasValidCharacterData = true;
		}
	}

	// 관전자 모드로 전환
	PC->UnPossess();
	PC->ChangeState(NAME_Spectating);
	PC->ClientGotoState(NAME_Spectating);

	// 관전 카메라 속도 설정
	FTimerHandle SpectatorSpeedTimer;
	GetWorldTimerManager().SetTimer(SpectatorSpeedTimer, [PC]()
		{
			if (PC && PC->GetSpectatorPawn())
			{
				ASpectatorPawn* SpectatorPawn = PC->GetSpectatorPawn();

				
				if (UFloatingPawnMovement* FloatingMovement = Cast<UFloatingPawnMovement>(SpectatorPawn->GetMovementComponent()))
				{
					FloatingMovement->MaxSpeed = 16000.0f;  
					FloatingMovement->Acceleration = 10000.0f;
					FloatingMovement->Deceleration = 10000.0f;

					UE_LOG(LogTemp, Warning, TEXT("Spectator camera speed increased!"));
				}
			}
		}, 0.1f, false);

	// 클라이언트에게 관전자 UI 표시
	ClientEnterSpectatorMode();

	
	//// 캐릭터 제거 (일정 시간 후)
	 if (HasAuthority())
	 { 
		FTimerHandle DestroyTimer;
		GetWorldTimerManager().SetTimer(DestroyTimer, [this]()
			{
				if (IsValid(this))
				{
					Destroy();
				}
			}, 3.0f, false);
	 }
	// GameMode에서 체크하도록 변경
	ADynamicDungeonModeBase* GameMode = Cast<ADynamicDungeonModeBase>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		FTimerHandle CheckTimer;
		GetWorld()->GetTimerManager().SetTimer(CheckTimer, [GameMode]()
			{
				if (IsValid(GameMode))
				{
					GameMode->CheckAllPlayersStatus();
				}
			}, 1.0f, false);
	}
}

void AMyDCharacter::ServerCheckAllPlayersFinished_Implementation()
{
	if (!HasAuthority()) return;

	int32 ActivePlayers = 0;
	int32 SpectatingPlayers = 0;

	// 모든 플레이어 상태 확인
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC)
		{
			if (PC->GetStateName() == NAME_Spectating)
			{
				SpectatingPlayers++;
			}
			else if (PC->GetPawn() && PC->GetPawn()->IsA<AMyDCharacter>())
			{
				ActivePlayers++;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Player Status - Active: %d, Spectating: %d"), ActivePlayers, SpectatingPlayers);

	// 모든 플레이어가 관전자가 되었거나 활성 플레이어가 없으면
	if (ActivePlayers == 0 && SpectatingPlayers > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("All players finished! Returning everyone to lobby..."));

		// 모든 플레이어를 로비로 이동
		GetWorld()->ServerTravel("/Game/Maps/LobbyMap?listen");
	}
}

void AMyDCharacter::UpdateEscapeProgressBar()
{
	CurrentEscapeTime += 0.05f; // 타이머 주기만큼 추가

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
		// 100% 채워지면 Progress 업데이트 타이머는 멈추기
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

	// 장비 장착
	for (const FItemData& EquipItem : Data.EquippedItems)
	{
		if (EquipItem.ItemClass && EquipItem.ItemType == EItemType::Weapon)
		{
			EquipWeaponFromClass(EquipItem.ItemClass, static_cast<EWeaponGrade>(EquipItem.Grade));

			break;
		}
	}

	// 인벤토리 복원
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

	// 장비창 UI에도 반영
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

		// 마법 시전 몽타주 재생
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
	TryCastSpellMultiplayer(0);      // 0 - 파이어볼, 1 - 힐, 2 - 저주.
}

void AMyDCharacter::CastSpell2()
{
	TryCastSpellMultiplayer(2);
}

void AMyDCharacter::TryCastSpellMultiplayer(int32 SpellIndex)
{
	if (PlayerClass != EPlayerClass::Mage) return;

	// 유효성 검사
	if (!SpellSet.IsValidIndex(SpellIndex) || !SpellSet[SpellIndex]) return;

	// 간단한 스팸 방지 (0.1초)
	

	// 리소스 확인
	if (!SpellSet[SpellIndex]->CanActivate(this)) return;

	
	

	
	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = GetControlRotation();

	if (SpellIndex == 0) // Fireball
	{
		TargetLocation = GetActorLocation() + GetActorForwardVector() * 200.0f + FVector(0, 0, 50.0f);
	}
	else if (SpellIndex == 1) // Heal
	{
		TargetLocation = GetActorLocation(); // Heal은 자기 위치
		UE_LOG(LogTemp, Warning, TEXT("Client: Requesting Heal spell at %s"), *TargetLocation.ToString());
	}
	else if (SpellIndex == 2) // Curse
	{
		FVector Start = FirstPersonCameraComponent->GetComponentLocation() +
			FirstPersonCameraComponent->GetForwardVector() * 30.0f;
		FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 1000.0f;
		TargetLocation = End;
	}

	// 서버에 마법 시전 요청
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
//		// 현재 카메라 위치 저장 (회전 포함)
//		DefaultCameraLocation = FirstPersonCameraComponent->GetComponentLocation();
//		DefaultCameraRotation = FirstPersonCameraComponent->GetComponentRotation();
//
//		// 맵 전체 보기 시점 설정
//		FVector OverheadLocation = FVector(15000.0f, 16000.0f, 50500.0f);
//		FRotator OverheadRotation = FRotator(-90.0f, 0.0f, 0.0f); // 진짜로 수직으로 내려다봄
//
//		FirstPersonCameraComponent->SetWorldLocation(OverheadLocation);
//		FirstPersonCameraComponent->SetWorldRotation(OverheadRotation);
//
//		if (PC)
//		{
//			PC->SetControlRotation(OverheadRotation); // ← 중요: 컨트롤러 회전도 고정
//			PC->SetIgnoreLookInput(true);              // 마우스 회전 막기
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
//		// 원래 시점으로 복구
//		FirstPersonCameraComponent->SetWorldLocation(DefaultCameraLocation);
//		FirstPersonCameraComponent->SetWorldRotation(DefaultCameraRotation);
//
//		if (PC)
//		{
//			PC->SetControlRotation(DefaultCameraRotation); // ← 중요: 복구할 때도 같이 설정
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
	//// 인벤토리 리셋
	//if (InventoryComponent)
	//{
	//	InventoryComponent->ClearInventory();
	//}

	// 레벨 이동 (예: LobbyMap이라는 이름의 레벨로 전환)
	//UGameplayStatics::OpenLevel(this, FName("LobbyMap"));

	//if (!IsLocallyControlled()) return;

	////// GameInstance에 플래그 설정
	//UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	//if (GameInstance)
	//{
	//	GameInstance->bIsReturningFromGame = true;
	//	if (!GameInstance->CurrentCharacterData.PlayerName.IsEmpty())
	//	{
	//		GameInstance->bHasValidCharacterData = true;
	//	}
	//}

	////// ClientTravel로 개별 이동
	////APlayerController* PC = Cast<APlayerController>(GetController());
	////if (PC && IsLocallyControlled())
	////{
	////	// Seamless Travel - TRAVEL_Relative 사용
	////	PC->ClientTravel(TEXT("/Game/Maps/LobbyMap"), ETravelType::TRAVEL_Relative);
	////}

	//if (HasAuthority())
	//{
	//	ServerHandleDeath();
	//}

	// 캐릭터 제거 (일정 시간 후)
	/*if (HasAuthority())
	{
		FTimerHandle DestroyTimer;
		GetWorldTimerManager().SetTimer(DestroyTimer, [this]()
			{
				if (IsValid(this))
				{
					Destroy();
				}
			}, 3.0f, false);
	}*/
	ExecuteEscape();

	//// 필요시 로그
	//UE_LOG(LogTemp, Warning, TEXT("Player died. Returning to lobby..."));
}

void AMyDCharacter::ServerHostTravel_Implementation()
{
	// 서버에서만 실행
	if (!HasAuthority()) return;

	// 현재 플레이어 컨트롤러 가져오기
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 게임모드에서 이 플레이어만 제거
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
	if (GameMode)
	{
		// 플레이어를 관전자로 전환
		PC->ChangeState(NAME_Spectating);
		PC->ClientGotoState(NAME_Spectating);

		// 캐릭터 제거
		if (PC->GetPawn())
		{
			PC->GetPawn()->Destroy();
		}
	}

	// 서버 플레이어만 로비로 이동
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ServerTravel,
		[PC]()
		{
			if (PC && PC->IsLocalController())
			{
				// 서버 컨트롤러만 이동
				PC->ClientTravel(TEXT("LobbyMap"), ETravelType::TRAVEL_Absolute);
			}
		},
		0.5f,
		false
	);
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

	// 가장 가까운 WFCRegen 찾기
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
		FVector TargetLocation = Closest->GetActorLocation() + FVector(0, 0, 100); // 약간 위로 띄워주기
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

	// 클라이언트에서 서버 RPC 호출
	ServerTeleportToWFCRegen();
}

// 클라이언트가 호출하면 서버 RPC로 위임
void AMyDCharacter::TeleportToEscapeObject()
{
	if (!IsLocallyControlled()) return;
	ServerTeleportToEscapeObject();
}

// 서버에서 모든 플레이어의 위치를 변경 → 이동 정보가 네트워크로 자동 전파됩니다
void AMyDCharacter::ServerTeleportToEscapeObject_Implementation()
 {
	UWorld * World = GetWorld();
	if (!World) return;
	
		    // 태그 "Escape"가 붙은 액터 찾기
		TArray<AActor*> EscapeActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Escape"), EscapeActors);
	if (EscapeActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No escape objects found!"));
		return;
	}
	
		    // 가장 가까운 오브젝트 선택
		AActor * Closest = nullptr;
	float MinDistSq = TNumericLimits<float>::Max();
	const FVector MyLoc = GetActorLocation();
	for (AActor* A : EscapeActors)
	{
		const float DSq = FVector::DistSquared(MyLoc, A->GetActorLocation());
		if (DSq < MinDistSq)
		{
			MinDistSq = DSq;
			Closest = A;
		}
	}
	
		if (Closest)
		{
			const FVector Dest = Closest->GetActorLocation() +  FVector(0, 0, 100);
		        // 순간이동 (물리 텔레포트)
			SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Log, TEXT("Teleported to escape object at %s"), *Dest.ToString());
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
		DefaultItem->Use(this);  // AScrollItem이면 스킬 발동됨

		// 공통 처리
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

		// 공통 처리
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

		// 이미 슬로우 상태라면 기존 타이머 클리어
		if (bIsSlowed)
		{
			GetWorldTimerManager().ClearTimer(DebuffRecoveryTimerHandle);
		}

		// 원래 속도 저장 (처음 적용될 때만)
		if (!bIsSlowed)
		{
			OriginalWalkSpeed = Movement->MaxWalkSpeed;
		}

		bIsSlowed = true;

		// 속도 배율 적용 (기존 속도 저장하지 않고 배율만 사용)
		SpeedMultiplier = FMath::Clamp(1.0f - Magnitude, 0.1f, 1.0f);

		// 현재 속도에 배율 적용
		ApplySpeedMultiplier();

		//// 속도 감소 적용
		//const float SlowFactor = FMath::Clamp(1.0f - Magnitude, 0.1f, 1.0f);
		//Movement->MaxWalkSpeed = OriginalWalkSpeed * SlowFactor;

		UE_LOG(LogTemp, Warning, TEXT("Applied slow debuff: %f -> %f for %f seconds"),
			OriginalWalkSpeed, Movement->MaxWalkSpeed, Duration);

		// 디버프 해제용 타이머 설정
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
	SpeedMultiplier = 1.0f;  // 정상 배율로 복구

	// 현재 속도에 정상 배율 적용
	ApplySpeedMultiplier();

	UE_LOG(LogTemp, Warning, TEXT("Removed slow debuff: SpeedMultiplier restored to 1.0"));
	GetWorldTimerManager().ClearTimer(DebuffRecoveryTimerHandle);
}

void AMyDCharacter::ApplySpeedMultiplier()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement) return;

	float CurrentBaseSpeed = Movement->MaxWalkSpeed;

	// 현재 속도가 SprintSpeed 범위인지 WalkSpeed 범위인지 판단
	float NormalizedSpeed = CurrentBaseSpeed / (WalkSpeed * Agility);

	if (NormalizedSpeed > 1.5f)  // 달리기 상태로 추정
	{
		// 달리기 속도에 배율 적용
		Movement->MaxWalkSpeed = (SprintSpeed * Agility) * SpeedMultiplier;
	}
	else  // 걷기 상태
	{
		// 걷기 속도에 배율 적용
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
		FVector TargetLocation = NearestEnemy->GetActorLocation() + FVector(0, 0, 200); // 살짝 위
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

void AMyDCharacter::UpdateGoldUI()
{
	if (GoldWidgetInstance)
	{
		GoldWidgetInstance->UpdateGoldAmount(Gold);
	}
}

void AMyDCharacter::RestoreDataFromLobby()
{
	if (!IsLocallyControlled()) return; // 로컬 플레이어만

	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("GameInstance not found for data restoration"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Restoring data from lobby..."));

	// 1. 골드 복원
	if (GameInstance->CurrentCharacterData.Gold > 0)
	{
		Gold = GameInstance->CurrentCharacterData.Gold;
		UE_LOG(LogTemp, Warning, TEXT("Restored gold: %d"), Gold);

		// 골드 UI 업데이트
		if (GoldWidgetInstance)
		{
			GoldWidgetInstance->UpdateGoldAmount(Gold);
		}
	}
	else if (GameInstance->LobbyGold > 0)
	{
		Gold = GameInstance->LobbyGold;
		UE_LOG(LogTemp, Warning, TEXT("Restored gold from LobbyGold: %d"), Gold);

		if (GoldWidgetInstance)
		{
			GoldWidgetInstance->UpdateGoldAmount(Gold);
		}
	}

	// 2. 인벤토리 복원
	if (InventoryComponent && GameInstance->SavedInventoryItems.Num() > 0)
	{
		// 인벤토리 크기 확보
		InventoryComponent->InventoryItemsStruct.SetNum(InventoryComponent->Capacity);

		// 저장된 아이템들 복원
		for (int32 i = 0; i < GameInstance->SavedInventoryItems.Num() && i < InventoryComponent->Capacity; ++i)
		{
			InventoryComponent->InventoryItemsStruct[i] = GameInstance->SavedInventoryItems[i];

			if (GameInstance->SavedInventoryItems[i].ItemClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("Restored item to slot %d: %s"),
					i, *GameInstance->SavedInventoryItems[i].ItemName);
			}
		}

		// 인벤토리 UI 새로고침
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->RefreshInventoryStruct();
		}

		UE_LOG(LogTemp, Warning, TEXT("Restored %d inventory items"), GameInstance->SavedInventoryItems.Num());
	}

	// 3. 장비 복원
	if (EquipmentWidgetInstance && GameInstance->SavedEquipmentItems.Num() > 0)
	{
		EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
		UE_LOG(LogTemp, Warning, TEXT("Restored equipment data"));

		//// 각 장비 아이템을 실제 캐릭터에 장착
		//for (int32 i = 0; i < GameInstance->SavedEquipmentItems.Num(); ++i)
		//{
		//	const FItemData& EquipData = GameInstance->SavedEquipmentItems[i];

		//	if (EquipData.ItemClass)
		//	{
		//		if (EquipData.ItemType == EItemType::Weapon && i == EQUIP_SLOT_WEAPON)
		//		{
		//			// 무기 장착
		//			EquipWeaponFromClass(EquipData.ItemClass, static_cast<EWeaponGrade>(EquipData.Grade));
		//			UE_LOG(LogTemp, Warning, TEXT("Restored weapon: %s"), *EquipData.ItemName);
		//		}
		//		else if (EquipData.ItemType == EItemType::Armor)
		//		{
		//			// 방어구 장착
		//			EquipArmorFromClass(i, EquipData.ItemClass, EquipData.Grade);
		//			UE_LOG(LogTemp, Warning, TEXT("Restored armor to slot %d: %s"), i, *EquipData.ItemName);
		//		}
		//	}
		//}
		EquipmentWidgetInstance->RefreshEquipmentSlots();
	}

	// 4. 캐릭터 스탯 복원 (기존 ApplyCharacterData 함수 활용)
	if (GameInstance->CurrentCharacterData.PlayerName != TEXT("DefaultName"))
	{
		ApplyCharacterData(GameInstance->CurrentCharacterData);
		UE_LOG(LogTemp, Warning, TEXT("Applied character data: %s"),
			*GameInstance->CurrentCharacterData.PlayerName);
	}

	UE_LOG(LogTemp, Warning, TEXT("Data restoration completed!"));
}

