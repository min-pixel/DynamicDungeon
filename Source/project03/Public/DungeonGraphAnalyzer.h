#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGraphAnalyzer.generated.h"

// Ÿ�� Ÿ�� (�Է¿�)
UENUM(BlueprintType)
enum class EDungeonTileType : uint8
{
    Empty = 0,
    Room = 1,
    Corridor = 2,
    Door = 3,
    Wall = 4,
    BridgeCorridor = 5,

    // �м� �� Ÿ��
    DeadEnd = 10,
    Junction = 11,
    CrossRoad = 12
};

// �׷��� ��� Ÿ��
UENUM(BlueprintType)
enum class EGraphNodeType : uint8
{
    Room,           // ��
    DeadEnd,        // ���ٸ� ��
    Junction,       // T�� ������
    CrossRoad       // ���� ������
};

// �׷��� ���
USTRUCT(BlueprintType)
struct FDungeonGraphNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 NodeId = -1;

    UPROPERTY(BlueprintReadOnly)
    FIntVector Position;  // ��� ��ġ (���� ��� �߽�)

    UPROPERTY(BlueprintReadOnly)
    EGraphNodeType NodeType;

    UPROPERTY(BlueprintReadOnly)
    int32 RoomId = -1;  // �� ����� ��� �� ID

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> ConnectedNodes;  // ����� ��� ID��

    UPROPERTY(BlueprintReadOnly)
    FBox2D Bounds;  // ���� ��� ���� ����
};

// �׷��� ����
USTRUCT(BlueprintType)
struct FDungeonGraphEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 StartNodeId;

    UPROPERTY(BlueprintReadOnly)
    int32 EndNodeId;

    UPROPERTY(BlueprintReadOnly)
    TArray<FIntVector> Path;  // ���� ���

    UPROPERTY(BlueprintReadOnly)
    float Length;  // ��� ����
};

// �׷��� �м� ���
USTRUCT(BlueprintType)
struct FDungeonGraphAnalysis
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 NodeCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EdgeCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RoomCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DeadEndCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 JunctionCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CrossRoadCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 CyclomaticComplexity = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 ConnectedComponents = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageNodeDegree = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AveragePathLength = 0.0f;
};

// �� ���� (BSP�� �ٸ� �����⿡�� ����)
USTRUCT(BlueprintType)
struct FRoomInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 RoomId;

    UPROPERTY()
    FIntVector Center;  // �� �߽�

    UPROPERTY()
    FIntVector Min;     // �� �ּ� ��ǥ

    UPROPERTY()
    FIntVector Max;     // �� �ִ� ��ǥ
};

// Ÿ�ϸ� ���� ����ü (2���� �迭�� ���α� ����)
USTRUCT(BlueprintType)
struct FDungeonTileMap
{
    GENERATED_BODY()

    // 2���� Ÿ�ϸ� (Blueprint�� ������� ����)
    TArray<TArray<EDungeonTileType>> Tiles;

    // �� ũ��
    UPROPERTY(BlueprintReadOnly)
    int32 Width;

    UPROPERTY(BlueprintReadOnly)
    int32 Height;

    FDungeonTileMap()
    {
        Width = 0;
        Height = 0;
    }
};

UCLASS()
class PROJECT03_API UDungeonGraphAnalyzer : public UObject
{
    GENERATED_BODY()

public:
    // ���� �м� �Լ� - Ÿ�ϸʰ� �� ������ �޾Ƽ� �׷��� ����
    void AnalyzeDungeon(
        const TArray<TArray<EDungeonTileType>>& TileMap,
        const TArray<FRoomInfo>& RoomInfos,
        float TileSize = 500.0f
    );

    // Blueprint�� ���� �Լ� (�ʿ��� ���)
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void AnalyzeDungeonBP(
        const FDungeonTileMap& TileMapWrapper,
        const TArray<FRoomInfo>& RoomInfos,
        float TileSize = 500.0f
    );

    // �׷��� ���� ��������
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    TArray<FDungeonGraphNode> GetNodes() const { return Nodes; }

    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    TArray<FDungeonGraphEdge> GetEdges() const { return Edges; }

    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    FDungeonGraphAnalysis GetAnalysis() const { return Analysis; }

    // ����� �ð�ȭ
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void DrawDebugVisualization(UWorld* World, float Duration = 10.0f);

    // ��� ���
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void PrintStatistics();

    // �׷��� �ʱ�ȭ
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void ClearGraph();

    int32 TraceCorridorToNextNode(
        const FIntVector& StartPos,
        int32 StartNodeId,
        TArray<FIntVector>& OutPath);

    UFUNCTION(BlueprintCallable, Category = "DungeonGraph|Debug")
    void SetDebugLabelVisibility(bool bInDrawNodeLabels, bool bInDrawEdgeLabels, bool bInDrawStatsText = true);

    bool bDrawNodeLabels = true;
    bool bDrawEdgeLabels = true;
    bool bDrawStatsText = true;

    UFUNCTION(BlueprintCallable, Category = "DungeonGraph|Probe")
    void SetRoomBoundaryProbeDepth(int32 Depth);

    int32 RoomBoundaryProbeDepth = 1; // �⺻: 1ĭ 

private:
    // ���� ������
    TArray<FDungeonGraphNode> Nodes;
    TArray<FDungeonGraphEdge> Edges;
    FDungeonGraphAnalysis Analysis;

    // �۾��� ������
    TArray<TArray<EDungeonTileType>> WorkingTileMap;
    TArray<FRoomInfo> WorkingRoomInfos;
    FIntVector MapSize;
    float WorkingTileSize;

    // Step 1: ���� Ÿ�� �м� (���ٸ���, ������ ã��)
    void AnalyzeCorridorTypes();

    // Step 2: ��� ���� (��, ���ٸ���, ������)
    void ExtractNodes();

    // Step 3: ���� ���� (��� �� ���� ���)
    void ExtractEdges();

    // Step 4: �׷��� �м�
    void AnalyzeGraph();

    // ���� �Լ���
    bool IsCorridorTile(int32 x, int32 y) const;
    bool IsCorridorTile(const FIntVector& Pos) const;
    int32 CountCorridorConnections(int32 x, int32 y) const;
    EDungeonTileType DetermineCorridorType(int32 x, int32 y) const;
    bool IsPartOfWideCorrridor(int32 x, int32 y) const;

    // ��� ã��
    bool FindCorridorPath(const FIntVector& Start, const FIntVector& End,
        TArray<FIntVector>& OutPath,
        const TSet<FIntVector>& AvoidNodes = TSet<FIntVector>());

    // ��� ã��
    int32 FindNodeAtPosition(const FIntVector& Pos) const;
    int32 FindRoomNodeContaining(const FIntVector& Pos) const;

    // �׷��� ���Ἲ üũ
    void DFSComponent(int32 NodeIndex, TArray<bool>& Visited);
    int32 CountConnectedComponents();

    // ���� �߰��� ���� �Լ���
    int32 TraceCorridorPathInDirection(
        const FIntVector& StartPos,
        const FIntVector& FirstStep,
        int32 StartNodeId,
        TArray<FIntVector>& OutPath);

    bool IsPositionAtNode(const FIntVector& Pos, int32 NodeId) const;
    bool IsValidCorridorStep(const FIntVector& Pos, int32 AvoidNodeId) const;
    bool ArePathsSimilar(const TArray<FIntVector>& Path1,
        const TArray<FIntVector>& Path2) const;
};