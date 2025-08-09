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
    BridgeCorridor
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
};