// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BSPMapGenerator.generated.h"

USTRUCT()
struct FBSPNode
{
    GENERATED_BODY()

    FIntVector Min;
    FIntVector Max;

    TSharedPtr<FBSPNode> LeftChild;
    TSharedPtr<FBSPNode> RightChild;

    bool bIsLeaf = false;
    bool bHasRoom = false;

    // ���� �� ����
    FIntVector RoomMin;
    FIntVector RoomMax;

    FBSPNode() {}

    FBSPNode(const FIntVector& InMin, const FIntVector& InMax)
        : Min(InMin), Max(InMax), bIsLeaf(true) {}
};

UENUM(BlueprintType)
enum class ETileType : uint8
{
    Empty,
    Room,
    Corridor,
    Door
};

USTRUCT()
struct FTilePart
{
    GENERATED_BODY()

    UPROPERTY() UInstancedStaticMeshComponent* ISM = nullptr;
    UPROPERTY() FTransform PartWorld;
};


UCLASS()
class PROJECT03_API ABSPMapGenerator : public AActor
{
    GENERATED_BODY()

public:
    ABSPMapGenerator();

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

    UPROPERTY()
    UInstancedStaticMeshComponent* ISM_CorridorHorizontal;

    UPROPERTY()
    UInstancedStaticMeshComponent* ISM_CorridorVertical;

    UPROPERTY()
    UInstancedStaticMeshComponent* ISM_CorridorCorner;

    UPROPERTY() TArray<FTilePart> Parts_H;
    UPROPERTY() TArray<FTilePart> Parts_V;
    UPROPERTY() TArray<FTilePart> Parts_C;



protected:
    virtual void BeginPlay() override;

private:
    TSharedPtr<FBSPNode> RootNode;
    TArray<TSharedPtr<FBSPNode>> LeafNodes;
    TArray<TArray<ETileType>> TileMap;
    FRandomStream RandomStream;



    // BSP Ʈ�� ����
    TSharedPtr<FBSPNode> CreateBSPTree(const FIntVector& Min, const FIntVector& Max, int32 Depth = 0);

    // ��� ����
    bool SplitNode(TSharedPtr<FBSPNode> Node, int32 Depth);

    // ���� ��忡 �� ����
    void CreateRooms();

    // ����� ������ ����
    void ConnectRooms();

    // �� �� ���̿� ���� ����
    void CreateCorridor(TSharedPtr<FBSPNode> NodeA, TSharedPtr<FBSPNode> NodeB);

    // L�� ���� ����
    void CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End);

    // Ÿ�� ���� ���� ���ͷ� ����
    void SpawnTiles();

    // ���� �� ���� ����
    FString GetRoomDoorDirection(const FIntVector& RoomPos);

    // ���� ��� ����
    void CollectLeafNodes(TSharedPtr<FBSPNode> Node);

    // �� �ʱ�ȭ
    void InitializeTileMap();

    // Ÿ�� ��ġ ���� ���� Ȯ��
    bool CanPlaceTile(const FIntVector& Pos);

    // ���� �߽��� ��������
    FIntVector GetRoomCenter(TSharedPtr<FBSPNode> Node);

    FString GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode> Node);
};
