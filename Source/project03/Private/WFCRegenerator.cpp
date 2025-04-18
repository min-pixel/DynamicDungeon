// Fill out your copyright notice in the Description page of Project Settings.


#include "WFCRegenerator.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"

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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Game/StarterContent/Shapes/Shape_Cone.Shape_Cone"));
	if (ConeMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(ConeMesh.Object);
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		MeshComp->SetWorldScale3D(FVector(1.0f));
	}

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AWFCRegenerator::OnOverlapBegin);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AWFCRegenerator::OnOverlapEnd);

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
	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		MeshComp->SetRenderCustomDepth(true); // �ܰ��� ON

		if (bGenerateOnOverlap)
		{
			GenerateWFCAtLocation();
		}
	}
}

void AWFCRegenerator::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		MeshComp->SetRenderCustomDepth(false); // �ܰ��� OFF
	}
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

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("WFCGenerated"), FoundActors);

	UE_LOG(LogTemp, Log, TEXT("Found %d WFCGenerated actors to delete."), FoundActors.Num());

	for (AActor* Actor : FoundActors)
	{
		if (IsValid(Actor))
		{
			FString Name = Actor->GetName();
			Actor->Destroy();
			UE_LOG(LogTemp, Log, TEXT("Destroyed actor: %s"), *Name);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid actor encountered, skipping."));
		}
	}
}

void AWFCRegenerator::GenerateWFCAtLocation()
{
	UE_LOG(LogTemp, Warning, TEXT("GenerateWFCAtLocation started"));

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

	// WFC ����
	WFCSubsystem->ExecuteWFC(TryCount, RandomSeed);
	UE_LOG(LogTemp, Warning, TEXT("ExecuteWFC called"));

	// ��ó�� ����
	WFCSubsystem->PostProcessFixedRoomTileAt(TileCoord);
}
