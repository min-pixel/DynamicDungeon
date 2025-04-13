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
#include "TreasureChest.h"
#include "EnemyCharacter.h"
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
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapBegin);
	InteractionBox->OnComponentEndOverlap.AddDynamic(this, &AMyDCharacter::OnOverlapEnd);

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

	static ConstructorHelpers::FClassFinder<UUserWidget> CombinedWidgetBP(TEXT("/Game/BP/UI/CombinedInventoryWidget.CombinedInventoryWidget_C"));
	if (CombinedWidgetBP.Succeeded())
	{
		CombinedInventoryWidgetClass = CombinedWidgetBP.Class;
	}

}

// 게임 시작 시 호출
void AMyDCharacter::BeginPlay()
{
	Super::BeginPlay();


	// 애니메이션 블루프린트 수동 설정
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
	// Agility 값에 따라 초기 이동 속도 설정
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

	// 위젯 생성은 항상!
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

				InventoryWidgetInstance->RefreshInventoryStruct(); // 다시 갱신
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

		// 오버랩 종료 시 NewGameInstance 변수 초기화
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
	FTransform HandSocketTransform = CharacterMesh->GetSocketTransform(FName("hand_r"));
	UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());

	AWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	EquippedWeapon = SpawnedWeapon;

	if (CharacterMesh && CharacterMesh->DoesSocketExist("hand_r"))
	{
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeapon->AttachToComponent(CharacterMesh, AttachRules, FName("hand_r"));

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