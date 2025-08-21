// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGraphAnalyzer.h"
#include "BSPMapGenerator02.generated.h"

USTRUCT()
struct FBSPNode02
{
    GENERATED_BODY()

    FIntVector Min;
    FIntVector Max;

    TSharedPtr<FBSPNode02> LeftChild;
    TSharedPtr<FBSPNode02> RightChild;

    bool bIsLeaf = false;
    bool bHasRoom = false;

    // 실제 방 영역
    FIntVector RoomMin;
    FIntVector RoomMax;

    FBSPNode02() {}

    FBSPNode02(const FIntVector& InMin, const FIntVector& InMax)
        : Min(InMin), Max(InMax), bIsLeaf(true) {}
};

UENUM(BlueprintType)
enum class ETileType02 : uint8
{
    Empty,
    Room,
    Corridor,
    Door,
    BridgeCorridor,
    DeadEnd,        // 막다른 길 추가
    Junction,       // 갈림길 추가
    CrossRoad       // 교차로 추가
};

// 맵 통계 구조체 추가
USTRUCT(BlueprintType)
struct FMapStatistics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 RoomCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CorridorCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DeadEndCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 JunctionCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CrossRoadCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DoorCount = 0;

    UPROPERTY(BlueprintReadOnly)
    float TotalCorridorLength = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageRoomSize = 0;

    FMapStatistics() {}
};

UCLASS()
class PROJECT03_API ABSPMapGenerator02 : public AActor
{
    GENERATED_BODY()

public:
    ABSPMapGenerator02();

    // BSP 설정
    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    FIntVector MapSize = FIntVector(60, 60, 1);

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MinRoomSize = 6;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MaxRoomSize = 15;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MinNodeSize = 12;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    float SplitRatio = 0.45f;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MaxDepth = 5;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    float TileSize = 500.0f;

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 RandomSeed = 0;

    // 타일 클래스들
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomNorthClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomSouthClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomEastClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomWestClass;

    // 문 클래스
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> DoorClass;

    // 벽 클래스 추가
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> WallClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorHorizontalClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorVerticalClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorCornerClass;

    //20250816
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> DeadEndClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> JunctionClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CrossRoadClass;

    // 추가 연결 설정
    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtraConnectionChance = 0.3f;  // 추가 연결 생성 확률

    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0", ClampMax = "20"))
    int32 MinExtraConnections = 3;  // 최소 추가 연결 수

    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0", ClampMax = "50"))
    int32 MaxExtraConnections = 10;  // 최대 추가 연결 수

    UPROPERTY(EditAnywhere, Category = "Maze Settings")
    float MaxConnectionDistance = 20.0f;  // 추가 연결 최대 거리 (타일 단위)

    UPROPERTY(EditAnywhere, Category = "Maze Settings")
    bool bCreateLoops = true;  // 순환 경로 생성 여부

    // 통계 표시 옵션
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bShowStatistics = true;

    // 맵 통계 (읽기 전용)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
    FMapStatistics MapStats;


    UFUNCTION(BlueprintCallable, Category = "BSP")
    void GenerateBSPMap();

    UFUNCTION(BlueprintCallable, Category = "BSP")
    void ClearMap();

    // 통계 관련 함수 20250816
    UFUNCTION(BlueprintCallable, Category = "Statistics")
    FMapStatistics AnalyzeMap();

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    void PrintMapStatistics();

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    int32 GetDeadEndCount() const { return MapStats.DeadEndCount; }

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    int32 GetJunctionCount() const { return MapStats.JunctionCount; }

    UFUNCTION(BlueprintCallable, Category = "Statistics")
    int32 GetRoomCount() const { return MapStats.RoomCount; }

    // 그래프 분석기
    UPROPERTY()
    UDungeonGraphAnalyzer* GraphAnalyzer;

protected:
    virtual void BeginPlay() override;

private:

    // 그래프 분석용 구조체
    struct GraphNode
    {
        FIntVector Position;
        ETileType02 Type;
        TArray<int32> Edges; // 연결된 다른 노드들의 인덱스
        int32 RoomId; // 방인 경우 어느 방에 속하는지 (-1이면 방 아님)
    };

    struct GraphEdge
    {
        int32 StartNode;
        int32 EndNode;
        int32 Length; // 복도 길이
        TArray<FIntVector> Path;
    };

    // 그래프 분석 함수들
    void BuildGraphFromMap();
    void CalculateCyclomaticComplexity();
    int32 CountGraphComponents();
    void DFSComponent(int32 NodeIndex, TArray<bool>& Visited);
    bool IsNodeTile(int32 x, int32 y);
    FIntVector FindRoomCenter(int32 RoomId);
    bool FindCorridorPath(const FIntVector& Start, const FIntVector& End, TArray<FIntVector>& OutPath);
    TArray<TPair<int32, int32>> ExtraConnectionPairs;

    bool IsGraphPassable(int32 x, int32 y) const;

    void CleanupParallelCorridors();

    bool WouldCreateLongParallel(const FIntVector& Start, const FIntVector& End, float Tolerance);

    bool WouldCreateParallelCorridor(const FIntVector& Start, const FIntVector& End, float MinDistance);
    bool CheckLShapePathForParallel(const FIntVector& Start, const FIntVector& End, bool bHorizontalFirst, float MinDistance);
    bool HasParallelInRange(int32 start, int32 end, int32 fixed, bool bHorizontal, float MinDistance);
    bool IsValidPosition(int32 x, int32 y) const;
    void CreateLShapedCorridorSafe(const FIntVector& Start, const FIntVector& End);
    void CreateCorridorStopAtContact(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB);
    void CollapseThickCorridorBlobsFavorDoor();
    void PatchCorridorSingleTileGaps();



    // 그래프 데이터
    TArray<GraphNode> GraphNodes;
    TArray<GraphEdge> GraphEdges;
    TMap<FIntVector, int32> NodePositionToIndex;


    TSharedPtr<FBSPNode02> RootNode;
    TArray<TSharedPtr<FBSPNode02>> LeafNodes;
    TArray<TArray<ETileType02>> TileMap;
    FRandomStream RandomStream;

    // BSP 트리 생성
    TSharedPtr<FBSPNode02> CreateBSPTree(const FIntVector& Min, const FIntVector& Max, int32 Depth = 0);

    // 노드 분할
    bool SplitNode(TSharedPtr<FBSPNode02> Node, int32 Depth);

    // 리프 노드에 방 생성
    void CreateRooms();

    // 방들을 복도로 연결
    void ConnectRooms();

    // 두 방 사이에 복도 생성
    void CreateCorridor(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB);

    // L자 복도 생성
    void CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End);

    // 타일 맵을 실제 액터로 스폰
    void SpawnTiles();

    // 방의 문 방향 결정
    FString GetRoomDoorDirection(const FIntVector& RoomPos);

    // 리프 노드 수집
    void CollectLeafNodes(TSharedPtr<FBSPNode02> Node);

    // 맵 초기화
    void InitializeTileMap();

    // 타일 배치 가능 여부 확인
    bool CanPlaceTile(const FIntVector& Pos);

    // 방의 중심점 가져오기
    FIntVector GetRoomCenter(TSharedPtr<FBSPNode02> Node);

    FString GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode02> Node);

    // 문 생성 함수
    void SpawnDoorsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor);

    // 벽 생성 함수
    void SpawnWallsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor);

    // 문과 복도 사이의 연결을 보장하는 함수
    void EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Direction, TSharedPtr<FBSPNode02> Node);

    // 단일 복도 타일을 스폰하는 함수 (연결 복도용)
    void SpawnSingleCorridorTile(const FIntVector& TilePos);

    // 복도 연결 방향 확인
    TArray<FString> GetCorridorDirections(TSharedPtr<FBSPNode02> Node);

    // 추가 연결 생성 함수
    void CreateExtraConnections();

    void CreateZigzagCorridor(const FIntVector& Start, const FIntVector& End);

    // 두 방 사이의 거리가 적절한지 확인
    bool IsValidConnectionDistance(int32 RoomA, int32 RoomB);

    // 복도가 이미 존재하는지 확인 (중복 방지)
    bool CorridorExists(const FIntVector& Start, const FIntVector& End);

    //20250816
    void AnalyzeCorridorTypes();
    int32 CountConnections(int32 x, int32 y);
    ETileType02 DetermineCorridorType(int32 x, int32 y);
    bool IsCorridorTile(int32 x, int32 y);
    void DrawDebugVisualization();

    // ===== Retry settings =====
    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MaxGenerateAttempts = 8;      // 실패 시 최대 재시도 횟수

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    bool bReseedOnRetry = true;         // 재시도마다 시드 변경할지

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bMarkRejected2x2 = true;       // 실패 지점 디버그 박스 표시

    // 2x2 복도 블록 존재 여부 검사 (있으면 OutTopLeft에 좌상단 좌표 반환)
    UFUNCTION(BlueprintCallable, Category = "BSP|Validation")
    bool Has2x2CorridorBlob(FIntVector& OutTopLeft);

    // 타일맵 확정 후, 그래프 분석만 따로 수행
    void RunGraphAnalysis();

};