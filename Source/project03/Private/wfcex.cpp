// Fill out your copyright notice in the Description page of Project Settings.


#include "wfcex.h"
#include "Engine/World.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h" 
#include "Editor.h" 

// Sets default values
Awfcex::Awfcex()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}


void Awfcex::BeginPlay()
{
	Super::BeginPlay();
	ExecuteWFCInSubsystem(90, 1967664897); //�׽�Ʈ�� �õ� 1967664897, 1094396673, 1172835073, 1382874881
    SpawnPlayerOnCorridor();
}


void Awfcex::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void Awfcex::ExecuteWFCInSubsystem(int32 TryCount, int32 RandomSeed)
{
    // WFC ����ý��� ��������
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (WFCSubsystem)
    {
        WFCSubsystem->ExecuteWFC(TryCount, RandomSeed);
        
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WFCSubsystemNO"));
    }
}

UWaveFunctionCollapseSubsystem02* Awfcex::GetWFCSubsystem()
{
    // ���� ������ GameInstance�� ���� ����ý��� ��������
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
        }
    }

    UE_LOG(LogTemp, Error, TEXT("WFCSubsystemNOS!"));
    return nullptr;
}


void Awfcex::SpawnPlayerOnCorridor()
{
    // WFC ����ý��� ��������
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (!WFCSubsystem || !WFCSubsystem->WFCModel)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WFCSubsystem or WFCModel"));
        return;
    }

    // CollapseCustom ���� �� ���� Ÿ�� ������ ���� ����
    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles; // CollapseCustom ���� �� Ÿ�� ������
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> CorridorTilePositions;

    // ���� Ÿ�� ��ġ ����
    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];

            // ���� Ÿ������ Ȯ��
            if (Option.bIsCorridorTile) // ���� Ÿ�� ���� Ȯ��
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                CorridorTilePositions.Add(TilePosition);

                // �α׷� ��ġ ���
                UE_LOG(LogTemp, Log, TEXT("Corridor tile found at location: %s"), *TilePosition.ToString());
            }
        }
    }

    if (CorridorTilePositions.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No corridor tiles available for player spawn."));
        return;
    }

    // �α׷� ������ ���� Ÿ�� ��ġ ���
    UE_LOG(LogTemp, Log, TEXT("Collected %d corridor tile positions."), CorridorTilePositions.Num());

    // ���� ���� Ÿ�� ����
    int32 RandomIndex = FMath::RandRange(0, CorridorTilePositions.Num() - 1);
    FVector SpawnLocation = CorridorTilePositions[RandomIndex];

    // �÷��̾� ����
    SpawnPlayerAtLocation(SpawnLocation);
}

void Awfcex::SpawnPlayerAtLocation(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (World && PlayerClass)
    {
        // Z�� ��¦ �÷��� ��ġ ����
        FVector AdjustedLocation = Location + FVector(0.0f, 0.0f, 50.0f); // �ٴڰ� �浹 ����

        // �÷��̾� ����
        FActorSpawnParameters SpawnParams;
        AActor* SpawnedPlayer = World->SpawnActor<AActor>(PlayerClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams);

        if (SpawnedPlayer)
        {

            // �÷��̾� ��Ʈ�ѷ��� ������ ĳ���͸� ����
            APlayerController* PlayerController = World->GetFirstPlayerController();
            if (PlayerController)
            {
                PlayerController->Possess(Cast<APawn>(SpawnedPlayer));
            }

            UE_LOG(LogTemp, Log, TEXT("Player successfully spawned at location: %s"), *AdjustedLocation.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn player at location: %s"), *AdjustedLocation.ToString());
        }
    }
    else
    {
        if (!PlayerClass)
        {
            UE_LOG(LogTemp, Error, TEXT("PlayerClass is not set."));
        }
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("World is not available."));
        }
    }
}

void Awfcex::UpdatePlayerStartLocation(const FVector& NewLocation)
{
    // ���忡�� PlayerStart ���� �˻�
    APlayerStart* PlayerStart = nullptr;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        PlayerStart = *It;
        break; // ù ��° PlayerStart�� ���
    }

    if (PlayerStart)
    {
        // PlayerStart�� ��ġ�� ���� Ÿ�� ��ġ�� �̵�
        PlayerStart->SetActorLocation(NewLocation);
        UE_LOG(LogTemp, Log, TEXT("PlayerStart location updated to: %s"), *NewLocation.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No PlayerStart found in the level."));
    }
}