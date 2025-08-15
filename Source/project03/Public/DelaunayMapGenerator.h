#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DelaunayMapGenerator.generated.h"

UENUM(BlueprintType)
enum class EDelaunayTileType : uint8
{
    Empty,
    Room,
    Corridor,
    Door,
    BridgeCorridor
};

// ��γ� �ﰢ�� ����ü
USTRUCT()
struct FDelaunayTriangle
{
    GENERATED_BODY()

    int32 V1;
    int32 V2;
    int32 V3;

    FVector2D Circumcenter;
    float CircumradiusSquared;

    FDelaunayTriangle()
    {
        V1 = V2 = V3 = -1;
        Circumcenter = FVector2D::ZeroVector;
        CircumradiusSquared = 0.0f;
    }

    bool operator==(const FDelaunayTriangle& Other) const
    {
        return V1 == Other.V1 && V2 == Other.V2 && V3 == Other.V3;
    }
};

// ���� ����ü
USTRUCT()
struct FDelaunayEdge
{
    GENERATED_BODY()

    int32 V1;
    int32 V2;
    float Weight;

    FDelaunayEdge()
    {
        V1 = V2 = -1;
        Weight = 0.0f;
    }

    FDelaunayEdge(int32 InV1, int32 InV2, float InWeight)
    {
        V1 = FMath::Min(InV1, InV2);
        V2 = FMath::Max(InV1, InV2);
        Weight = InWeight;
    }

    bool operator==(const FDelaunayEdge& Other) const
    {
        return V1 == Other.V1 && V2 == Other.V2;
    }

    bool operator<(const FDelaunayEdge& Other) const
    {
        return Weight < Other.Weight;
    }

    friend uint32 GetTypeHash(const FDelaunayEdge& Edge)
    {
        return HashCombine(GetTypeHash(Edge.V1), GetTypeHash(Edge.V2));
    }
};

// �� ���� ����ü
USTRUCT()
struct FRoomData
{
    GENERATED_BODY()

    FIntVector Center;
    FIntVector Size;
    TArray<FIntVector> DoorPositions;

    FRoomData()
    {
        Center = FIntVector::ZeroValue;
        Size = FIntVector(3, 3, 1);
    }
};

UCLASS()
class PROJECT03_API ADelaunayMapGenerator : public AActor
{
    GENERATED_BODY()

public:
    ADelaunayMapGenerator();

    // ========== �� ���� ==========
    UPROPERTY(EditAnywhere, Category = "Map Settings")
    FIntVector MapSize = FIntVector(60, 60, 1);

    UPROPERTY(EditAnywhere, Category = "Map Settings", meta = (ClampMin = "5", ClampMax = "30"))
    int32 RoomCount = 15;

    UPROPERTY(EditAnywhere, Category = "Map Settings", meta = (ClampMin = "3", ClampMax = "10"))
    int32 MinRoomSize = 3;

    UPROPERTY(EditAnywhere, Category = "Map Settings", meta = (ClampMin = "3", ClampMax = "15"))
    int32 MaxRoomSize = 7;

    UPROPERTY(EditAnywhere, Category = "Map Settings", meta = (ClampMin = "5.0", ClampMax = "20.0"))
    float MinRoomDistance = 8.0f;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    float TileSize = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Map Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ExtraConnectionRatio = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    int32 RandomSeed = 0;

    UPROPERTY(EditAnywhere, Category = "Map Settings")
    bool bUseAStar = false;  // true: A* ���Ž��, false: �ܼ� L�� ����

    // ========== Ÿ�� Ŭ������ (BSP�� ����) ==========
    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomNorthClass;  // /Game/BP/BSP/romm

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomSouthClass;  // /Game/BP/BSP/rommBack

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomEastClass;   // /Game/BP/BSP/rommRIGHT

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> RoomWestClass;   // /Game/BP/BSP/rommLEFT

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> DoorClass;       // /Game/BP/BSP/Door

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> WallClass;       // /Game/BP/BSP/Wall

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorHorizontalClass;  // /Game/BP/t01-01

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorVerticalClass;    // /Game/BP/t01

    UPROPERTY(EditAnywhere, Category = "Tiles")
    TSubclassOf<AActor> CorridorCornerClass;      // /Game/BP/goalt01

    // ========== ���� �Լ� ==========
    UFUNCTION(BlueprintCallable, Category = "Delaunay")
    void GenerateDelaunayMap();

    UFUNCTION(BlueprintCallable, Category = "Delaunay")
    void ClearMap();

    UFUNCTION(BlueprintCallable, Category = "Delaunay")
    void DebugDrawTriangulation();

    UFUNCTION(BlueprintCallable, Category = "Delaunay")
    void DebugDrawMST();

protected:
    virtual void BeginPlay() override;

private:
    // ========== ���� ������ ==========
    TArray<FRoomData> Rooms;
    TArray<FVector2D> RoomPoints;  // ��γ׿� 2D ����
    TArray<FDelaunayTriangle> Triangles;
    TArray<FDelaunayEdge> MST;
    TArray<FDelaunayEdge> AllConnections;  // MST + �߰� ����
    TArray<TArray<EDelaunayTileType>> TileMap;
    FRandomStream RandomStream;

    // ========== �� ���� �ܰ躰 �Լ� ==========
    void GenerateRoomPositions();
    void PerformDelaunayTriangulation();
    void ExtractMinimumSpanningTree();
    void AddExtraConnections();
    void PlaceRoomsOnTileMap();
    void CreateAllCorridors();
    void SpawnTiles();

    // ========== ��γ� �ﰢ���� ���� ==========
    FDelaunayTriangle CreateSuperTriangle();
    bool IsPointInCircumcircle(const FVector2D& Point, const FDelaunayTriangle& Triangle);
    void CalculateCircumcircle(FDelaunayTriangle& Triangle);
    void AddUniqueEdge(TArray<FDelaunayEdge>& Edges, int32 V1, int32 V2);
    void RemoveSuperTriangleConnections();

    // ========== MST ���� (Kruskal) ==========
    int32 FindRoot(TArray<int32>& Parent, int32 Node);
    void UnionNodes(TArray<int32>& Parent, int32 Node1, int32 Node2);

    // ========== ���� ���� ==========
    void CreateCorridorBetweenRooms(int32 RoomIndex1, int32 RoomIndex2);
    void CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End);
    TArray<FIntVector> FindPathAStar(const FIntVector& Start, const FIntVector& End);

    // ========== Ÿ�ϸ� ���� ==========
    void InitializeTileMap();
    bool IsValidTilePosition(int32 X, int32 Y) const;
    bool CanPlaceTile(int32 X, int32 Y) const;
    void SetTile(int32 X, int32 Y, EDelaunayTileType Type);
    EDelaunayTileType GetTile(int32 X, int32 Y) const;

    // ========== Ÿ�� ���� ==========
    void SpawnRoomTile(const FRoomData& Room);
    void SpawnCorridorTile(int32 X, int32 Y);
    FString DetermineCorridorDirection(int32 X, int32 Y);
    TSubclassOf<AActor> GetCorridorTileClass(const FString& Direction);

    // ��/�� ����
    void SpawnDoorsForRoom(const FRoomData& Room, AActor* RoomActor);
    void SpawnWallsForRoom(const FRoomData& Room, AActor* RoomActor);
    TArray<FString> GetCorridorDirections(const FRoomData& Room) const;

    // ���� ���� ������ ���� ����
    void EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Dir);

    // ���� ���� Ÿ�� ��� ���� (���� ������, ������ �ֺ� �̿� ���� ����)
    void SpawnSingleCorridorTile(const FIntVector& TilePos);


    // ========== ��ƿ��Ƽ ==========
    float GetDistance2D(const FVector2D& A, const FVector2D& B) const;
};