// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "GameFramework/PlayerStart.h"
#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"


ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ 클래스는 StaticClass() 사용 (블루프린트가 아니므로 경로 찾기 불필요)
	DefaultPawnClass = AMyDCharacter::StaticClass();

	//DefaultPawnClass = nullptr;

	//플레이어 컨트롤러가 없으면 움직일 수 없으므로 명시적으로 설정
	//PlayerControllerClass = APlayerController::StaticClass();

    PlayerControllerClass = AMyPlayerController::StaticClass();

}

void ADynamicDungeonModeBase::BeginPlay()
{
	Super::BeginPlay(); // 반드시 호출

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = false; // 마우스 커서 숨기기
		FInputModeGameOnly InputMode; // 게임 전용 입력 모드
		PC->SetInputMode(InputMode);
	}
}

FVector ADynamicDungeonModeBase::GetRandomCorridorLocation() const
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WFCSubsystem or WFCModel"));
        return FVector::ZeroVector;
    }

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    TArray<FVector> CorridorTilePositions;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
            if (Option.bIsCorridorTile)
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                CorridorTilePositions.Add(TilePosition);
            }
        }
    }

    if (CorridorTilePositions.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No corridor tiles found! Using default (0,0,0)."));
        return FVector::ZeroVector;
    }

    int32 RandomIndex = FMath::RandRange(0, CorridorTilePositions.Num() - 1);
    return CorridorTilePositions[RandomIndex] + FVector(0.0f, 0.0f, 100.0f); // 살짝 띄움
}

AActor* ADynamicDungeonModeBase::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();

    FVector SpawnLocation = FVector::ZeroVector; // 기본값 (임시)

    if (WFCSubsystem && WFCSubsystem->bWFCCompleted)
    {
        // Collapse 후 복도 위치
        SpawnLocation = GetRandomCorridorLocation();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WFC not completed yet, using fallback (0,0,0)"));
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

    if (StartSpot)
    {
        UE_LOG(LogTemp, Log, TEXT("PlayerStart at: %s"), *SpawnLocation.ToString());
        return StartSpot;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn StartSpot, fallback Super"));
        return Super::FindPlayerStart_Implementation(Player, IncomingName);
    }
}

void ADynamicDungeonModeBase::RestartAllPlayers()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 모든 플레이어를 일단 정리
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->GetPawn())
        {
            PC->GetPawn()->Destroy();
            PC->UnPossess();
        }
    }

    // 1초 후 모든 플레이어를 재시작
    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]()
        {
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                APlayerController* PC = It->Get();
                if (PC)
                {
                    RestartPlayer(PC);
                }
            }
        }), 1.0f, false);
}

void ADynamicDungeonModeBase::ForceRestartAllPlayers()
{
    RestartAllPlayers();
}