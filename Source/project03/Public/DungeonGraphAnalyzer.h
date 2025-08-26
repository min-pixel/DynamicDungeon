#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGraphAnalyzer.generated.h"

// 타일 타입 (입력용)
UENUM(BlueprintType)
enum class EDungeonTileType : uint8
{
    Empty = 0,
    Room = 1,
    Corridor = 2,
    Door = 3,
    Wall = 4,
    BridgeCorridor = 5,

    // 분석 후 타입
    DeadEnd = 10,
    Junction = 11,
    CrossRoad = 12
};

// 그래프 노드 타입
UENUM(BlueprintType)
enum class EGraphNodeType : uint8
{
    Room,           // 방
    DeadEnd,        // 막다른 길
    Junction,       // T자 갈림길
    CrossRoad       // 십자 교차로
};

// 그래프 노드
USTRUCT(BlueprintType)
struct FDungeonGraphNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 NodeId = -1;

    UPROPERTY(BlueprintReadOnly)
    FIntVector Position;  // 노드 위치 (방의 경우 중심)

    UPROPERTY(BlueprintReadOnly)
    EGraphNodeType NodeType;

    UPROPERTY(BlueprintReadOnly)
    int32 RoomId = -1;  // 방 노드인 경우 방 ID

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> ConnectedNodes;  // 연결된 노드 ID들

    UPROPERTY(BlueprintReadOnly)
    FBox2D Bounds;  // 방의 경우 실제 영역
};

// 그래프 간선
USTRUCT(BlueprintType)
struct FDungeonGraphEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 StartNodeId;

    UPROPERTY(BlueprintReadOnly)
    int32 EndNodeId;

    UPROPERTY(BlueprintReadOnly)
    TArray<FIntVector> Path;  // 복도 경로

    UPROPERTY(BlueprintReadOnly)
    float Length;  // 경로 길이
};

// 그래프 분석 결과
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

// 방 정보 (BSP나 다른 생성기에서 제공)
USTRUCT(BlueprintType)
struct FRoomInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 RoomId;

    UPROPERTY()
    FIntVector Center;  // 방 중심

    UPROPERTY()
    FIntVector Min;     // 방 최소 좌표

    UPROPERTY()
    FIntVector Max;     // 방 최대 좌표
};

// 타일맵 래퍼 구조체 (2차원 배열을 감싸기 위함)
USTRUCT(BlueprintType)
struct FDungeonTileMap
{
    GENERATED_BODY()

    // 2차원 타일맵 (Blueprint에 노출되지 않음)
    TArray<TArray<EDungeonTileType>> Tiles;

    // 맵 크기
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
    // 메인 분석 함수 - 타일맵과 방 정보를 받아서 그래프 생성
    void AnalyzeDungeon(
        const TArray<TArray<EDungeonTileType>>& TileMap,
        const TArray<FRoomInfo>& RoomInfos,
        float TileSize = 500.0f
    );

    // Blueprint용 래퍼 함수 (필요한 경우)
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void AnalyzeDungeonBP(
        const FDungeonTileMap& TileMapWrapper,
        const TArray<FRoomInfo>& RoomInfos,
        float TileSize = 500.0f
    );

    // 그래프 정보 가져오기
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    TArray<FDungeonGraphNode> GetNodes() const { return Nodes; }

    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    TArray<FDungeonGraphEdge> GetEdges() const { return Edges; }

    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    FDungeonGraphAnalysis GetAnalysis() const { return Analysis; }

    // 디버그 시각화
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void DrawDebugVisualization(UWorld* World, float Duration = 10.0f);

    // 통계 출력
    UFUNCTION(BlueprintCallable, Category = "Dungeon Graph")
    void PrintStatistics();

    // 그래프 초기화
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

    int32 RoomBoundaryProbeDepth = 1; // 기본: 1칸 

private:
    // 내부 데이터
    TArray<FDungeonGraphNode> Nodes;
    TArray<FDungeonGraphEdge> Edges;
    FDungeonGraphAnalysis Analysis;

    // 작업용 데이터
    TArray<TArray<EDungeonTileType>> WorkingTileMap;
    TArray<FRoomInfo> WorkingRoomInfos;
    FIntVector MapSize;
    float WorkingTileSize;

    // Step 1: 복도 타입 분석 (막다른길, 갈림길 찾기)
    void AnalyzeCorridorTypes();

    // Step 2: 노드 추출 (방, 막다른길, 갈림길)
    void ExtractNodes();

    // Step 3: 간선 추출 (노드 간 복도 경로)
    void ExtractEdges();

    // Step 4: 그래프 분석
    void AnalyzeGraph();

    // 헬퍼 함수들
    bool IsCorridorTile(int32 x, int32 y) const;
    bool IsCorridorTile(const FIntVector& Pos) const;
    int32 CountCorridorConnections(int32 x, int32 y) const;
    EDungeonTileType DetermineCorridorType(int32 x, int32 y) const;
    bool IsPartOfWideCorrridor(int32 x, int32 y) const;

    // 경로 찾기
    bool FindCorridorPath(const FIntVector& Start, const FIntVector& End,
        TArray<FIntVector>& OutPath,
        const TSet<FIntVector>& AvoidNodes = TSet<FIntVector>());

    // 노드 찾기
    int32 FindNodeAtPosition(const FIntVector& Pos) const;
    int32 FindRoomNodeContaining(const FIntVector& Pos) const;

    // 그래프 연결성 체크
    void DFSComponent(int32 NodeIndex, TArray<bool>& Visited);
    int32 CountConnectedComponents();

    // 새로 추가된 헬퍼 함수들
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