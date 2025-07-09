// Fill out your copyright notice in the Description page of Project Settings.


#include "wfcex.h"
#include "Engine/World.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "DynamicDungeonModeBase.h"
#include "EngineUtils.h" 
//#include "Editor.h" 

// Sets default values
Awfcex::Awfcex()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    EscapeObjectClass = AEscapeObject::StaticClass();
    TreasureChestClass = ATreasureChest::StaticClass();

    RageEnemyClass = ARageEnemyCharacter::StaticClass();

}





void Awfcex::BeginPlay()
{
	Super::BeginPlay();

    /*if (!HasAuthority() || GetNetMode() == NM_Client)
    {
        UE_LOG(LogTemp, Warning, TEXT("Awfcex BeginPlay: Client detected, skipping WFC!"));
        return;
    }*/

    if (HasAuthority()) // ���� ���� ����
    {

        UE_LOG(LogTemp, Warning, TEXT("Awfcex HasAuthority: TRUE, Executing WFC"));

        const double StartTime = FPlatformTime::Seconds();

        ExecuteWFCInSubsystem(90, 0); //�׽�Ʈ�� �õ� 1967664897, 1094396673, �׽�Ʈ01: 1172835073, 1966419713, 984042241, 1925703041, 1435413505--1, 767089153, 1948641409, 1358936321, 1964145409,  2078383361, 1231524609, 46204289

        const double EndTime = FPlatformTime::Seconds();
        const double ElapsedTime = EndTime - StartTime;


        UE_LOG(LogTemp, Warning, TEXT("[WFC] maptime: %.3f sec"), ElapsedTime);

        //Ǯ��
        if (UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem())
        {
            WFCSubsystem->PrepareTilePrefabPool(GetWorld());
        }




        
        SpawnEnemiesOnCorridor(1);

        SpawnWFCRegeneratorOnRoom();

        SpawnEscapeObjectsOnRoom();
        SpawnTreasureChestsOnTiles();

        // WFC �Ϸ� �÷��� ����
        if (UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem())
        {
            WFCSubsystem->bWFCCompleted = true;
            UE_LOG(LogTemp, Log, TEXT("WFC completed, players can now spawn"));
        }

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Awfcex HasAuthority: FALSE, Skipping WFC"));
    }


    //SpawnPlayerOnCorridor();
    if (ADynamicDungeonModeBase* GameMode = Cast<ADynamicDungeonModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->ForceRestartAllPlayers();
        UE_LOG(LogTemp, Warning, TEXT("Players forced to restart after WFC completion."));
    }

}


void Awfcex::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void Awfcex::ExecuteWFCInSubsystem(int32 TryCount, int32 RandomSeed)
{
    // WFC ����ý��� ��������
    UWorld* World = GetWorld(); // ����
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (WFCSubsystem)
    {
        WFCSubsystem->ExecuteWFC(TryCount, RandomSeed, World);
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

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex) {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0 &&
            Tiles[TileIndex].RemainingOptions[0].bIsCorridorTile) {
            FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
            CorridorTilePositions.Add(TilePosition);
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
    if (!World || !HasAuthority() || !PlayerClass) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && !PC->GetPawn())
        {
            AMyDCharacter* NewPawn = World->SpawnActor<AMyDCharacter>(PlayerClass, Location, FRotator::ZeroRotator, SpawnParams);
            if (NewPawn)
            {
                PC->Possess(NewPawn);
                UE_LOG(LogTemp, Log, TEXT("Spawned & Possessed at %s for %s"), *Location.ToString(), *PC->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn pawn for %s"), *PC->GetName());
            }
        }
    }
}

//void Awfcex::SpawnPlayerAtLocation(const FVector& Location)
//{
//    UWorld* World = GetWorld();
//    if (World)
//    {
//        //**C++ ĳ���� ���� ���� (�⺻ C++ Ŭ����)**
//        if (!PlayerClass)
//        {
//            PlayerClass = AMyDCharacter::StaticClass();
//        }
//
//        // Z�� ��¦ �÷��� �ٴڰ� �浹 ����
//        FVector AdjustedLocation = Location + FVector(0.0f, 0.0f, 100.0f);
//
//        // �÷��̾� ����
//        FActorSpawnParameters SpawnParams;
//        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
//
//        AMyDCharacter* SpawnedPlayer = World->SpawnActor<AMyDCharacter>(PlayerClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams);
//
//        if (SpawnedPlayer)
//        {
//            UE_LOG(LogTemp, Log, TEXT("olmkmknunwn: %s"), *AdjustedLocation.ToString());
//
//            FVector FinalLocation = SpawnedPlayer->GetActorLocation();
//
//            if (!FinalLocation.Equals(AdjustedLocation, 1.0f)) // ��ġ�� �ٲ������
//            {
//                UE_LOG(LogTemp, Warning, TEXT("Spawn location adjusted due to collision. Requested: %s | Actual: %s"),
//                    *AdjustedLocation.ToString(), *FinalLocation.ToString());
//            }
//            else
//            {
//                UE_LOG(LogTemp, Log, TEXT("Spawned at intended location: %s"), *FinalLocation.ToString());
//            }
//
//
//
//            //��Ʈ�ѷ� Ȯ��
//            APlayerController* PlayerController = World->GetFirstPlayerController();
//            if (PlayerController)
//            {
//                UE_LOG(LogTemp, Log, TEXT("Found PlayerController: %s"), *PlayerController->GetName());
//
//                //������ Possess �õ�
//                PlayerController->Possess(SpawnedPlayer);
//
//                //Possession ���� ���� Ȯ��
//                if (SpawnedPlayer->GetController() == PlayerController)
//                {
//                    UE_LOG(LogTemp, Log, TEXT("Player successfully possessed by controller."));
//                }
//                else
//                {
//                    UE_LOG(LogTemp, Error, TEXT("Possession failed!"));
//                }
//            }
//            else
//            {
//                UE_LOG(LogTemp, Error, TEXT("No PlayerController found!"));
//            }
//        }
//        else
//        {
//            SpawnPlayerOnCorridor();
//        }
//    }
//    else
//    {
//        UE_LOG(LogTemp, Error, TEXT("World is not available."));
//    }
//}


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

        //AEnemyCharacter* SpawnedEnemy = World->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

        ARageEnemyCharacter* SpawnedEnemy = World->SpawnActor<ARageEnemyCharacter>(RageEnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

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

    FIntVector CenterCoord = FIntVector(Resolution.X / 2, Resolution.Y / 2, Resolution.Z / 2);

    auto GetManhattanDistance = [](const FIntVector& A, const FIntVector& B)
        {
            return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
        };

    TArray<int32> CentralRoomIndices;
    for (int32 i = 0; i < RoomTileCoords.Num(); ++i)
    {
        int32 Distance = GetManhattanDistance(RoomTileCoords[i], CenterCoord);
        if (Distance <= 15)
        {
            CentralRoomIndices.Add(i);
        }
    }

    if (CentralRoomIndices.Num() < 3)
    {
        for (int32 i = 0; i < RoomTileCoords.Num(); ++i)
        {
            if (!CentralRoomIndices.Contains(i))
            {
                CentralRoomIndices.Add(i);
                if (CentralRoomIndices.Num() >= 3) break;
            }
        }
    }

    TSet<int32> SelectedIndices;
    while (SelectedIndices.Num() < 3 && CentralRoomIndices.Num() > 0)
    {
        int32 RandIndex = CentralRoomIndices[FMath::RandRange(0, CentralRoomIndices.Num() - 1)];
        CentralRoomIndices.Remove(RandIndex);
        SelectedIndices.Add(RandIndex);
    }

    for (int32 SelectedIndex : SelectedIndices)
    {
        FVector SpawnLocation = RoomTilePositions[SelectedIndex] + FVector(0, 0, 0);
        FIntVector Coord = RoomTileCoords[SelectedIndex];
        FWaveFunctionCollapseOptionCustom Option = RoomTileOptions[SelectedIndex];

        // ���� Ÿ�� ���
        WFCSubsystem->UserFixedOptions.Add(Coord, Option);
        UE_LOG(LogTemp, Warning, TEXT("Fixed Regenerator Tile at (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);

        // ������Ʈ ����
        FActorSpawnParameters Params;
        GetWorld()->SpawnActor<AWFCRegenerator>(WFCRegeneratorClass, SpawnLocation, FRotator::ZeroRotator, Params);
    }
}

void Awfcex::SpawnEscapeObjectsOnRoom()
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel) return;

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> RoomTilePositions;
    TArray<FIntVector> RoomTileCoords;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0 &&
            Tiles[TileIndex].RemainingOptions[0].bIsRoomTile)
        {
            FIntVector Coord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
            RoomTileCoords.Add(Coord);
            RoomTilePositions.Add(FVector(Coord) * TileSize);
        }
    }

    if (RoomTileCoords.Num() == 0) return;

    // �ߺ� ����: ����� ������Ʈ�� ��ġ�� ��ǥ�� ����
    TArray<int32> ValidIndices;
    for (int32 i = 0; i < RoomTileCoords.Num(); ++i)
    {
        if (!WFCSubsystem->UserFixedOptions.Contains(RoomTileCoords[i]))
        {
            ValidIndices.Add(i);
        }
    }

    if (ValidIndices.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid room tiles available for EscapeObject spawn (all overlap with regenerators)."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 i = 0; i < 2; ++i)
    {
        if (ValidIndices.Num() == 0) break;

        int32 RandIdx = FMath::RandRange(0, ValidIndices.Num() - 1);
        int32 SelectedIndex = ValidIndices[RandIdx];
        ValidIndices.RemoveAt(RandIdx); // �ߺ� ����

        FVector SpawnLocation = RoomTilePositions[SelectedIndex] + FVector(0, 0, 50); // �ణ ����

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
    TArray<FWaveFunctionCollapseOptionCustom> ValidTileOptions;

    // ���� �Լ�: t01 �Ǵ� t01-01�� ���
    auto IsAllowedTile = [](const FWaveFunctionCollapseOptionCustom& Option) -> bool
        {
            FString Name = Option.BaseObject.ToString();
            return Name.Contains("t01") || Name.Contains("t01-01");
        };


    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];

            if ((Option.bIsCorridorTile || Option.bIsRoomTile) && IsAllowedTile(Option))
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                ValidTilePositions.Add(TilePosition);
                ValidTileOptions.Add(Option);
            }
        }
    }

    if (ValidTilePositions.Num() == 0) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const float WallOffset = TileSize * 0.4f;

    // ��/�� �� ���̱�
    const TArray<TPair<FVector, float>> NorthSouthConfigs = {
        { FVector(0, -WallOffset, 0), 0.f },     // ���� ��, ���� �ٶ�
        { FVector(0, +WallOffset, 0), 180.f }    // ���� ��, ���� �ٶ�
    };

    // ��/�� �� ���̱� (t01-01 Ÿ�ϸ�)
    const TArray<TPair<FVector, float>> EastWestConfigs = {
        { FVector(-WallOffset, 0, 0), 90.f },    // ���� ��, ���� �ٶ�
        { FVector(+WallOffset, 0, 0), 270.f }    // ���� ��, ���� �ٶ�
    };

    for (int32 i = 0; i < 30; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidTilePositions.Num() - 1);
        FVector TileCenter = ValidTilePositions[RandomIndex];
        const FWaveFunctionCollapseOptionCustom& Option = ValidTileOptions[RandomIndex];

        FString TileMeshName = Option.BaseObject.ToString();
        bool bIsHorizontal = TileMeshName.Contains("t01-01");

        const TArray<TPair<FVector, float>>& Configs = bIsHorizontal ? EastWestConfigs : NorthSouthConfigs;

        int32 ConfigIndex = FMath::RandRange(0, Configs.Num() - 1);
        FVector FinalOffset = Configs[ConfigIndex].Key;
        float FinalYaw = Configs[ConfigIndex].Value;

        FVector SpawnLocation = TileCenter + FinalOffset + FVector(0, 0, 5);
        FRotator SpawnRotation = FRotator(0.f, FinalYaw, 0.f);

        if (!CanSpawnAtLocation(World, SpawnLocation, 4.0f)) // �� �������� ���� ���� ����� �°�
        {
            UE_LOG(LogTemp, Warning, TEXT("Spawn blocked at %s"), *SpawnLocation.ToString());
            continue; // �ǳʶٱ�
        }

        FActorSpawnParameters Params;
        World->SpawnActor<ATreasureChest>(TreasureChestClass, SpawnLocation, SpawnRotation, Params);

        UE_LOG(LogTemp, Log, TEXT("Treasure Chest spawned at %s with rotation %.1f (tile: %s)"),
            *SpawnLocation.ToString(), FinalYaw, *TileMeshName);
    }
}

bool Awfcex::CanSpawnAtLocation(UWorld* World, const FVector& Location, float Radius)
{
    if (!World) return false;

    FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    //Params.bReturnPhysicalMaterial = false;

    // ���� ������Ʈ �浹 �˻� (��: ����, �� ��)
    return !World->OverlapBlockingTestByChannel(
        Location,
        FQuat::Identity,
        ECC_WorldStatic,
        Shape,
        Params
    );
}


