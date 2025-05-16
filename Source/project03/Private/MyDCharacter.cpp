// Fill out your copyright notice in the Description page of Project Settings.


#include "MyDCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DynamicDungeonInstance.h"
#include "Animation/AnimInstance.h"  // 애니메이션 관련 클래스 추가
#include "Components/BoxComponent.h"  // 콜리전 박스 추가
#include "Weapon.h"
#include "GreatWeapon.h"
#include "Kismet/GameplayStatics.h"
#include "UCharacterHUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Item.h"
#include "InventoryWidget.h"
#include "PlayerCharacterData.h"
#include "TreasureChest.h"
#include "EnemyCharacter.h"
#include "Potion.h"
#include "FireballSpell.h"
#include "Camera/CameraShakeBase.h"
#include "WFCRegenerator.h"
#include "Components/ProgressBar.h"
#include "Animation/AnimBlueprintGeneratedClass.h"

// 기본 생성자
AMyDCharacter::AMyDCharacter()
{
	// 매 프레임 Tick을 활성화
	PrimaryActorTick.bCanEverTick = true;



	AutoPossessPlayer = EAutoReceiveInput::Player0;

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

	//스프링암(SprintArm) 생성 (메쉬와 카메라를 독립적으로 배치하기 위해)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f; // 1인칭이므로 거리 없음
	SpringArm->bUsePawnControlRotation = false; // 카메라 회전에 영향을 주지 않음

	// 캐릭터 메쉬 생성
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(SpringArm);

	// 충돌 설정: 래그돌 전환을 위한 기본 설정
	CharacterMesh->SetCollisionProfileName(TEXT("CharacterMesh")); // 나중에 Ragdoll로 바꿀 수 있도록
	
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	

	// 메쉬 로드 (ConstructorHelpers 사용)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny'"));
	if (MeshAsset.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MeshAsset.Object);
	}


	// 애니메이션 블루프린트 로드
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
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
	FirstPersonCameraComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 50.0f));  // 머리 위치 , 원래값 20.0f, 0.0f, 50.0f, 테스트값 -100.0f, 0.0f, 80.0f
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

	// 위젯 생성은 항상!
	if (InventoryWidgetClass)
	{
		InventoryWidgetInstance = CreateWidget<UInventoryWidget>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidgetInstance)
		{
			InventoryWidgetInstance->InventoryRef = InventoryComponent;
			InventoryWidgetInstance->RefreshInventoryStruct();

			//if (InventoryComponent)
			//{
			//	InventoryComponent->TryAddItemByClass(AGreatWeapon::StaticClass());

			//	InventoryWidgetInstance->RefreshInventoryStruct(); // 다시 갱신
			//}

		}
	}

	if (EquipmentWidgetClass)
	{
		EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);

		if (EquipmentWidgetClass)
		{
			EquipmentWidgetInstance = CreateWidget<UEquipmentWidget>(GetWorld(), EquipmentWidgetClass);

			EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;
		}
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

	UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
	if (GameInstance)
	{
		ApplyCharacterData(GameInstance->CurrentCharacterData);

		// 인벤토리 복원
		if (InventoryComponent)
		{
			InventoryComponent->InventoryItemsStruct = GameInstance->SavedInventoryItems;
		}

		// 장비창 복원
		if (EquipmentWidgetInstance)
		{
			EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
		}

	}

	if (PlayerClass == EPlayerClass::Mage)
	{
		UFireballSpell* Fireball = NewObject<UFireballSpell>(this);
		SpellSet.Add(Fireball);
	}

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

}

// 매 프레임 호출
void AMyDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 구르는 동안 카메라 부드럽게 이동
	//moothCameraFollow();

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
}

// 달리기 중지
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
	GetWorldTimerManager().ClearTimer(TimerHandle_SprintDrain);
	ManageStaminaRegen();  //달리기 끝나고 스테미나 회복 시키기
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

	PlayerInputComponent->BindAction("ToggleMapView", IE_Pressed, this, &AMyDCharacter::ToggleMapView);

	//f키
	PlayerInputComponent->BindAction("ToggleTorch", IE_Pressed, this, &AMyDCharacter::ToggleTorch);

	//m
	PlayerInputComponent->BindAction("TeleportToRegen", IE_Pressed, this, &AMyDCharacter::TeleportToWFCRegen);

	//n
	PlayerInputComponent->BindAction("TeleportToEscape", IE_Pressed, this, &AMyDCharacter::TeleportToEscapeObject);

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
			


		}

		if (OverlappedActor && OverlappedActor->ActorHasTag("WFCRegen") && !bIsWFCCountdownActive)
		{
			PendingRegenActor = OverlappedActor;
			bIsWFCCountdownActive = true;

			//경고 UI 표시 (UMG로 표시하는 방식에 따라 구현 필요)
			UE_LOG(LogTemp, Warning, TEXT("55555se"));

			PlayWFCRegenCameraShake();

			TriggerDelayedWFC();

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

void AMyDCharacter::EquipWeaponFromClass(TSubclassOf<AItem> WeaponClass)
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


void AMyDCharacter::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);
		HUDWidget->UpdateMana(Knowledge, MaxKnowledge);
		HUDWidget->UpdateStamina(Stamina, MaxStamina);
		
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

void AMyDCharacter::PlayAttackAnimation()
{
	if (bIsAttacking)  // 공격 중이면 입력 무시
	{
		UE_LOG(LogTemp, Log, TEXT("Already attacking! Ignoring input."));
		return;
	}

	if (bIsAttacking || bIsRolling)  // 구르기 중이면 공격 입력 무시
	{
		UE_LOG(LogTemp, Log, TEXT("Already attacking or rolling! Ignoring input."));
		return;
	}

	if (Stamina <= 0)  // 스태미나가 0이면 공격 불가
	{
		ReduceStamina(0.0f);
		return;
	}

	// 공격 상태 설정
	bIsAttacking = true;

	ReduceStamina(AttackStaminaCost);
	

	// 사용할 몽타주 결정 (무기 장착 여부에 따라 다르게 설정)
	UAnimMontage* SelectedMontage = nullptr;

	if (EquippedWeapon)
	{
		switch (EquippedWeapon->WeaponType)
		{
		case EWeaponType::GreatWeapon:
			SelectedMontage = GreatWeaponMontage; // 이건 변수로 미리 만들어둬야 함
			break;
		case EWeaponType::Dagger:
			SelectedMontage = DaggerWeaponMontage;
			break;
		case EWeaponType::Staff:
			//SelectedMontage = StaffMontage;
			break;
		default:
			SelectedMontage = WeaponAttackMontage; // 기본값
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

	// **현재 콤보 수에 따라 재생할 섹션 선택**
	FName SelectedSection = (AttackComboIndex == 0) ? FName("Combo1") : FName("Combo2");

	// 특정 슬롯에서만 실행 (UpperBody 슬롯에서 재생)
	FName UpperBodySlot = FName("UpperBody");

	// UpperBody 슬롯에서 실행되도록 설정
	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(SelectedMontage); 
	if (MontageInstance) 
	{
		MontageInstance->Montage->SlotAnimTracks[0].SlotName = UpperBodySlot; 
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->StartTrace(); 
	}

	// 애니메이션 실행 (선택된 섹션 처음부터 재생)
	AnimInstance->Montage_Play(SelectedMontage, 1.0f);
	AnimInstance->Montage_JumpToSection(SelectedSection, SelectedMontage);
	
	UE_LOG(LogTemp, Log, TEXT("Playing Attack Montage Section: %s"), *SelectedSection.ToString());

	// **현재 섹션의 길이 가져오기**
	float SectionDuration = SelectedMontage->GetSectionLength(SelectedMontage->GetSectionIndex(SelectedSection));

	// **타이머 설정: 정확한 섹션 종료 후 공격 상태 초기화**
	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, SectionDuration, false);

	// **다음 입력 시 다른 섹션 실행되도록 설정 (콤보 증가)**
	AttackComboIndex = (AttackComboIndex == 0) ? 1 : 0;
}


void AMyDCharacter::ResetAttack()
{
	// 공격 상태 초기화
	bIsAttacking = false;
	UE_LOG(LogTemp, Log, TEXT("Attack Ended, Resetting Attack State"));

	ResetHitActors();
}


void AMyDCharacter::PlayRollAnimation()
{
	if (bIsRolling || !RollMontage) return;

	if (Stamina <= 0)  // 스태미나가 0이면 공격 불가
	{
		ReduceStamina(0.0f);
		return;
	}

	bIsRolling = true;

	// 현재 이동 방향 계산
	FVector RollDirection;
	FName SelectedSection = "RollF"; // 기본값 (앞구르기)

	ReduceStamina(RollStaminaCost);

	if (FMath::Abs(MoveForwardValue) > 0.1f || FMath::Abs(MoveRightValue) > 0.1f)
	{
		// 방향키 입력이 있을 경우: 해당 방향으로 구르기
		FRotator ControlRotation = GetControlRotation();
		FVector ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
		FVector RightVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y);

		RollDirection = ForwardVector * MoveForwardValue + RightVector * MoveRightValue;
		RollDirection.Normalize();

		// **입력 방향에 따라 적절한 섹션 선택**
		if (MoveForwardValue > 0.1f)
		{
			SelectedSection = "RollF";  // 앞구르기
		}
		else if (MoveForwardValue < -0.1f)
		{
			SelectedSection = "RollB";  // 뒤구르기
		}
		else if (MoveRightValue > 0.1f)
		{
			SelectedSection = "RollR";  // 오른쪽 구르기
		}
		else if (MoveRightValue < -0.1f)
		{
			SelectedSection = "RollL";  // 왼쪽 구르기
		}
	}
	else
	{
		// 방향키 입력이 없을 경우: 기본적으로 앞구르기
		RollDirection = GetActorForwardVector();
	}

	// RollDirection을 멤버 변수로 저장 (이동 함수에서 사용)
	StoredRollDirection = RollDirection;

	// **구르기 방향으로 캐릭터 회전 (카메라는 고정)**
	if (RollDirection.SizeSquared() > 0)
	{
		FRotator NewRotation = RollDirection.Rotation();
		SetActorRotation(NewRotation);
	}

	// **애니메이션 실행 (선택된 섹션으로 점프)**
	UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(RollMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(SelectedSection, RollMontage);
		UE_LOG(LogTemp, Log, TEXT("Playing Roll Montage Section: %s"), *SelectedSection.ToString());
	}

	// **구르기 이동 적용**
	ApplyRollMovement(RollDirection);

	// **구르기 후 원래 상태 복구**
	GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetRoll, RollDuration, false);
}

void AMyDCharacter::ResetRoll()
{
	bIsRolling = false;
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

	Health -= Damage;
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);

	UpdateHUD();

	if (Health <= 0)
	{

		// 랙돌 활성화
		DoRagDoll();

		// 컨트롤러 비활성화 (입력 차단)
		AController* PlayerController = GetController();
		if (PlayerController)
		{
			PlayerController->UnPossess();
		}

		// 애니메이션 중지
		GetMesh()->bPauseAnims = true;
		GetMesh()->bNoSkeletonUpdate = true;

		// 이동 멈추기
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

	// 5초 후에 암전 및 재생성 진행
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFade, this, &AMyDCharacter::FadeAndRegenWFC, 5.0f, false);
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

	GetWorldTimerManager().SetTimer(TimerHandle_DelayedWFCFinal, this, &AMyDCharacter::ExecuteWFCNow, 0.5f, false);
}

void AMyDCharacter::ExecuteWFCNow()
{
	if (PendingRegenActor)
	{
		if (AWFCRegenerator* Regen = Cast<AWFCRegenerator>(PendingRegenActor))
		{
			Regen->GenerateWFCAtLocation();
		}
	}

	if (WFCDoneWidgetInstance)
	{
		WFCDoneWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	bIsWFCCountdownActive = false;
	PendingRegenActor = nullptr;

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

	UGameplayStatics::OpenLevel(this, FName("LobbyMap")); // 로비로 이동

	bIsEscapeCountdownActive = false;
	PendingEscapeActor = nullptr;
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
			EquipWeaponFromClass(EquipItem.ItemClass);
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
		SpellSet[Index]->ActivateSpell(this);
	}
}

void AMyDCharacter::CastSpell1()
{
	TryCastSpell(0);
}

void AMyDCharacter::ToggleMapView()
{

	if (!FirstPersonCameraComponent) return;

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (!bIsInOverheadView)
	{
		// 현재 카메라 위치 저장 (회전 포함)
		DefaultCameraLocation = FirstPersonCameraComponent->GetComponentLocation();
		DefaultCameraRotation = FirstPersonCameraComponent->GetComponentRotation();

		// 맵 전체 보기 시점 설정
		FVector OverheadLocation = FVector(15000.0f, 16000.0f, 50500.0f);
		FRotator OverheadRotation = FRotator(-90.0f, 0.0f, 0.0f); // 진짜로 수직으로 내려다봄

		FirstPersonCameraComponent->SetWorldLocation(OverheadLocation);
		FirstPersonCameraComponent->SetWorldRotation(OverheadRotation);

		if (PC)
		{
			PC->SetControlRotation(OverheadRotation); // ← 중요: 컨트롤러 회전도 고정
			PC->SetIgnoreLookInput(true);              // 마우스 회전 막기
		}

		

		if (CachedDirectionalLight)
		{
			CachedDirectionalLight->SetActorHiddenInGame(false);
			CachedDirectionalLight->SetEnabled(true);
		}

		bUseControllerRotationYaw = false;
		bIsInOverheadView = true;
	}
	else
	{
		// 원래 시점으로 복구
		FirstPersonCameraComponent->SetWorldLocation(DefaultCameraLocation);
		FirstPersonCameraComponent->SetWorldRotation(DefaultCameraRotation);

		if (PC)
		{
			PC->SetControlRotation(DefaultCameraRotation); // ← 중요: 복구할 때도 같이 설정
			PC->SetIgnoreLookInput(false);
		}

		if (CachedDirectionalLight)
		{
			CachedDirectionalLight->SetActorHiddenInGame(true);
			CachedDirectionalLight->SetEnabled(false);
		}

		bUseControllerRotationYaw = true;
		bIsInOverheadView = false;
	}
}

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
	//	InventoryComponent->ClearInventory(); // 너가 정의한 초기화 함수로 교체
	//}

	// 레벨 이동 (예: LobbyMap이라는 이름의 레벨로 전환)
	UGameplayStatics::OpenLevel(this, FName("LobbyMap"));

	// 필요시 로그
	UE_LOG(LogTemp, Warning, TEXT("Player died. Returning to lobby..."));
}

void AMyDCharacter::TeleportToWFCRegen()
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

	// 가장 가까운 Escape 오브젝트 찾기
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
		SetActorLocation(ClosestEscape->GetActorLocation() + FVector(0, 0, 100)); // 살짝 위로 띄워서 이동
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


	if (Item.ItemClass && Item.ItemType != EItemType::Potion)
	{
		Item.ItemType = EItemType::Potion;
		UE_LOG(LogTemp, Warning, TEXT("ItemType hhhhh: Potion"));
	}

	if (Item.ItemType == EItemType::Potion && Item.ItemClass)
	{
		AItem* DefaultItem = Item.ItemClass->GetDefaultObject<AItem>();
		APotion* DefaultPotion = Cast<APotion>(DefaultItem);

		UE_LOG(LogTemp, Warning, TEXT("ItemClass name: %s"), *Item.ItemClass->GetName());

		if (DefaultPotion)
		{
			int32 Heal = DefaultPotion->GetHealAmount();

			HealPlayer(Heal);
			UE_LOG(LogTemp, Log, TEXT("Healed %d from potion"), DefaultPotion->GetHealAmount());
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Healedsssssssssssssssssssss"), DefaultPotion->GetHealAmount());
		}

		Item.Count--;
		if (Item.Count <= 0)
		{
			EquipmentWidgetInstance->ClearSlot(EquipSlotIndex);
		}

		EquipmentWidgetInstance->RefreshEquipmentSlots();
	}
}

void AMyDCharacter::UseHotkey1() { UseHotkey(0); }
void AMyDCharacter::UseHotkey2() { UseHotkey(1); }
void AMyDCharacter::UseHotkey3() { UseHotkey(2); }
void AMyDCharacter::UseHotkey4() { UseHotkey(3); }
void AMyDCharacter::UseHotkey5() { UseHotkey(4); }