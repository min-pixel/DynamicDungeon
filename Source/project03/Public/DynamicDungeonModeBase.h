// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicDungeonModeBase.generated.h"

/**
 * 
 */

 // 대기 중인 플레이어 정보 구조체
USTRUCT()
struct FPendingPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY()
	AController* Controller = nullptr;

	UPROPERTY()
	FString IncomingName;

	FTimerHandle RetryTimer;
	int32 RetryCount = 0;

	FPendingPlayerInfo()
	{
		Controller = nullptr;
		IncomingName = TEXT("");
		RetryCount = 0;
	}
};


UCLASS()
class PROJECT03_API ADynamicDungeonModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADynamicDungeonModeBase();

	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

    

	void RestartAllPlayers();

	
	void ForceRestartAllPlayers();

	UFUNCTION()
	void CheckWFCCompletion();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void PreparePlayerStarts();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	TArray<FVector> GetAllCorridorLocations() const;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	FVector GetRandomCorridorLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	bool IsValidSpawnLocation(const FVector& Location) const;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	AActor* CreateTemporaryPlayerStart(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void ProcessPendingPlayers();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void ProcessPendingPlayersWithFallback();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	FVector GetFallbackSpawnLocation() const;

private:

    UPROPERTY()
    TArray<AActor*> CachedPlayerStarts;

    FTimerHandle WFCCheckTimer;
    int32 WFCCheckAttempts = 0;
    static constexpr int32 MaxWFCCheckAttempts = 10;

	UPROPERTY()
    TArray<FPendingPlayerInfo> PendingPlayers;

protected:
	virtual void BeginPlay() override;
	
};
