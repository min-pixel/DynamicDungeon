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

    // WFC 완료 상태를 주기적으로 체크
    GetWorld()->GetTimerManager().SetTimer(WFCCheckTimer, this, &ADynamicDungeonModeBase::CheckWFCCompletion, 0.5f, true);

}

void ADynamicDungeonModeBase::CheckWFCCompletion()
{
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();

    if (WFCSubsystem && WFCSubsystem->bWFCCompleted)
    {
        UE_LOG(LogTemp, Log, TEXT("WFC completed! Preparing player spawn points..."));

        // WFC 완료되면 타이머 정리
        GetWorld()->GetTimerManager().ClearTimer(WFCCheckTimer);

        // PlayerStart들을 미리 생성
        PreparePlayerStarts();

        // 대기 중인 플레이어들 처리
        ProcessPendingPlayers();

        return;
    }

    WFCCheckAttempts++;
    if (WFCCheckAttempts >= MaxWFCCheckAttempts)
    {
        UE_LOG(LogTemp, Error, TEXT("WFC completion check timeout! Using fallback spawn logic."));
        GetWorld()->GetTimerManager().ClearTimer(WFCCheckTimer);

        // 대기 중인 플레이어들을 폴백 방식으로 처리
        ProcessPendingPlayersWithFallback();
    }
}

void ADynamicDungeonModeBase::PreparePlayerStarts()
{
    TArray<FVector> CorridorLocations = GetAllCorridorLocations();

    // 최소 4개의 스폰 포인트 확보
    int32 NumSpawnPoints = FMath::Max(4, CorridorLocations.Num() / 3);

    for (int32 i = 0; i < NumSpawnPoints && i < CorridorLocations.Num(); i++)
    {
        FVector SpawnLocation = CorridorLocations[i];

        // 스폰 위치 유효성 검사
        if (IsValidSpawnLocation(SpawnLocation))
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

            AActor* PlayerStart = GetWorld()->SpawnActor<APlayerStart>(
                APlayerStart::StaticClass(),
                SpawnLocation,
                FRotator::ZeroRotator,
                SpawnParams
            );

            if (PlayerStart)
            {
                CachedPlayerStarts.Add(PlayerStart);
                UE_LOG(LogTemp, Log, TEXT("Created PlayerStart at: %s"), *SpawnLocation.ToString());
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Prepared %d PlayerStart actors"), CachedPlayerStarts.Num());
}

TArray<FVector> ADynamicDungeonModeBase::GetAllCorridorLocations() const
{
    TArray<FVector> CorridorLocations;

    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
    if (!WFCSubsystem || !WFCSubsystem->WFCModel)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get WFCSubsystem or WFCModel"));
        return CorridorLocations;
    }

    const TArray<FWaveFunctionCollapseTileCustom>& Tiles = WFCSubsystem->LastCollapsedTiles;
    FIntVector Resolution = WFCSubsystem->Resolution;
    float TileSize = WFCSubsystem->WFCModel->TileSize;

    for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
    {
        if (Tiles[TileIndex].RemainingOptions.Num() > 0)
        {
            const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
            if (Option.bIsCorridorTile)
            {
                FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;
                TilePosition.Z += 100.0f; // 살짝 띄움
                CorridorLocations.Add(TilePosition);
            }
        }
    }

    return CorridorLocations;
}

bool ADynamicDungeonModeBase::IsValidSpawnLocation(const FVector& Location) const
{
    // 기본 유효성 검사
    if (Location.IsZero())
        return false;

    // 충돌 검사
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(this);

    FHitResult HitResult;
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Location + FVector(0, 0, 200),
        Location - FVector(0, 0, 200),
        ECC_WorldStatic,
        QueryParams
    );

    // 바닥이 있는지 확인
    return bHit && HitResult.ImpactPoint.Z < Location.Z;
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

    // WFC가 완료되지 않았으면 대기열에 추가
    if (!WFCSubsystem || !WFCSubsystem->bWFCCompleted)
    {
        UE_LOG(LogTemp, Warning, TEXT("WFC not completed, adding player to pending queue"));

        FPendingPlayerInfo PendingInfo;
        PendingInfo.Controller = Player;
        PendingInfo.IncomingName = IncomingName;
        PendingInfo.RetryCount = 0;

        PendingPlayers.Add(PendingInfo);

        // 임시 위치에 스폰 (나중에 이동시킬 예정)
        return CreateTemporaryPlayerStart(FVector::ZeroVector);
    }

    // 캐시된 PlayerStart 사용
    if (CachedPlayerStarts.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, CachedPlayerStarts.Num() - 1);
        AActor* SelectedStart = CachedPlayerStarts[RandomIndex];

        if (IsValid(SelectedStart))
        {
            UE_LOG(LogTemp, Log, TEXT("Using cached PlayerStart at: %s"), *SelectedStart->GetActorLocation().ToString());
            return SelectedStart;
        }
    }

    // 캐시가 없으면 즉시 생성
    FVector SpawnLocation = GetRandomCorridorLocation();
    if (SpawnLocation.IsZero())
    {
        UE_LOG(LogTemp, Error, TEXT("No valid spawn location found, using fallback"));
        SpawnLocation = GetFallbackSpawnLocation();
    }

    return CreateTemporaryPlayerStart(SpawnLocation);
}

AActor* ADynamicDungeonModeBase::CreateTemporaryPlayerStart(const FVector& Location)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* StartSpot = GetWorld()->SpawnActor<APlayerStart>(
        APlayerStart::StaticClass(),
        Location,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (StartSpot)
    {
        UE_LOG(LogTemp, Log, TEXT("Created temporary PlayerStart at: %s"), *Location.ToString());
        return StartSpot;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to create temporary PlayerStart, using Super"));
    return Super::FindPlayerStart_Implementation(nullptr, FString());
}

void ADynamicDungeonModeBase::ProcessPendingPlayers()
{
    for (const FPendingPlayerInfo& PendingInfo : PendingPlayers)
    {
        if (IsValid(PendingInfo.Controller))
        {
            // 플레이어 재시작
            RestartPlayer(PendingInfo.Controller);
            UE_LOG(LogTemp, Log, TEXT("Restarted pending player"));
        }
    }

    PendingPlayers.Empty();
}

void ADynamicDungeonModeBase::ProcessPendingPlayersWithFallback()
{
    for (const FPendingPlayerInfo& PendingInfo : PendingPlayers)
    {
        if (IsValid(PendingInfo.Controller))
        {
            // 폴백 위치에서 플레이어 스폰
            FVector FallbackLocation = GetFallbackSpawnLocation();
            AActor* FallbackStart = CreateTemporaryPlayerStart(FallbackLocation);

            if (FallbackStart)
            {
                RestartPlayer(PendingInfo.Controller);
                UE_LOG(LogTemp, Warning, TEXT("Restarted player at fallback location: %s"), *FallbackLocation.ToString());
            }
        }
    }

    PendingPlayers.Empty();
}

FVector ADynamicDungeonModeBase::GetFallbackSpawnLocation() const
{
    // 여러 후보 위치 시도
    TArray<FVector> FallbackLocations = {
        FVector(0, 0, 200),
        FVector(500, 0, 200),
        FVector(0, 500, 200),
        FVector(-500, 0, 200),
        FVector(0, -500, 200)
    };

    for (const FVector& Location : FallbackLocations)
    {
        if (IsValidSpawnLocation(Location))
        {
            return Location;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("All fallback locations invalid, using (0,0,200)"));
    return FVector(0, 0, 200);
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

    // WFC 완료 상태에 따라 딜레이 조정
    float RestartDelay = 1.0f;
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetGameInstance()->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
    if (!WFCSubsystem || !WFCSubsystem->bWFCCompleted)
    {
        RestartDelay = 5.0f; // WFC 미완료시 더 긴 딜레이
    }

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
        }), RestartDelay, false);
}

void ADynamicDungeonModeBase::ForceRestartAllPlayers()
{
    RestartAllPlayers();
}