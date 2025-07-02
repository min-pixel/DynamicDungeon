// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "GameFramework/PlayerStart.h"
#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"


ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ Ŭ������ StaticClass() ��� (�������Ʈ�� �ƴϹǷ� ��� ã�� ���ʿ�)
	DefaultPawnClass = AMyDCharacter::StaticClass();

	//DefaultPawnClass = nullptr;

	//�÷��̾� ��Ʈ�ѷ��� ������ ������ �� �����Ƿ� ��������� ����
	//PlayerControllerClass = APlayerController::StaticClass();

    PlayerControllerClass = AMyPlayerController::StaticClass();

}

void ADynamicDungeonModeBase::BeginPlay()
{
	Super::BeginPlay(); // �ݵ�� ȣ��

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = false; // ���콺 Ŀ�� �����
		FInputModeGameOnly InputMode; // ���� ���� �Է� ���
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
    return CorridorTilePositions[RandomIndex] + FVector(0.0f, 0.0f, 100.0f); // ��¦ ���
}

AActor* ADynamicDungeonModeBase::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
    FVector SpawnLocation = GetRandomCorridorLocation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // �ݵ�� APlayerStart�� ����
    AActor* StartSpot = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

    if (StartSpot)
    {
        UE_LOG(LogTemp, Log, TEXT("Custom PlayerStart at: %s"), *SpawnLocation.ToString());
        return StartSpot;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn PlayerStart, using Super."));
        return Super::FindPlayerStart_Implementation(Player, IncomingName);
    }
}

void ADynamicDungeonModeBase::RestartAllPlayers()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && !PC->GetPawn())
        {
            RestartPlayer(PC);
        }
    }
}