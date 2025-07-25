// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "MyDCharacter.h"
#include "EnemyCharacter.h"
#include "RageEnemyCharacter.h"
#include "WFCRegenerator.h"
#include "TreasureChest.h"
#include "EscapeObject.h"
#include "wfcex.generated.h"

UCLASS()
class PROJECT03_API Awfcex : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	Awfcex();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// 실행 메서드
	UFUNCTION(BlueprintCallable, Category = "WFC")
	void ExecuteWFCInSubsystem(int32 TryCount, int32 RandomSeed);

	// 복도 타일 위에 플레이어 스폰
	void SpawnPlayerOnCorridor();


	// 지정된 위치에 플레이어 스폰
	void SpawnPlayerAtLocation(const FVector& Location);

	UPROPERTY(EditAnywhere, Category = "WFC")
	UWaveFunctionCollapseModel02* WFCModelAsset;

	UPROPERTY(EditAnywhere, Category = "WFC | CookKeepAlive")
	TArray<TSoftObjectPtr<UObject>> ReferencedTileAssets;

	// WFC 서브시스템에 접근
	UWaveFunctionCollapseSubsystem02* GetWFCSubsystem();

	// 플레이어 클래스
	UPROPERTY(EditAnywhere, Category = "Player")
	TSubclassOf<AMyDCharacter> PlayerClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<AEnemyCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<ARageEnemyCharacter> RageEnemyClass;

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnEnemiesOnCorridor(int32 EnemyCount);

	UPROPERTY(EditAnywhere, Category = "WFC Regenerator")
	TSubclassOf<AWFCRegenerator> WFCRegeneratorClass;

	
	UFUNCTION(BlueprintCallable, Category = "WFC")
	void SpawnWFCRegeneratorOnRoom();

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class AEscapeObject> EscapeObjectClass;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class ATreasureChest> TreasureChestClass;

	void SpawnEscapeObjectsOnRoom();
	void SpawnTreasureChestsOnTiles();

	bool CanSpawnAtLocation(UWorld* World, const FVector& Location, float Radius);

	// 실험 환경 정리 함수
	void CleanupBeforeExperiment();

	void SetBirdEyeView();

	
	static int32 CurrentExperimentIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experiment")
	bool bAutoIncrement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Experiment")
	bool bExperimentCompleted = false;
};
