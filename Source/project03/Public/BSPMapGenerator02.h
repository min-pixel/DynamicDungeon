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

    // ���� �� ����
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
    DeadEnd,        // ���ٸ� �� �߰�
    Junction,       // ������ �߰�
    CrossRoad       // ������ �߰�
};

// �� ��� ����ü �߰�
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

    // BSP ����
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

    // Ÿ�� Ŭ������
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomNorthClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomSouthClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomEastClass;

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomWestClass;

    // �� Ŭ����
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> DoorClass;

    // �� Ŭ���� �߰�
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

    // �߰� ���� ����
    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtraConnectionChance = 0.3f;  // �߰� ���� ���� Ȯ��

    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0", ClampMax = "20"))
    int32 MinExtraConnections = 3;  // �ּ� �߰� ���� ��

    UPROPERTY(EditAnywhere, Category = "Maze Settings", meta = (ClampMin = "0", ClampMax = "50"))
    int32 MaxExtraConnections = 10;  // �ִ� �߰� ���� ��

    UPROPERTY(EditAnywhere, Category = "Maze Settings")
    float MaxConnectionDistance = 20.0f;  // �߰� ���� �ִ� �Ÿ� (Ÿ�� ����)

    UPROPERTY(EditAnywhere, Category = "Maze Settings")
    bool bCreateLoops = true;  // ��ȯ ��� ���� ����

    // ��� ǥ�� �ɼ�
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bShowStatistics = true;

    // �� ��� (�б� ����)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
    FMapStatistics MapStats;


    UFUNCTION(BlueprintCallable, Category = "BSP")
    void GenerateBSPMap();

    UFUNCTION(BlueprintCallable, Category = "BSP")
    void ClearMap();

    // ��� ���� �Լ� 20250816
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

    // �׷��� �м���
    UPROPERTY()
    UDungeonGraphAnalyzer* GraphAnalyzer;

protected:
    virtual void BeginPlay() override;

private:

    // �׷��� �м��� ����ü
    struct GraphNode
    {
        FIntVector Position;
        ETileType02 Type;
        TArray<int32> Edges; // ����� �ٸ� ������ �ε���
        int32 RoomId; // ���� ��� ��� �濡 ���ϴ��� (-1�̸� �� �ƴ�)
    };

    struct GraphEdge
    {
        int32 StartNode;
        int32 EndNode;
        int32 Length; // ���� ����
        TArray<FIntVector> Path;
    };

    // �׷��� �м� �Լ���
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



    // �׷��� ������
    TArray<GraphNode> GraphNodes;
    TArray<GraphEdge> GraphEdges;
    TMap<FIntVector, int32> NodePositionToIndex;


    TSharedPtr<FBSPNode02> RootNode;
    TArray<TSharedPtr<FBSPNode02>> LeafNodes;
    TArray<TArray<ETileType02>> TileMap;
    FRandomStream RandomStream;

    // BSP Ʈ�� ����
    TSharedPtr<FBSPNode02> CreateBSPTree(const FIntVector& Min, const FIntVector& Max, int32 Depth = 0);

    // ��� ����
    bool SplitNode(TSharedPtr<FBSPNode02> Node, int32 Depth);

    // ���� ��忡 �� ����
    void CreateRooms();

    // ����� ������ ����
    void ConnectRooms();

    // �� �� ���̿� ���� ����
    void CreateCorridor(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB);

    // L�� ���� ����
    void CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End);

    // Ÿ�� ���� ���� ���ͷ� ����
    void SpawnTiles();

    // ���� �� ���� ����
    FString GetRoomDoorDirection(const FIntVector& RoomPos);

    // ���� ��� ����
    void CollectLeafNodes(TSharedPtr<FBSPNode02> Node);

    // �� �ʱ�ȭ
    void InitializeTileMap();

    // Ÿ�� ��ġ ���� ���� Ȯ��
    bool CanPlaceTile(const FIntVector& Pos);

    // ���� �߽��� ��������
    FIntVector GetRoomCenter(TSharedPtr<FBSPNode02> Node);

    FString GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode02> Node);

    // �� ���� �Լ�
    void SpawnDoorsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor);

    // �� ���� �Լ�
    void SpawnWallsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor);

    // ���� ���� ������ ������ �����ϴ� �Լ�
    void EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Direction, TSharedPtr<FBSPNode02> Node);

    // ���� ���� Ÿ���� �����ϴ� �Լ� (���� ������)
    void SpawnSingleCorridorTile(const FIntVector& TilePos);

    // ���� ���� ���� Ȯ��
    TArray<FString> GetCorridorDirections(TSharedPtr<FBSPNode02> Node);

    // �߰� ���� ���� �Լ�
    void CreateExtraConnections();

    void CreateZigzagCorridor(const FIntVector& Start, const FIntVector& End);

    // �� �� ������ �Ÿ��� �������� Ȯ��
    bool IsValidConnectionDistance(int32 RoomA, int32 RoomB);

    // ������ �̹� �����ϴ��� Ȯ�� (�ߺ� ����)
    bool CorridorExists(const FIntVector& Start, const FIntVector& End);

    //20250816
    void AnalyzeCorridorTypes();
    int32 CountConnections(int32 x, int32 y);
    ETileType02 DetermineCorridorType(int32 x, int32 y);
    bool IsCorridorTile(int32 x, int32 y);
    void DrawDebugVisualization();

    // ===== Retry settings =====
    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    int32 MaxGenerateAttempts = 8;      // ���� �� �ִ� ��õ� Ƚ��

    UPROPERTY(EditAnywhere, Category = "BSP Settings")
    bool bReseedOnRetry = true;         // ��õ����� �õ� ��������

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bMarkRejected2x2 = true;       // ���� ���� ����� �ڽ� ǥ��

    // 2x2 ���� ��� ���� ���� �˻� (������ OutTopLeft�� �»�� ��ǥ ��ȯ)
    UFUNCTION(BlueprintCallable, Category = "BSP|Validation")
    bool Has2x2CorridorBlob(FIntVector& OutTopLeft);

    // Ÿ�ϸ� Ȯ�� ��, �׷��� �м��� ���� ����
    void RunGraphAnalysis();

};