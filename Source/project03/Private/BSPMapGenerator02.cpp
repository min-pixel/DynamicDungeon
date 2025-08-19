#include "BSPMapGenerator02.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h" 

ABSPMapGenerator02::ABSPMapGenerator02()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // �⺻ Ÿ�� ��� ����
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

    bReplicates = false;
}

void ABSPMapGenerator02::BeginPlay()
{
    Super::BeginPlay();

    GraphAnalyzer = NewObject<UDungeonGraphAnalyzer>(this);

    GenerateBSPMap();

    
}

void ABSPMapGenerator02::GenerateBSPMap()
{
    // ���� �� ����
    ClearMap();

    // ���� �õ� ����
    if (RandomSeed == 0)
    {
        RandomSeed = FMath::RandRange(1, INT32_MAX);
    }
    RandomStream.Initialize(RandomSeed);

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Started with Seed: %d"), RandomSeed);

    // Ÿ�� �� �ʱ�ȭ
    InitializeTileMap();

    // BSP Ʈ�� ����
    RootNode = CreateBSPTree(FIntVector(0, 0, 0), MapSize, 0);

    // ���� ��� ����
    LeafNodes.Empty();
    CollectLeafNodes(RootNode);

    // �� ����
    CreateRooms();

    // �� ����
    ConnectRooms();

    // �߰� ���� ���� (�̷� ���⵵ ����)
    if (bCreateLoops)
    {
        CreateExtraConnections();
    }

    CleanupParallelCorridors();

   

    // Ÿ�� ����
    SpawnTiles();

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed. Created %d rooms"), LeafNodes.Num());

    // ===== �׷��� �м� ���� =====
    if (GraphAnalyzer)
    {
        // TileMap�� EDungeonTileType���� ��ȯ
        TArray<TArray<EDungeonTileType>> ConvertedTileMap;
        ConvertedTileMap.SetNum(MapSize.X);

        for (int32 x = 0; x < MapSize.X; ++x)
        {
            ConvertedTileMap[x].SetNum(MapSize.Y);
            for (int32 y = 0; y < MapSize.Y; ++y)
            {
                // ETileType02�� EDungeonTileType���� ��ȯ
                switch (TileMap[x][y])
                {
                case ETileType02::Empty:
                    ConvertedTileMap[x][y] = EDungeonTileType::Empty;
                    break;
                case ETileType02::Room:
                    ConvertedTileMap[x][y] = EDungeonTileType::Room;
                    break;
                case ETileType02::Corridor:
                    ConvertedTileMap[x][y] = EDungeonTileType::Corridor;
                    break;
                case ETileType02::BridgeCorridor:
                    ConvertedTileMap[x][y] = EDungeonTileType::BridgeCorridor;
                    break;
                case ETileType02::Door:
                    ConvertedTileMap[x][y] = EDungeonTileType::Door;
                    break;
                case ETileType02::DeadEnd:
                    ConvertedTileMap[x][y] = EDungeonTileType::DeadEnd;
                    break;
                case ETileType02::Junction:
                    ConvertedTileMap[x][y] = EDungeonTileType::Junction;
                    break;
                case ETileType02::CrossRoad:
                    ConvertedTileMap[x][y] = EDungeonTileType::CrossRoad;
                    break;
                default:
                    ConvertedTileMap[x][y] = EDungeonTileType::Empty;
                    break;
                }
            }
        }

        // �� ���� �غ�
        TArray<FRoomInfo> RoomInfos;
        for (int32 i = 0; i < LeafNodes.Num(); ++i)
        {
            if (LeafNodes[i]->bHasRoom)
            {
                FRoomInfo Info;
                Info.RoomId = i;
                Info.Center = GetRoomCenter(LeafNodes[i]);
                Info.Min = LeafNodes[i]->RoomMin;
                Info.Max = LeafNodes[i]->RoomMax;
                RoomInfos.Add(Info);
            }
        }

        // �׷��� �м� ���� (C++ �Լ� ���� ȣ��)
        GraphAnalyzer->AnalyzeDungeon(ConvertedTileMap, RoomInfos, TileSize);

        // ��� ���
        if (bShowStatistics)
        {
            GraphAnalyzer->PrintStatistics();

            // ����� �ð�ȭ
            GraphAnalyzer->DrawDebugVisualization(GetWorld(), -1.0f);
        }

        // �м� ����� MapStats�� �ݿ� (���û���)
        FDungeonGraphAnalysis GraphAnalysis = GraphAnalyzer->GetAnalysis();
        MapStats.RoomCount = GraphAnalysis.RoomCount;
        MapStats.DeadEndCount = GraphAnalysis.DeadEndCount;
        MapStats.JunctionCount = GraphAnalysis.JunctionCount;
        MapStats.CrossRoadCount = GraphAnalysis.CrossRoadCount;

        // ���� �� �α�
        UE_LOG(LogTemp, Warning, TEXT("Graph Analysis: Found %d nodes and %d edges"),
            GraphAnalysis.NodeCount, GraphAnalysis.EdgeCount);
    }

    //// �� ��� �м� �� ���
    //if (bShowStatistics)
    //{
    //    PrintMapStatistics();
    //}

}

void ABSPMapGenerator02::ClearMap()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "BSPGenerated", FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }
}

void ABSPMapGenerator02::InitializeTileMap()
{
    TileMap.SetNum(MapSize.X);
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        TileMap[x].SetNum(MapSize.Y);
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            TileMap[x][y] = ETileType02::Empty;
        }
    }
}

TSharedPtr<FBSPNode02> ABSPMapGenerator02::CreateBSPTree(const FIntVector& Min, const FIntVector& Max, int32 Depth)
{
    TSharedPtr<FBSPNode02> Node = MakeShareable(new FBSPNode02(Min, Max));

    // �ִ� ���� ���� �Ǵ� �ּ� ũ�� ����
    if (Depth >= MaxDepth || !SplitNode(Node, Depth))
    {
        Node->bIsLeaf = true;
        return Node;
    }

    return Node;
}

bool ABSPMapGenerator02::SplitNode(TSharedPtr<FBSPNode02> Node, int32 Depth)
{
    FIntVector Size = Node->Max - Node->Min;

    // ���� �������� Ȯ��
    bool bCanSplitHorizontally = Size.X > MinNodeSize * 2;
    bool bCanSplitVertically = Size.Y > MinNodeSize * 2;

    if (!bCanSplitHorizontally && !bCanSplitVertically)
    {
        return false;
    }

    // ���� ���� ����
    bool bSplitHorizontally;
    if (bCanSplitHorizontally && !bCanSplitVertically)
    {
        bSplitHorizontally = true;
    }
    else if (!bCanSplitHorizontally && bCanSplitVertically)
    {
        bSplitHorizontally = false;
    }
    else
    {
        bSplitHorizontally = RandomStream.FRand() > 0.5f;
    }

    // ���� ��ġ ���
    if (bSplitHorizontally)
    {
        int32 MinSplit = Node->Min.X + MinNodeSize;
        int32 MaxSplit = Node->Max.X - MinNodeSize;
        int32 SplitPos = RandomStream.RandRange(MinSplit, MaxSplit);

        Node->LeftChild = CreateBSPTree(
            Node->Min,
            FIntVector(SplitPos, Node->Max.Y, Node->Max.Z),
            Depth + 1
        );

        Node->RightChild = CreateBSPTree(
            FIntVector(SplitPos, Node->Min.Y, Node->Min.Z),
            Node->Max,
            Depth + 1
        );
    }
    else
    {
        int32 MinSplit = Node->Min.Y + MinNodeSize;
        int32 MaxSplit = Node->Max.Y - MinNodeSize;
        int32 SplitPos = RandomStream.RandRange(MinSplit, MaxSplit);

        Node->LeftChild = CreateBSPTree(
            Node->Min,
            FIntVector(Node->Max.X, SplitPos, Node->Max.Z),
            Depth + 1
        );

        Node->RightChild = CreateBSPTree(
            FIntVector(Node->Min.X, SplitPos, Node->Min.Z),
            Node->Max,
            Depth + 1
        );
    }

    Node->bIsLeaf = false;
    return true;
}

void ABSPMapGenerator02::CreateRooms()
{
    for (auto& LeafNode : LeafNodes)
    {
        FIntVector NodeSize = LeafNode->Max - LeafNode->Min;

        // �� ũ�� ���
        int32 RoomWidth = RandomStream.RandRange(MinRoomSize, FMath::Min(NodeSize.X - 2, MaxRoomSize));
        int32 RoomHeight = RandomStream.RandRange(MinRoomSize, FMath::Min(NodeSize.Y - 2, MaxRoomSize));

        // �� ��ġ ��� (��� ������ ���� ��ġ)
        int32 RoomX = LeafNode->Min.X + RandomStream.RandRange(1, NodeSize.X - RoomWidth - 1);
        int32 RoomY = LeafNode->Min.Y + RandomStream.RandRange(1, NodeSize.Y - RoomHeight - 1);

        LeafNode->RoomMin = FIntVector(RoomX, RoomY, 0);
        LeafNode->RoomMax = FIntVector(RoomX + RoomWidth, RoomY + RoomHeight, 1);
        LeafNode->bHasRoom = true;

        // Ÿ�� �ʿ� �� ǥ��
        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
        {
            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
            {
                if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
                {
                    TileMap[x][y] = ETileType02::Room;
                }
            }
        }
    }
}

//void ABSPMapGenerator02::ConnectRooms()
//{
//    // ������ ���� ���� ����
//    for (int32 i = 0; i < LeafNodes.Num() - 1; ++i)
//    {
//        // ���� ����� �� ã��
//        int32 NearestIndex = -1;
//        float MinDistance = FLT_MAX;
//
//        FIntVector CenterA = GetRoomCenter(LeafNodes[i]);
//
//        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
//        {
//            FIntVector CenterB = GetRoomCenter(LeafNodes[j]);
//            float Distance = FVector::Dist(FVector(CenterA), FVector(CenterB));
//
//            if (Distance < MinDistance)
//            {
//                MinDistance = Distance;
//                NearestIndex = j;
//            }
//        }
//
//        if (NearestIndex != -1)
//        {
//            CreateCorridor(LeafNodes[i], LeafNodes[NearestIndex]);
//        }
//    }
//}
//
//void ABSPMapGenerator02::CreateCorridor(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB)
//{
//    if (!NodeA->bHasRoom || !NodeB->bHasRoom) return;
//
//    FIntVector CenterA = GetRoomCenter(NodeA);
//    FIntVector CenterB = GetRoomCenter(NodeB);
//
//    CreateLShapedCorridor(CenterA, CenterB);
//}

void ABSPMapGenerator02::CreateCorridor(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB)
{
    if (!NodeA.IsValid() || !NodeB.IsValid()) return;
    if (!NodeA->bHasRoom || !NodeB->bHasRoom) return;

    auto Carve = [&](int32 x, int32 y)
        {
            if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
                if (TileMap[x][y] == ETileType02::Empty) TileMap[x][y] = ETileType02::Corridor;
        };
    auto CarveHorizontal = [&](int32 y, int32 x0, int32 x1)
        {
            if (x0 > x1) Swap(x0, x1);
            for (int32 x = x0; x <= x1; ++x) Carve(x, y);
        };
    auto CarveVertical = [&](int32 x, int32 y0, int32 y1)
        {
            if (y0 > y1) Swap(y0, y1);
            for (int32 y = y0; y <= y1; ++y) Carve(x, y);
        };

    const auto& A = *NodeA.Get();
    const auto& B = *NodeB.Get();

    // �¿� �̿�(�Ǵ� �¿�� �� �����) + Y ���� ��ħ �� ���� ����
    bool bLeftRight = (A.RoomMax.X <= B.RoomMin.X) || (B.RoomMax.X <= A.RoomMin.X);
    if (bLeftRight)
    {
        const FBSPNode02* Left = (A.RoomMax.X <= B.RoomMin.X) ? &A : &B;
        const FBSPNode02* Right = (Left == &A) ? &B : &A;

        int32 Y0 = FMath::Max(Left->RoomMin.Y, Right->RoomMin.Y);
        int32 Y1 = FMath::Min(Left->RoomMax.Y - 1, Right->RoomMax.Y - 1);
        if (Y0 <= Y1)
        {
            int32 Y = RandomStream.RandRange(Y0, Y1);
            CarveHorizontal(Y, Left->RoomMax.X, Right->RoomMin.X);
            return; // ���� ����
        }
    }

    // ���� �̿�(�Ǵ� ���Ϸ� �� �����) + X ���� ��ħ �� ���� ����
    bool bTopBottom = (A.RoomMax.Y <= B.RoomMin.Y) || (B.RoomMax.Y <= A.RoomMin.Y);
    if (bTopBottom)
    {
        const FBSPNode02* Bottom = (A.RoomMax.Y <= B.RoomMin.Y) ? &A : &B;
        const FBSPNode02* Top = (Bottom == &A) ? &B : &A;

        int32 X0 = FMath::Max(Bottom->RoomMin.X, Top->RoomMin.X);
        int32 X1 = FMath::Min(Bottom->RoomMax.X - 1, Top->RoomMax.X - 1);
        if (X0 <= X1)
        {
            int32 X = RandomStream.RandRange(X0, X1);
            CarveVertical(X, Bottom->RoomMax.Y, Top->RoomMin.Y);
            return; // ���� ����
        }
    }

    // ���� �Ұ� �� L��(������ �ǹ� ����)
    FIntVector CenterA = GetRoomCenter(NodeA);
    FIntVector CenterB = GetRoomCenter(NodeB);
    CreateLShapedCorridor(CenterA, CenterB);
}

void ABSPMapGenerator02::ConnectRooms()
{
    if (!RootNode) return;

    // ����(�� ���� ���) ������
    std::function<void(TSharedPtr<FBSPNode02>, TArray<TSharedPtr<FBSPNode02>>&)> GatherLeavesWithRooms;
    GatherLeavesWithRooms = [&](TSharedPtr<FBSPNode02> Node, TArray<TSharedPtr<FBSPNode02>>& Out)
        {
            if (!Node) return;
            if (Node->bIsLeaf)
            {
                if (Node->bHasRoom) Out.Add(Node);
                return;
            }
            GatherLeavesWithRooms(Node->LeftChild, Out);
            GatherLeavesWithRooms(Node->RightChild, Out);
        };

    // �� ����Ʈ�� ���̿��� ���� ����� �� ���� ��� ����
    auto ConnectClosestBetween = [&](TArray<TSharedPtr<FBSPNode02>>& Ls, TArray<TSharedPtr<FBSPNode02>>& Rs)
        {
            if (Ls.Num() == 0 || Rs.Num() == 0) return;

            float Best = FLT_MAX;
            TSharedPtr<FBSPNode02> BestA, BestB;

            for (auto& A : Ls)
            {
                const FIntVector CA = GetRoomCenter(A);
                for (auto& B : Rs)
                {
                    const FIntVector CB = GetRoomCenter(B);
                    const float D = FVector::Dist(FVector(CA), FVector(CB));
                    if (D < Best)
                    {
                        Best = D; BestA = A; BestB = B;
                    }
                }
            }
            if (BestA && BestB) CreateCorridor(BestA, BestB);
        };

    // BSP Ʈ���� ��ͷ� ��������: LeftChild �� RightChild�� ����
    std::function<void(TSharedPtr<FBSPNode02>)> ConnectSiblingRooms;
    ConnectSiblingRooms = [&](TSharedPtr<FBSPNode02> Node)
        {
            if (!Node || Node->bIsLeaf) return;

            ConnectSiblingRooms(Node->LeftChild);
            ConnectSiblingRooms(Node->RightChild);

            TArray<TSharedPtr<FBSPNode02>> Ls, Rs;
            GatherLeavesWithRooms(Node->LeftChild, Ls);
            GatherLeavesWithRooms(Node->RightChild, Rs);
            ConnectClosestBetween(Ls, Rs);
        };

    ConnectSiblingRooms(RootNode);
}


//void ABSPMapGenerator02::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
//{
//    // ���� ����, ���� ���߿�
//    if (RandomStream.FRand() > 0.5f)
//    {
//        // ���� ����
//        int32 StartX = FMath::Min(Start.X, End.X);
//        int32 EndX = FMath::Max(Start.X, End.X);
//
//        for (int32 x = StartX; x <= EndX; ++x)
//        {
//
//            // �� ���� �Ʒ��� �̹� ���� Ÿ���� �ִ��� Ȯ�� (���� ����)
//            if ((Start.Y > 0 && IsCorridorTile(x, Start.Y - 1)) ||
//                (Start.Y < MapSize.Y - 1 && IsCorridorTile(x, Start.Y + 1)))
//            {
//                // ������ �����Ǹ�, �� L�� ���� ���� ��ü�� �ߴ��ϰ� �Լ��� ��������
//                // �ٸ� ���� �õ����� �� ���� ��ΰ� �׷����� ���
//                return;
//            }
//
//            if (x >= 0 && x < MapSize.X && Start.Y >= 0 && Start.Y < MapSize.Y)
//            {
//                if (TileMap[x][Start.Y] == ETileType02::Empty)
//                {
//                    TileMap[x][Start.Y] = ETileType02::Corridor;
//                }
//            }
//        }
//
//        // ���� ����
//        int32 StartY = FMath::Min(Start.Y, End.Y);
//        int32 EndY = FMath::Max(Start.Y, End.Y);
//
//        for (int32 y = StartY; y <= EndY; ++y)
//        {
//
//            if ((End.X > 0 && IsCorridorTile(End.X - 1, y)) ||
//                (End.X < MapSize.X - 1 && IsCorridorTile(End.X + 1, y)))
//            {
//                return;
//            }
//
//            if (End.X >= 0 && End.X < MapSize.X && y >= 0 && y < MapSize.Y)
//            {
//                if (TileMap[End.X][y] == ETileType02::Empty)
//                {
//                    TileMap[End.X][y] = ETileType02::Corridor;
//                }
//            }
//        }
//    }
//    else
//    {
//        // ���� ����, ���� ���߿�
//        int32 StartY = FMath::Min(Start.Y, End.Y);
//        int32 EndY = FMath::Max(Start.Y, End.Y);
//
//        for (int32 y = StartY; y <= EndY; ++y)
//        {
//            if (Start.X >= 0 && Start.X < MapSize.X && y >= 0 && y < MapSize.Y)
//            {
//                if (TileMap[Start.X][y] == ETileType02::Empty)
//                {
//                    TileMap[Start.X][y] = ETileType02::Corridor;
//                }
//            }
//        }
//
//        int32 StartX = FMath::Min(Start.X, End.X);
//        int32 EndX = FMath::Max(Start.X, End.X);
//
//        for (int32 x = StartX; x <= EndX; ++x)
//        {
//            if (x >= 0 && x < MapSize.X && End.Y >= 0 && End.Y < MapSize.Y)
//            {
//                if (TileMap[x][End.Y] == ETileType02::Empty)
//                {
//                    TileMap[x][End.Y] = ETileType02::Corridor;
//                }
//            }
//        }
//    }
//}

//void ABSPMapGenerator02::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
//{
//    auto Carve = [&](int32 x, int32 y)
//        {
//            if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
//            {
//                if (TileMap[x][y] == ETileType02::Empty)
//                {
//                    TileMap[x][y] = ETileType02::Corridor;
//                }
//            }
//        };
//
//    auto CarveHorizontal = [&](int32 y, int32 x0, int32 x1)
//        {
//            int32 step = (x0 <= x1) ? 1 : -1;
//            for (int32 x = x0; x != x1 + step; x += step) Carve(x, y);
//        };
//
//    auto CarveVertical = [&](int32 x, int32 y0, int32 y1)
//        {
//            int32 step = (y0 <= y1) ? 1 : -1;
//            for (int32 y = y0; y != y1 + step; y += step) Carve(x, y);
//        };
//
//    auto HorizontalFirst = [&]()
//        {
//            int32 y = Start.Y;
//            int32 startX = Start.X;
//            int32 endX = End.X;
//            int32 step = (startX <= endX) ? 1 : -1;
//
//            // �������� �����ϴٰ� ���� ���� �� "�� ��������" pivot
//            int32 pivotX = endX;
//            bool  bPivot = false;
//
//            for (int32 x = startX; x != endX + step; x += step)
//            {
//                const bool up = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
//                const bool down = (y > 0) && IsCorridorTile(x, y - 1);
//                if (up || down)
//                {
//                    pivotX = x;         // ���⼭ ���´�
//                    bPivot = true;
//                    break;
//                }
//                Carve(x, y);
//            }
//
//            if (!bPivot)
//            {
//                // ���� ���� ������ ��: ���� ��Ĵ�� ������ X���� ����
//                CarveVertical(endX, y, End.Y);
//                return;
//            }
//
//            // 1) ������ pivotX���� ���߰�, pivotX ������ Start.Y -> End.Y�� �������� ���� ����
//            CarveVertical(pivotX, y, End.Y);
//
//            // 2) End.Y �������� pivotX -> End.X�� �������� ������
//            CarveHorizontal(End.Y, pivotX, endX);
//        };
//
//    auto VerticalFirst = [&]()
//        {
//            int32 x = Start.X;
//            int32 startY = Start.Y;
//            int32 endY = End.Y;
//            int32 step = (startY <= endY) ? 1 : -1;
//
//            int32 pivotY = endY;
//            bool  bPivot = false;
//
//            for (int32 y = startY; y != endY + step; y += step)
//            {
//                const bool left = (x > 0) && IsCorridorTile(x - 1, y);
//                const bool right = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
//                if (left || right)
//                {
//                    pivotY = y;         // ���⼭ ���´�
//                    bPivot = true;
//                    break;
//                }
//                Carve(x, y);
//            }
//
//            if (!bPivot)
//            {
//                // ���� ���� ������ ��: ���� ��Ĵ�� ������ Y���� ����
//                CarveHorizontal(endY, x, End.X);
//                return;
//            }
//
//            // 1) ������ pivotY���� ���߰�, pivotY �࿡�� Start.X -> End.X�� �������� ���� ����
//            CarveHorizontal(pivotY, x, End.X);
//
//            // 2) End.X ������ pivotY -> End.Y�� �������� ������
//            CarveVertical(End.X, pivotY, endY);
//        };
//
//    if (RandomStream.FRand() > 0.5f) HorizontalFirst();
//    else                             VerticalFirst();
//}

void ABSPMapGenerator02::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
{
    auto Carve = [&](int32 x, int32 y)
        {
            if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
                if (TileMap[x][y] == ETileType02::Empty) TileMap[x][y] = ETileType02::Corridor;
        };
    auto CarveHorizontal = [&](int32 y, int32 x0, int32 x1)
        {
            int32 step = (x0 <= x1) ? 1 : -1;
            for (int32 x = x0; x != x1 + step; x += step) Carve(x, y);
        };
    auto CarveVertical = [&](int32 x, int32 y0, int32 y1)
        {
            int32 step = (y0 <= y1) ? 1 : -1;
            for (int32 y = y0; y != y1 + step; y += step) Carve(x, y);
        };

    auto HorizontalFirst = [&]()
        {
            int32 y = Start.Y;
            int32 step = (Start.X <= End.X) ? 1 : -1;
            int32 pivotX = End.X;
            bool  bPivot = false;

            for (int32 x = Start.X; x != End.X + step; x += step)
            {
                bool up = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
                bool down = (y > 0) && IsCorridorTile(x, y - 1);
                if (up || down) { pivotX = x; bPivot = true; break; }
                Carve(x, y);
            }

            if (!bPivot) { CarveVertical(End.X, y, End.Y); return; }
            CarveVertical(pivotX, y, End.Y);          // �ǹ� �������� ���η�
            CarveHorizontal(End.Y, pivotX, End.X);    // ��ǥ Y���� ���η� ������
        };

    auto VerticalFirst = [&]()
        {
            int32 x = Start.X;
            int32 step = (Start.Y <= End.Y) ? 1 : -1;
            int32 pivotY = End.Y;
            bool  bPivot = false;

            for (int32 y = Start.Y; y != End.Y + step; y += step)
            {
                bool left = (x > 0) && IsCorridorTile(x - 1, y);
                bool right = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
                if (left || right) { pivotY = y; bPivot = true; break; }
                Carve(x, y);
            }

            if (!bPivot) { CarveHorizontal(End.Y, x, End.X); return; }
            CarveHorizontal(pivotY, x, End.X);       // �ǹ� �������� ���η�
            CarveVertical(End.X, pivotY, End.Y);     // ��ǥ X���� ���η� ������
        };

    if (RandomStream.FRand() > 0.5f) HorizontalFirst();
    else                             VerticalFirst();
}


void ABSPMapGenerator02::SpawnTiles()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("World is null!"));
        return;
    }

    int32 SpawnedCount = 0;

    // ���� ����� ó��
    TSet<FIntVector> ProcessedRoomTiles;

    for (auto& LeafNode : LeafNodes)
    {
        if (!LeafNode->bHasRoom) continue;

        // ���� ���� ũ�� ���
        FIntVector RoomSize = LeafNode->RoomMax - LeafNode->RoomMin;
        FIntVector RoomCenter = GetRoomCenter(LeafNode);

        // �̹� ó���� �������� Ȯ��
        bool bAlreadyProcessed = false;
        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
        {
            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
            {
                if (ProcessedRoomTiles.Contains(FIntVector(x, y, 0)))
                {
                    bAlreadyProcessed = true;
                    break;
                }
            }
            if (bAlreadyProcessed) break;
        }

        if (bAlreadyProcessed) continue;

        // �� ���� ��ü�� ó�������� ǥ��
        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
        {
            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
            {
                ProcessedRoomTiles.Add(FIntVector(x, y, 0));
            }
        }

        // �� ���� ���� - ���� �ϳ��� RoomNorthClass�� ��� (ȸ�� ����)
        FVector SpawnLocation = FVector(RoomCenter.X * TileSize, RoomCenter.Y * TileSize, 0);
        FRotator SpawnRotation = FRotator::ZeroRotator; // ȸ�� ����

        // RoomNorthClass�� �⺻ ������ ���
        if (RoomNorthClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            AActor* SpawnedRoom = World->SpawnActor<AActor>(RoomNorthClass, SpawnLocation, SpawnRotation, SpawnParams);
            if (SpawnedRoom)
            {
                SpawnedRoom->Tags.Add("BSPGenerated");

                // �� Ÿ�Ͽ� 3�� ������ ����
                FVector RoomScale = FVector(3.0f, 3.0f, 3.0f);
                SpawnedRoom->SetActorScale3D(RoomScale);

                // �� ���� (���� ���� ���⿡)
                SpawnWallsForRoom(LeafNode, SpawnedRoom);

                // �� ���� (������ ����� ���⿡)
                SpawnDoorsForRoom(LeafNode, SpawnedRoom);

                SpawnedCount++;

                // ����� ����� Ȯ���Ͽ� �α� ���
                TArray<FString> ConnectedDirs = GetCorridorDirections(LeafNode);
                FString DirsString = FString::Join(ConnectedDirs, TEXT(", "));

                UE_LOG(LogTemp, Warning, TEXT("Spawned room at (%d, %d) with doors at: %s"),
                    RoomCenter.X, RoomCenter.Y,
                    ConnectedDirs.Num() > 0 ? *DirsString : TEXT("No connections"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn room at (%d, %d)"),
                    RoomCenter.X, RoomCenter.Y);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RoomNorthClass is null!"));
        }
    }

    // ���� Ÿ�� ���� (�� Ÿ�ϸ��� ���� ���ͷ�)
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            // �� Ÿ���� �̹� ó�������Ƿ� �ǳʶٱ�
            if (TileMap[x][y] == ETileType02::Room) continue;
            if (TileMap[x][y] == ETileType02::BridgeCorridor)
            {
                UE_LOG(LogTemp, Verbose, TEXT("Skipping already spawned bridge corridor at (%d, %d)"), x, y);
                continue;
            }
            if (TileMap[x][y] != ETileType02::Corridor) continue;
            if (ProcessedRoomTiles.Contains(FIntVector(x, y, 0))) continue;

            FVector SpawnLocation = FVector(x * TileSize, y * TileSize, 0);
            FRotator SpawnRotation = FRotator::ZeroRotator;
            TSubclassOf<AActor> TileClass = nullptr;

            // ���� Ÿ�� ���� Ȯ��
            /*bool bHasNorth = (y < MapSize.Y - 1 && TileMap[x][y + 1] != ETileType02::Empty);
            bool bHasSouth = (y > 0 && TileMap[x][y - 1] != ETileType02::Empty);
            bool bHasEast = (x < MapSize.X - 1 && TileMap[x + 1][y] != ETileType02::Empty);
            bool bHasWest = (x > 0 && TileMap[x - 1][y] != ETileType02::Empty);*/
            bool bHasNorth = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
            bool bHasSouth = (y > 0) && IsCorridorTile(x, y - 1);
            bool bHasEast = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
            bool bHasWest = (x > 0) && IsCorridorTile(x - 1, y);


            int32 ConnectionCount = (bHasNorth ? 1 : 0) + (bHasSouth ? 1 : 0) +
                (bHasEast ? 1 : 0) + (bHasWest ? 1 : 0);

            // �ڳʳ� ������
            if (ConnectionCount > 2 ||
                (bHasNorth && bHasEast) || (bHasNorth && bHasWest) ||
                (bHasSouth && bHasEast) || (bHasSouth && bHasWest))
            {
                TileClass = CorridorCornerClass;
            }
            else if (bHasNorth || bHasSouth)
            {
                // Y�� ���� ���� = ���� ����
                TileClass = CorridorHorizontalClass;
            }
            else if (bHasEast || bHasWest)
            {
                // X�� ���� ���� = ���� ����
                TileClass = CorridorVerticalClass;
            }

            if (TileClass)
            {
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                AActor* SpawnedTile = World->SpawnActor<AActor>(TileClass, SpawnLocation, SpawnRotation, SpawnParams);
                if (SpawnedTile)
                {
                    SpawnedTile->Tags.Add("BSPGenerated");
                    SpawnedCount++;
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned %d tiles total (including %d rooms)"),
        SpawnedCount, LeafNodes.Num());
}

FString ABSPMapGenerator02::GetRoomDoorDirection(const FIntVector& RoomCenter)
{
    // �� �ֺ��� ���� Ȯ�� (3x3 �� ����)
    TArray<FString> PossibleDirections;

    // ���� Ȯ�� (Y+)
    for (int32 dx = -1; dx <= 1; ++dx)
    {
        int32 checkX = RoomCenter.X + dx;
        int32 checkY = RoomCenter.Y + 2;
        if (checkX >= 0 && checkX < MapSize.X && checkY < MapSize.Y &&
            TileMap[checkX][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("North");
            break;
        }
    }

    // ���� Ȯ�� (Y-)
    for (int32 dx = -1; dx <= 1; ++dx)
    {
        int32 checkX = RoomCenter.X + dx;
        int32 checkY = RoomCenter.Y - 2;
        if (checkX >= 0 && checkX < MapSize.X && checkY >= 0 &&
            TileMap[checkX][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("South");
            break;
        }
    }

    // ���� Ȯ�� (X+)
    for (int32 dy = -1; dy <= 1; ++dy)
    {
        int32 checkX = RoomCenter.X + 2;
        int32 checkY = RoomCenter.Y + dy;
        if (checkX < MapSize.X && checkY >= 0 && checkY < MapSize.Y &&
            TileMap[checkX][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("East");
            break;
        }
    }

    // ���� Ȯ�� (X-)
    for (int32 dy = -1; dy <= 1; ++dy)
    {
        int32 checkX = RoomCenter.X - 2;
        int32 checkY = RoomCenter.Y + dy;
        if (checkX >= 0 && checkY >= 0 && checkY < MapSize.Y &&
            TileMap[checkX][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("West");
            break;
        }
    }

    // ������ ���� �� �ϳ� ����
    if (PossibleDirections.Num() > 0)
    {
        return PossibleDirections[RandomStream.RandRange(0, PossibleDirections.Num() - 1)];
    }

    // �⺻��
    return "North";
}

FString ABSPMapGenerator02::GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode02> Node)
{
    if (!Node || !Node->bHasRoom) return "North";

    TArray<FString> PossibleDirections;

    // ���� Ȯ�� (Y+ ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMax.Y;
        if (checkY < MapSize.Y && TileMap[x][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("North");
            break;
        }
    }

    // ���� Ȯ�� (Y- ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMin.Y - 1;
        if (checkY >= 0 && TileMap[x][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("South");
            break;
        }
    }

    // ���� Ȯ�� (X+ ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMax.X;
        if (checkX < MapSize.X && TileMap[checkX][y] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("East");
            break;
        }
    }

    // ���� Ȯ�� (X- ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMin.X - 1;
        if (checkX >= 0 && TileMap[checkX][y] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("West");
            break;
        }
    }

    // ������ ���� �� �ϳ� ����
    if (PossibleDirections.Num() > 0)
    {
        return PossibleDirections[RandomStream.RandRange(0, PossibleDirections.Num() - 1)];
    }

    return "North";
}

void ABSPMapGenerator02::CollectLeafNodes(TSharedPtr<FBSPNode02> Node)
{
    if (!Node) return;

    if (Node->bIsLeaf)
    {
        LeafNodes.Add(Node);
    }
    else
    {
        CollectLeafNodes(Node->LeftChild);
        CollectLeafNodes(Node->RightChild);
    }
}

FIntVector ABSPMapGenerator02::GetRoomCenter(TSharedPtr<FBSPNode02> Node)
{
    if (!Node->bHasRoom) return FIntVector::ZeroValue;

    return FIntVector(
        (Node->RoomMin.X + Node->RoomMax.X) / 2,
        (Node->RoomMin.Y + Node->RoomMax.Y) / 2,
        0
    );
}

bool ABSPMapGenerator02::CanPlaceTile(const FIntVector& Pos)
{
    return Pos.X >= 0 && Pos.X < MapSize.X &&
        Pos.Y >= 0 && Pos.Y < MapSize.Y;
}

void ABSPMapGenerator02::SpawnDoorsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor)
{
    if (!Node || !Node->bHasRoom || !RoomActor || !DoorClass) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FVector RoomLocation = RoomActor->GetActorLocation();
    FVector RoomScale = RoomActor->GetActorScale3D();

    // ������ ����� ���� Ȯ��
    TArray<FString> Directions = GetCorridorDirections(Node);

    for (const FString& Direction : Directions)
    {
        FVector DoorOffset = FVector::ZeroVector;
        FRotator DoorRotation = FRotator::ZeroRotator;

        // ���� ���� ũ�� (������ ����� ũ��)
        float HalfSize = TileSize * RoomScale.X / 2.0f;

        // �� ��ġ�� ȸ�� ����
        FIntVector DoorTilePos;

        if (Direction == "North")
        {
            // ���� �� (Y+ ����)
            DoorOffset = FVector(0, HalfSize, 0);
            DoorRotation = FRotator(0, 0, 0);
            DoorTilePos = FIntVector(GetRoomCenter(Node).X, Node->RoomMax.Y-1, 0);

            // ���� ���� Ȯ�� �� ����
            EnsureCorridorConnection(DoorTilePos, FIntVector(0, 1, 0), Node);
        }
        else if (Direction == "South")
        {
            // ���� �� (Y- ����)
            DoorOffset = FVector(0, -HalfSize, 0);
            DoorRotation = FRotator(0, 180, 0);
            DoorTilePos = FIntVector(GetRoomCenter(Node).X, Node->RoomMin.Y, 0);

            // ���� ���� Ȯ�� �� ����
            EnsureCorridorConnection(DoorTilePos, FIntVector(0, -1, 0), Node);
        }
        else if (Direction == "East")
        {
            // ���� �� (X+ ����)
            DoorOffset = FVector(HalfSize, 0, 0);
            DoorRotation = FRotator(0, 90, 0);
            DoorTilePos = FIntVector(Node->RoomMax.X-1, GetRoomCenter(Node).Y, 0);

            // ���� ���� Ȯ�� �� ����
            EnsureCorridorConnection(DoorTilePos, FIntVector(1, 0, 0), Node);
        }
        else if (Direction == "West")
        {
            // ���� �� (X- ����)
            DoorOffset = FVector(-HalfSize, 0, 0);
            DoorRotation = FRotator(0, -90, 0);
            DoorTilePos = FIntVector(Node->RoomMin.X, GetRoomCenter(Node).Y, 0);

            // ���� ���� Ȯ�� �� ����
            EnsureCorridorConnection(DoorTilePos, FIntVector(-1, 0, 0), Node);
        }

        // �� ����
        FVector DoorLocation = RoomLocation + DoorOffset;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedDoor = World->SpawnActor<AActor>(DoorClass, DoorLocation, DoorRotation, SpawnParams);
        if (SpawnedDoor)
        {
            SpawnedDoor->Tags.Add("BSPGenerated");
            SpawnedDoor->Tags.Add("Door");

            // ���� ��� ���� ������ ���� (�ʿ�� ����)
            SpawnedDoor->SetActorScale3D(RoomScale);

            // ���� ���� �ڽ����� ���� (���û���)
            SpawnedDoor->AttachToActor(RoomActor, FAttachmentTransformRules::KeepWorldTransform);

            UE_LOG(LogTemp, Warning, TEXT("Spawned door at %s direction for room at (%f, %f)"),
                *Direction, RoomLocation.X / TileSize, RoomLocation.Y / TileSize);
        }
    }
}

TArray<FString> ABSPMapGenerator02::GetCorridorDirections(TSharedPtr<FBSPNode02> Node)
{
    TArray<FString> ConnectedDirections;

    if (!Node || !Node->bHasRoom) return ConnectedDirections;

    // ���� Ȯ�� (Y+ ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMax.Y;
        if (checkY < MapSize.Y && TileMap[x][checkY] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("North");
            break;
        }
    }

    // ���� Ȯ�� (Y- ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMin.Y - 1;
        if (checkY >= 0 && TileMap[x][checkY] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("South");
            break;
        }
    }

    // ���� Ȯ�� (X+ ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMax.X;
        if (checkX < MapSize.X && TileMap[checkX][y] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("East");
            break;
        }
    }

    // ���� Ȯ�� (X- ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMin.X - 1;
        if (checkX >= 0 && TileMap[checkX][y] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("West");
            break;
        }
    }

    return ConnectedDirections;
}

void ABSPMapGenerator02::SpawnWallsForRoom(TSharedPtr<FBSPNode02> Node, AActor* RoomActor)
{
    if (!Node || !Node->bHasRoom || !RoomActor || !WallClass) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FVector RoomLocation = RoomActor->GetActorLocation();
    FVector RoomScale = RoomActor->GetActorScale3D();

    // ������ ����� ���� Ȯ��
    TArray<FString> ConnectedDirections = GetCorridorDirections(Node);

    // 4���� ��� Ȯ���Ͽ� ������� ���� ���⿡ �� ����
    TArray<FString> AllDirections = { "North", "South", "East", "West" };

    for (const FString& Direction : AllDirections)
    {
        // �̹� ������ ����� ������ �ǳʶٱ� (���� ������ ����)
        if (ConnectedDirections.Contains(Direction))
        {
            continue;
        }

        // �� ��ġ�� ȸ�� ���
        FVector WallOffset = FVector::ZeroVector;
        FRotator WallRotation = FRotator::ZeroRotator;

        // ���� ���� ũ�� (������ ����� ũ��)
        float HalfSize = TileSize * RoomScale.X / 2.0f;

        if (Direction == "North")
        {
            // ���� �� (Y+ ����)
            WallOffset = FVector(0, HalfSize, 0);
            WallRotation = FRotator(0, 0, 0);
        }
        else if (Direction == "South")
        {
            // ���� �� (Y- ����)
            WallOffset = FVector(0, -HalfSize, 0);
            WallRotation = FRotator(0, 180, 0);
        }
        else if (Direction == "East")
        {
            // ���� �� (X+ ����)
            WallOffset = FVector(HalfSize, 0, 0);
            WallRotation = FRotator(0, 90, 0);
        }
        else if (Direction == "West")
        {
            // ���� �� (X- ����)
            WallOffset = FVector(-HalfSize, 0, 0);
            WallRotation = FRotator(0, -90, 0);
        }

        // �� ����
        FVector WallLocation = RoomLocation + WallOffset;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedWall = World->SpawnActor<AActor>(WallClass, WallLocation, WallRotation, SpawnParams);
        if (SpawnedWall)
        {
            SpawnedWall->Tags.Add("BSPGenerated");
            SpawnedWall->Tags.Add("Wall");

            // ���� ��� ���� ������ ���� (�ʿ�� ����)
            SpawnedWall->SetActorScale3D(RoomScale);

            // ���� ���� �ڽ����� ���� (���û���)
            SpawnedWall->AttachToActor(RoomActor, FAttachmentTransformRules::KeepWorldTransform);

            UE_LOG(LogTemp, Verbose, TEXT("Spawned wall at %s direction for room at (%f, %f)"),
                *Direction, RoomLocation.X / TileSize, RoomLocation.Y / TileSize);
        }
    }
}

void ABSPMapGenerator02::EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Direction, TSharedPtr<FBSPNode02> Node)
{
    // �� ��ġ���� Direction �������� 1ĭ ��ġ
    FIntVector CorridorPos = DoorPos + Direction;

    // �� ���� Ȯ��
    if (CorridorPos.X < 0 || CorridorPos.X >= MapSize.X ||
        CorridorPos.Y < 0 || CorridorPos.Y >= MapSize.Y)
    {
        return;
    }

    // �ش� ��ġ�� ��������� ���� ����
    if (TileMap[CorridorPos.X][CorridorPos.Y] == ETileType02::Empty)
    {
        // Ÿ�ϸʿ� ���� ǥ��
        TileMap[CorridorPos.X][CorridorPos.Y] = ETileType02::BridgeCorridor;

        // ���� Ÿ�� ��� ����
        SpawnSingleCorridorTile(CorridorPos);

        UE_LOG(LogTemp, Warning, TEXT("Created bridge BridgeCorridor at (%d, %d) for door connection"),
            CorridorPos.X, CorridorPos.Y);
    }
    // �̹� ������ ���� ������ �ƹ��͵� �� �� (���������� �����)
}

// ���� ���� Ÿ�� ���� �Լ�
void ABSPMapGenerator02::SpawnSingleCorridorTile(const FIntVector& TilePos)
{
    UWorld* World = GetWorld();
    if (!World || !CorridorCornerClass) return;

    FVector SpawnLocation = FVector(TilePos.X * TileSize, TilePos.Y * TileSize, 0);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // ���� ������ �׻� CorridorCornerClass(goalt01) ���
    // ��� ����� ���� �����ϹǷ� ���� ������

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SpawnedTile = World->SpawnActor<AActor>(CorridorCornerClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedTile)
    {
        SpawnedTile->Tags.Add("BSPGenerated");
        SpawnedTile->Tags.Add("BridgeCorridor"); // ���� �������� ǥ��

        UE_LOG(LogTemp, Verbose, TEXT("Spawned bridge corridor (corner tile) at (%d, %d)"),
            TilePos.X, TilePos.Y);
    }
}

//void ABSPMapGenerator02::CreateExtraConnections()
//{
//    if (LeafNodes.Num() < 3) return;  // ���� 3�� �̸��̸� �߰� ���� ���ʿ�
//
//    int32 ConnectionsAdded = 0;
//    int32 TargetConnections = RandomStream.RandRange(MinExtraConnections, MaxExtraConnections);
//
//    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);
//
//    // �߰� ������ ������ �ӽ� �迭
//    TArray<TPair<int32, int32>> ExtraEdges;
//
//    // ��� �� �ֿ� ���� ����
//    TArray<TPair<int32, int32>> PotentialConnections;
//
//    for (int32 i = 0; i < LeafNodes.Num(); ++i)
//    {
//        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
//        {
//            // �Ÿ��� ������ ��鸸 �ĺ��� �߰�
//            if (IsValidConnectionDistance(i, j))
//            {
//                PotentialConnections.Add(TPair<int32, int32>(i, j));
//            }
//        }
//    }
//
//    // �ĺ����� ��� �����ϰ� ����
//    for (int32 i = PotentialConnections.Num() - 1; i > 0; --i)
//    {
//        int32 j = RandomStream.RandRange(0, i);
//        PotentialConnections.Swap(i, j);
//    }
//
//    // �߰� ���� ����
//    for (const auto& Connection : PotentialConnections)
//    {
//        if (ConnectionsAdded >= TargetConnections)
//        {
//            break;
//        }
//
//        // Ȯ�������� ���� ����
//        if (RandomStream.FRand() < ExtraConnectionChance)
//        {
//            TSharedPtr<FBSPNode02> NodeA = LeafNodes[Connection.Key];
//            TSharedPtr<FBSPNode02> NodeB = LeafNodes[Connection.Value];
//
//            FIntVector CenterA = GetRoomCenter(NodeA);
//            FIntVector CenterB = GetRoomCenter(NodeB);
//
//            // �̹� ������ �����ϴ��� Ȯ��
//            if (!CorridorExists(CenterA, CenterB))
//            {
//                // �پ��� ���� ��Ÿ�� �� ���� ����
//                float CorridorStyle = RandomStream.FRand();
//
//                if (CorridorStyle < 0.5f)
//                {
//                    // L�� ���� (���� ��Ÿ��)
//                    CreateLShapedCorridor(CenterA, CenterB);
//                }
//                else if (CorridorStyle < 0.8f)
//                {
//                    // �� L�� ���� (�ݴ� ����)
//                    CreateLShapedCorridor(CenterB, CenterA);
//                }
//                else
//                {
//                    // ������� ���� (�� ������ ���)
//                    CreateZigzagCorridor(CenterA, CenterB);
//                }
//
//                ExtraEdges.Add(TPair<int32, int32>(Connection.Key, Connection.Value));
//
//                ConnectionsAdded++;
//
//                UE_LOG(LogTemp, Verbose, TEXT("Added extra connection %d between room %d and %d"),
//                    ConnectionsAdded, Connection.Key, Connection.Value);
//            }
//        }
//    }
//
//    ExtraConnectionPairs = ExtraEdges;
//
//    UE_LOG(LogTemp, Warning, TEXT("Created %d extra connections for maze complexity"), ConnectionsAdded);
//}

// �߰�: �ĺ� ��ΰ� ���� ������ "��¦ ����"�ϰ� ���� ������ ū�� ��
bool ABSPMapGenerator02::WouldCreateLongParallel(const FIntVector& Start, const FIntVector& End, float Tolerance /*=0.6f*/)
{
    int32 CheckPoints = 7; // ���� ���� �� (Ȧ�� ����)
    int32 AdjacentCount = 0;

    auto InBounds = [&](int32 x, int32 y)
        {
            return (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y);
        };

    for (int32 i = 1; i < CheckPoints; ++i)
    {
        float t = (float)i / (float)CheckPoints;
        int32 x = FMath::Lerp(Start.X, End.X, t);
        int32 y = FMath::Lerp(Start.Y, End.Y, t);

        if (!InBounds(x, y)) continue;

        // �����¿� �� ��� �� ĭ�̶� ������ "���� ����"���� ����
        bool bAdj =
            (InBounds(x + 1, y) && TileMap[x + 1][y] == ETileType02::Corridor) ||
            (InBounds(x - 1, y) && TileMap[x - 1][y] == ETileType02::Corridor) ||
            (InBounds(x, y + 1) && TileMap[x][y + 1] == ETileType02::Corridor) ||
            (InBounds(x, y - 1) && TileMap[x][y - 1] == ETileType02::Corridor);

        AdjacentCount += (bAdj ? 1 : 0);
    }

    // ������ Tolerance(60% ��) �̻��� ������ ������ "���� ���� ����"���� ����
    return AdjacentCount > (int32)(Tolerance * (float)CheckPoints);
}

void ABSPMapGenerator02::CreateExtraConnections()
{
    if (!bCreateLoops) return;

    int32 TargetConnections = RandomStream.RandRange(MinExtraConnections, MaxExtraConnections);
    int32 ConnectionsAdded = 0;

    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);

    // ��� �� �� �� �Ÿ� ����� �����ϴ� �ĺ� ����
    TArray<TPair<int32, int32>> PotentialConnections;
    for (int32 i = 0; i < LeafNodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
        {
            if (IsValidConnectionDistance(i, j))
            {
                PotentialConnections.Add(TPair<int32, int32>(i, j));
            }
        }
    }

    // ����
    for (int32 i = PotentialConnections.Num() - 1; i > 0; --i)
    {
        int32 j = RandomStream.RandRange(0, i);
        PotentialConnections.Swap(i, j);
    }

    // �߰� ���� ��� ����
    TArray<TPair<int32, int32>> ExtraEdges;

    for (const auto& Conn : PotentialConnections)
    {
        if (ConnectionsAdded >= TargetConnections) break;

        // ���� Ȯ��
        if (RandomStream.FRand() >= ExtraConnectionChance) continue;

        TSharedPtr<FBSPNode02> NodeA = LeafNodes[Conn.Key];
        TSharedPtr<FBSPNode02> NodeB = LeafNodes[Conn.Value];

        FIntVector CenterA = GetRoomCenter(NodeA);
        FIntVector CenterB = GetRoomCenter(NodeB);

        // 1) ���� ��ο� "���� ������" ������ �̹� ��� ������ ��ŵ (�ߺ� ����)
        if (CorridorExists(CenterA, CenterB)) continue; // ���� ���� Ȱ��. 

        // 2) �ĺ��� ���� ������ ��¦ �پ� (=, ||) �޸� ���ɼ��� ũ�� ��ŵ
        if (WouldCreateLongParallel(CenterA, CenterB, /*Tolerance=*/0.6f)) continue;

        // 3) ���� ����: "���� �켱 �� �Ұ� �� L-�ǹ�" ���� ����
        CreateCorridor(NodeA, NodeB); // ���� L/��-L/������� ���� ȣ���� ��ü. 

        ExtraEdges.Add(TPair<int32, int32>(Conn.Key, Conn.Value));
        ++ConnectionsAdded;

        UE_LOG(LogTemp, Verbose, TEXT("Added extra connection %d between room %d and %d"),
            ConnectionsAdded, Conn.Key, Conn.Value);
    }

    ExtraConnectionPairs = ExtraEdges;

    UE_LOG(LogTemp, Warning, TEXT("Created %d extra connections for maze complexity"), ConnectionsAdded);

    // ��ó���� ���� ���� �ܿ� ����(���Ἲ ������ ä ||, =�� �Ⱦ)
    CleanupParallelCorridors(); 



}


// �� �� ������ �Ÿ��� �������� Ȯ��
bool ABSPMapGenerator02::IsValidConnectionDistance(int32 RoomA, int32 RoomB)
{
    if (RoomA < 0 || RoomA >= LeafNodes.Num() ||
        RoomB < 0 || RoomB >= LeafNodes.Num())
    {
        return false;
    }

    FIntVector CenterA = GetRoomCenter(LeafNodes[RoomA]);
    FIntVector CenterB = GetRoomCenter(LeafNodes[RoomB]);

    float Distance = FVector::Dist(FVector(CenterA), FVector(CenterB));

    // �ʹ� �����ų� �ʹ� �� ���� ����
    return Distance > 5.0f && Distance < MaxConnectionDistance;
}

// ������ �̹� �����ϴ��� Ȯ��
bool ABSPMapGenerator02::CorridorExists(const FIntVector& Start, const FIntVector& End)
{
    // �� �� ������ ���� ��λ� �̹� ������ ���� �ִ��� üũ
    int32 CorridorCount = 0;
    int32 CheckPoints = 5;  // 5�� ������ ���ø�

    for (int32 i = 1; i < CheckPoints; ++i)
    {
        float t = (float)i / (float)CheckPoints;
        int32 CheckX = FMath::Lerp(Start.X, End.X, t);
        int32 CheckY = FMath::Lerp(Start.Y, End.Y, t);

        if (CheckX >= 0 && CheckX < MapSize.X &&
            CheckY >= 0 && CheckY < MapSize.Y)
        {
            if (TileMap[CheckX][CheckY] == ETileType02::Corridor ||
                TileMap[CheckX][CheckY] == ETileType02::BridgeCorridor)
            {
                CorridorCount++;
            }
        }
    }

    // �̹� ���� �̻��� ������ ����Ǿ� �ִٰ� �Ǵ�
    return CorridorCount > CheckPoints / 2;
}

// ������� ���� ����
void ABSPMapGenerator02::CreateZigzagCorridor(const FIntVector& Start, const FIntVector& End)
{
    // �߰� ���� ���
    int32 MidX = (Start.X + End.X) / 2;
    int32 MidY = (Start.Y + End.Y) / 2;

    // ���� ������ �߰�
    int32 OffsetRange = 3;
    MidX += RandomStream.RandRange(-OffsetRange, OffsetRange);
    MidY += RandomStream.RandRange(-OffsetRange, OffsetRange);

    // �� ���� ����
    MidX = FMath::Clamp(MidX, 0, MapSize.X - 1);
    MidY = FMath::Clamp(MidY, 0, MapSize.Y - 1);

    FIntVector MidPoint(MidX, MidY, 0);

    // Start -> Mid -> End ��� ����
    CreateLShapedCorridor(Start, MidPoint);
    CreateLShapedCorridor(MidPoint, End);
}

// �� �м� �� ��� ����
FMapStatistics ABSPMapGenerator02::AnalyzeMap()
{
    MapStats = FMapStatistics(); // �ʱ�ȭ

    // ���� ���� Ÿ���� �м��Ͽ� ������� ���ٸ� �� ����
    AnalyzeCorridorTypes();

    // �� ���� ī��Ʈ
    MapStats.RoomCount = LeafNodes.Num();

    // Ÿ�Ϻ� ī��Ʈ
    int32 TotalRoomTiles = 0;
    TSet<FIntVector> CountedDoors;

    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            switch (TileMap[x][y])
            {
            case ETileType02::Room:
                TotalRoomTiles++;
                break;

            case ETileType02::Corridor:
            case ETileType02::BridgeCorridor:
                MapStats.CorridorCount++;
                break;

            case ETileType02::DeadEnd:
                MapStats.DeadEndCount++;
                break;

            case ETileType02::Junction:
                MapStats.JunctionCount++;
                break;

            case ETileType02::CrossRoad:
                MapStats.CrossRoadCount++;
                break;
            }
        }
    }

    // �� ���� ī��Ʈ (���� ������ ���� ���)
    TArray<AActor*> FoundDoors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Door", FoundDoors);
    MapStats.DoorCount = FoundDoors.Num();

    // ��� �� ũ�� ���
    if (MapStats.RoomCount > 0)
    {
        MapStats.AverageRoomSize = (float)TotalRoomTiles / (float)MapStats.RoomCount;
    }

    // ��ü ���� ���� (��� ���� Ÿ���� ��)
    MapStats.TotalCorridorLength = MapStats.CorridorCount + MapStats.DeadEndCount +
        MapStats.JunctionCount + MapStats.CrossRoadCount;

    return MapStats;
}

// ���� Ÿ�� �м� �Լ�
void ABSPMapGenerator02::AnalyzeCorridorTypes()
{
    // ��� ���� Ÿ���� ��ȸ�ϸ� ���� ������ ���� Ÿ�� ����
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            // ���� Ÿ���� ��츸 ó��
            if (TileMap[x][y] == ETileType02::Corridor ||
                TileMap[x][y] == ETileType02::BridgeCorridor)
            {
                ETileType02 NewType = DetermineCorridorType(x, y);
                TileMap[x][y] = NewType;
            }
        }
    }
}

// Ư�� ��ġ�� ���� Ÿ�� ����
//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    int32 Connections = CountConnections(x, y);
//
//    switch (Connections)
//    {
//    case 0:
//        // ���� Ÿ�� (�Ϲ������� �߻����� ����)
//        return ETileType02::Corridor;
//
//    case 1:
//        // ���ٸ� ��
//        return ETileType02::DeadEnd;
//
//    case 2:
//        // �Ϲ� ����
//        return ETileType02::Corridor;
//
//    case 3:
//        // T�� ������
//        return ETileType02::Junction;
//
//    case 4:
//        // ���� ������
//        return ETileType02::CrossRoad;
//
//    default:
//        // ���� ��Ȳ
//        return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // ���������� ���� ����
//    int32 CorridorConnections = CountConnections(x, y);
//
//    // ��ü ���� ���� (�� ����) ���
//    int32 TotalConnections = CorridorConnections;
//
//    // ����� ���� Ȯ��
//    bool bConnectedToRoom = false;
//
//    // ���� Ȯ��
//    if (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // ���� Ȯ��
//    if (y > 0 && TileMap[x][y - 1] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // ���� Ȯ��
//    if (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // ���� Ȯ��
//    if (x > 0 && TileMap[x - 1][y] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // ���������� ���Ḹ���� Ÿ�� ����
//    switch (CorridorConnections)
//    {
//    case 0:
//        // ���� ������ ���� ���
//        if (bConnectedToRoom)
//        {
//            // ����� ����� ��� - ���ٸ� ��
//            return ETileType02::DeadEnd;
//        }
//        else
//        {
//            // ������ �� (�Ϲ������� �߻����� ����)
//            return ETileType02::Corridor;
//        }
//
//    case 1:
//        // ���� 1���� ����
//        if (bConnectedToRoom)
//        {
//            // ����� ����Ǿ� ������ �Ϲ� ���� (��-����-���� ����)
//            return ETileType02::Corridor;
//        }
//        else
//        {
//            // �����ϰ� ���� 1���� ���� - ���ٸ� ��
//            return ETileType02::DeadEnd;
//        }
//
//    case 2:
//        // ���� 2���� ���� - �Ϲ� ����
//        return ETileType02::Corridor;
//
//    case 3:
//        // ���� 3���� ���� - T�� ������
//        return ETileType02::Junction;
//
//    case 4:
//        // ���� 4���� ���� - ���ڷ�
//        return ETileType02::CrossRoad;
//
//    default:
//        return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // �̿�(���� �迭��) ? IsCorridorTile�� DeadEnd/Junction/CrossRoad�� ������ ��
//    // (�� ���� ����)  :contentReference[oaicite:3]{index=3}
//    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
//    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
//    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
//    const bool W = (x > 0) && IsCorridorTile(x - 1, y);
//
//    // ����: �ش� ��ǥ�� ����/���� "���(���� ����)"�� �Ϻ�����
//    auto IsHorizontalBand = [this](int32 cx, int32 cy)->bool {
//        return IsCorridorTile(cx - 1, cy) && IsCorridorTile(cx + 1, cy);
//        };
//    auto IsVerticalBand = [this](int32 cx, int32 cy)->bool {
//        return IsCorridorTile(cx, cy - 1) && IsCorridorTile(cx, cy + 1);
//        };
//
//    // ���� ���� ����(�� Ȯ��) ���� ó�� ����������������������������������������������������������
//    // ���� ����(E&W) + ��/�Ʒ� ���ʸ� ���� ���: ��/�Ʒ� �̿��� ���� ���� �б� �ƴ�
//    if (E && W && (N ^ S)) {
//        const int ny = N ? (y + 1) : (y - 1);
//        if (IsHorizontalBand(x, ny)) {
//            return ETileType02::Corridor; // �� Ȯ��� �������� ���
//        }
//    }
//    // ���� ����(N&S) + ��/�� ���ʸ� ���� ���: ��/�� �̿��� ���� ���� �б� �ƴ�
//    if (N && S && (E ^ W)) {
//        const int nx = E ? (x + 1) : (x - 1);
//        if (IsVerticalBand(nx, y)) {
//            return ETileType02::Corridor;
//        }
//    }
//    // ������������������������������������������������������������������������������������������������������������������������
//
//    // ���� ���� ���ܿ� �� �ɸ��� ���� ��Ģ���� �Ǵ�
//    const int connections = (N ? 1 : 0) + (S ? 1 : 0) + (E ? 1 : 0) + (W ? 1 : 0);
//
//    switch (connections)
//    {
//    case 1:  return ETileType02::DeadEnd;    // ���ٸ� ��
//    case 2:  return ETileType02::Corridor;   // ����/�ڳ�(�������� ȸ��)
//    case 3:  return ETileType02::Junction;   // T ������
//    case 4:  return ETileType02::CrossRoad;  // ����
//    default: return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // 1) ���� ���� �̿���
//    bool N = (y < MapSize.Y - 1 && IsCorridorTile(x, y + 1));
//    bool S = (y > 0 && IsCorridorTile(x, y - 1));
//    bool E = (x < MapSize.X - 1 && IsCorridorTile(x + 1, y));
//    bool W = (x > 0 && IsCorridorTile(x - 1, y));
//    int32 CorridorConnections = (int)N + (int)S + (int)E + (int)W;
//
//    // 2) �� ���� ����
//    bool RoomAdj =
//        (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room) ||
//        (y > 0 && TileMap[x][y - 1] == ETileType02::Room) ||
//        (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room) ||
//        (x > 0 && TileMap[x - 1][y] == ETileType02::Room);
//
//    // 3) BridgeCorridor�� �׻� �Ϲ� ���� ��� (�� �� �� ĭ ��ȣ)
//    if (TileMap[x][y] == ETileType02::BridgeCorridor)
//        return ETileType02::Corridor;
//
//    // 4) �濡 ���� ������ DeadEnd�� ���� �ʱ�
//    if (RoomAdj && CorridorConnections <= 1)
//        return ETileType02::Corridor;
//
//    // 5) 2ĭ �� ����(���� ����) ����: ���� ���� �پ����� �� ĭ�� ���������� �Ϲ� ����
//    auto conn = [&](int32 cx, int32 cy) { return CountConnections(cx, cy); }; // :contentReference[oaicite:4]{index=4}
//    if ((N && S && (E ^ W))) {
//        int sx = E ? x + 1 : x - 1;
//        if (sx >= 0 && sx < MapSize.X && conn(sx, y) == 2) return ETileType02::Corridor;
//    }
//    if ((E && W && (N ^ S))) {
//        int sy = N ? y + 1 : y - 1;
//        if (sy >= 0 && sy < MapSize.Y && conn(x, sy) == 2) return ETileType02::Corridor;
//    }
//
//    // 6) �⺻ ��Ģ
//    switch (CorridorConnections) {
//    case 0: return ETileType02::DeadEnd;
//    case 1: return ETileType02::DeadEnd;
//    case 2: return ETileType02::Corridor;
//    case 3: return ETileType02::Junction;
//    case 4: return ETileType02::CrossRoad;
//    default: return ETileType02::Corridor;
//    }
//}

ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
{
    // ���� �̿�(���� �迭�� ī��Ʈ)
    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
    const bool W = (x > 0) && IsCorridorTile(x - 1, y);
    const int connections = (N ? 1 : 0) + (S ? 1 : 0) + (E ? 1 : 0) + (W ? 1 : 0);

    // --- ��/�� ���� ��ȣ ----------------------------
    // 1) �� �� �� ĭ(BridgeCorridor)�� �׻� �Ϲ� ����
    if (TileMap[x][y] == ETileType02::BridgeCorridor)
        return ETileType02::Corridor;

    // 2) �� ���� ����
    const bool RoomAdj =
        (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room) ||
        (y > 0 && TileMap[x][y - 1] == ETileType02::Room) ||
        (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room) ||
        (x > 0 && TileMap[x - 1][y] == ETileType02::Room);

    // �濡 �پ� �ְ� ���� ������ 0~1���� DeadEnd�� ���� ����
    if (RoomAdj && connections <= 1)
        return ETileType02::Corridor;
    // -----------------------------------------------

    // --- �� �� ����(���� ����) ���� -----------------
    auto IsHorizontalBand = [this](int32 cx, int32 cy)->bool {
        return IsCorridorTile(cx - 1, cy) && IsCorridorTile(cx + 1, cy);
        };
    auto IsVerticalBand = [this](int32 cx, int32 cy)->bool {
        return IsCorridorTile(cx, cy - 1) && IsCorridorTile(cx, cy + 1);
        };

    // ���� ����(E&W) + ��/�Ʒ� ���ʸ� ���� ���: �ش� �̿��� ���� ���� �б� �ƴ�
    if (E && W && (N ^ S)) {
        const int ny = N ? (y + 1) : (y - 1);
        if (IsHorizontalBand(x, ny)) return ETileType02::Corridor;
    }
    // ���� ����(N&S) + ��/�� ���ʸ� ���� ���: �ش� �̿��� ���� ���� �б� �ƴ�
    if (N && S && (E ^ W)) {
        const int nx = E ? (x + 1) : (x - 1);
        if (IsVerticalBand(nx, y)) return ETileType02::Corridor;
    }
    // -----------------------------------------------

    // �⺻ ��Ģ
    switch (connections)
    {
    case 1:  return ETileType02::DeadEnd;
    case 2:  return ETileType02::Corridor;
    case 3:  return ETileType02::Junction;
    case 4:  return ETileType02::CrossRoad;
    default: return ETileType02::Corridor;
    }
}

// Ư�� ��ġ�� ���� ���� ī��Ʈ
int32 ABSPMapGenerator02::CountConnections(int32 x, int32 y)
{
    int32 ConnectionCount = 0;

    // 4���� Ȯ��
    // ���� (Y+)
    if (y < MapSize.Y - 1 && IsCorridorTile(x, y + 1))
    {
        ConnectionCount++;
    }

    // ���� (Y-)
    if (y > 0 && IsCorridorTile(x, y - 1))
    {
        ConnectionCount++;
    }

    // ���� (X+)
    if (x < MapSize.X - 1 && IsCorridorTile(x + 1, y))
    {
        ConnectionCount++;
    }

    // ���� (X-)
    if (x > 0 && IsCorridorTile(x - 1, y))
    {
        ConnectionCount++;
    }

    return ConnectionCount;
}

// Ÿ���� ���� �迭���� Ȯ��
bool ABSPMapGenerator02::IsCorridorTile(int32 x, int32 y)
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y)
    {
        return false;
    }

    ETileType02 Type = TileMap[x][y];

    // ����, ��, �� ��� ����� ������ ����
    return Type == ETileType02::Corridor ||
        Type == ETileType02::BridgeCorridor ||
        Type == ETileType02::DeadEnd ||
        Type == ETileType02::Junction ||
        Type == ETileType02::CrossRoad;
}

// ��� ��� �Լ�
void ABSPMapGenerator02::PrintMapStatistics()
{
    // ���� �� �м�
    AnalyzeMap();

    UE_LOG(LogTemp, Warning, TEXT("========== Map Statistics =========="));
    UE_LOG(LogTemp, Warning, TEXT("Rooms: %d"), MapStats.RoomCount);
    UE_LOG(LogTemp, Warning, TEXT("Average Room Size: %.2f tiles"), MapStats.AverageRoomSize);
    UE_LOG(LogTemp, Warning, TEXT("Doors: %d"), MapStats.DoorCount);
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== Corridor Analysis ==="));
    UE_LOG(LogTemp, Warning, TEXT("Normal Corridors: %d"), MapStats.CorridorCount);
    UE_LOG(LogTemp, Warning, TEXT("Dead Ends: %d"), MapStats.DeadEndCount);
    UE_LOG(LogTemp, Warning, TEXT("T-Junctions: %d"), MapStats.JunctionCount);
    UE_LOG(LogTemp, Warning, TEXT("Cross Roads: %d"), MapStats.CrossRoadCount);
    UE_LOG(LogTemp, Warning, TEXT("Total Corridor Length: %.0f tiles"), MapStats.TotalCorridorLength);
    UE_LOG(LogTemp, Warning, TEXT(""));

    // ���⵵ ���� ���
    float ComplexityIndex = 0.0f;
    if (MapStats.TotalCorridorLength > 0)
    {
        ComplexityIndex = (MapStats.JunctionCount * 2.0f + MapStats.CrossRoadCount * 3.0f) /
            MapStats.TotalCorridorLength * 100.0f;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Map Complexity ==="));   // �׽�Ʈ��
    UE_LOG(LogTemp, Warning, TEXT("Complexity Index: %.2f%%"), ComplexityIndex);

    if (ComplexityIndex < 10.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Map Type: Simple Linear Dungeon"));
    }
    else if (ComplexityIndex < 20.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Map Type: Moderate Branching Dungeon"));
    }
    else if (ComplexityIndex < 30.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Map Type: Complex Maze"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Map Type: Highly Complex Labyrinth"));
    }

    UE_LOG(LogTemp, Warning, TEXT("===================================="));

    

    // �׷��� �м� �߰�
    CalculateCyclomaticComplexity();

    // ����׿� �ð�ȭ
    if (bShowStatistics)
    {
        DrawDebugVisualization();
    }


}

// ����� �ð�ȭ �Լ�
void ABSPMapGenerator02::DrawDebugVisualization()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // ���ٸ� ���� ���������� ǥ��
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            FVector Location = FVector(x * TileSize, y * TileSize, 100.0f);
            FColor DebugColor = FColor::White;

            switch (TileMap[x][y])
            {
            case ETileType02::DeadEnd:
                DebugColor = FColor::Red;
                DrawDebugSphere(World, Location, 50.0f, 8, DebugColor, true, 10.0f);
                break;

            case ETileType02::Junction:
                DebugColor = FColor::Yellow;
                DrawDebugSphere(World, Location, 50.0f, 8, DebugColor, true, 10.0f);
                break;

            case ETileType02::CrossRoad:
                DebugColor = FColor::Green;
                DrawDebugSphere(World, Location, 50.0f, 8, DebugColor, true, 10.0f);
                break;
            }
        }
    }

    // ���� �ð�ȭ - ���� ���� ��θ� ���� �׸���
    for (const GraphEdge& Edge : GraphEdges)
    {
        // ��ΰ� ������ ��θ� ���� �׸���
        if (Edge.Path.Num() > 1)
        {
            for (int32 i = 0; i < Edge.Path.Num() - 1; ++i)
            {
                FVector StartLocation = FVector(Edge.Path[i].X * TileSize,
                    Edge.Path[i].Y * TileSize,
                    150.0f);
                FVector EndLocation = FVector(Edge.Path[i + 1].X * TileSize,
                    Edge.Path[i + 1].Y * TileSize,
                    150.0f);

                // ���� ��θ� ���� ����� �� �׸���
                DrawDebugLine(World, StartLocation, EndLocation, FColor::Magenta, true, 10.0f, 0, 10.0f);
            }
        }
        //else if (Edge.StartNode < GraphNodes.Num() && Edge.EndNode < GraphNodes.Num())
        //{
        //    // ��ΰ� ������ �������� (����)
        //    const GraphNode& StartNode = GraphNodes[Edge.StartNode];
        //    const GraphNode& EndNode = GraphNodes[Edge.EndNode];

        //    FVector StartLocation = FVector(StartNode.Position.X * TileSize,
        //        StartNode.Position.Y * TileSize,
        //        150.0f);
        //    FVector EndLocation = FVector(EndNode.Position.X * TileSize,
        //        EndNode.Position.Y * TileSize,
        //        150.0f);

        //    DrawDebugLine(World, StartLocation, EndLocation, FColor::Cyan, true, 10.0f, 0, 6.0f);
        //}
    }

    UE_LOG(LogTemp, Warning, TEXT("Drew %d edges"), GraphEdges.Num());

}


// Ÿ���� ������� Ȯ�� (������, ���ٸ� ��, �����θ� - ���� ���� ó��)
bool ABSPMapGenerator02::IsNodeTile(int32 x, int32 y)
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y)
        return false;

    ETileType02 Type = TileMap[x][y];
    return Type == ETileType02::DeadEnd ||
        Type == ETileType02::Junction ||
        Type == ETileType02::CrossRoad;
}

// Ư�� ���� �߽��� ã��
FIntVector ABSPMapGenerator02::FindRoomCenter(int32 RoomId)
{
    if (RoomId >= 0 && RoomId < LeafNodes.Num() && LeafNodes[RoomId]->bHasRoom)
    {
        return GetRoomCenter(LeafNodes[RoomId]);
    }
    return FIntVector::ZeroValue;
}

// �ʿ��� �׷��� ���� ����
void ABSPMapGenerator02::BuildGraphFromMap()
{
    GraphNodes.Empty();
    GraphEdges.Empty();
    NodePositionToIndex.Empty();

    // 1�ܰ�: ���� ���� �߰� (�� ��� �ϳ��� ���)
    for (int32 i = 0; i < LeafNodes.Num(); ++i)
    {
        if (LeafNodes[i]->bHasRoom)
        {
            GraphNode NewNode;
            NewNode.Position = GetRoomCenter(LeafNodes[i]);
            NewNode.Type = ETileType02::Room;
            NewNode.RoomId = i;

            int32 NodeIndex = GraphNodes.Add(NewNode);
            NodePositionToIndex.Add(NewNode.Position, NodeIndex);

            // ���� ��� Ÿ�� ��ġ�� �� ���� ����
            for (int32 x = LeafNodes[i]->RoomMin.X; x < LeafNodes[i]->RoomMax.X; ++x)
            {
                for (int32 y = LeafNodes[i]->RoomMin.Y; y < LeafNodes[i]->RoomMax.Y; ++y)
                {
                    if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
                    {
                        NodePositionToIndex.Add(FIntVector(x, y, 0), NodeIndex);
                    }
                }
            }
        }
    }

    // 2�ܰ�: ������ Ư���� �������� ���� �߰� (������, ���ٸ� ��, ������)
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            if (IsNodeTile(x, y))
            {
                GraphNode NewNode;
                NewNode.Position = FIntVector(x, y, 0);
                NewNode.Type = TileMap[x][y];
                NewNode.RoomId = -1;

                int32 NodeIndex = GraphNodes.Add(NewNode);
                NodePositionToIndex.Add(NewNode.Position, NodeIndex);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Found %d nodes (Rooms: %d, Corridor nodes: %d)"),
        GraphNodes.Num(), LeafNodes.Num(), GraphNodes.Num() - LeafNodes.Num());

    // 3�ܰ�: ��� �� ����(����) ã��
    TSet<TPair<int32, int32>> ProcessedEdges; // �ߺ� ����

    for (int32 i = 0; i < GraphNodes.Num(); ++i)
    {
        FIntVector StartPos = GraphNodes[i].Position;

        // ���� ��ġ ���� (���� ��� �����ڸ� Ÿ�ϵ鿡�� ����)
        TArray<FIntVector> StartPositions;

        if (GraphNodes[i].Type == ETileType02::Room)
        {
            // ���� �����ڸ����� ������ ����� ���� ã��
            int32 RoomId = GraphNodes[i].RoomId;
            if (RoomId >= 0 && RoomId < LeafNodes.Num())
            {
                auto& Node = LeafNodes[RoomId];

                // ���� �����ڸ� Ÿ�� �˻�
                for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
                {
                    // ���� �����ڸ�
                    int32 y = Node->RoomMax.Y - 1;
                    if (y + 1 < MapSize.Y && IsCorridorTile(x, y + 1))
                    {
                        StartPositions.Add(FIntVector(x, y + 1, 0));
                    }
                    // ���� �����ڸ�
                    y = Node->RoomMin.Y;
                    if (y - 1 >= 0 && IsCorridorTile(x, y - 1))
                    {
                        StartPositions.Add(FIntVector(x, y - 1, 0));
                    }
                }

                for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
                {
                    // ���� �����ڸ�
                    int32 x = Node->RoomMax.X - 1;
                    if (x + 1 < MapSize.X && IsCorridorTile(x + 1, y))
                    {
                        StartPositions.Add(FIntVector(x + 1, y, 0));
                    }
                    // ���� �����ڸ�
                    x = Node->RoomMin.X;
                    if (x - 1 >= 0 && IsCorridorTile(x - 1, y))
                    {
                        StartPositions.Add(FIntVector(x - 1, y, 0));
                    }
                }
            }
        }
        else
        {
            // ���� ���� �ڱ� ��ġ���� ����
            StartPositions.Add(StartPos);
        }

        // �� ���� ��ġ���� ������ ���󰡸� ���� ��� ã��
        for (const FIntVector& StartCorridorPos : StartPositions)
        {
            // ������ ���󰡸� ���� ������ ����
            TArray<FIntVector> Path;
            Path.Add(StartCorridorPos);

            FIntVector CurrentPos = StartCorridorPos;
            FIntVector PrevPos;
            if (GraphNodes[i].Type == ETileType02::Room)
            {
                // �濡�� ������ ������ ������ �ݴ� ���� ���
                FIntVector Direction = StartCorridorPos - StartPos;
                FIntVector SignVector(
                    Direction.X != 0 ? (Direction.X > 0 ? 1 : -1) : 0,
                    Direction.Y != 0 ? (Direction.Y > 0 ? 1 : -1) : 0,
                    0
                );
                PrevPos = StartCorridorPos - SignVector;
            }
            else
            {
                PrevPos = StartPos;
            }

            bool bFoundNode = false;

            // �ִ� 100ĭ������ ���󰡱� (���ѷ��� ����)
            for (int32 Steps = 0; Steps < 100; ++Steps)
            {
                // ���� ��ġ���� ���� ��ġ ã��
                TArray<FIntVector> Directions = {
                    FIntVector(0, 1, 0), FIntVector(0, -1, 0),
                    FIntVector(1, 0, 0), FIntVector(-1, 0, 0)
                };

                bool bFoundNext = false;
                for (const FIntVector& Dir : Directions)
                {
                    FIntVector NextPos = CurrentPos + Dir;

                    if (NextPos == PrevPos) continue; // �ǵ��ư��� �ʱ�

                    if (NextPos.X >= 0 && NextPos.X < MapSize.X &&
                        NextPos.Y >= 0 && NextPos.Y < MapSize.Y)
                    {
                        // �ٸ� ��忡 �����ߴ��� Ȯ��
                        if (NodePositionToIndex.Contains(NextPos))
                        {
                            int32 EndNodeIndex = NodePositionToIndex[NextPos];

                            if (EndNodeIndex != i) // �ڱ� �ڽ��� �ƴ� ���
                            {
                                // �ߺ� ���� üũ
                                TPair<int32, int32> EdgePair(FMath::Min(i, EndNodeIndex),
                                    FMath::Max(i, EndNodeIndex));

                                if (!ProcessedEdges.Contains(EdgePair))
                                {
                                    ProcessedEdges.Add(EdgePair);

                                    GraphEdge NewEdge;
                                    NewEdge.StartNode = i;
                                    NewEdge.EndNode = EndNodeIndex;
                                    NewEdge.Length = Path.Num();
                                    NewEdge.Path = Path;

                                    GraphEdges.Add(NewEdge);
                                    GraphNodes[i].Edges.Add(EndNodeIndex);
                                    GraphNodes[EndNodeIndex].Edges.Add(i);
                                }
                                bFoundNode = true;
                                break;
                            }
                        }
                        // ���� Ÿ���̸� ��� ����
                        else if (IsGraphPassable(NextPos.X, NextPos.Y) &&
                            !IsNodeTile(NextPos.X, NextPos.Y))
                        {
                            PrevPos = CurrentPos;
                            CurrentPos = NextPos;
                            Path.Add(CurrentPos);
                            bFoundNext = true;
                            break;
                        }
                    }
                }

                if (bFoundNode || !bFoundNext) break;
            }
        }
    }

    auto FindDirectCorridorPath =
        [&](const FIntVector& Start, const FIntVector& Goal,
            TArray<FIntVector>& OutPath, int32 StartNodeIndex, int32 EndNodeIndex) -> bool
        {
            OutPath.Reset();

            auto InBounds = [&](int x, int y) {
                return (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y);
                };

            // ����/�긴��/���� ���, �߰� ���(����/�� ����)�� ����
            auto IsPassableDirect = [&](const FIntVector& P) -> bool {
                if (!InBounds(P.X, P.Y)) return false;
                ETileType02 T = TileMap[P.X][P.Y];
                bool bPass = (T == ETileType02::Corridor ||
                    T == ETileType02::BridgeCorridor ||
                    T == ETileType02::Door);
                if (!bPass) return false;

                if (NodePositionToIndex.Contains(P)) {
                    int32 idx = NodePositionToIndex[P];
                    if (idx != StartNodeIndex && idx != EndNodeIndex) return false; // �߰� ��� ����
                }
                return true;
                };

            // BFS
            TQueue<FIntVector> Q;
            TSet<FIntVector> Visited;
            TMap<FIntVector, FIntVector> Parent;

            Q.Enqueue(Start);
            Visited.Add(Start);

            const FIntVector Dirs[4] = { {1,0,0},{-1,0,0},{0,1,0},{0,-1,0} };

            auto Reconstruct = [&](FIntVector cur) {
                OutPath.Empty();
                while (Parent.Contains(cur)) { OutPath.Add(cur); cur = Parent[cur]; }
                OutPath.Add(Start);
                Algo::Reverse(OutPath);
                };

            while (!Q.IsEmpty()) {
                FIntVector cur; Q.Dequeue(cur);

                // ��ǥ ����: ��ǥ ��ǥ�� �����ϰų�, End ��� Ÿ�Ͽ� ������ ����
                if (cur == Goal ||
                    (NodePositionToIndex.Contains(cur) && NodePositionToIndex[cur] == EndNodeIndex)) {
                    Reconstruct(cur);
                    return true;
                }

                for (auto d : Dirs) {
                    FIntVector nxt = cur + d;
                    if (Visited.Contains(nxt)) continue;

                    // End ��� Ÿ���� �ٷ� ���
                    if (NodePositionToIndex.Contains(nxt) && NodePositionToIndex[nxt] == EndNodeIndex) {
                        Parent.Add(nxt, cur);
                        Visited.Add(nxt);
                        Reconstruct(nxt);
                        return true;
                    }

                    if (!IsPassableDirect(nxt)) continue;

                    Visited.Add(nxt);
                    Parent.Add(nxt, cur);
                    Q.Enqueue(nxt);
                }
            }
            return false;
        };

    // 3.5�ܰ�: ��ģ ���� ã�� (�߰� ������ ���� ���� ����)
    // ��� ��� ���� Ȯ���Ͽ� ������ ���� ����Ǿ� �ִ��� Ȯ��
    for (int32 i = 0; i < GraphNodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < GraphNodes.Num(); ++j)
        {
            // �̹� ������ �ִ��� Ȯ��
            bool bAlreadyConnected = false;
            for (const GraphEdge& Edge : GraphEdges)
            {
                if ((Edge.StartNode == i && Edge.EndNode == j) ||
                    (Edge.StartNode == j && Edge.EndNode == i))
                {
                    bAlreadyConnected = true;
                    break;
                }
            }

            if (!bAlreadyConnected)
            {
                // BFS�� ������ ���� ���� ��� ã��
                TArray<FIntVector> Path;
                if (FindDirectCorridorPath(GraphNodes[i].Position, GraphNodes[j].Position, Path, i, j))
                {
                    GraphEdge NewEdge;
                    NewEdge.StartNode = i;
                    NewEdge.EndNode = j;
                    NewEdge.Length = Path.Num();
                    NewEdge.Path = Path;

                    GraphEdges.Add(NewEdge);
                    GraphNodes[i].Edges.Add(j);
                    GraphNodes[j].Edges.Add(i);
                }
            }
        }
    }

    // �������� �߰� ���� �������� �׷����� �߰�
    for (const auto& ExtraPair : ExtraConnectionPairs)
    {
        // �� �ε����� ��� �ε����� ��ȯ
        int32 NodeIndexA = -1;
        int32 NodeIndexB = -1;

        // �� ID�� ��� �ε��� ã��
        for (int32 i = 0; i < GraphNodes.Num(); ++i)
        {
            if (GraphNodes[i].Type == ETileType02::Room)
            {
                if (GraphNodes[i].RoomId == ExtraPair.Key)
                    NodeIndexA = i;
                if (GraphNodes[i].RoomId == ExtraPair.Value)
                    NodeIndexB = i;
            }
        }

        if (NodeIndexA != -1 && NodeIndexB != -1)
        {
            // �ߺ� üũ
            bool bAlreadyExists = false;
            for (const GraphEdge& Edge : GraphEdges)
            {
                if ((Edge.StartNode == NodeIndexA && Edge.EndNode == NodeIndexB) ||
                    (Edge.StartNode == NodeIndexB && Edge.EndNode == NodeIndexA))
                {
                    bAlreadyExists = true;
                    break;
                }
            }

            if (!bAlreadyExists)
            {
                /*GraphEdge NewEdge;
                NewEdge.StartNode = NodeIndexA;
                NewEdge.EndNode = NodeIndexB;*/

                //// ��� ã�� (�ɼ�)
                //TArray<FIntVector> Path;
                //if (FindCorridorPath(GraphNodes[NodeIndexA].Position,
                //    GraphNodes[NodeIndexB].Position, Path))
                //{
                //    NewEdge.Path = Path;
                //    NewEdge.Length = Path.Num();
                //}
                //else
                //{
                //    // ��θ� �� ã���� ���� �Ÿ� ���
                //    NewEdge.Length = FVector::Dist(
                //        FVector(GraphNodes[NodeIndexA].Position),
                //        FVector(GraphNodes[NodeIndexB].Position)
                //    );
                //}

                TArray<FIntVector> Path;
                if (FindDirectCorridorPath(GraphNodes[NodeIndexA].Position,
                    GraphNodes[NodeIndexB].Position, Path, NodeIndexA, NodeIndexB))
                {
                    GraphEdge NewEdge;
                    NewEdge.StartNode = NodeIndexA;
                    NewEdge.EndNode = NodeIndexB;
                    NewEdge.Path = Path;
                    NewEdge.Length = Path.Num();

                    GraphEdges.Add(NewEdge);
                    GraphNodes[NodeIndexA].Edges.AddUnique(NodeIndexB);
                    GraphNodes[NodeIndexB].Edges.AddUnique(NodeIndexA);
                }

                /*GraphEdges.Add(NewEdge);
                GraphNodes[NodeIndexA].Edges.AddUnique(NodeIndexB);
                GraphNodes[NodeIndexB].Edges.AddUnique(NodeIndexA);*/

               
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Found %d edges in the graph"), GraphEdges.Num());
}

// DFS�� ����� ������Ʈ Ž��
void ABSPMapGenerator02::DFSComponent(int32 NodeIndex, TArray<bool>& Visited)
{
    if (NodeIndex < 0 || NodeIndex >= GraphNodes.Num() || Visited[NodeIndex])
        return;

    Visited[NodeIndex] = true;

    for (int32 ConnectedNode : GraphNodes[NodeIndex].Edges)
    {
        DFSComponent(ConnectedNode, Visited);
    }
}

// �׷����� ����� ������Ʈ �� ���
int32 ABSPMapGenerator02::CountGraphComponents()
{
    if (GraphNodes.Num() == 0) return 0;

    TArray<bool> Visited;
    Visited.SetNum(GraphNodes.Num());
    for (int32 i = 0; i < Visited.Num(); ++i)
    {
        Visited[i] = false;
    }

    int32 ComponentCount = 0;

    for (int32 i = 0; i < GraphNodes.Num(); ++i)
    {
        if (!Visited[i])
        {
            DFSComponent(i, Visited);
            ComponentCount++;
        }
    }

    return ComponentCount;
}

// ��ȯ ���⵵ ��� �� ���
void ABSPMapGenerator02::CalculateCyclomaticComplexity()
{
    // ���� �׷��� ����
    BuildGraphFromMap();

    // ��� Ÿ�Ժ� ī��Ʈ
    int32 RoomNodes = 0;
    int32 DeadEndNodes = 0;
    int32 JunctionNodes = 0;
    int32 CrossRoadNodes = 0;

    for (const GraphNode& Node : GraphNodes)
    {
        switch (Node.Type)
        {
        case ETileType02::Room:
            RoomNodes++;
            break;
        case ETileType02::DeadEnd:
            DeadEndNodes++;
            break;
        case ETileType02::Junction:
            JunctionNodes++;
            break;
        case ETileType02::CrossRoad:
            CrossRoadNodes++;
            break;
        }
    }

    // �׷��� ��Ʈ����
    int32 N = GraphNodes.Num();  // ��� ��
    int32 E = GraphEdges.Num();  // ���� ��
    int32 P = CountGraphComponents();  // ����� ������Ʈ ��

    // ��ȯ ���⵵ ��� (V(G) = E - N + P)
    int32 CyclomaticComplexity = E - N + P;

    // ��� ��� ���� (Average Degree)
    float AvgDegree = 0.0f;
    if (N > 0)
    {
        int32 TotalDegree = 0;
        for (const GraphNode& Node : GraphNodes)
        {
            TotalDegree += Node.Edges.Num();
        }
        AvgDegree = (float)TotalDegree / (float)N;
    }

    // ��� ���� ���� (���� ����)
    float AvgEdgeLength = 0.0f;
    if (E > 0)
    {
        int32 TotalLength = 0;
        for (const GraphEdge& Edge : GraphEdges)
        {
            TotalLength += Edge.Length;
        }
        AvgEdgeLength = (float)TotalLength / (float)E;
    }

    // ��� ���
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("========== Graph Analysis =========="));
    UE_LOG(LogTemp, Warning, TEXT("=== Node Breakdown ==="));
    UE_LOG(LogTemp, Warning, TEXT("Room Nodes: %d"), RoomNodes);
    UE_LOG(LogTemp, Warning, TEXT("Dead End Nodes: %d"), DeadEndNodes);
    UE_LOG(LogTemp, Warning, TEXT("T-Junction Nodes: %d"), JunctionNodes);
    UE_LOG(LogTemp, Warning, TEXT("CrossRoad Nodes: %d"), CrossRoadNodes);
    UE_LOG(LogTemp, Warning, TEXT("Total Nodes (N): %d"), N);
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== Graph Metrics ==="));
    UE_LOG(LogTemp, Warning, TEXT("Edges (E): %d"), E);
    UE_LOG(LogTemp, Warning, TEXT("Connected Components (P): %d"), P);
    UE_LOG(LogTemp, Warning, TEXT("Average Node Degree: %.2f"), AvgDegree);
    UE_LOG(LogTemp, Warning, TEXT("Average Corridor Length: %.2f tiles"), AvgEdgeLength);
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== Cyclomatic Complexity ==="));
    UE_LOG(LogTemp, Warning, TEXT("V(G) = E - N + P = %d - %d + %d = %d"), E, N, P, CyclomaticComplexity);

    // ���⵵ �ؼ�
    if (CyclomaticComplexity <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Tree-like (No cycles)"));
    }
    else if (CyclomaticComplexity <= 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Simple with few loops"));
    }
    else if (CyclomaticComplexity <= 7)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Moderate complexity"));
    }
    else if (CyclomaticComplexity <= 15)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Complex maze"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Highly complex labyrinth"));
    }

    // ���Ἲ �м�
    if (P == 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: Fully connected dungeon"));
    }
    else if (P == N)
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: No connections (isolated nodes)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: %d separate regions (should be 1 for proper dungeon)"), P);
    }

    // �߰� �м�
    if (E < N - 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("WARNING: Graph is not fully connected! Need at least %d edges."), N - 1);
    }

    UE_LOG(LogTemp, Warning, TEXT("===================================="));
}

bool ABSPMapGenerator02::FindCorridorPath(const FIntVector& Start, const FIntVector& End, TArray<FIntVector>& OutPath)
{
    OutPath.Reset();

    auto InBounds = [this](int32 x, int32 y)
        {
            return (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y);
        };

    auto IsPassable = [this, &InBounds](const FIntVector& P)
        {
            if (!InBounds(P.X, P.Y)) return false;
            ETileType02 T = TileMap[P.X][P.Y];
            // ���� �迭 + ���� ��� ����
            return T == ETileType02::Corridor
                || T == ETileType02::BridgeCorridor
                || T == ETileType02::DeadEnd
                || T == ETileType02::Junction
                || T == ETileType02::CrossRoad
                || T == ETileType02::Door;
        };

    // ������/�������� �� �߽� �� ���� �ٱ��� �� ������, ���� ���� Ÿ���� �õ�� ���
    auto CollectSeeds = [&](const FIntVector& P, TArray<FIntVector>& OutSeeds)
        {
            if (IsPassable(P))
            {
                OutSeeds.Add(P);
                return;
            }
            static const int32 DX[4] = { 1, -1, 0, 0 };
            static const int32 DY[4] = { 0, 0, 1, -1 };
            for (int k = 0; k < 4; ++k)
            {
                FIntVector N(P.X + DX[k], P.Y + DY[k], 0);
                if (IsPassable(N)) OutSeeds.Add(N);
            }
        };

    TArray<FIntVector> StartSeeds, EndSeeds;
    CollectSeeds(Start, StartSeeds);
    CollectSeeds(End, EndSeeds);
    if (StartSeeds.Num() == 0 || EndSeeds.Num() == 0) return false;

    TSet<FIntVector> GoalSet;
    for (const auto& G : EndSeeds) GoalSet.Add(G);

    // BFS
    TQueue<FIntVector> Q;
    TSet<FIntVector> Visited;
    TMap<FIntVector, FIntVector> Prev;

    for (const auto& S : StartSeeds)
    {
        Q.Enqueue(S);
        Visited.Add(S);
        Prev.Add(S, FIntVector(INT32_MIN, INT32_MIN, INT32_MIN)); // ���� ǥ��
    }

    static const int32 DX[4] = { 1, -1, 0, 0 };
    static const int32 DY[4] = { 0, 0, 1, -1 };

    bool bFound = false;
    FIntVector Found;
    while (!Q.IsEmpty())
    {
        FIntVector Cur; Q.Dequeue(Cur);

        if (GoalSet.Contains(Cur))
        {
            Found = Cur;
            bFound = true;
            break;
        }

        for (int k = 0; k < 4; ++k)
        {
            FIntVector N(Cur.X + DX[k], Cur.Y + DY[k], 0);
            if (IsPassable(N) && !Visited.Contains(N))
            {
                Visited.Add(N);
                Prev.Add(N, Cur);
                Q.Enqueue(N);
            }
        }
    }

    if (!bFound) return false;

    // ��� ����
    TArray<FIntVector> Rev;
    FIntVector P = Found;
    while (Prev.Contains(P) && Prev[P].X != INT32_MIN)
    {
        Rev.Add(P);
        P = Prev[P];
    }
    Rev.Add(P);
    Algo::Reverse(Rev);
    OutPath = MoveTemp(Rev);
    return true;
}

bool ABSPMapGenerator02::IsGraphPassable(int32 x, int32 y) const
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y) return false;
    ETileType02 t = TileMap[x][y];
    // �׷��� ������: ���� �迭 + Door�� ��� ��� (Room�� �Ұ�)
    return (t == ETileType02::Corridor ||
        t == ETileType02::BridgeCorridor ||
        t == ETileType02::DeadEnd ||
        t == ETileType02::Junction ||
        t == ETileType02::CrossRoad ||
        t == ETileType02::Door);
}

void ABSPMapGenerator02::CleanupParallelCorridors()
{
    UE_LOG(LogTemp, Warning, TEXT("Starting parallel corridor cleanup..."));

    int32 RemovedCount = 0;

    // ��Ȯ�� || �����̳� = ���ϸ� ã�Ƽ� ����
    for (int32 x = 1; x < MapSize.X - 2; ++x)  // ���� ���� Ȯ��
    {
        for (int32 y = 1; y < MapSize.Y - 2; ++y)
        {
            // ���� ��ġ�� �������� Ȯ��
            if (TileMap[x][y] != ETileType02::Corridor) continue;

            // ���� ���� ���� üũ (|| ����)
            // ����� �������� ��� �����̰�, �� �� ���Ʒ��θ� ����� ���
            if (TileMap[x + 1][y] == ETileType02::Corridor)
            {
                // ���� Ÿ��: ���Ʒ��� ����, �¿�� ���� �ȵ�
                bool currentVerticalOnly =
                    IsCorridorTile(x, y - 1) && IsCorridorTile(x, y + 1) &&  // ���Ʒ� ����
                    !IsCorridorTile(x - 1, y) && !IsCorridorTile(x + 2, y);  // �翷 ����

                // ������ Ÿ��: ���Ʒ��� ����, �¿�� ���� �ȵ� (���� ����)
                bool rightVerticalOnly =
                    IsCorridorTile(x + 1, y - 1) && IsCorridorTile(x + 1, y + 1) &&  // ���Ʒ� ����
                    !IsCorridorTile(x + 2, y);  // ������ ���� (������ ����� üũ ����)

                if (currentVerticalOnly && rightVerticalOnly)
                {
                    // �� �� �ϳ� ���� (������ ����)
                    TileMap[x + 1][y] = ETileType02::Empty;
                    RemovedCount++;

                    UE_LOG(LogTemp, Verbose, TEXT("Removed vertical parallel corridor at (%d, %d)"), x + 1, y);
                }
            }

            // ���� ���� ���� üũ (= ����)  
            // ����� ������ ��� �����̰�, �� �� �¿�θ� ����� ���
            if (TileMap[x][y + 1] == ETileType02::Corridor)
            {
                // ���� Ÿ��: �¿츸 ����, ���Ʒ��� ���� �ȵ�
                bool currentHorizontalOnly =
                    IsCorridorTile(x - 1, y) && IsCorridorTile(x + 1, y) &&  // �¿� ����
                    !IsCorridorTile(x, y - 1) && !IsCorridorTile(x, y + 2);  // ���Ʒ� ����

                // ���� Ÿ��: �¿츸 ����, ���Ʒ��� ���� �ȵ� (���� ����)
                bool upHorizontalOnly =
                    IsCorridorTile(x - 1, y + 1) && IsCorridorTile(x + 1, y + 1) &&  // �¿� ����
                    !IsCorridorTile(x, y + 2);  // ���� ���� (�Ʒ��� ����� üũ ����)

                if (currentHorizontalOnly && upHorizontalOnly)
                {
                    // �� �� �ϳ� ���� (���� ����)
                    TileMap[x][y + 1] = ETileType02::Empty;
                    RemovedCount++;

                    UE_LOG(LogTemp, Verbose, TEXT("Removed horizontal parallel corridor at (%d, %d)"), x, y + 1);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Removed %d parallel corridor tiles"), RemovedCount);
}