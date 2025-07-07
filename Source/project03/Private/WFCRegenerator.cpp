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

	// �޽� ������Ʈ ����
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	MeshComp->SetupAttachment(RootComponent);

	// �޽� ����
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
	

		// OverlappedActor ���� �߰�
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
	
		MeshComp->SetRenderCustomDepth(false); // �ܰ��� OFF
	
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

	// 1. Ÿ�� ����
	DestroyActorsWithTag("WFCGenerated");

	// 2. �������� ����
	DestroyActorsWithTag("Chest");

	// 3. Ż�� ������Ʈ ����
	DestroyActorsWithTag("Escape");

	// 4. �� ���� (�±װ� ������ Class ���)
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
//	// ���� WFC ���� ����
//	ClearPreviousWFCActors();
//	UE_LOG(LogTemp, Warning, TEXT("Previous WFC actors cleared"));
//
//	// ����ý��� ��������
//	UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
//	if (!WFCSubsystem || !WFCSubsystem->WFCModel)
//	{
//		UE_LOG(LogTemp, Error, TEXT("Failed to acquire WFCSubsystem or WFCModel"));
//		return;
//	}
//
//	// Ÿ�� ��ǥ ���
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
//	// �� Ÿ�� �ɼ� ã��
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
//			FixedOption = Option; //�ݵ�� Constraints���� ã��
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
//	// ���� Ÿ�� ����
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
//	// WFC ����
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
//	// ��ó�� ����
//	//WFCSubsystem->PostProcessFixedRoomTileAt(TileCoord, Tiles);
//}

void AWFCRegenerator::GenerateWFCAtLocation()
{
	UE_LOG(LogTemp, Warning, TEXT("GenerateWFCAtLocation started"));

	if (!HasAuthority())
	{
		return;
	}

	// ���� WFC ���� ����
	ClearPreviousWFCActors();
	UE_LOG(LogTemp, Warning, TEXT("Previous WFC actors cleared"));

	// ����ý��� ��������
	UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
	if (!WFCSubsystem || !WFCSubsystem->WFCModel)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to acquire WFCSubsystem or WFCModel"));
		return;
	}

	


	// Ÿ�� ��ǥ ���
	FVector LocalOffset = GetActorLocation() - WFCSubsystem->OriginLocation;
	int32 TileSize = WFCSubsystem->GetTileSize();
	FIntVector TileCoord = FIntVector(
		FMath::FloorToInt(LocalOffset.X / TileSize),
		FMath::FloorToInt(LocalOffset.Y / TileSize),
		FMath::FloorToInt(LocalOffset.Z / TileSize)
	);

	UE_LOG(LogTemp, Warning, TEXT("Tile coordinate calculated: (%d, %d, %d)"), TileCoord.X, TileCoord.Y, TileCoord.Z);

	// �� Ÿ�� �ɼ� ã��
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
			FixedOption = Option; //�ݵ�� Constraints���� ã��
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
	// ���� Ÿ�� ����
	WFCSubsystem->UserFixedOptions.Add(TileCoord, FixedOption);
	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions set: %s (RoomTile: %s)"),
		*FixedOption.BaseObject.ToString(), FixedOption.bIsRoomTile ? TEXT("true") : TEXT("false"));

	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions.Num() after add: %d"), WFCSubsystem->UserFixedOptions.Num());

	WFCSubsystem->RegeneratorFixedTileCoord = TileCoord;
	WFCSubsystem->RegeneratorFixedTileOption = FixedOption;
	WFCSubsystem->bHasRegeneratorFixedTile = true;
	UWorld* World = GetWorld();
	// WFC ����


	//WFCSubsystem->PrecomputeMapAsync(TryCount, RandomSeed, [this]() {});

	RandomSeed = 0;

	// Ÿ�̸� ����
	double StartTime = FPlatformTime::Seconds();

	WFCSubsystem->PrecomputeMapAsync(TryCount, RandomSeed, [this, StartTime]()
	{

			//Ÿ�̸� ��
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

			// �� ������ ���� ��, ĳ���� �߷� ����
			
			if (AMyDCharacter* Player = Cast<AMyDCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				UCharacterMovementComponent* MovementComp = Player->GetCharacterMovement();
				if (MovementComp)
				{
					MovementComp->GravityScale = 1.0f;
					//MovementComp->SetMovementMode(EMovementMode::MOVE_Walking); // Flying�� �ƴ� Walking����

					// Velocity �ʱ�ȭ
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

	// ��ó�� ����
	//WFCSubsystem->PostProcessFixedRoomTileAt(TileCoord, Tiles);
}

