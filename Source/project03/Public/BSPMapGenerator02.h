// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
    BridgeCorridor
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

    UFUNCTION(BlueprintCallable, Category = "BSP")
    void GenerateBSPMap();

    UFUNCTION(BlueprintCallable, Category = "BSP")
    void ClearMap();

protected:
    virtual void BeginPlay() override;

private:
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
};