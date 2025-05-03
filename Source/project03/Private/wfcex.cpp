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
	ExecuteWFCInSubsystem(90, 1966419713); //테스트용 시드 1967664897, 1094396673, 테스트01: 1172835073, 1966419713, 984042241, 1925703041, 1435413505
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
    // WFC 서브시스템 가져오기
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
    // 현재 월드의 GameInstance를 통해 서브시스템 가져오기
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
    // WFC 서브시스템 가져오기
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (!WFCSubsystem || !WFCSubsystem->WFCModel)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WFCSubsystem or WFCModel"));
        return;
    }

    // CollapseCustom 실행 후 내부 타일 데이터 직접 참조
    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles; // CollapseCustom 실행 후 타일 데이터
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> CorridorTilePositions;

    // 복도 타일 위치 수집
    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];

            // 복도 타일인지 확인
            if (Option.bIsCorridorTile) // 복도 타일 조건 확인
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                CorridorTilePositions.Add(TilePosition);

                // 로그로 위치 출력
                UE_LOG(LogTemp, Log, TEXT("Corridor tile found at location: %s"), *TilePosition.ToString());
            }
        }
    }

    if (CorridorTilePositions.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No corridor tiles available for player spawn."));
        return;
    }

    // 로그로 수집된 복도 타일 위치 출력
    UE_LOG(LogTemp, Log, TEXT("Collected %d corridor tile positions."), CorridorTilePositions.Num());

    // 랜덤 복도 타일 선택
    int32 RandomIndex = FMath::RandRange(0, CorridorTilePositions.Num() - 1);
    FVector SpawnLocation = CorridorTilePositions[RandomIndex];

    // 플레이어 스폰
    SpawnPlayerAtLocation(SpawnLocation);
}

void Awfcex::SpawnPlayerAtLocation(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (World)
    {
        //**C++ 캐릭터 직접 설정 (기본 C++ 클래스)**
        if (!PlayerClass)
        {
            PlayerClass = AMyDCharacter::StaticClass();
        }

        // Z축 살짝 올려서 바닥과 충돌 방지
        FVector AdjustedLocation = Location + FVector(0.0f, 0.0f, 50.0f);

        // 플레이어 스폰
        FActorSpawnParameters SpawnParams;
        AMyDCharacter* SpawnedPlayer = World->SpawnActor<AMyDCharacter>(PlayerClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams);

        if (SpawnedPlayer)
        {
            UE_LOG(LogTemp, Log, TEXT("olmkmknunwn: %s"), *AdjustedLocation.ToString());

            //컨트롤러 확인
            APlayerController* PlayerController = World->GetFirstPlayerController();
            if (PlayerController)
            {
                UE_LOG(LogTemp, Log, TEXT("Found PlayerController: %s"), *PlayerController->GetName());

                //강제로 Possess 시도
                PlayerController->Possess(SpawnedPlayer);

                //Possession 성공 여부 확인
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
        FVector SpawnLocation = ValidCorridorPositions[RandomIndex] + FVector(0.f, 0.f, 50.f); // Z축 올려서 바닥 충돌 방지

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

        // 고정 타일 등록
        WFCSubsystem->UserFixedOptions.Add(Coord, Option);
        UE_LOG(LogTemp, Warning, TEXT("Fixed Regenerator Tile at (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);

        // 오브젝트 스폰
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

    // 중복 방지: 재생성 오브젝트가 위치한 좌표는 제외
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
        ValidIndices.RemoveAt(RandIdx); // 중복 방지

        FVector SpawnLocation = RoomTilePositions[SelectedIndex] + FVector(0, 0, 50); // 약간 위로

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

            // 복도 또는 방 타일 모두 허용
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
    const float ClampMargin = 50.f; // 여유 공간
    const FVector2D TileHalfSize = FVector2D(TileSize * 0.5f, TileSize * 0.5f);

    for (int32 i = 0; i < 30; ++i)
    {
        int32 RandomIndex = FMath::RandRange(0, ValidTilePositions.Num() - 1);
        FVector BaseLocation = ValidTilePositions[RandomIndex];

        FVector2D RandomOffset = FMath::RandPointInCircle(MaxOffsetRadius);

        // Clamp: 타일 경계 넘지 않게 조정
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