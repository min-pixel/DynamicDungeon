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
	ExecuteWFCInSubsystem(90, 1172835073); //테스트용 시드 1967664897, 1094396673, 1172835073, 1966419713
    SpawnPlayerOnCorridor();

    SpawnEnemiesOnCorridor(15);
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