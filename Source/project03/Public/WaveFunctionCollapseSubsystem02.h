// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
//#if WITH_EDITOR
//#include "EditorSubsystem.h"
//#endif
#include "WaveFunctionCollapseModel02.h"
#include "Components/ActorComponent.h"
#include "WaveFunctionCollapseSubsystem02.generated.h"

PROJECT03_API DECLARE_LOG_CATEGORY_EXTERN(LogWFC, Log, All);



UCLASS()
class PROJECT03_API UWaveFunctionCollapseSubsystem02 : public UGameInstanceSubsystem
{
	GENERATED_BODY()



public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	TObjectPtr<UWaveFunctionCollapseModel02> WFCModel;

	// Tile 종류 → 원본 액터 20250531
	TMap<FWaveFunctionCollapseOptionCustom, AActor*> TilePrefabPool;

	// WFC 완료 여부를 나타내는 플래그
	UPROPERTY(BlueprintReadOnly, Category = "WFC")
	bool bWFCCompleted = false;

	void PrepareTilePrefabPool(UWorld* World);

	void ReuseTilePrefabsFromPool(const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* World);

	void SetExecutionContext(UObject* InContext) { ExecutionContext = InContext; }

	// WFC 실행 함수
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	void ExecuteWFC(int32 TryCount = 1, int32 RandomSeed = 0, UWorld* WorldContext = nullptr);

	// WFC 모델 설정 메서드
	void SetWFCModel();

	void PrecomputeMapAsync(int32 TryCount, int32 RandomSeed, TFunction<void()> OnCompleted);

	// 캐시된 결과 저장
	
	bool bHasCachedTiles = false;

	// Subsystem 초기화 메서드
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	FIntVector Resolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	FVector OriginLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	FRotator Orientation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	bool bUseEmptyBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WFCSettings")
	TMap<FIntVector, FWaveFunctionCollapseOptionCustom> StarterOptions;

	TArray<int32> GetAdjacentIndices(int32 TileIndex, FIntVector Resolution);

	TArray<int32> GetCardinalAdjacentIndices(int32 TileIndex, FIntVector GridResolution);

	void RemoveIsolatedCorridorTiles(TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	void FloodFillCorridors(
		int32 StartIndex,
		const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		TSet<int32>& OutGroup,
		TSet<int32>& VisitedTiles);

	void RemoveDisconnectedCorridors(TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	// CollapseCustom 실행 후 생성된 타일 데이터를 저장
	const TArray<FWaveFunctionCollapseTileCustom>& GetLastCollapsedTiles() const;

	// CollapseCustom 실행 후 타일 데이터를 저장
	TArray<FWaveFunctionCollapseTileCustom> LastCollapsedTiles;

	void AdjustRoomTileBasedOnCorridors(int32 TileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles);
	

	void RotateRoomTile(FWaveFunctionCollapseOptionCustom& RoomTileOption, const FString& Direction);

	int32 FindClosestCorridor(int32 StartIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution);

	TArray<int32> FindPathAStar(int32 StartIndex, int32 GoalIndex, int32 PreviousIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution);

	void ConnectIsolatedRooms(TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	void FillEmptyTilesAlongPath(const TArray<int32>& Path, TArray<FWaveFunctionCollapseTileCustom>& Tiles);
	
	TArray<int32>GetRoomBoundaryIndices(int32 RoomIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution);
	bool IsRoomBoundary(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution);
	bool IsInsideRoom(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution);

	TArray<int32>GetAllAdjacentIndices(int32 TileIndex, FIntVector GridResolution);

	TArray<int32>GetTwoStepAdjacentIndices(int32 TileIndex, FIntVector GridResolution);
	void ReplaceGoalTileWithCustomTile(
		int32 GoalTileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles);
	
	void PlaceGoalTileInFrontOfRoom(
		int32 RoomTileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	void RemoveIsolatedTestTiles(TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	bool IsIsolatedTile(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	float GetTileSize() const;

	//다익스트라
	void FindPathDijkstra(
		const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		const TSet<int32>& StartIndices, // 복도 타일들
		TMap<int32, int32>& OutPrevious, // 역추적용 prev[]
		TMap<int32, int32>& OutDistance  // 거리 정보 dist[]
	);

	TArray<int32> BuildPathFromDijkstra(
		int32 TargetIndex,
		const TMap<int32, int32>& Previous
	);

	void ConnectIsolatedRoomsDijkstra(TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	bool IsCorridorTile(const FWaveFunctionCollapseTileCustom& Tile);

	TSet<int32> FindDisconnectedRoomIndices(
		const TSet<int32>& RoomIndices,
		const TSet<int32>& CorridorIndices,
		const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		const FIntVector& Resolution);



	/**
	* Solve a grid using a WFC model.  If successful, spawn an actor.
	* @param TryCount Amount of times to attempt a successful solve
	* @param RandomSeed Seed for deterministic results.  When this value is 0 the seed will be generated. Seed value will be logged during the solve.
	*/
	/*UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	AActor* CollapseCustom(int32 TryCount = 1, int32 RandomSeed = 0);*/

	//20250419
	AActor* CollapseCustom(int32 TryCount, int32 RandomSeed, UWorld* WorldContext);

	//UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	//AActor* CollapseCustom002(int32 TryCount, int32 RandomSeed);



	// 새로운 함수 선언 추가
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	void SpawnBorderBlueprints();

	TMap<FIntVector, FWaveFunctionCollapseOptionCustom> UserFixedOptions;

	FIntVector RegeneratorFixedTileCoord;
	FWaveFunctionCollapseOptionCustom RegeneratorFixedTileOption;
	bool bHasRegeneratorFixedTile = false;


	/**
	* Initialize WFC process which sets up Tiles and RemainingTiles arrays
	* Pre-populates Tiles with StarterOptions, BorderOptions and InitialTiles
	* @param Tiles Array of tiles (by ref)
	* @param RemainingTiles Array of remaining tile indices.  Semi-sorted: Min Entropy tiles at the front, the rest remains unsorted (by ref)
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	void InitializeWFC(TArray<FWaveFunctionCollapseTileCustom>& Tiles, TArray<int32>& RemainingTiles);

	/**
	* Observation phase:
	* This process randomly selects one tile from minimum entropy tiles
	* then randomly selects a valid option for that tile
	* @param Tiles Array of tiles (by ref)
	* @param RemainingTiles Array of remaining tile indices.  Semi-sorted: Min Entropy tiles at the front, the rest remains unsorted (by ref)
	* @param ObservationQueue Array to store tiles that need to be checked whether remaining options are affected during propagation phase (by ref)
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	bool Observe(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		TArray<int32>& RemainingTiles,
		TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
		int32 RandomSeed);

	/**
	* Propagation phase:
	* This process checks if the selection made during the observation is valid by checking constraint validity with neighboring tiles.
	* Neighboring tiles may reduce their remaining options to include only valid options.
	* If the remaining options of a tile were modified, the neighboring tiles of the modified tile will be added to a queue.
	* During this process, if any contradiction (a tile with zero remaining options) is encountered, the current solve will fail.
	* @param Tiles Array of tiles (by ref)
	* @param RemainingTiles Array of remaining tile indices.  Semi-sorted: Min Entropy tiles at the front, the rest remains unsorted (by ref)
	* @param ObservationQueue Array to store tiles that need to be checked whether remaining options are affected (by ref)
	* @param PropagationCount Counter for propagation passes
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	bool Propagate(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		TArray<int32>& RemainingTiles,
		TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
		int32& PropagationCount);

	/**
	* Recursive Observation and Propagation cycle
	* @param Tiles Array of tiles (by ref)
	* @param RemainingTiles Array of remaining tile indices (by ref)
	* @param ObservationQueue Array to store tiles that need to be checked whether remaining options are affected (by ref)
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	bool ObservationPropagation(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
		TArray<int32>& RemainingTiles,
		TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
		int32 RandomSeed);

	/**
	* Derive grid from the bounds of an array of transforms
	* Assumptions:
	*	-Transforms can only represent a single grid
	*   -Sets empty starter option if there is a valid grid position with no transform
	*   -Orientation is determined by the yaw of the first transform in the array
	* @param Transforms Array of transforms (by ref)
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	void DeriveGridFromTransformBounds(const TArray<FTransform>& Transforms);

	/**
	* Derive grid from an array of transforms
	* Assumptions:
	*   -Every transform represents the center point of a tile position
	*   -Sets empty starter option if there is a valid grid position with no transform
	*   -Orientation is determined by the yaw of the first transform in the array
	* @param Transforms Array of transforms (by ref)
	*/
	UFUNCTION(BlueprintCallable, Category = "WFCFunctions")
	void DeriveGridFromTransforms(const TArray<FTransform>& Transforms);

	TOptional<FIntVector>  PostProcessFixedRoomTileAt(const FIntVector& Coord, TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	void AdjustRoomTileFacingDirection(int32 TileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles, const FIntVector& CorridorDirection);

private:

	TArray<FVector> RoomTilePositions; // 방 타일의 월드 좌표를 저장

	TWeakObjectPtr<UObject> ExecutionContext;

	/**
	* Builds the Initial Tile which is a tile containing all possible options
	* @param InitialTile The Initial Tile (by ref)
	*/
	bool BuildInitialTile(FWaveFunctionCollapseTileCustom& InitialTile);

	/**
	* Get valid options for a border tile
	* @param Position Position of border tile
	* @param TmpInitialOptions Initial options which should contain all possible options
	*/
	TArray<FWaveFunctionCollapseOptionCustom> GetInnerBorderOptions(FIntVector Position, const TArray<FWaveFunctionCollapseOptionCustom>& InitialOptions);

	/**
	* Used in GetBorderOptions to gather invalid options for border tiles that should be removed from the InitialOptions set
	* @param Adjacency Direction from Exterior Border to Inner Border
	* @param InInitialOptions This should be the InitialOptions
	* @param OutBorderOptionsToRemove Array containing gathered options to remove
	*/
	void GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom Adjacency, const TArray<FWaveFunctionCollapseOptionCustom>& InitialOptions, TArray<FWaveFunctionCollapseOptionCustom>& OutBorderOptionsToRemove);

	/**
	* Checks if a position is an inner border
	* @param Position
	*/
	bool IsPositionInnerBorder(FIntVector Position);

	/**
	* Used in Observe and Propagate to add adjacent indices to a queue
	* @param CenterIndex Index of the center object
	* @param RemainingTiles Used to check if index still remains in RemainingTiles
	* @param OutQueue Queue to add indices to
	*/
	void AddAdjacentIndicesToQueue(int32 CenterIndex, const TArray<int32>& RemainingTiles, TMap<int32, FWaveFunctionCollapseQueueElementCustom>& OutQueue);

	/**
	* Add an Instance Component with a given name
	* @param Actor Actor to add component to
	* @param ComponentClass
	* @param ComponentName
	*/
	UActorComponent* AddNamedInstanceComponent(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName);

	/**
	* Spawn an actor given an array of successfully solved tiles
	* @param Tiles Successfully solved array of tiles
	*/
	AActor* SpawnActorFromTiles(const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* WorldContext);

	/**
	* Returns true if no remaining options in given tiles are an empty/void option or included in the SpawnExclusion list
	* @param Tiles Successfully solved array of tiles
	*/
	bool AreAllTilesNonSpawnable(const TArray<FWaveFunctionCollapseTileCustom>& Tiles);

	void SetTileNetworkPriority(AActor* TileActor, const FVector& TilePosition);

protected:
		UWorld* GetEffectiveWorld() const
		{
			if (ExecutionContext.IsValid())
			{
				return ExecutionContext->GetWorld();
			}
			return nullptr;
		}

};