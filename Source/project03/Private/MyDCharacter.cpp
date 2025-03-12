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
#include "Kismet/GameplayStatics.h"
#include "UCharacterHUDWidget.h"
#include "Blueprint/UserWidget.h"
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

	//스프링암(SprintArm) 생성 (메쉬와 카메라를 독립적으로 배치하기 위해)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 0.0f; // 1인칭이므로 거리 없음
	SpringArm->bUsePawnControlRotation = false; // 카메라 회전에 영향을 주지 않음

	// 캐릭터 메쉬 생성
	CharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh"));
	CharacterMesh->SetupAttachment(SpringArm);

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

	// 주먹 공격 애니메이션 로드 (콤보 시스템 적용)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> PunchAnim1(TEXT("/Game/BP/character/Retarget/RTA_Punching_Anim_mixamo_com.RTA_Punching_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> PunchAnim2(TEXT("/Game/BP/character/Retarget/RTA_Punching__1__Anim_mixamo_com.RTA_Punching__1__Anim_mixamo_com"));

	if (PunchAnim1.Succeeded() && PunchAnim2.Succeeded())
	{
		UnarmedAttackAnimations.Add(PunchAnim1.Object);
		UnarmedAttackAnimations.Add(PunchAnim2.Object);
		UE_LOG(LogTemp, Log, TEXT("Unarmed Attack Animations Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load one or more Unarmed Attack Animations! Check the path."));
	}

	// 무기 공격 애니메이션 로드 (콤보 시스템 적용)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SwordSlash1(TEXT("/Game/BP/character/Retarget/RTA_Stable_Sword_Outward_Slash_Anim_mixamo_com.RTA_Stable_Sword_Outward_Slash_Anim_mixamo_com"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SwordSlash2(TEXT("/Game/BP/character/Retarget/RTA_Stable_Sword_Inward_Slash_Anim_mixamo_com.RTA_Stable_Sword_Inward_Slash_Anim_mixamo_com"));

	if (SwordSlash1.Succeeded() && SwordSlash2.Succeeded())
	{
		WeaponAttackAnimations.Add(SwordSlash1.Object);
		WeaponAttackAnimations.Add(SwordSlash2.Object);
		UE_LOG(LogTemp, Log, TEXT("Weapon Attack Animations Loaded Successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load one or more Weapon Attack Animations! Check the path."));
	}

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

// 매 프레임 호출
void AMyDCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// 이동 함수 (W/S)
void AMyDCharacter::MoveForward(float Value)
{
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
}

// 달리기 중지
void AMyDCharacter::StopSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * Agility;
}

// 입력 바인딩 설정
void AMyDCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Log, TEXT("SetupPlayerInputComponent called!"));

	// 이동 입력 바인딩 (WASD)
	PlayerInputComponent->BindAxis("MoveForward", this, &AMyDCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyDCharacter::MoveRight);

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

			// 무기 중력 및 물리 시뮬레이션 비활성화
			if (EquippedWeapon->WeaponMesh)
			{
				EquippedWeapon->WeaponMesh->SetSimulatePhysics(false);  //물리 비활성화
				EquippedWeapon->WeaponMesh->SetEnableGravity(false);    //중력 비활성화
			}

			// 무기 크기 조정
			EquippedWeapon->SetActorScale3D(FVector(0.25f, 0.25f, 1.0f));

			// 무기 충돌 비활성화 및 숨김 해제
			EquippedWeapon->SetActorEnableCollision(false);
			EquippedWeapon->SetActorHiddenInGame(false);
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


void AMyDCharacter::UpdateHUD()
{
	if (HUDWidget)
	{
		HUDWidget->UpdateHealth(Health, MaxHealth);
		HUDWidget->UpdateMana(Knowledge, MaxKnowledge);
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

// 공격 애니메이션 실행
void AMyDCharacter::PlayAttackAnimation()
{

	// 이미 공격 중이면 입력을 무시
	if (bIsAttacking)
	{
		// 콤보가 가능하면 콤보 입력을 받아서 다음 공격 준비
		if (bCanCombo)
		{
			UE_LOG(LogTemp, Log, TEXT("Combo Attack Registered!"));
			AttackComboIndex++; // 콤보 인덱스 증가
			bCanCombo = false;  // 콤보 입력 후 다시 false로 설정
		}
		return;
	}

	// 공격 시작
	bIsAttacking = true;
	AttackComboIndex = (AttackComboIndex) % 2; // 0과 1을 반복 (2타 콤보)


	// 현재 공격 타입 결정 (무기 or 맨손)
	EAttackType AttackType = (EquippedWeapon != nullptr) ? EAttackType::Weapon : EAttackType::Unarmed;
	UAnimSequence* SelectedAnimation = nullptr;

	// 애니메이션 선택 (콤보 인덱스 활용)
	switch (AttackType)
	{
	case EAttackType::Weapon:
		if (!WeaponAttackAnimations.IsEmpty())
		{
			SelectedAnimation = WeaponAttackAnimations[AttackComboIndex % WeaponAttackAnimations.Num()];
		}
		break;

	case EAttackType::Unarmed:
		if (!UnarmedAttackAnimations.IsEmpty())
		{
			SelectedAnimation = UnarmedAttackAnimations[AttackComboIndex % UnarmedAttackAnimations.Num()];
		}
		break;

	default:
		UE_LOG(LogTemp, Error, TEXT("No valid attack animation found!"));
		bIsAttacking = false; // 공격 불가 상태 해제
		return;
	}

	// 선택된 애니메이션이 유효하면 실행
	if (SelectedAnimation)
	{
		UE_LOG(LogTemp, Log, TEXT("Playing Attack Animation: %d"), AttackComboIndex);
		CharacterMesh->PlayAnimation(SelectedAnimation, false);

		// 애니메이션 길이 가져오기
		float AttackAnimDuration = SelectedAnimation->GetPlayLength();

		// 콤보 입력을 받을 수 있도록 활성화 (애니메이션 중반에 콤보 가능)
		GetWorldTimerManager().SetTimer(TimerHandle_Combo, this, &AMyDCharacter::EnableCombo, AttackAnimDuration * 0.4f, false);

		// 공격이 끝난 후 상태 초기화
		GetWorldTimerManager().SetTimer(TimerHandle_Reset, this, &AMyDCharacter::ResetAttack, AttackAnimDuration, false);

		// 공격 중 일정 시간이 지나면 강제로 초기화 (콤보 초기화용)
		GetWorldTimerManager().SetTimer(TimerHandle_ForceReset, this, &AMyDCharacter::ForceResetCombo, 1.5f, false);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to play attack animation."));
		bIsAttacking = false;
	}
}

void AMyDCharacter::ResetToIdleAnimation()
{
	if (CharacterMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("Resetting to Idle Animation..."));

		// 원래 애니메이션 블루프린트로 복구
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}
}

void AMyDCharacter::EnableCombo()
{
	bCanCombo = true;
	//AttackComboIndex = (AttackComboIndex + 1) % 2; // 0과 1을 반복 (2타 콤보)
	UE_LOG(LogTemp, Log, TEXT("Combo Window Open!"));
}

void AMyDCharacter::ResetAttack()
{
	// 공격 상태 초기화
	bIsAttacking = false;
	bCanCombo = false;
	

	UE_LOG(LogTemp, Log, TEXT("Attack Ended, Resetting Attack State"));

	// 원래 애니메이션 블루프린트로 복구, 얘가 문제...
	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
}

void AMyDCharacter::ForceResetCombo()
{
	AttackComboIndex = 0;
	UE_LOG(LogTemp, Log, TEXT("Combo reset due to timeout."));
}