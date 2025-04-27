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

    EscapeObjectClass = AEscapeObject::StaticClass();
    TreasureChestClass = ATreasureChest::StaticClass();

}


void Awfcex::BeginPlay()
{
	Super::BeginPlay();
	ExecuteWFCInSubsystem(90, 1172835073); //�׽�Ʈ�� �õ� 1967664897, 1094396673, �׽�Ʈ01: 1172835073, 1966419713, 984042241, 1925703041, 1435413505
    SpawnPlayerOnCorridor();

    SpawnEnemiesOnCorridor(15);

    SpawnWFCRegeneratorOnRoom();

    SpawnEscapeObjectsOnRoom();
    SpawnTreasureChestsOnTiles();
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
    if (World)
    {
        //**C++ ĳ���� ���� ���� (�⺻ C++ Ŭ����)**
        if (!PlayerClass)
        {
            PlayerClass = AMyDCharacter::StaticClass();
        }

        // Z�� ��¦ �÷��� �ٴڰ� �浹 ����
        FVector AdjustedLocation = Location + FVector(0.0f, 0.0f, 50.0f);

        // �÷��̾� ����
        FActorSpawnParameters SpawnParams;
        AMyDCharacter* SpawnedPlayer = World->SpawnActor<AMyDCharacter>(PlayerClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams);

        if (SpawnedPlayer)
        {
            UE_LOG(LogTemp, Log, TEXT("olmkmknunwn: %s"), *AdjustedLocation.ToString());

            //��Ʈ�ѷ� Ȯ��
            APlayerController* PlayerController = World->GetFirstPlayerController();
            if (PlayerController)
            {
                UE_LOG(LogTemp, Log, TEXT("Found PlayerController: %s"), *PlayerController->GetName());

                //������ Possess �õ�
                PlayerController->Possess(SpawnedPlayer);

                //Possession ���� ���� Ȯ��
                if (SpawnedPlayer->GetController() == PlayerController)
                {
                    UE_LOG(LogTemp, Log, TEXT("Player successfully possessed by controller."));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Possession failed!"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("No PlayerController found!"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn AMyDCharacter at location: %s"), *AdjustedLocation.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("World is not available."));
    }
}


void Awfcex::SpawnEnemiesOnCorridor(int32 EnemyCount)
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (!WFCSubsystem || !WFCSubsystem->WFCModel)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WFCSubsystem or WFCModel"));
        return;
    }

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> ValidCorridorPositions;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
            if (Option.bIsCorridorTile)
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                ValidCorridorPositions.Add(TilePosition);
            }
        }
    }

    if (ValidCorridorPositions.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No corridor tiles available for enemy spawn."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 i = 0; i < EnemyCount; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidCorridorPositions.Num() - 1);
        FVector SpawnLocation = ValidCorridorPositions[RandomIndex] + FVector(0.f, 0.f, 50.f); // Z�� �÷��� �ٴ� �浹 ����

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AEnemyCharacter* SpawnedEnemy = World->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

        if (SpawnedEnemy)
        {
            UE_LOG(LogTemp, Log, TEXT("Spawned enemy at location: %s"), *SpawnLocation.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn enemy at location: %s"), *SpawnLocation.ToString());
        }
    }
}

void Awfcex::SpawnWFCRegeneratorOnRoom()
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel) return;

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> RoomTilePositions;
    TArray<FIntVector> RoomTileCoords;
    TArray<FWaveFunctionCollapseOptionCustom> RoomTileOptions;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0 &&
            Tiles[TileIndex].RemainingOptions[0].bIsRoomTile)
        {
            FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
            RoomTilePositions.Add(TilePosition);

            FIntVector Coord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
            RoomTileCoords.Add(Coord);
            RoomTileOptions.Add(Tiles[TileIndex].RemainingOptions[0]);
        }
    }

    if (RoomTilePositions.Num() == 0) return;

    //�߽� ��ǥ ���
    FIntVector CenterCoord = FIntVector(Resolution.X / 2, Resolution.Y / 2, Resolution.Z / 2);

    // �߽����κ��� ����� �游 ���͸� (����ư �Ÿ� ���)
    auto GetManhattanDistance = [](const FIntVector& A, const FIntVector& B)
        {
            return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
        };

    TArray<int32> CentralRoomIndices;
    for (int32 i = 0; i < RoomTileCoords.Num(); ++i)
    {
        int32 Distance = GetManhattanDistance(RoomTileCoords[i], CenterCoord);
        if (Distance <= 10) // �߽ɿ��� 10ĭ �̳��� ��� (�ʿ��ϸ� ����)
        {
            CentralRoomIndices.Add(i);
        }
    }

    //���ǿ� �´� �� ���ٸ� ��ü���� ����
    int32 SelectedIndex = -1;
    if (CentralRoomIndices.Num() > 0)
    {
        SelectedIndex = CentralRoomIndices[FMath::RandRange(0, CentralRoomIndices.Num() - 1)];
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No central room found. Using fallback random room."));
        SelectedIndex = FMath::RandRange(0, RoomTileCoords.Num() - 1);
    }

    FVector SpawnLocation = RoomTilePositions[SelectedIndex] + FVector(0, 0, 0);

    // StarterOption ���� ���
    WFCSubsystem->UserFixedOptions.Add(RoomTileCoords[SelectedIndex], RoomTileOptions[SelectedIndex]);
    UE_LOG(LogTemp, Warning, TEXT("StarterOption fixed for Regenerator at (%d, %d, %d)"),
        RoomTileCoords[SelectedIndex].X, RoomTileCoords[SelectedIndex].Y, RoomTileCoords[SelectedIndex].Z);

    // WFC Regenerator ������Ʈ ����
    FActorSpawnParameters Params;
    GetWorld()->SpawnActor<AWFCRegenerator>(WFCRegeneratorClass, SpawnLocation, FRotator::ZeroRotator, Params);
}

void Awfcex::SpawnEscapeObjectsOnRoom()
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel) return;

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> RoomTilePositions;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0 &&
            Tiles[TileIndex].RemainingOptions[0].bIsRoomTile)
        {
            FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
            RoomTilePositions.Add(TilePosition);
        }
    }

    if (RoomTilePositions.Num() == 0) return;

    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 i = 0; i < 2; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, RoomTilePositions.Num() - 1);
        FVector SpawnLocation = RoomTilePositions[RandomIndex] + FVector(0, 0, 50); // �ణ ����

        FActorSpawnParameters Params;
        World->SpawnActor<AEscapeObject>(EscapeObjectClass, SpawnLocation, FRotator::ZeroRotator, Params);

        UE_LOG(LogTemp, Log, TEXT("Escape Object spawned at %s"), *SpawnLocation.ToString());
    }
}


void Awfcex::SpawnTreasureChestsOnTiles()
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel) return;

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> ValidTilePositions;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];

            // ���� �Ǵ� �� Ÿ�� ��� ���
            if (Option.bIsCorridorTile || Option.bIsRoomTile)
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                ValidTilePositions.Add(TilePosition);
            }
        }
    }

    if (ValidTilePositions.Num() == 0) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const float MaxOffsetRadius = 200.f;
    const float ClampMargin = 50.f; // ���� ����
    const FVector2D TileHalfSize = FVector2D(TileSize * 0.5f, TileSize * 0.5f);

    for (int32 i = 0; i < 30; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidTilePositions.Num() - 1);
        FVector BaseLocation = ValidTilePositions[RandomIndex];

        FVector2D RandomOffset = FMath::RandPointInCircle(MaxOffsetRadius);

        // Clamp: Ÿ�� ��� ���� �ʰ� ����
        RandomOffset.X = FMath::Clamp(RandomOffset.X, -TileHalfSize.X + ClampMargin, TileHalfSize.X - ClampMargin);
        RandomOffset.Y = FMath::Clamp(RandomOffset.Y, -TileHalfSize.Y + ClampMargin, TileHalfSize.Y - ClampMargin);

        FVector SpawnLocation = BaseLocation + FVector(RandomOffset.X, RandomOffset.Y, 5.0f);

        float RandomYaw = FMath::FRandRange(0.f, 360.f);
        FRotator SpawnRotation = FRotator(0.f, RandomYaw, 0.f);

        FActorSpawnParameters Params;
        World->SpawnActor<ATreasureChest>(TreasureChestClass, SpawnLocation, SpawnRotation, Params);

        UE_LOG(LogTemp, Log, TEXT("Treasure Chest spawned at %s with rotation %.2f"), *SpawnLocation.ToString(), RandomYaw);
    }
}