// Fill out your copyright notice in the Description page of Project Settings.


#include "WFCRegenerator.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "MyDCharacter.h"
#include "wfcex.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyCharacter.h"
#include "Async/Async.h"

// Sets default values
AWFCRegenerator::AWFCRegenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	RootComponent = TriggerVolume;
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
	TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));

	// 메쉬 컴포넌트 생성
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	MeshComp->SetupAttachment(RootComponent);

	// 메쉬 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Game/BP/SM_Table.SM_Table"));
	if (ConeMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(ConeMesh.Object);
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		MeshComp->SetWorldScale3D(FVector(0.5f));
	}

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AWFCRegenerator::OnOverlapBegin);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AWFCRegenerator::OnOverlapEnd);

	bReplicates = true;
	bAlwaysRelevant = true;

	Tags.Add(FName("WFCRegen"));

}

// Called when the game starts or when spawned
void AWFCRegenerator::BeginPlay()
{
	Super::BeginPlay();
	
	
}

// Called every frame
void AWFCRegenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWFCRegenerator::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	

		// OverlappedActor 설정 추가
		AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
		if (Player)
		{
			MeshComp->SetRenderCustomDepth(true);
			Player->OverlappedActor = this;
			UE_LOG(LogTemp, Warning, TEXT("AWFCRegenerator: Registered self as OverlappedActor"));
		}

}

void AWFCRegenerator::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
		MeshComp->SetRenderCustomDepth(false); // 외곽선 OFF
	
}

//void AWFCRegenerator::GenerateWFCAtLocation()
//{
//
//	ClearPreviousWFCActors();
//
//	UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
//	if (WFCSubsystem)
//	{
//
//
//		FVector SpawnOrigin = GetActorLocation();
//		FRotator SpawnRotation = GetActorRotation();
//
//		WFCSubsystem->OriginLocation = SpawnOrigin;
//		WFCSubsystem->Orientation = SpawnRotation;
//		WFCSubsystem->ExecuteWFC(TryCount, RandomSeed);
//
//
//	}
//}

void AWFCRegenerator::ClearPreviousWFCActors()
{
	UWorld* World = GetWorld();
	if (!World) return;

	auto DestroyActorsWithTag = [&](FName Tag)
		{
			TArray<AActor*> FoundActors;
			UGameplayStatics::GetAllActorsWithTag(World, Tag, FoundActors);

			for (AActor* Actor : FoundActors)
			{
				if (IsValid(Actor))
				{
					Actor->Destroy();
				}
			}
			UE_LOG(LogTemp, Log, TEXT("Destroyed %d actors with tag '%s'"), FoundActors.Num(), *Tag.ToString());
		};

	// 1. 타일 삭제
	DestroyActorsWithTag("WFCGenerated");

	// 2. 보물상자 삭제
	DestroyActorsWithTag("Chest");

	// 3. 탈출 오브젝트 삭제
	DestroyActorsWithTag("Escape");

	// 4. 적 삭제 (태그가 없으니 Class 기반)
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyCharacter::StaticClass(), FoundEnemies);

	for (AActor* Actor : FoundEnemies)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Destroyed %d Enemy actors."), FoundEnemies.Num());
}

//void AWFCRegenerator::GenerateWFCAtLocation()
//{
//	UE_LOG(LogTemp, Warning, TEXT("GenerateWFCAtLocation started"));
//
//	// 기존 WFC 액터 제거
//	ClearPreviousWFCActors();
//	UE_LOG(LogTemp, Warning, TEXT("Previous WFC actors cleared"));
//
//	// 서브시스템 가져오기
//	UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
//	if (!WFCSubsystem || !WFCSubsystem->WFCModel)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Failed to acquire WFCSubsystem or WFCModel"));
//		return;
//	}
//
//	// 타일 좌표 계산
//	FVector LocalOffset = GetActorLocation() - WFCSubsystem->OriginLocation;
//	int32 TileSize = WFCSubsystem->GetTileSize();
//	FIntVector TileCoord = FIntVector(
//		FMath::FloorToInt(LocalOffset.X / TileSize),
//		FMath::FloorToInt(LocalOffset.Y / TileSize),
//		FMath::FloorToInt(LocalOffset.Z / TileSize)
//	);
//
//	UE_LOG(LogTemp, Warning, TEXT("Tile coordinate calculated: (%d, %d, %d)"), TileCoord.X, TileCoord.Y, TileCoord.Z);
//
//	// 방 타일 옵션 찾기
//	FWaveFunctionCollapseOptionCustom FixedOption;
//	bool bFound = false;
//
//	/*for (const auto& Elem : WFCSubsystem->WFCModel->Constraints)
//	{
//		const FWaveFunctionCollapseOptionCustom& Option = Elem.Key;
//		FString Path = Option.BaseObject.ToString();
//
//		UE_LOG(LogTemp, Warning, TEXT("Checking Option: %s"), *Path);
//
//		if (Path == TEXT("/Game/BP/romm.romm"))
//		{
//			FixedOption = Option;
//			bFound = true;
//			UE_LOG(LogTemp, Warning, TEXT("Matched room tile option from Constraints: %s (IsRoomTile: %s)"),
//				*Path, Option.bIsRoomTile ? TEXT("true") : TEXT("false"));
//			break;
//		}
//	}*/
//
//	
//
//	for (const auto& Elem : WFCSubsystem->WFCModel->Constraints)
//	{
//		const FWaveFunctionCollapseOptionCustom& Option = Elem.Key;
//		if (Option.BaseObject.ToString() == TEXT("/Game/BP/romm.romm"))
//		{
//			FixedOption = Option; //반드시 Constraints에서 찾기
//			bFound = true;
//			break;
//		}
//	}
//
//
//
//	if (!bFound)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Could not find valid room tile option for /Game/BP/romm.romm"));
//		return;
//	}
//
//
//	UE_LOG(LogTemp, Warning, TEXT("Adding UserFixedOptions before ExecuteWFC at (%d, %d, %d)"), TileCoord.X, TileCoord.Y, TileCoord.Z);
//	// 고정 타일 설정
//	WFCSubsystem->UserFixedOptions.Add(TileCoord, FixedOption);
//	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions set: %s (RoomTile: %s)"),
//		*FixedOption.BaseObject.ToString(), FixedOption.bIsRoomTile ? TEXT("true") : TEXT("false"));
//
//	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions.Num() after add: %d"), WFCSubsystem->UserFixedOptions.Num());
//
//	WFCSubsystem->RegeneratorFixedTileCoord = TileCoord;
//	WFCSubsystem->RegeneratorFixedTileOption = FixedOption;
//	WFCSubsystem->bHasRegeneratorFixedTile = true;
//	UWorld* World = GetWorld();
//	// WFC 실행
//	WFCSubsystem->ExecuteWFC(TryCount, RandomSeed, World);
//	UE_LOG(LogTemp, Warning, TEXT("ExecuteWFC called"));
//
//	if (Awfcex* WFCManager = Cast<Awfcex>(UGameplayStatics::GetActorOfClass(GetWorld(), Awfcex::StaticClass())))
//	{
//		WFCManager->SpawnEnemiesOnCorridor(15);
//		WFCManager->SpawnEscapeObjectsOnRoom();
//		WFCManager->SpawnTreasureChestsOnTiles();
//	}
//
//	// 후처리 실행
//	//WFCSubsystem->PostProcessFixedRoomTileAt(TileCoord, Tiles);
//}

void AWFCRegenerator::GenerateWFCAtLocation()
{
	UE_LOG(LogTemp, Warning, TEXT("GenerateWFCAtLocation started"));

	if (!HasAuthority())
	{
		return;
	}

	// 기존 WFC 액터 제거
	ClearPreviousWFCActors();
	UE_LOG(LogTemp, Warning, TEXT("Previous WFC actors cleared"));

	// 서브시스템 가져오기
	UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
	if (!WFCSubsystem || !WFCSubsystem->WFCModel)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to acquire WFCSubsystem or WFCModel"));
		return;
	}

	


	// 타일 좌표 계산
	FVector LocalOffset = GetActorLocation() - WFCSubsystem->OriginLocation;
	int32 TileSize = WFCSubsystem->GetTileSize();
	FIntVector TileCoord = FIntVector(
		FMath::FloorToInt(LocalOffset.X / TileSize),
		FMath::FloorToInt(LocalOffset.Y / TileSize),
		FMath::FloorToInt(LocalOffset.Z / TileSize)
	);

	UE_LOG(LogTemp, Warning, TEXT("Tile coordinate calculated: (%d, %d, %d)"), TileCoord.X, TileCoord.Y, TileCoord.Z);

	// 방 타일 옵션 찾기
	FWaveFunctionCollapseOptionCustom FixedOption;
	bool bFound = false;

	/*for (const auto& Elem : WFCSubsystem->WFCModel->Constraints)
	{
		const FWaveFunctionCollapseOptionCustom& Option = Elem.Key;
		FString Path = Option.BaseObject.ToString();

		UE_LOG(LogTemp, Warning, TEXT("Checking Option: %s"), *Path);

		if (Path == TEXT("/Game/BP/romm.romm"))
		{
			FixedOption = Option;
			bFound = true;
			UE_LOG(LogTemp, Warning, TEXT("Matched room tile option from Constraints: %s (IsRoomTile: %s)"),
				*Path, Option.bIsRoomTile ? TEXT("true") : TEXT("false"));
			break;
		}
	}*/

	

	for (const auto& Elem : WFCSubsystem->WFCModel->Constraints)
	{
		const FWaveFunctionCollapseOptionCustom& Option = Elem.Key;
		if (Option.BaseObject.ToString() == TEXT("/Game/BP/romm.romm"))
		{
			FixedOption = Option; //반드시 Constraints에서 찾기
			bFound = true;
			break;
		}
	}



	if (!bFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find valid room tile option for /Game/BP/romm.romm"));
		return;
	}


	UE_LOG(LogTemp, Warning, TEXT("Adding UserFixedOptions before ExecuteWFC at (%d, %d, %d)"), TileCoord.X, TileCoord.Y, TileCoord.Z);
	// 고정 타일 설정
	WFCSubsystem->UserFixedOptions.Add(TileCoord, FixedOption);
	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions set: %s (RoomTile: %s)"),
		*FixedOption.BaseObject.ToString(), FixedOption.bIsRoomTile ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions.Num() after add: %d"), WFCSubsystem->UserFixedOptions.Num());

	WFCSubsystem->RegeneratorFixedTileCoord = TileCoord;
	WFCSubsystem->RegeneratorFixedTileOption = FixedOption;
	WFCSubsystem->bHasRegeneratorFixedTile = true;
	UWorld* World = GetWorld();
	// WFC 실행


	//WFCSubsystem->PrecomputeMapAsync(TryCount, RandomSeed, [this]() {});

	RandomSeed = 0;

	// 타이머 시작
	double StartTime = FPlatformTime::Seconds();

	WFCSubsystem->PrecomputeMapAsync(TryCount, RandomSeed, [this, StartTime]()
	{

			//타이머 끝
			double EndTime = FPlatformTime::Seconds();
			double Duration = EndTime - StartTime;
			UE_LOG(LogTemp, Warning, TEXT("WFC finished. Total time: %.3f seconds"), Duration);

			UE_LOG(LogTemp, Warning, TEXT("WFC good enemy start"));

			if (Awfcex* WFCManager = Cast<Awfcex>(UGameplayStatics::GetActorOfClass(GetWorld(), Awfcex::StaticClass())))
			{
				//WFCManager->SpawnEnemiesOnCorridor(15);
				WFCManager->SpawnEscapeObjectsOnRoom();
				WFCManager->SpawnTreasureChestsOnTiles();
			}

			// 맵 생성이 끝난 후, 캐릭터 중력 복구
			
			if (AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				UCharacterMovementComponent* MovementComp = Player->GetCharacterMovement();
				if (MovementComp)
				{
					MovementComp->GravityScale = 1.0f;
					//MovementComp->SetMovementMode(EMovementMode::MOVE_Walking); // Flying이 아닌 Walking으로

					// Velocity 초기화
					//MovementComp->Velocity = FVector::ZeroVector;
				}

			}
			

	});

	


	//WFCSubsystem->ExecuteWFC(TryCount, RandomSeed, World);
	UE_LOG(LogTemp, Warning, TEXT("ExecuteWFC called"));

	/*if (Awfcex* WFCManager = Cast<Awfcex>(UGameplayStatics::GetActorOfClass(GetWorld(), Awfcex::StaticClass())))
	{
		WFCManager->SpawnEnemiesOnCorridor(15);
		WFCManager->SpawnEscapeObjectsOnRoom();
		WFCManager->SpawnTreasureChestsOnTiles();
	}*/

	// 후처리 실행
	//WFCSubsystem->PostProcessFixedRoomTileAt(TileCoord, Tiles);
}

