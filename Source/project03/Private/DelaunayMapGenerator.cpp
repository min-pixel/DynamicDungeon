#include "DelaunayMapGenerator.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

ADelaunayMapGenerator::ADelaunayMapGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    static ConstructorHelpers::FClassFinder<AActor> RoomNorth(TEXT("/Game/BP/BSP/romm"));
    static ConstructorHelpers::FClassFinder<AActor> Door(TEXT("/Game/BP/BSP/Door"));
    static ConstructorHelpers::FClassFinder<AActor> Wall(TEXT("/Game/BP/BSP/Wall"));
    static ConstructorHelpers::FClassFinder<AActor> RoomSouth(TEXT("/Game/BP/BSP/rommBack"));
    static ConstructorHelpers::FClassFinder<AActor> RoomWest(TEXT("/Game/BP/BSP/rommLEFT"));
    static ConstructorHelpers::FClassFinder<AActor> RoomEast(TEXT("/Game/BP/BSP/rommRIGHT"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorH(TEXT("/Game/BP/BSP/t01-01"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorV(TEXT("/Game/BP/BSP/t01"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorCorner(TEXT("/Game/BP/BSP/goalt01"));

    if (RoomNorth.Succeeded()) RoomNorthClass = RoomNorth.Class;
    if (Door.Succeeded()) DoorClass = Door.Class;
    if (Wall.Succeeded()) WallClass = Wall.Class;
    if (RoomSouth.Succeeded()) RoomSouthClass = RoomSouth.Class;
    if (RoomWest.Succeeded()) RoomWestClass = RoomWest.Class;
    if (RoomEast.Succeeded()) RoomEastClass = RoomEast.Class;
    if (CorridorH.Succeeded()) CorridorHorizontalClass = CorridorH.Class;
    if (CorridorV.Succeeded()) CorridorVerticalClass = CorridorV.Class;
    if (CorridorCorner.Succeeded()) CorridorCornerClass = CorridorCorner.Class;

}

void ADelaunayMapGenerator::BeginPlay()
{
    Super::BeginPlay();
    GenerateDelaunayMap();
}

void ADelaunayMapGenerator::GenerateDelaunayMap()
{
    // ���� �� ����
    ClearMap();

    // ���� �õ� ����
    if (RandomSeed == 0)
    {
        RandomSeed = FMath::RandRange(1, INT32_MAX);
    }
    RandomStream.Initialize(RandomSeed);

    UE_LOG(LogTemp, Warning, TEXT("=== Delaunay Map Generation Started (Seed: %d) ==="), RandomSeed);

    // 1�ܰ�: �� ��ġ ����
    GenerateRoomPositions();

    // 2�ܰ�: ��γ� �ﰢ����
    PerformDelaunayTriangulation();

    // 3�ܰ�: MST ����
    ExtractMinimumSpanningTree();

    // 4�ܰ�: �߰� ���� ����
    AddExtraConnections();

    // 4.5�ܰ�: ����� �� ���
    DebugDrawTriangulation();
    //DebugDrawMST();

    // 5�ܰ�: Ÿ�ϸʿ� �� ��ġ
    PlaceRoomsOnTileMap();

    // 6�ܰ�: ���� ����
    CreateAllCorridors();



    // 7�ܰ�: Ÿ�� ����
    SpawnTiles();

    UE_LOG(LogTemp, Warning, TEXT("=== Map Generation Complete ==="));
}

void ADelaunayMapGenerator::ClearMap()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "DelaunayGenerated", FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }

    Rooms.Empty();
    RoomPoints.Empty();
    Triangles.Empty();
    MST.Empty();
    AllConnections.Empty();
}

// ========== 1�ܰ�: �� ��ġ ���� ==========
void ADelaunayMapGenerator::GenerateRoomPositions()
{
    Rooms.Empty();
    RoomPoints.Empty();

    int32 Attempts = 0;
    int32 MaxAttempts = RoomCount * 100;

    while (Rooms.Num() < RoomCount && Attempts < MaxAttempts)
    {
        Attempts++;

        FRoomData NewRoom;

        // ���� ũ�� ����
        NewRoom.Size.X = RandomStream.RandRange(MinRoomSize, MaxRoomSize);
        NewRoom.Size.Y = RandomStream.RandRange(MinRoomSize, MaxRoomSize);
        NewRoom.Size.Z = 1;

        // ���� ��ġ (�����ڸ� ����)
        int32 Margin = FMath::Max(MaxRoomSize, 5);
        NewRoom.Center.X = RandomStream.RandRange(Margin, MapSize.X - Margin);
        NewRoom.Center.Y = RandomStream.RandRange(Margin, MapSize.Y - Margin);
        NewRoom.Center.Z = 0;

        // ���� ���� �Ÿ� üũ
        bool bValidPosition = true;
        for (const FRoomData& ExistingRoom : Rooms)
        {
            float Distance = FVector::Dist(
                FVector(NewRoom.Center),
                FVector(ExistingRoom.Center)
            );

            // �ּ� �Ÿ� = �� �� ũ���� ��/2 + MinRoomDistance
            float MinDist = (NewRoom.Size.GetMax() + ExistingRoom.Size.GetMax()) / 2.0f + MinRoomDistance;

            if (Distance < MinDist)
            {
                bValidPosition = false;
                break;
            }
        }

        if (bValidPosition)
        {
            Rooms.Add(NewRoom);
            RoomPoints.Add(FVector2D(NewRoom.Center.X, NewRoom.Center.Y));
            UE_LOG(LogTemp, Verbose, TEXT("Room %d placed at (%d, %d) with size (%d, %d)"),
                Rooms.Num(), NewRoom.Center.X, NewRoom.Center.Y, NewRoom.Size.X, NewRoom.Size.Y);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Generated %d rooms"), Rooms.Num());
}

// ========== 2�ܰ�: ��γ� �ﰢ���� (Bowyer-Watson) ==========
//void ADelaunayMapGenerator::PerformDelaunayTriangulation()
//{
//    if (RoomPoints.Num() < 3)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Need at least 3 rooms for triangulation"));
//        return;
//    }
//
//    Triangles.Empty();
//
//    // Super Triangle ����
//    FDelaunayTriangle SuperTri = CreateSuperTriangle();
//    CalculateCircumcircle(SuperTri);
//    Triangles.Add(SuperTri);
//
//    // �� ���� �ϳ��� �߰�
//    for (int32 i = 0; i < RoomPoints.Num(); i++)
//    {
//        TArray<FDelaunayTriangle> BadTriangles;
//
//        // ���� ���� ������ �ȿ� �ִ� �ﰢ�� ã��
//        for (const FDelaunayTriangle& Tri : Triangles)
//        {
//            if (IsPointInCircumcircle(RoomPoints[i], Tri))
//            {
//                BadTriangles.Add(Tri);
//            }
//        }
//
//        // Bad triangles�� ��� ���� ã��
//        TArray<FDelaunayEdge> Polygon;
//        for (const FDelaunayTriangle& BadTri : BadTriangles)
//        {
//            FDelaunayEdge E1(BadTri.V1, BadTri.V2, 0);
//            FDelaunayEdge E2(BadTri.V2, BadTri.V3, 0);
//            FDelaunayEdge E3(BadTri.V3, BadTri.V1, 0);
//
//            // �ٸ� bad triangle�� �������� �ʴ� ������ �߰�
//            bool bE1Shared = false, bE2Shared = false, bE3Shared = false;
//
//            for (const FDelaunayTriangle& OtherBad : BadTriangles)
//            {
//                if (OtherBad == BadTri) continue;
//
//                if ((OtherBad.V1 == E1.V1 && OtherBad.V2 == E1.V2) ||
//                    (OtherBad.V2 == E1.V1 && OtherBad.V3 == E1.V2) ||
//                    (OtherBad.V3 == E1.V1 && OtherBad.V1 == E1.V2) ||
//                    (OtherBad.V1 == E1.V2 && OtherBad.V2 == E1.V1) ||
//                    (OtherBad.V2 == E1.V2 && OtherBad.V3 == E1.V1) ||
//                    (OtherBad.V3 == E1.V2 && OtherBad.V1 == E1.V1))
//                {
//                    bE1Shared = true;
//                }
//
//                // E2, E3�� ������ ������� üũ...
//            }
//
//            if (!bE1Shared) Polygon.Add(E1);
//            if (!bE2Shared) Polygon.Add(E2);
//            if (!bE3Shared) Polygon.Add(E3);
//        }
//
//        // Bad triangles ����
//        for (const FDelaunayTriangle& BadTri : BadTriangles)
//        {
//            Triangles.Remove(BadTri);
//        }
//
//        // �� �ﰢ�� ����
//        for (const FDelaunayEdge& Edge : Polygon)
//        {
//            FDelaunayTriangle NewTri;
//            NewTri.V1 = Edge.V1;
//            NewTri.V2 = Edge.V2;
//            NewTri.V3 = i;
//
//            CalculateCircumcircle(NewTri);
//            Triangles.Add(NewTri);
//        }
//    }
//
//    // Super Triangle ���� ����
//    RemoveSuperTriangleConnections();
//
//    UE_LOG(LogTemp, Warning, TEXT("Triangulation complete: %d triangles"), Triangles.Num());
//}

void ADelaunayMapGenerator::PerformDelaunayTriangulation()
{
    if (RoomPoints.Num() < 3)
    {
        UE_LOG(LogTemp, Error, TEXT("Need at least 3 rooms for triangulation"));
        return;
    }

    Triangles.Empty();

    // ���� �� ����Ʈ ���� ���� (���� ������ �ڿ� �߰�)
    const int32 OriginalCount = RoomPoints.Num();

    // ===== Super Triangle �� ���� ��ǥ�� �߰� =====
    const float MinX = 0.f, MinY = 0.f;
    const float MaxX = MapSize.X, MaxY = MapSize.Y;
    const float DX = MaxX - MinX, DY = MaxY - MinY;
    const float DMax = FMath::Max(DX, DY);
    const float MidX = (MinX + MaxX) * 0.5f;
    const float MidY = (MinY + MaxY) * 0.5f;

    // �˳��ϰ� ū ���� �ﰢ�� ��ǥ
    const FVector2D SA(MidX - 2.f * DMax, MidY - 1.f * DMax);
    const FVector2D SB(MidX + 0.f * DMax, MidY + 2.f * DMax);
    const FVector2D SC(MidX + 2.f * DMax, MidY - 1.f * DMax);

    const int32 SAi = RoomPoints.Add(SA);
    const int32 SBi = RoomPoints.Add(SB);
    const int32 SCi = RoomPoints.Add(SC);

    FDelaunayTriangle SuperTri;
    SuperTri.V1 = SAi; SuperTri.V2 = SBi; SuperTri.V3 = SCi;
    CalculateCircumcircle(SuperTri);
    Triangles.Add(SuperTri);

    // ===== BowyerWatson: ���� �� ����Ʈ�� ���� ���� =====
    for (int32 i = 0; i < OriginalCount; ++i)
    {
        // �� ���� ������ �ȿ� �ִ� �ﰢ�� ����
        TArray<FDelaunayTriangle> BadTriangles;
        for (const FDelaunayTriangle& Tri : Triangles)
        {
            if (IsPointInCircumcircle(RoomPoints[i], Tri))
            {
                BadTriangles.Add(Tri);
            }
        }

        TMap<TPair<int32, int32>, int32> EdgeCount;
        auto Norm = [](int32 a, int32 b) { if (a > b) Swap(a, b); return TPair<int32, int32>(a, b); };
        auto AddEdge = [&](int32 a, int32 b) { EdgeCount.FindOrAdd(Norm(a, b))++; };

        for (const FDelaunayTriangle& BT : BadTriangles)
        {
            AddEdge(BT.V1, BT.V2);
            AddEdge(BT.V2, BT.V3);
            AddEdge(BT.V3, BT.V1);
        }

        TArray<FDelaunayEdge> Polygon;
        for (const auto& KVP : EdgeCount)
        {
            if (KVP.Value == 1)
            {
                Polygon.Emplace(KVP.Key.Key, KVP.Key.Value, 0.f);
            }
        }

        // BadTriangles ����
        Triangles.RemoveAll([&](const FDelaunayTriangle& T) { return BadTriangles.Contains(T); });

        // ��� �� + �� �� i �� �� �ﰢ�� ����
        for (const FDelaunayEdge& E : Polygon)
        {
            FDelaunayTriangle NewTri;
            NewTri.V1 = E.V1; NewTri.V2 = E.V2; NewTri.V3 = i;
            CalculateCircumcircle(NewTri);
            Triangles.Add(NewTri);
        }
    }

    // ===== ���� ������ ���� �ﰢ�� ���� =====
    const int32 SuperStart = OriginalCount; // RoomPoints�� ������ 3���� ����
    Triangles.RemoveAll([&](const FDelaunayTriangle& T) {
        return (T.V1 >= SuperStart) || (T.V2 >= SuperStart) || (T.V3 >= SuperStart);
        });

    // ���� ���� 3�� ����Ʈ�� ����
    RoomPoints.RemoveAt(OriginalCount, 3, false);

    UE_LOG(LogTemp, Warning, TEXT("Triangulation complete: %d triangles"), Triangles.Num());
}

FDelaunayTriangle ADelaunayMapGenerator::CreateSuperTriangle()
{
    // ��� ���� �����ϴ� ū �ﰢ��
    float MinX = 0, MinY = 0;
    float MaxX = MapSize.X, MaxY = MapSize.Y;

    float DX = MaxX - MinX;
    float DY = MaxY - MinY;
    float DMax = FMath::Max(DX, DY);
    float MidX = (MinX + MaxX) / 2.0f;
    float MidY = (MinY + MaxY) / 2.0f;

    // Super triangle ����
    FDelaunayTriangle SuperTri;
    SuperTri.V1 = -3;  // Super vertex 1
    SuperTri.V2 = -2;  // Super vertex 2  
    SuperTri.V3 = -1;  // Super vertex 3

    return SuperTri;
}

void ADelaunayMapGenerator::CalculateCircumcircle(FDelaunayTriangle& Triangle)
{
    // �ﰢ���� ������ ���
    FVector2D P1, P2, P3;

    if (Triangle.V1 < 0 || Triangle.V2 < 0 || Triangle.V3 < 0) {
    Triangle.Circumcenter = FVector2D(MapSize.X * 0.5f, MapSize.Y * 0.5f);
    Triangle.CircumradiusSquared = MapSize.X * MapSize.X + MapSize.Y * MapSize.Y;
    return;
    }

    //if (Triangle.V1 < 0) // Super triangle vertex
    //{
    //    // Super triangle ó�� ����
    //    Triangle.Circumcenter = FVector2D(MapSize.X / 2, MapSize.Y / 2);
    //    Triangle.CircumradiusSquared = MapSize.X * MapSize.X + MapSize.Y * MapSize.Y;
    //    return;
    //}

    P1 = RoomPoints[Triangle.V1];
    P2 = RoomPoints[Triangle.V2];
    P3 = RoomPoints[Triangle.V3];

    float D = 2.0f * (P1.X * (P2.Y - P3.Y) + P2.X * (P3.Y - P1.Y) + P3.X * (P1.Y - P2.Y));

    if (FMath::Abs(D) < SMALL_NUMBER)
    {
        Triangle.Circumcenter = (P1 + P2 + P3) / 3.0f;
        Triangle.CircumradiusSquared = FVector2D::DistSquared(P1, Triangle.Circumcenter);
        return;
    }

    float P1Sq = P1.X * P1.X + P1.Y * P1.Y;
    float P2Sq = P2.X * P2.X + P2.Y * P2.Y;
    float P3Sq = P3.X * P3.X + P3.Y * P3.Y;

    float CX = (P1Sq * (P2.Y - P3.Y) + P2Sq * (P3.Y - P1.Y) + P3Sq * (P1.Y - P2.Y)) / D;
    float CY = (P1Sq * (P3.X - P2.X) + P2Sq * (P1.X - P3.X) + P3Sq * (P2.X - P1.X)) / D;

    Triangle.Circumcenter = FVector2D(CX, CY);
    Triangle.CircumradiusSquared = FVector2D::DistSquared(P1, Triangle.Circumcenter);
}

bool ADelaunayMapGenerator::IsPointInCircumcircle(const FVector2D& Point, const FDelaunayTriangle& Triangle)
{
    float DistSq = FVector2D::DistSquared(Point, Triangle.Circumcenter);
    return DistSq < Triangle.CircumradiusSquared - SMALL_NUMBER;
}

void ADelaunayMapGenerator::RemoveSuperTriangleConnections()
{
    // Super triangle ������ ������ �ﰢ�� ����
    /*Triangles.RemoveAll([](const FDelaunayTriangle& Tri) {
        return Tri.V1 < 0 || Tri.V2 < 0 || Tri.V3 < 0;
        });*/

    const int32 SuperStart = RoomPoints.Num() - 3;
    Triangles.RemoveAll([&](const FDelaunayTriangle& T) {
        return (T.V1 >= SuperStart) || (T.V2 >= SuperStart) || (T.V3 >= SuperStart);
        });

}

// ========== 3�ܰ�: MST ���� (Kruskal) ==========
void ADelaunayMapGenerator::ExtractMinimumSpanningTree()
{
    // ��� ���� ����
    TSet<FDelaunayEdge> UniqueEdges;

    for (const FDelaunayTriangle& Tri : Triangles)
    {
        float W1 = GetDistance2D(RoomPoints[Tri.V1], RoomPoints[Tri.V2]);
        float W2 = GetDistance2D(RoomPoints[Tri.V2], RoomPoints[Tri.V3]);
        float W3 = GetDistance2D(RoomPoints[Tri.V3], RoomPoints[Tri.V1]);

        UniqueEdges.Add(FDelaunayEdge(Tri.V1, Tri.V2, W1));
        UniqueEdges.Add(FDelaunayEdge(Tri.V2, Tri.V3, W2));
        UniqueEdges.Add(FDelaunayEdge(Tri.V3, Tri.V1, W3));
    }

    // ����ġ�� ����
    TArray<FDelaunayEdge> SortedEdges = UniqueEdges.Array();
    SortedEdges.Sort();

    // Union-Find
    TArray<int32> Parent;
    Parent.SetNum(Rooms.Num());
    for (int32 i = 0; i < Parent.Num(); i++)
    {
        Parent[i] = i;
    }

    MST.Empty();

    for (const FDelaunayEdge& Edge : SortedEdges)
    {
        int32 Root1 = FindRoot(Parent, Edge.V1);
        int32 Root2 = FindRoot(Parent, Edge.V2);

        if (Root1 != Root2)
        {
            MST.Add(Edge);
            UnionNodes(Parent, Root1, Root2);

            if (MST.Num() >= Rooms.Num() - 1)
            {
                break;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("MST extracted: %d edges"), MST.Num());
}

int32 ADelaunayMapGenerator::FindRoot(TArray<int32>& Parent, int32 Node)
{
    if (Parent[Node] != Node)
    {
        Parent[Node] = FindRoot(Parent, Parent[Node]);  // Path compression
    }
    return Parent[Node];
}

void ADelaunayMapGenerator::UnionNodes(TArray<int32>& Parent, int32 Node1, int32 Node2)
{
    Parent[Node1] = Node2;
}

// ========== 4�ܰ�: �߰� ���� ==========
//void ADelaunayMapGenerator::AddExtraConnections()
//{
//    AllConnections = MST;
//
//    // ��γ׿��� MST�� ���� ������ ����
//    TArray<FDelaunayEdge> CandidateEdges;
//
//    for (const FDelaunayTriangle& Tri : Triangles)
//    {
//        FDelaunayEdge E1(Tri.V1, Tri.V2, GetDistance2D(RoomPoints[Tri.V1], RoomPoints[Tri.V2]));
//        FDelaunayEdge E2(Tri.V2, Tri.V3, GetDistance2D(RoomPoints[Tri.V2], RoomPoints[Tri.V3]));
//        FDelaunayEdge E3(Tri.V3, Tri.V1, GetDistance2D(RoomPoints[Tri.V3], RoomPoints[Tri.V1]));
//
//        if (!MST.Contains(E1)) CandidateEdges.AddUnique(E1);
//        if (!MST.Contains(E2)) CandidateEdges.AddUnique(E2);
//        if (!MST.Contains(E3)) CandidateEdges.AddUnique(E3);
//    }
//
//    // �����ϰ� �߰� ���� ����
//    int32 ExtraCount = FMath::RoundToInt(MST.Num() * ExtraConnectionRatio);
//
//    for (int32 i = 0; i < ExtraCount && CandidateEdges.Num() > 0; i++)
//    {
//        int32 RandomIndex = RandomStream.RandRange(0, CandidateEdges.Num() - 1);
//        AllConnections.Add(CandidateEdges[RandomIndex]);
//        CandidateEdges.RemoveAt(RandomIndex);
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("Added %d extra connections (total: %d)"),
//        AllConnections.Num() - MST.Num(), AllConnections.Num());
//}

void ADelaunayMapGenerator::AddExtraConnections()
{
    AllConnections = MST;

    // ��γ׿��� MST�� ���� ������ ����
    TArray<FDelaunayEdge> CandidateEdges;
    CandidateEdges.Reserve(Triangles.Num() * 3);

    auto AddIfNotInMST = [&](int32 A, int32 B)
        {
            const FDelaunayEdge E(A, B, GetDistance2D(RoomPoints[A], RoomPoints[B]));
            if (!MST.Contains(E)) CandidateEdges.AddUnique(E);
        };

    for (const FDelaunayTriangle& Tri : Triangles)
    {
        AddIfNotInMST(Tri.V1, Tri.V2);
        AddIfNotInMST(Tri.V2, Tri.V3);
        AddIfNotInMST(Tri.V3, Tri.V1);
    }

    const int32 OriginalCandidates = CandidateEdges.Num();

    // ? �ĺ� ���� �� �� ����
    int32 ExtraCount = FMath::RoundToInt(CandidateEdges.Num() * ExtraConnectionRatio);
    ExtraCount = FMath::Clamp(ExtraCount, 0, CandidateEdges.Num());

    while (ExtraCount-- > 0 && CandidateEdges.Num() > 0)
    {
        const int32 Idx = RandomStream.RandRange(0, CandidateEdges.Num() - 1);
        AllConnections.AddUnique(CandidateEdges[Idx]); // �ߺ� ����
        CandidateEdges.RemoveAtSwap(Idx);
    }

    UE_LOG(LogTemp, Warning, TEXT("Added %d extra (candidates=%d, MST=%d, total=%d)"),
        AllConnections.Num() - MST.Num(), OriginalCandidates, MST.Num(), AllConnections.Num());
}

// ========== 5�ܰ�: Ÿ�ϸʿ� �� ��ġ ==========
void ADelaunayMapGenerator::PlaceRoomsOnTileMap()
{
    InitializeTileMap();

    for (const FRoomData& Room : Rooms)
    {
        // �� Ÿ�� ��ġ
        int32 HalfWidth = Room.Size.X / 2;
        int32 HalfHeight = Room.Size.Y / 2;

        for (int32 dx = -HalfWidth; dx <= HalfWidth; dx++)
        {
            for (int32 dy = -HalfHeight; dy <= HalfHeight; dy++)
            {
                int32 X = Room.Center.X + dx;
                int32 Y = Room.Center.Y + dy;

                if (IsValidTilePosition(X, Y))
                {
                    SetTile(X, Y, EDelaunayTileType::Room);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Rooms placed on tilemap"));
}

// ========== 6�ܰ�: ���� ���� ==========
void ADelaunayMapGenerator::CreateAllCorridors()
{
    for (const FDelaunayEdge& Connection : AllConnections)
    {
        CreateCorridorBetweenRooms(Connection.V1, Connection.V2);
    }

    UE_LOG(LogTemp, Warning, TEXT("All corridors created"));
}

void ADelaunayMapGenerator::CreateCorridorBetweenRooms(int32 RoomIndex1, int32 RoomIndex2)
{
    if (!Rooms.IsValidIndex(RoomIndex1) || !Rooms.IsValidIndex(RoomIndex2))
    {
        return;
    }

    FIntVector Start = Rooms[RoomIndex1].Center;
    FIntVector End = Rooms[RoomIndex2].Center;

    if (bUseAStar)
    {
        // A* ��� Ž��
        TArray<FIntVector> Path = FindPathAStar(Start, End);

        for (const FIntVector& Point : Path)
        {
            if (IsValidTilePosition(Point.X, Point.Y) &&
                GetTile(Point.X, Point.Y) == EDelaunayTileType::Empty)
            {
                SetTile(Point.X, Point.Y, EDelaunayTileType::Corridor);
            }
        }
    }
    else
    {
        // �ܼ� L�� ����
        CreateLShapedCorridor(Start, End);
    }
}

void ADelaunayMapGenerator::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
{
    // 50% Ȯ���� ���� ���� �Ǵ� ���� ����
    if (RandomStream.FRand() > 0.5f)
    {
        // ���� ����
        int32 StartX = FMath::Min(Start.X, End.X);
        int32 EndX = FMath::Max(Start.X, End.X);

        for (int32 X = StartX; X <= EndX; X++)
        {
            if (IsValidTilePosition(X, Start.Y) &&
                GetTile(X, Start.Y) == EDelaunayTileType::Empty)
            {
                SetTile(X, Start.Y, EDelaunayTileType::Corridor);
            }
        }

        // ���� ����
        int32 StartY = FMath::Min(Start.Y, End.Y);
        int32 EndY = FMath::Max(Start.Y, End.Y);

        for (int32 Y = StartY; Y <= EndY; Y++)
        {
            if (IsValidTilePosition(End.X, Y) &&
                GetTile(End.X, Y) == EDelaunayTileType::Empty)
            {
                SetTile(End.X, Y, EDelaunayTileType::Corridor);
            }
        }
    }
    else
    {
        // ���� ����
        int32 StartY = FMath::Min(Start.Y, End.Y);
        int32 EndY = FMath::Max(Start.Y, End.Y);

        for (int32 Y = StartY; Y <= EndY; Y++)
        {
            if (IsValidTilePosition(Start.X, Y) &&
                GetTile(Start.X, Y) == EDelaunayTileType::Empty)
            {
                SetTile(Start.X, Y, EDelaunayTileType::Corridor);
            }
        }

        // ���� ����
        int32 StartX = FMath::Min(Start.X, End.X);
        int32 EndX = FMath::Max(Start.X, End.X);

        for (int32 X = StartX; X <= EndX; X++)
        {
            if (IsValidTilePosition(X, End.Y) &&
                GetTile(X, End.Y) == EDelaunayTileType::Empty)
            {
                SetTile(X, End.Y, EDelaunayTileType::Corridor);
            }
        }
    }
}

TArray<FIntVector> ADelaunayMapGenerator::FindPathAStar(const FIntVector& Start, const FIntVector& End)
{
    TArray<FIntVector> Path;

    // A* ���� (���� ����)
    struct FNode
    {
        FIntVector Position;
        float G;  // Start�κ����� ���
        float H;  // End������ �޸���ƽ
        float F() const { return G + H; }
        FIntVector Parent;
    };

    TArray<FNode> OpenList;
    TSet<FIntVector> ClosedList;
    TMap<FIntVector, FIntVector> CameFrom;

    // ���� ���
    FNode StartNode;
    StartNode.Position = Start;
    StartNode.G = 0;
    StartNode.H = FVector::Dist(FVector(Start), FVector(End));
    StartNode.Parent = Start;
    OpenList.Add(StartNode);

    while (OpenList.Num() > 0)
    {
        // F���� ���� ���� ��� ����
        int32 BestIndex = 0;
        float BestF = OpenList[0].F();

        for (int32 i = 1; i < OpenList.Num(); i++)
        {
            if (OpenList[i].F() < BestF)
            {
                BestF = OpenList[i].F();
                BestIndex = i;
            }
        }

        FNode Current = OpenList[BestIndex];
        OpenList.RemoveAt(BestIndex);

        // ��ǥ ����
        if (Current.Position == End)
        {
            // ��� ������
            FIntVector Pos = End;
            while (Pos != Start)
            {
                Path.Add(Pos);
                Pos = CameFrom[Pos];
            }
            Path.Add(Start);

            Algo::Reverse(Path);
            return Path;
        }

        ClosedList.Add(Current.Position);

        // ���� Ÿ�� Ž�� (4����)
        TArray<FIntVector> Neighbors = {
            Current.Position + FIntVector(1, 0, 0),
            Current.Position + FIntVector(-1, 0, 0),
            Current.Position + FIntVector(0, 1, 0),
            Current.Position + FIntVector(0, -1, 0)
        };

        for (const FIntVector& Neighbor : Neighbors)
        {
            if (!IsValidTilePosition(Neighbor.X, Neighbor.Y) ||
                ClosedList.Contains(Neighbor))
            {
                continue;
            }

            float TentativeG = Current.G + 1.0f;

            // �̹� OpenList�� �ִ��� Ȯ��
            int32 ExistingIndex = OpenList.IndexOfByPredicate([&](const FNode& N) {
                return N.Position == Neighbor;
                });

            if (ExistingIndex == INDEX_NONE || TentativeG < OpenList[ExistingIndex].G)
            {
                FNode NeighborNode;
                NeighborNode.Position = Neighbor;
                NeighborNode.G = TentativeG;
                NeighborNode.H = FVector::Dist(FVector(Neighbor), FVector(End));
                NeighborNode.Parent = Current.Position;

                CameFrom.Add(Neighbor, Current.Position);

                if (ExistingIndex != INDEX_NONE)
                {
                    OpenList[ExistingIndex] = NeighborNode;
                }
                else
                {
                    OpenList.Add(NeighborNode);
                }
            }
        }
    }

    // ��θ� ã�� ���� ��� ���� ��� ��ȯ
    Path.Add(Start);
    Path.Add(End);
    return Path;
}

// ========== 7�ܰ�: Ÿ�� ���� ==========
void ADelaunayMapGenerator::SpawnTiles()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    int32 SpawnedCount = 0;

    // �� Ÿ�� ����
    for (const FRoomData& Room : Rooms)
    {
        SpawnRoomTile(Room);
        SpawnedCount++;
    }



    // ���� Ÿ�� ����
    for (int32 X = 0; X < MapSize.X; X++)
    {
        for (int32 Y = 0; Y < MapSize.Y; Y++)
        {


            if (GetTile(X, Y) == EDelaunayTileType::Corridor)
            {



                SpawnCorridorTile(X, Y);
                SpawnedCount++;


            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned %d tiles"), SpawnedCount);
}

void ADelaunayMapGenerator::SpawnRoomTile(const FRoomData& Room)
{
    FVector SpawnLocation = FVector(Room.Center) * TileSize;
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // ���� ���� (���� ���� ���⿡ ����)
    TSubclassOf<AActor> RoomClass = RoomNorthClass;  // �⺻��

    // ����� ���� ���� Ȯ��
    bool bHasNorth = false, bHasSouth = false, bHasEast = false, bHasWest = false;

    // ���� Ȯ��
    if (IsValidTilePosition(Room.Center.X, Room.Center.Y + Room.Size.Y / 2 + 1))
    {
        if (GetTile(Room.Center.X, Room.Center.Y + Room.Size.Y / 2 + 1) == EDelaunayTileType::Corridor)
        {
            bHasNorth = true;
        }
    }

    // ���� Ȯ��
    if (IsValidTilePosition(Room.Center.X, Room.Center.Y - Room.Size.Y / 2 - 1))
    {
        if (GetTile(Room.Center.X, Room.Center.Y - Room.Size.Y / 2 - 1) == EDelaunayTileType::Corridor)
        {
            bHasSouth = true;
        }
    }

    // ���� Ȯ��
    if (IsValidTilePosition(Room.Center.X + Room.Size.X / 2 + 1, Room.Center.Y))
    {
        if (GetTile(Room.Center.X + Room.Size.X / 2 + 1, Room.Center.Y) == EDelaunayTileType::Corridor)
        {
            bHasEast = true;
        }
    }

    // ���� Ȯ��
    if (IsValidTilePosition(Room.Center.X - Room.Size.X / 2 - 1, Room.Center.Y))
    {
        if (GetTile(Room.Center.X - Room.Size.X / 2 - 1, Room.Center.Y) == EDelaunayTileType::Corridor)
        {
            bHasWest = true;
        }
    }

    // ���⿡ ���� �� Ÿ�� ����
    if (bHasNorth && RoomNorthClass) RoomClass = RoomNorthClass;
    else if (bHasSouth && RoomSouthClass) RoomClass = RoomSouthClass;
    else if (bHasEast && RoomEastClass) RoomClass = RoomEastClass;
    else if (bHasWest && RoomWestClass) RoomClass = RoomWestClass;

    if (RoomClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedRoom = GetWorld()->SpawnActor<AActor>(RoomClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (SpawnedRoom)
        {
            SpawnedRoom->Tags.Add("DelaunayGenerated");
            SpawnedRoom->SetActorScale3D(FVector(3.0f, 3.0f, 3.0f));

            SpawnDoorsForRoom(Room, SpawnedRoom);
            SpawnWallsForRoom(Room, SpawnedRoom);

        }



    }
}

void ADelaunayMapGenerator::SpawnCorridorTile(int32 X, int32 Y)
{
    FVector SpawnLocation = FVector(X * TileSize, Y * TileSize, 0);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // �ֺ� Ÿ�� Ȯ��
    bool bHasNorth = IsValidTilePosition(X, Y + 1) && GetTile(X, Y + 1) != EDelaunayTileType::Empty;
    bool bHasSouth = IsValidTilePosition(X, Y - 1) && GetTile(X, Y - 1) != EDelaunayTileType::Empty;
    bool bHasEast = IsValidTilePosition(X + 1, Y) && GetTile(X + 1, Y) != EDelaunayTileType::Empty;
    bool bHasWest = IsValidTilePosition(X - 1, Y) && GetTile(X - 1, Y) != EDelaunayTileType::Empty;

    int32 ConnectionCount = (bHasNorth ? 1 : 0) + (bHasSouth ? 1 : 0) +
        (bHasEast ? 1 : 0) + (bHasWest ? 1 : 0);

    TSubclassOf<AActor> TileClass = nullptr;


    // ���� ���¿� ���� Ÿ�� ����
    if (ConnectionCount > 2 ||
        (bHasNorth && bHasEast) || (bHasNorth && bHasWest) ||
        (bHasSouth && bHasEast) || (bHasSouth && bHasWest))
    {
        // �ڳ� �Ǵ� ������
        TileClass = CorridorCornerClass;
    }
    else if (bHasNorth || bHasSouth)
    {
        // ���� ����
        TileClass = CorridorVerticalClass;
    }
    else if (bHasEast || bHasWest)
    {
        // ���� ����
        TileClass = CorridorHorizontalClass;
    }

    if (TileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedTile = GetWorld()->SpawnActor<AActor>(TileClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (SpawnedTile)
        {
            SpawnedTile->Tags.Add("DelaunayGenerated");
        }
    }
}

// ========== Ÿ�ϸ� ���� ==========
void ADelaunayMapGenerator::InitializeTileMap()
{
    TileMap.SetNum(MapSize.X);
    for (int32 X = 0; X < MapSize.X; X++)
    {
        TileMap[X].SetNum(MapSize.Y);
        for (int32 Y = 0; Y < MapSize.Y; Y++)
        {
            TileMap[X][Y] = EDelaunayTileType::Empty;
        }
    }
}

bool ADelaunayMapGenerator::IsValidTilePosition(int32 X, int32 Y) const
{
    return X >= 0 && X < MapSize.X && Y >= 0 && Y < MapSize.Y;
}

void ADelaunayMapGenerator::SetTile(int32 X, int32 Y, EDelaunayTileType Type)
{
    if (IsValidTilePosition(X, Y))
    {
        TileMap[X][Y] = Type;
    }
}

EDelaunayTileType ADelaunayMapGenerator::GetTile(int32 X, int32 Y) const
{
    if (IsValidTilePosition(X, Y))
    {
        return TileMap[X][Y];
    }
    return EDelaunayTileType::Empty;
}

// ========== ��ƿ��Ƽ ==========
float ADelaunayMapGenerator::GetDistance2D(const FVector2D& A, const FVector2D& B) const
{
    return FVector2D::Distance(A, B);
}

// ========== ����� ==========
void ADelaunayMapGenerator::DebugDrawTriangulation()
{
    if (!GetWorld()) return;

    for (const FDelaunayTriangle& Tri : Triangles)
    {
        if (Tri.V1 < 0 || Tri.V2 < 0 || Tri.V3 < 0) continue;

        FVector P1 = FVector(RoomPoints[Tri.V1].X * TileSize, RoomPoints[Tri.V1].Y * TileSize, 100);
        FVector P2 = FVector(RoomPoints[Tri.V2].X * TileSize, RoomPoints[Tri.V2].Y * TileSize, 100);
        FVector P3 = FVector(RoomPoints[Tri.V3].X * TileSize, RoomPoints[Tri.V3].Y * TileSize, 100);

        DrawDebugLine(GetWorld(), P1, P2, FColor::Blue, true, 10.0f, 0, 10.0f);
        DrawDebugLine(GetWorld(), P2, P3, FColor::Blue, true, 10.0f, 0, 10.0f);
        DrawDebugLine(GetWorld(), P3, P1, FColor::Blue, true, 10.0f, 0, 10.0f);
    }
}

void ADelaunayMapGenerator::DebugDrawMST()
{
    if (!GetWorld()) return;

    for (const FDelaunayEdge& Edge : MST)
    {
        FVector P1 = FVector(RoomPoints[Edge.V1].X * TileSize, RoomPoints[Edge.V1].Y * TileSize, 150);
        FVector P2 = FVector(RoomPoints[Edge.V2].X * TileSize, RoomPoints[Edge.V2].Y * TileSize, 150);

        DrawDebugLine(GetWorld(), P1, P2, FColor::Red, true, 10.0f, 0, 5.0f);
    }

    // �߰� ������ �ٸ� ������
    for (const FDelaunayEdge& Edge : AllConnections)
    {
        if (!MST.Contains(Edge))
        {
            FVector P1 = FVector(RoomPoints[Edge.V1].X * TileSize, RoomPoints[Edge.V1].Y * TileSize, 150);
            FVector P2 = FVector(RoomPoints[Edge.V2].X * TileSize, RoomPoints[Edge.V2].Y * TileSize, 150);

            DrawDebugLine(GetWorld(), P1, P2, FColor::Yellow, true, 10.0f, 0, 3.0f);
        }
    }
}

TArray<FString> ADelaunayMapGenerator::GetCorridorDirections(const FRoomData& Room) const
{
    TArray<FString> Connected;
    const int32 HalfX = Room.Size.X / 2;
    const int32 HalfY = Room.Size.Y / 2;

    const FIntVector RoomMin(Room.Center.X - HalfX, Room.Center.Y - HalfY, 0);
    const FIntVector RoomMax(RoomMin.X + Room.Size.X, RoomMin.Y + Room.Size.Y, 0); // [min, max)

    // North (Y+): �� ���� �ٷ� �ٱ���
    for (int32 x = RoomMin.X; x < RoomMax.X; ++x)
    {
        int32 y = RoomMax.Y;
        if (IsValidTilePosition(x, y) && (GetTile(x, y) == EDelaunayTileType::Corridor ||
            GetTile(x, y) == EDelaunayTileType::BridgeCorridor))
        {
            Connected.AddUnique("North"); break;
        }
    }

    // South (Y-): �� �Ʒ��� �ٷ� �ٱ���
    for (int32 x = RoomMin.X; x < RoomMax.X; ++x)
    {
        int32 y = RoomMin.Y - 1;
        if (IsValidTilePosition(x, y) && (GetTile(x, y) == EDelaunayTileType::Corridor ||
            GetTile(x, y) == EDelaunayTileType::BridgeCorridor))
        {
            Connected.AddUnique("South"); break;
        }
    }

    // East (X+): �� ������ �ٷ� �ٱ���
    for (int32 y = RoomMin.Y; y < RoomMax.Y; ++y)
    {
        int32 x = RoomMax.X;
        if (IsValidTilePosition(x, y) && (GetTile(x, y) == EDelaunayTileType::Corridor ||
            GetTile(x, y) == EDelaunayTileType::BridgeCorridor))
        {
            Connected.AddUnique("East"); break;
        }
    }

    // West (X-): �� ���� �ٷ� �ٱ���
    for (int32 y = RoomMin.Y; y < RoomMax.Y; ++y)
    {
        int32 x = RoomMin.X - 1;
        if (IsValidTilePosition(x, y) && (GetTile(x, y) == EDelaunayTileType::Corridor ||
            GetTile(x, y) == EDelaunayTileType::BridgeCorridor))
        {
            Connected.AddUnique("West"); break;
        }
    }

    return Connected;
}

void ADelaunayMapGenerator::EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Direction)
{
    const FIntVector CorridorPos = DoorPos + Direction;
    if (!IsValidTilePosition(CorridorPos.X, CorridorPos.Y)) return;

    if (GetTile(CorridorPos.X, CorridorPos.Y) == EDelaunayTileType::Empty)
    {
        SetTile(CorridorPos.X, CorridorPos.Y, EDelaunayTileType::BridgeCorridor); 
        SpawnSingleCorridorTile(CorridorPos);                                     
    }
}

void ADelaunayMapGenerator::SpawnSingleCorridorTile(const FIntVector& TilePos)
{
    UWorld* World = GetWorld();
    if (!World || !CorridorCornerClass) return;

    const FVector  Loc(TilePos.X * TileSize, TilePos.Y * TileSize, 0);
    const FRotator Rot = FRotator::ZeroRotator;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (AActor* Tile = World->SpawnActor<AActor>(CorridorCornerClass, Loc, Rot, Params))
    {
        Tile->Tags.Add("DelaunayGenerated");
        Tile->Tags.Add("BridgeCorridor");
    }
}

void ADelaunayMapGenerator::SpawnDoorsForRoom(const FRoomData& Room, AActor* RoomActor)
{
    if (!RoomActor || !DoorClass) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector RoomLocation = RoomActor->GetActorLocation();
    const FVector RoomScale = RoomActor->GetActorScale3D();
    const float   HalfSize = TileSize * RoomScale.X / 2.0f;

    const int32 HalfX = Room.Size.X / 2;
    const int32 HalfY = Room.Size.Y / 2;
    const FIntVector RoomMin(Room.Center.X - HalfX, Room.Center.Y - HalfY, 0);
    const FIntVector RoomMax(RoomMin.X + Room.Size.X, RoomMin.Y + Room.Size.Y, 0);

    const TArray<FString> Dirs = GetCorridorDirections(Room);

    for (const FString& Dir : Dirs)
    {
        FVector  DoorOffset = FVector::ZeroVector;
        FRotator DoorRot = FRotator::ZeroRotator;
        FIntVector DoorTilePos = FIntVector::ZeroValue;

        if (Dir == "North") { DoorOffset = FVector(0, HalfSize, 0); DoorRot = FRotator(0, 0, 0); DoorTilePos = FIntVector(Room.Center.X, RoomMax.Y - 1, 0); EnsureCorridorConnection(DoorTilePos, FIntVector(0, 1, 0)); }
        else if (Dir == "South") { DoorOffset = FVector(0, -HalfSize, 0); DoorRot = FRotator(0, 180, 0); DoorTilePos = FIntVector(Room.Center.X, RoomMin.Y, 0); EnsureCorridorConnection(DoorTilePos, FIntVector(0, -1, 0)); }
        else if (Dir == "East") { DoorOffset = FVector(HalfSize, 0, 0); DoorRot = FRotator(0, 90, 0); DoorTilePos = FIntVector(RoomMax.X - 1, Room.Center.Y, 0); EnsureCorridorConnection(DoorTilePos, FIntVector(1, 0, 0)); }
        else if (Dir == "West") { DoorOffset = FVector(-HalfSize, 0, 0); DoorRot = FRotator(0, -90, 0); DoorTilePos = FIntVector(RoomMin.X, Room.Center.Y, 0); EnsureCorridorConnection(DoorTilePos, FIntVector(-1, 0, 0)); }

        // �� ����
        const FVector DoorLocation = RoomLocation + DoorOffset;
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AActor* Door = World->SpawnActor<AActor>(DoorClass, DoorLocation, DoorRot, Params))
        {
            Door->Tags.Add("DelaunayGenerated");
            Door->Tags.Add("Door");
            Door->SetActorScale3D(RoomScale);
            Door->AttachToActor(RoomActor, FAttachmentTransformRules::KeepWorldTransform);
            SetTile(DoorTilePos.X, DoorTilePos.Y, EDelaunayTileType::Door);
        }
    }
}

void ADelaunayMapGenerator::SpawnWallsForRoom(const FRoomData& Room, AActor* RoomActor)
{
    if (!RoomActor || !WallClass) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector RoomLocation = RoomActor->GetActorLocation();
    const FVector RoomScale = RoomActor->GetActorScale3D();
    const float   HalfSize = TileSize * RoomScale.X / 2.0f;

    const TArray<FString> Connected = GetCorridorDirections(Room);
    const TArray<FString> AllDirs{ "North","South","East","West" };

    for (const FString& Dir : AllDirs)
    {
        if (Connected.Contains(Dir)) continue; // ���� ���� ������ �� ����

        FVector  WallOffset = FVector::ZeroVector;
        FRotator WallRot = FRotator::ZeroRotator;

        if (Dir == "North") { WallOffset = FVector(0, HalfSize, 0); WallRot = FRotator(0, 0, 0); }
        else if (Dir == "South") { WallOffset = FVector(0, -HalfSize, 0); WallRot = FRotator(0, 180, 0); }
        else if (Dir == "East") { WallOffset = FVector(HalfSize, 0, 0); WallRot = FRotator(0, 90, 0); }
        else if (Dir == "West") { WallOffset = FVector(-HalfSize, 0, 0); WallRot = FRotator(0, -90, 0); }

        const FVector WallLocation = RoomLocation + WallOffset;
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (AActor* Wall = World->SpawnActor<AActor>(WallClass, WallLocation, WallRot, Params))
        {
            Wall->Tags.Add("DelaunayGenerated");
            Wall->Tags.Add("Wall");
            Wall->SetActorScale3D(RoomScale);
            Wall->AttachToActor(RoomActor, FAttachmentTransformRules::KeepWorldTransform);
        }
    }
}

