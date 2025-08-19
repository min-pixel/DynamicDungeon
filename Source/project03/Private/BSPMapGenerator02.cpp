#include "BSPMapGenerator02.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h" 

ABSPMapGenerator02::ABSPMapGenerator02()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // 기본 타일 경로 설정
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
    // 기존 맵 정리
    ClearMap();

    // 랜덤 시드 설정
    if (RandomSeed == 0)
    {
        RandomSeed = FMath::RandRange(1, INT32_MAX);
    }
    RandomStream.Initialize(RandomSeed);

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Started with Seed: %d"), RandomSeed);

    // 타일 맵 초기화
    InitializeTileMap();

    // BSP 트리 생성
    RootNode = CreateBSPTree(FIntVector(0, 0, 0), MapSize, 0);

    // 리프 노드 수집
    LeafNodes.Empty();
    CollectLeafNodes(RootNode);

    // 방 생성
    CreateRooms();

    // 방 연결
    ConnectRooms();

    // 추가 연결 생성 (미로 복잡도 증가)
    if (bCreateLoops)
    {
        CreateExtraConnections();
    }

    CleanupParallelCorridors();

   

    // 타일 스폰
    SpawnTiles();

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed. Created %d rooms"), LeafNodes.Num());

    // ===== 그래프 분석 시작 =====
    if (GraphAnalyzer)
    {
        // TileMap을 EDungeonTileType으로 변환
        TArray<TArray<EDungeonTileType>> ConvertedTileMap;
        ConvertedTileMap.SetNum(MapSize.X);

        for (int32 x = 0; x < MapSize.X; ++x)
        {
            ConvertedTileMap[x].SetNum(MapSize.Y);
            for (int32 y = 0; y < MapSize.Y; ++y)
            {
                // ETileType02를 EDungeonTileType으로 변환
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

        // 방 정보 준비
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

        // 그래프 분석 실행 (C++ 함수 직접 호출)
        GraphAnalyzer->AnalyzeDungeon(ConvertedTileMap, RoomInfos, TileSize);

        // 통계 출력
        if (bShowStatistics)
        {
            GraphAnalyzer->PrintStatistics();

            // 디버그 시각화
            GraphAnalyzer->DrawDebugVisualization(GetWorld(), -1.0f);
        }

        // 분석 결과를 MapStats에 반영 (선택사항)
        FDungeonGraphAnalysis GraphAnalysis = GraphAnalyzer->GetAnalysis();
        MapStats.RoomCount = GraphAnalysis.RoomCount;
        MapStats.DeadEndCount = GraphAnalysis.DeadEndCount;
        MapStats.JunctionCount = GraphAnalysis.JunctionCount;
        MapStats.CrossRoadCount = GraphAnalysis.CrossRoadCount;

        // 간선 수 로그
        UE_LOG(LogTemp, Warning, TEXT("Graph Analysis: Found %d nodes and %d edges"),
            GraphAnalysis.NodeCount, GraphAnalysis.EdgeCount);
    }

    //// 맵 통계 분석 및 출력
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

    // 최대 깊이 도달 또는 최소 크기 이하
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

    // 분할 가능한지 확인
    bool bCanSplitHorizontally = Size.X > MinNodeSize * 2;
    bool bCanSplitVertically = Size.Y > MinNodeSize * 2;

    if (!bCanSplitHorizontally && !bCanSplitVertically)
    {
        return false;
    }

    // 분할 방향 결정
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

    // 분할 위치 계산
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

        // 방 크기 계산
        int32 RoomWidth = RandomStream.RandRange(MinRoomSize, FMath::Min(NodeSize.X - 2, MaxRoomSize));
        int32 RoomHeight = RandomStream.RandRange(MinRoomSize, FMath::Min(NodeSize.Y - 2, MaxRoomSize));

        // 방 위치 계산 (노드 내에서 랜덤 위치)
        int32 RoomX = LeafNode->Min.X + RandomStream.RandRange(1, NodeSize.X - RoomWidth - 1);
        int32 RoomY = LeafNode->Min.Y + RandomStream.RandRange(1, NodeSize.Y - RoomHeight - 1);

        LeafNode->RoomMin = FIntVector(RoomX, RoomY, 0);
        LeafNode->RoomMax = FIntVector(RoomX + RoomWidth, RoomY + RoomHeight, 1);
        LeafNode->bHasRoom = true;

        // 타일 맵에 방 표시
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
//    // 인접한 리프 노드들 연결
//    for (int32 i = 0; i < LeafNodes.Num() - 1; ++i)
//    {
//        // 가장 가까운 방 찾기
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

    // 좌우 이웃(또는 좌우로 더 가까움) + Y 구간 겹침 → 수평 직선
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
            return; // 직선 성공
        }
    }

    // 상하 이웃(또는 상하로 더 가까움) + X 구간 겹침 → 수직 직선
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
            return; // 직선 성공
        }
    }

    // 직선 불가 → L자(개선된 피벗 버전)
    FIntVector CenterA = GetRoomCenter(NodeA);
    FIntVector CenterB = GetRoomCenter(NodeB);
    CreateLShapedCorridor(CenterA, CenterB);
}

void ABSPMapGenerator02::ConnectRooms()
{
    if (!RootNode) return;

    // 리프(방 가진 노드) 모으기
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

    // 두 서브트리 사이에서 가장 가까운 방 쌍을 골라 연결
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

    // BSP 트리를 재귀로 내려가며: LeftChild ↔ RightChild만 연결
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
//    // 수평 먼저, 수직 나중에
//    if (RandomStream.FRand() > 0.5f)
//    {
//        // 수평 복도
//        int32 StartX = FMath::Min(Start.X, End.X);
//        int32 EndX = FMath::Max(Start.X, End.X);
//
//        for (int32 x = StartX; x <= EndX; ++x)
//        {
//
//            // 내 위나 아래에 이미 복도 타일이 있는지 확인 (평행 감지)
//            if ((Start.Y > 0 && IsCorridorTile(x, Start.Y - 1)) ||
//                (Start.Y < MapSize.Y - 1 && IsCorridorTile(x, Start.Y + 1)))
//            {
//                // 평행이 감지되면, 이 L자 복도 생성 자체를 중단하고 함수를 빠져나감
//                // 다른 연결 시도에서 더 나은 경로가 그려지길 기대
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
//        // 수직 복도
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
//        // 수직 먼저, 수평 나중에
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
//            // 수평으로 진행하다가 평행 감지 시 "그 지점에서" pivot
//            int32 pivotX = endX;
//            bool  bPivot = false;
//
//            for (int32 x = startX; x != endX + step; x += step)
//            {
//                const bool up = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
//                const bool down = (y > 0) && IsCorridorTile(x, y - 1);
//                if (up || down)
//                {
//                    pivotX = x;         // 여기서 꺾는다
//                    bPivot = true;
//                    break;
//                }
//                Carve(x, y);
//            }
//
//            if (!bPivot)
//            {
//                // 평행 없이 끝까지 옴: 기존 방식대로 마지막 X에서 수직
//                CarveVertical(endX, y, End.Y);
//                return;
//            }
//
//            // 1) 수평을 pivotX에서 멈추고, pivotX 열에서 Start.Y -> End.Y로 수직으로 먼저 연결
//            CarveVertical(pivotX, y, End.Y);
//
//            // 2) End.Y 레벨에서 pivotX -> End.X로 수평으로 마무리
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
//                    pivotY = y;         // 여기서 꺾는다
//                    bPivot = true;
//                    break;
//                }
//                Carve(x, y);
//            }
//
//            if (!bPivot)
//            {
//                // 평행 없이 끝까지 옴: 기존 방식대로 마지막 Y에서 수평
//                CarveHorizontal(endY, x, End.X);
//                return;
//            }
//
//            // 1) 수직을 pivotY에서 멈추고, pivotY 행에서 Start.X -> End.X로 수평으로 먼저 연결
//            CarveHorizontal(pivotY, x, End.X);
//
//            // 2) End.X 열에서 pivotY -> End.Y로 수직으로 마무리
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
            CarveVertical(pivotX, y, End.Y);          // 피벗 지점에서 세로로
            CarveHorizontal(End.Y, pivotX, End.X);    // 목표 Y에서 가로로 마무리
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
            CarveHorizontal(pivotY, x, End.X);       // 피벗 지점에서 가로로
            CarveVertical(End.X, pivotY, End.Y);     // 목표 X에서 세로로 마무리
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

    // 먼저 방들을 처리
    TSet<FIntVector> ProcessedRoomTiles;

    for (auto& LeafNode : LeafNodes)
    {
        if (!LeafNode->bHasRoom) continue;

        // 방의 실제 크기 계산
        FIntVector RoomSize = LeafNode->RoomMax - LeafNode->RoomMin;
        FIntVector RoomCenter = GetRoomCenter(LeafNode);

        // 이미 처리된 영역인지 확인
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

        // 방 영역 전체를 처리됨으로 표시
        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
        {
            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
            {
                ProcessedRoomTiles.Add(FIntVector(x, y, 0));
            }
        }

        // 방 액터 스폰 - 이제 하나의 RoomNorthClass만 사용 (회전 없이)
        FVector SpawnLocation = FVector(RoomCenter.X * TileSize, RoomCenter.Y * TileSize, 0);
        FRotator SpawnRotation = FRotator::ZeroRotator; // 회전 없음

        // RoomNorthClass를 기본 방으로 사용
        if (RoomNorthClass)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            AActor* SpawnedRoom = World->SpawnActor<AActor>(RoomNorthClass, SpawnLocation, SpawnRotation, SpawnParams);
            if (SpawnedRoom)
            {
                SpawnedRoom->Tags.Add("BSPGenerated");

                // 방 타일에 3배 스케일 적용
                FVector RoomScale = FVector(3.0f, 3.0f, 3.0f);
                SpawnedRoom->SetActorScale3D(RoomScale);

                // 벽 생성 (문이 없는 방향에)
                SpawnWallsForRoom(LeafNode, SpawnedRoom);

                // 문 생성 (복도가 연결된 방향에)
                SpawnDoorsForRoom(LeafNode, SpawnedRoom);

                SpawnedCount++;

                // 연결된 방향들 확인하여 로그 출력
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

    // 복도 타일 스폰 (각 타일마다 개별 액터로)
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            // 방 타일은 이미 처리했으므로 건너뛰기
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

            // 복도 타일 방향 확인
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

            // 코너나 교차점
            if (ConnectionCount > 2 ||
                (bHasNorth && bHasEast) || (bHasNorth && bHasWest) ||
                (bHasSouth && bHasEast) || (bHasSouth && bHasWest))
            {
                TileClass = CorridorCornerClass;
            }
            else if (bHasNorth || bHasSouth)
            {
                // Y축 방향 연결 = 세로 복도
                TileClass = CorridorHorizontalClass;
            }
            else if (bHasEast || bHasWest)
            {
                // X축 방향 연결 = 가로 복도
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
    // 방 주변의 복도 확인 (3x3 방 기준)
    TArray<FString> PossibleDirections;

    // 북쪽 확인 (Y+)
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

    // 남쪽 확인 (Y-)
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

    // 동쪽 확인 (X+)
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

    // 서쪽 확인 (X-)
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

    // 가능한 방향 중 하나 선택
    if (PossibleDirections.Num() > 0)
    {
        return PossibleDirections[RandomStream.RandRange(0, PossibleDirections.Num() - 1)];
    }

    // 기본값
    return "North";
}

FString ABSPMapGenerator02::GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode02> Node)
{
    if (!Node || !Node->bHasRoom) return "North";

    TArray<FString> PossibleDirections;

    // 북쪽 확인 (Y+ 방향)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMax.Y;
        if (checkY < MapSize.Y && TileMap[x][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("North");
            break;
        }
    }

    // 남쪽 확인 (Y- 방향)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMin.Y - 1;
        if (checkY >= 0 && TileMap[x][checkY] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("South");
            break;
        }
    }

    // 동쪽 확인 (X+ 방향)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMax.X;
        if (checkX < MapSize.X && TileMap[checkX][y] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("East");
            break;
        }
    }

    // 서쪽 확인 (X- 방향)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMin.X - 1;
        if (checkX >= 0 && TileMap[checkX][y] == ETileType02::Corridor)
        {
            PossibleDirections.AddUnique("West");
            break;
        }
    }

    // 가능한 방향 중 하나 선택
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

    // 복도가 연결된 방향 확인
    TArray<FString> Directions = GetCorridorDirections(Node);

    for (const FString& Direction : Directions)
    {
        FVector DoorOffset = FVector::ZeroVector;
        FRotator DoorRotation = FRotator::ZeroRotator;

        // 방의 실제 크기 (스케일 적용된 크기)
        float HalfSize = TileSize * RoomScale.X / 2.0f;

        // 문 위치와 회전 설정
        FIntVector DoorTilePos;

        if (Direction == "North")
        {
            // 북쪽 문 (Y+ 방향)
            DoorOffset = FVector(0, HalfSize, 0);
            DoorRotation = FRotator(0, 0, 0);
            DoorTilePos = FIntVector(GetRoomCenter(Node).X, Node->RoomMax.Y-1, 0);

            // 복도 연결 확인 및 생성
            EnsureCorridorConnection(DoorTilePos, FIntVector(0, 1, 0), Node);
        }
        else if (Direction == "South")
        {
            // 남쪽 문 (Y- 방향)
            DoorOffset = FVector(0, -HalfSize, 0);
            DoorRotation = FRotator(0, 180, 0);
            DoorTilePos = FIntVector(GetRoomCenter(Node).X, Node->RoomMin.Y, 0);

            // 복도 연결 확인 및 생성
            EnsureCorridorConnection(DoorTilePos, FIntVector(0, -1, 0), Node);
        }
        else if (Direction == "East")
        {
            // 동쪽 문 (X+ 방향)
            DoorOffset = FVector(HalfSize, 0, 0);
            DoorRotation = FRotator(0, 90, 0);
            DoorTilePos = FIntVector(Node->RoomMax.X-1, GetRoomCenter(Node).Y, 0);

            // 복도 연결 확인 및 생성
            EnsureCorridorConnection(DoorTilePos, FIntVector(1, 0, 0), Node);
        }
        else if (Direction == "West")
        {
            // 서쪽 문 (X- 방향)
            DoorOffset = FVector(-HalfSize, 0, 0);
            DoorRotation = FRotator(0, -90, 0);
            DoorTilePos = FIntVector(Node->RoomMin.X, GetRoomCenter(Node).Y, 0);

            // 복도 연결 확인 및 생성
            EnsureCorridorConnection(DoorTilePos, FIntVector(-1, 0, 0), Node);
        }

        // 문 스폰
        FVector DoorLocation = RoomLocation + DoorOffset;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedDoor = World->SpawnActor<AActor>(DoorClass, DoorLocation, DoorRotation, SpawnParams);
        if (SpawnedDoor)
        {
            SpawnedDoor->Tags.Add("BSPGenerated");
            SpawnedDoor->Tags.Add("Door");

            // 문도 방과 같은 스케일 적용 (필요시 조정)
            SpawnedDoor->SetActorScale3D(RoomScale);

            // 문을 방의 자식으로 설정 (선택사항)
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

    // 북쪽 확인 (Y+ 방향)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMax.Y;
        if (checkY < MapSize.Y && TileMap[x][checkY] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("North");
            break;
        }
    }

    // 남쪽 확인 (Y- 방향)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMin.Y - 1;
        if (checkY >= 0 && TileMap[x][checkY] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("South");
            break;
        }
    }

    // 동쪽 확인 (X+ 방향)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMax.X;
        if (checkX < MapSize.X && TileMap[checkX][y] == ETileType02::Corridor)
        {
            ConnectedDirections.AddUnique("East");
            break;
        }
    }

    // 서쪽 확인 (X- 방향)
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

    // 복도가 연결된 방향 확인
    TArray<FString> ConnectedDirections = GetCorridorDirections(Node);

    // 4방향 모두 확인하여 연결되지 않은 방향에 벽 생성
    TArray<FString> AllDirections = { "North", "South", "East", "West" };

    for (const FString& Direction : AllDirections)
    {
        // 이미 복도가 연결된 방향은 건너뛰기 (문이 생성될 예정)
        if (ConnectedDirections.Contains(Direction))
        {
            continue;
        }

        // 벽 위치와 회전 계산
        FVector WallOffset = FVector::ZeroVector;
        FRotator WallRotation = FRotator::ZeroRotator;

        // 방의 실제 크기 (스케일 적용된 크기)
        float HalfSize = TileSize * RoomScale.X / 2.0f;

        if (Direction == "North")
        {
            // 북쪽 벽 (Y+ 방향)
            WallOffset = FVector(0, HalfSize, 0);
            WallRotation = FRotator(0, 0, 0);
        }
        else if (Direction == "South")
        {
            // 남쪽 벽 (Y- 방향)
            WallOffset = FVector(0, -HalfSize, 0);
            WallRotation = FRotator(0, 180, 0);
        }
        else if (Direction == "East")
        {
            // 동쪽 벽 (X+ 방향)
            WallOffset = FVector(HalfSize, 0, 0);
            WallRotation = FRotator(0, 90, 0);
        }
        else if (Direction == "West")
        {
            // 서쪽 벽 (X- 방향)
            WallOffset = FVector(-HalfSize, 0, 0);
            WallRotation = FRotator(0, -90, 0);
        }

        // 벽 스폰
        FVector WallLocation = RoomLocation + WallOffset;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AActor* SpawnedWall = World->SpawnActor<AActor>(WallClass, WallLocation, WallRotation, SpawnParams);
        if (SpawnedWall)
        {
            SpawnedWall->Tags.Add("BSPGenerated");
            SpawnedWall->Tags.Add("Wall");

            // 벽도 방과 같은 스케일 적용 (필요시 조정)
            SpawnedWall->SetActorScale3D(RoomScale);

            // 벽을 방의 자식으로 설정 (선택사항)
            SpawnedWall->AttachToActor(RoomActor, FAttachmentTransformRules::KeepWorldTransform);

            UE_LOG(LogTemp, Verbose, TEXT("Spawned wall at %s direction for room at (%f, %f)"),
                *Direction, RoomLocation.X / TileSize, RoomLocation.Y / TileSize);
        }
    }
}

void ABSPMapGenerator02::EnsureCorridorConnection(const FIntVector& DoorPos, const FIntVector& Direction, TSharedPtr<FBSPNode02> Node)
{
    // 문 위치에서 Direction 방향으로 1칸 위치
    FIntVector CorridorPos = DoorPos + Direction;

    // 맵 범위 확인
    if (CorridorPos.X < 0 || CorridorPos.X >= MapSize.X ||
        CorridorPos.Y < 0 || CorridorPos.Y >= MapSize.Y)
    {
        return;
    }

    // 해당 위치가 비어있으면 복도 생성
    if (TileMap[CorridorPos.X][CorridorPos.Y] == ETileType02::Empty)
    {
        // 타일맵에 복도 표시
        TileMap[CorridorPos.X][CorridorPos.Y] = ETileType02::BridgeCorridor;

        // 복도 타일 즉시 스폰
        SpawnSingleCorridorTile(CorridorPos);

        UE_LOG(LogTemp, Warning, TEXT("Created bridge BridgeCorridor at (%d, %d) for door connection"),
            CorridorPos.X, CorridorPos.Y);
    }
    // 이미 복도나 방이 있으면 아무것도 안 함 (정상적으로 연결됨)
}

// 단일 복도 타일 스폰 함수
void ABSPMapGenerator02::SpawnSingleCorridorTile(const FIntVector& TilePos)
{
    UWorld* World = GetWorld();
    if (!World || !CorridorCornerClass) return;

    FVector SpawnLocation = FVector(TilePos.X * TileSize, TilePos.Y * TileSize, 0);
    FRotator SpawnRotation = FRotator::ZeroRotator;

    // 연결 복도는 항상 CorridorCornerClass(goalt01) 사용
    // 모든 방향과 연결 가능하므로 가장 안전함

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* SpawnedTile = World->SpawnActor<AActor>(CorridorCornerClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedTile)
    {
        SpawnedTile->Tags.Add("BSPGenerated");
        SpawnedTile->Tags.Add("BridgeCorridor"); // 연결 복도임을 표시

        UE_LOG(LogTemp, Verbose, TEXT("Spawned bridge corridor (corner tile) at (%d, %d)"),
            TilePos.X, TilePos.Y);
    }
}

//void ABSPMapGenerator02::CreateExtraConnections()
//{
//    if (LeafNodes.Num() < 3) return;  // 방이 3개 미만이면 추가 연결 불필요
//
//    int32 ConnectionsAdded = 0;
//    int32 TargetConnections = RandomStream.RandRange(MinExtraConnections, MaxExtraConnections);
//
//    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);
//
//    // 추가 간선을 저장할 임시 배열
//    TArray<TPair<int32, int32>> ExtraEdges;
//
//    // 모든 방 쌍에 대해 검토
//    TArray<TPair<int32, int32>> PotentialConnections;
//
//    for (int32 i = 0; i < LeafNodes.Num(); ++i)
//    {
//        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
//        {
//            // 거리가 적절한 방들만 후보로 추가
//            if (IsValidConnectionDistance(i, j))
//            {
//                PotentialConnections.Add(TPair<int32, int32>(i, j));
//            }
//        }
//    }
//
//    // 후보들을 섞어서 랜덤하게 선택
//    for (int32 i = PotentialConnections.Num() - 1; i > 0; --i)
//    {
//        int32 j = RandomStream.RandRange(0, i);
//        PotentialConnections.Swap(i, j);
//    }
//
//    // 추가 연결 생성
//    for (const auto& Connection : PotentialConnections)
//    {
//        if (ConnectionsAdded >= TargetConnections)
//        {
//            break;
//        }
//
//        // 확률적으로 연결 생성
//        if (RandomStream.FRand() < ExtraConnectionChance)
//        {
//            TSharedPtr<FBSPNode02> NodeA = LeafNodes[Connection.Key];
//            TSharedPtr<FBSPNode02> NodeB = LeafNodes[Connection.Value];
//
//            FIntVector CenterA = GetRoomCenter(NodeA);
//            FIntVector CenterB = GetRoomCenter(NodeB);
//
//            // 이미 복도가 존재하는지 확인
//            if (!CorridorExists(CenterA, CenterB))
//            {
//                // 다양한 복도 스타일 중 랜덤 선택
//                float CorridorStyle = RandomStream.FRand();
//
//                if (CorridorStyle < 0.5f)
//                {
//                    // L자 복도 (기존 스타일)
//                    CreateLShapedCorridor(CenterA, CenterB);
//                }
//                else if (CorridorStyle < 0.8f)
//                {
//                    // 역 L자 복도 (반대 방향)
//                    CreateLShapedCorridor(CenterB, CenterA);
//                }
//                else
//                {
//                    // 지그재그 복도 (더 복잡한 경로)
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

// 추가: 후보 경로가 기존 복도에 "바짝 평행"하게 붙을 위험이 큰지 평가
bool ABSPMapGenerator02::WouldCreateLongParallel(const FIntVector& Start, const FIntVector& End, float Tolerance /*=0.6f*/)
{
    int32 CheckPoints = 7; // 샘플 지점 수 (홀수 권장)
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

        // 상하좌우 중 어느 한 칸이라도 복도면 "평행 인접"으로 센다
        bool bAdj =
            (InBounds(x + 1, y) && TileMap[x + 1][y] == ETileType02::Corridor) ||
            (InBounds(x - 1, y) && TileMap[x - 1][y] == ETileType02::Corridor) ||
            (InBounds(x, y + 1) && TileMap[x][y + 1] == ETileType02::Corridor) ||
            (InBounds(x, y - 1) && TileMap[x][y - 1] == ETileType02::Corridor);

        AdjacentCount += (bAdj ? 1 : 0);
    }

    // 샘플의 Tolerance(60% 등) 이상이 인접해 있으면 "평행 위험 높음"으로 간주
    return AdjacentCount > (int32)(Tolerance * (float)CheckPoints);
}

void ABSPMapGenerator02::CreateExtraConnections()
{
    if (!bCreateLoops) return;

    int32 TargetConnections = RandomStream.RandRange(MinExtraConnections, MaxExtraConnections);
    int32 ConnectionsAdded = 0;

    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);

    // 모든 방 쌍 중 거리 요건을 만족하는 후보 수집
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

    // 셔플
    for (int32 i = PotentialConnections.Num() - 1; i > 0; --i)
    {
        int32 j = RandomStream.RandRange(0, i);
        PotentialConnections.Swap(i, j);
    }

    // 추가 간선 결과 저장
    TArray<TPair<int32, int32>> ExtraEdges;

    for (const auto& Conn : PotentialConnections)
    {
        if (ConnectionsAdded >= TargetConnections) break;

        // 생성 확률
        if (RandomStream.FRand() >= ExtraConnectionChance) continue;

        TSharedPtr<FBSPNode02> NodeA = LeafNodes[Conn.Key];
        TSharedPtr<FBSPNode02> NodeB = LeafNodes[Conn.Value];

        FIntVector CenterA = GetRoomCenter(NodeA);
        FIntVector CenterB = GetRoomCenter(NodeB);

        // 1) 기존 경로와 "거의 동일한" 직선이 이미 깔려 있으면 스킵 (중복 방지)
        if (CorridorExists(CenterA, CenterB)) continue; // 기존 구현 활용. 

        // 2) 후보가 기존 복도에 바짝 붙어 (=, ||) 달릴 가능성이 크면 스킵
        if (WouldCreateLongParallel(CenterA, CenterB, /*Tolerance=*/0.6f)) continue;

        // 3) 복도 생성: "직선 우선 → 불가 시 L-피벗" 로직 재사용
        CreateCorridor(NodeA, NodeB); // 기존 L/역-L/지그재그 랜덤 호출을 대체. 

        ExtraEdges.Add(TPair<int32, int32>(Conn.Key, Conn.Value));
        ++ConnectionsAdded;

        UE_LOG(LogTemp, Verbose, TEXT("Added extra connection %d between room %d and %d"),
            ConnectionsAdded, Conn.Key, Conn.Value);
    }

    ExtraConnectionPairs = ExtraEdges;

    UE_LOG(LogTemp, Warning, TEXT("Created %d extra connections for maze complexity"), ConnectionsAdded);

    // 후처리로 평행 라인 잔여 정리(연결성 유지한 채 ||, =만 걷어냄)
    CleanupParallelCorridors(); 



}


// 두 방 사이의 거리가 적절한지 확인
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

    // 너무 가깝거나 너무 먼 방은 제외
    return Distance > 5.0f && Distance < MaxConnectionDistance;
}

// 복도가 이미 존재하는지 확인
bool ABSPMapGenerator02::CorridorExists(const FIntVector& Start, const FIntVector& End)
{
    // 두 방 사이의 직선 경로상에 이미 복도가 많이 있는지 체크
    int32 CorridorCount = 0;
    int32 CheckPoints = 5;  // 5개 지점만 샘플링

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

    // 이미 절반 이상이 복도면 연결되어 있다고 판단
    return CorridorCount > CheckPoints / 2;
}

// 지그재그 복도 생성
void ABSPMapGenerator02::CreateZigzagCorridor(const FIntVector& Start, const FIntVector& End)
{
    // 중간 지점 계산
    int32 MidX = (Start.X + End.X) / 2;
    int32 MidY = (Start.Y + End.Y) / 2;

    // 랜덤 오프셋 추가
    int32 OffsetRange = 3;
    MidX += RandomStream.RandRange(-OffsetRange, OffsetRange);
    MidY += RandomStream.RandRange(-OffsetRange, OffsetRange);

    // 맵 범위 제한
    MidX = FMath::Clamp(MidX, 0, MapSize.X - 1);
    MidY = FMath::Clamp(MidY, 0, MapSize.Y - 1);

    FIntVector MidPoint(MidX, MidY, 0);

    // Start -> Mid -> End 경로 생성
    CreateLShapedCorridor(Start, MidPoint);
    CreateLShapedCorridor(MidPoint, End);
}

// 맵 분석 및 통계 생성
FMapStatistics ABSPMapGenerator02::AnalyzeMap()
{
    MapStats = FMapStatistics(); // 초기화

    // 먼저 복도 타입을 분석하여 갈림길과 막다른 길 구분
    AnalyzeCorridorTypes();

    // 방 개수 카운트
    MapStats.RoomCount = LeafNodes.Num();

    // 타일별 카운트
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

    // 문 개수 카운트 (실제 스폰된 액터 기반)
    TArray<AActor*> FoundDoors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Door", FoundDoors);
    MapStats.DoorCount = FoundDoors.Num();

    // 평균 방 크기 계산
    if (MapStats.RoomCount > 0)
    {
        MapStats.AverageRoomSize = (float)TotalRoomTiles / (float)MapStats.RoomCount;
    }

    // 전체 복도 길이 (모든 복도 타일의 합)
    MapStats.TotalCorridorLength = MapStats.CorridorCount + MapStats.DeadEndCount +
        MapStats.JunctionCount + MapStats.CrossRoadCount;

    return MapStats;
}

// 복도 타입 분석 함수
void ABSPMapGenerator02::AnalyzeCorridorTypes()
{
    // 모든 복도 타일을 순회하며 연결 개수에 따라 타입 결정
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            // 복도 타일인 경우만 처리
            if (TileMap[x][y] == ETileType02::Corridor ||
                TileMap[x][y] == ETileType02::BridgeCorridor)
            {
                ETileType02 NewType = DetermineCorridorType(x, y);
                TileMap[x][y] = NewType;
            }
        }
    }
}

// 특정 위치의 복도 타입 결정
//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    int32 Connections = CountConnections(x, y);
//
//    switch (Connections)
//    {
//    case 0:
//        // 고립된 타일 (일반적으로 발생하지 않음)
//        return ETileType02::Corridor;
//
//    case 1:
//        // 막다른 길
//        return ETileType02::DeadEnd;
//
//    case 2:
//        // 일반 복도
//        return ETileType02::Corridor;
//
//    case 3:
//        // T자 갈림길
//        return ETileType02::Junction;
//
//    case 4:
//        // 십자 교차로
//        return ETileType02::CrossRoad;
//
//    default:
//        // 예외 상황
//        return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // 복도끼리의 연결 개수
//    int32 CorridorConnections = CountConnections(x, y);
//
//    // 전체 연결 개수 (방 포함) 계산
//    int32 TotalConnections = CorridorConnections;
//
//    // 방과의 연결 확인
//    bool bConnectedToRoom = false;
//
//    // 북쪽 확인
//    if (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // 남쪽 확인
//    if (y > 0 && TileMap[x][y - 1] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // 동쪽 확인
//    if (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // 서쪽 확인
//    if (x > 0 && TileMap[x - 1][y] == ETileType02::Room)
//    {
//        TotalConnections++;
//        bConnectedToRoom = true;
//    }
//
//    // 복도끼리의 연결만으로 타입 결정
//    switch (CorridorConnections)
//    {
//    case 0:
//        // 복도 연결이 없는 경우
//        if (bConnectedToRoom)
//        {
//            // 방과만 연결된 경우 - 막다른 길
//            return ETileType02::DeadEnd;
//        }
//        else
//        {
//            // 완전히 고립 (일반적으로 발생하지 않음)
//            return ETileType02::Corridor;
//        }
//
//    case 1:
//        // 복도 1개와 연결
//        if (bConnectedToRoom)
//        {
//            // 방과도 연결되어 있으면 일반 복도 (방-복도-복도 구조)
//            return ETileType02::Corridor;
//        }
//        else
//        {
//            // 순수하게 복도 1개만 연결 - 막다른 길
//            return ETileType02::DeadEnd;
//        }
//
//    case 2:
//        // 복도 2개와 연결 - 일반 복도
//        return ETileType02::Corridor;
//
//    case 3:
//        // 복도 3개와 연결 - T자 갈림길
//        return ETileType02::Junction;
//
//    case 4:
//        // 복도 4개와 연결 - 십자로
//        return ETileType02::CrossRoad;
//
//    default:
//        return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // 이웃(복도 계열만) ? IsCorridorTile은 DeadEnd/Junction/CrossRoad도 복도로 봄
//    // (현 구조 유지)  :contentReference[oaicite:3]{index=3}
//    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
//    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
//    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
//    const bool W = (x > 0) && IsCorridorTile(x - 1, y);
//
//    // 보조: 해당 좌표가 가로/세로 "밴드(평행 레인)"의 일부인지
//    auto IsHorizontalBand = [this](int32 cx, int32 cy)->bool {
//        return IsCorridorTile(cx - 1, cy) && IsCorridorTile(cx + 1, cy);
//        };
//    auto IsVerticalBand = [this](int32 cx, int32 cy)->bool {
//        return IsCorridorTile(cx, cy - 1) && IsCorridorTile(cx, cy + 1);
//        };
//
//    // ── 평행 레인(폭 확장) 예외 처리 ─────────────────────────────
//    // 가로 진행(E&W) + 위/아래 한쪽만 열린 경우: 위/아래 이웃이 가로 밴드면 분기 아님
//    if (E && W && (N ^ S)) {
//        const int ny = N ? (y + 1) : (y - 1);
//        if (IsHorizontalBand(x, ny)) {
//            return ETileType02::Corridor; // 폭 확장된 직선으로 취급
//        }
//    }
//    // 세로 진행(N&S) + 좌/우 한쪽만 열린 경우: 좌/우 이웃이 세로 밴드면 분기 아님
//    if (N && S && (E ^ W)) {
//        const int nx = E ? (x + 1) : (x - 1);
//        if (IsVerticalBand(nx, y)) {
//            return ETileType02::Corridor;
//        }
//    }
//    // ────────────────────────────────────────────────────────────
//
//    // 평행 레인 예외에 안 걸리면 기존 규칙으로 판단
//    const int connections = (N ? 1 : 0) + (S ? 1 : 0) + (E ? 1 : 0) + (W ? 1 : 0);
//
//    switch (connections)
//    {
//    case 1:  return ETileType02::DeadEnd;    // 막다른 길
//    case 2:  return ETileType02::Corridor;   // 직선/코너(스폰에서 회전)
//    case 3:  return ETileType02::Junction;   // T 갈림길
//    case 4:  return ETileType02::CrossRoad;  // 십자
//    default: return ETileType02::Corridor;
//    }
//}

//ETileType02 ABSPMapGenerator02::DetermineCorridorType(int32 x, int32 y)
//{
//    // 1) 현재 복도 이웃들
//    bool N = (y < MapSize.Y - 1 && IsCorridorTile(x, y + 1));
//    bool S = (y > 0 && IsCorridorTile(x, y - 1));
//    bool E = (x < MapSize.X - 1 && IsCorridorTile(x + 1, y));
//    bool W = (x > 0 && IsCorridorTile(x - 1, y));
//    int32 CorridorConnections = (int)N + (int)S + (int)E + (int)W;
//
//    // 2) 방 인접 여부
//    bool RoomAdj =
//        (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room) ||
//        (y > 0 && TileMap[x][y - 1] == ETileType02::Room) ||
//        (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room) ||
//        (x > 0 && TileMap[x - 1][y] == ETileType02::Room);
//
//    // 3) BridgeCorridor는 항상 일반 복도 취급 (문 앞 한 칸 보호)
//    if (TileMap[x][y] == ETileType02::BridgeCorridor)
//        return ETileType02::Corridor;
//
//    // 4) 방에 붙은 복도는 DeadEnd로 보지 않기
//    if (RoomAdj && CorridorConnections <= 1)
//        return ETileType02::Corridor;
//
//    // 5) 2칸 폭 복도(평행 복도) 예외: 한쪽 옆만 붙었더라도 옆 칸이 직선복도면 일반 복도
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
//    // 6) 기본 규칙
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
    // 복도 이웃(복도 계열만 카운트)
    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
    const bool W = (x > 0) && IsCorridorTile(x - 1, y);
    const int connections = (N ? 1 : 0) + (S ? 1 : 0) + (E ? 1 : 0) + (W ? 1 : 0);

    // --- 문/방 인접 보호 ----------------------------
    // 1) 문 앞 한 칸(BridgeCorridor)은 항상 일반 복도
    if (TileMap[x][y] == ETileType02::BridgeCorridor)
        return ETileType02::Corridor;

    // 2) 방 인접 여부
    const bool RoomAdj =
        (y < MapSize.Y - 1 && TileMap[x][y + 1] == ETileType02::Room) ||
        (y > 0 && TileMap[x][y - 1] == ETileType02::Room) ||
        (x < MapSize.X - 1 && TileMap[x + 1][y] == ETileType02::Room) ||
        (x > 0 && TileMap[x - 1][y] == ETileType02::Room);

    // 방에 붙어 있고 복도 연결이 0~1개면 DeadEnd로 보지 않음
    if (RoomAdj && connections <= 1)
        return ETileType02::Corridor;
    // -----------------------------------------------

    // --- 두 줄 복도(평행 레인) 예외 -----------------
    auto IsHorizontalBand = [this](int32 cx, int32 cy)->bool {
        return IsCorridorTile(cx - 1, cy) && IsCorridorTile(cx + 1, cy);
        };
    auto IsVerticalBand = [this](int32 cx, int32 cy)->bool {
        return IsCorridorTile(cx, cy - 1) && IsCorridorTile(cx, cy + 1);
        };

    // 가로 진행(E&W) + 위/아래 한쪽만 열린 경우: 해당 이웃이 가로 밴드면 분기 아님
    if (E && W && (N ^ S)) {
        const int ny = N ? (y + 1) : (y - 1);
        if (IsHorizontalBand(x, ny)) return ETileType02::Corridor;
    }
    // 세로 진행(N&S) + 좌/우 한쪽만 열린 경우: 해당 이웃이 세로 밴드면 분기 아님
    if (N && S && (E ^ W)) {
        const int nx = E ? (x + 1) : (x - 1);
        if (IsVerticalBand(nx, y)) return ETileType02::Corridor;
    }
    // -----------------------------------------------

    // 기본 규칙
    switch (connections)
    {
    case 1:  return ETileType02::DeadEnd;
    case 2:  return ETileType02::Corridor;
    case 3:  return ETileType02::Junction;
    case 4:  return ETileType02::CrossRoad;
    default: return ETileType02::Corridor;
    }
}

// 특정 위치의 연결 개수 카운트
int32 ABSPMapGenerator02::CountConnections(int32 x, int32 y)
{
    int32 ConnectionCount = 0;

    // 4방향 확인
    // 북쪽 (Y+)
    if (y < MapSize.Y - 1 && IsCorridorTile(x, y + 1))
    {
        ConnectionCount++;
    }

    // 남쪽 (Y-)
    if (y > 0 && IsCorridorTile(x, y - 1))
    {
        ConnectionCount++;
    }

    // 동쪽 (X+)
    if (x < MapSize.X - 1 && IsCorridorTile(x + 1, y))
    {
        ConnectionCount++;
    }

    // 서쪽 (X-)
    if (x > 0 && IsCorridorTile(x - 1, y))
    {
        ConnectionCount++;
    }

    return ConnectionCount;
}

// 타일이 복도 계열인지 확인
bool ABSPMapGenerator02::IsCorridorTile(int32 x, int32 y)
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y)
    {
        return false;
    }

    ETileType02 Type = TileMap[x][y];

    // 복도, 방, 문 모두 연결된 것으로 간주
    return Type == ETileType02::Corridor ||
        Type == ETileType02::BridgeCorridor ||
        Type == ETileType02::DeadEnd ||
        Type == ETileType02::Junction ||
        Type == ETileType02::CrossRoad;
}

// 통계 출력 함수
void ABSPMapGenerator02::PrintMapStatistics()
{
    // 먼저 맵 분석
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

    // 복잡도 지수 계산
    float ComplexityIndex = 0.0f;
    if (MapStats.TotalCorridorLength > 0)
    {
        ComplexityIndex = (MapStats.JunctionCount * 2.0f + MapStats.CrossRoadCount * 3.0f) /
            MapStats.TotalCorridorLength * 100.0f;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Map Complexity ==="));   // 테스트용
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

    

    // 그래프 분석 추가
    CalculateCyclomaticComplexity();

    // 디버그용 시각화
    if (bShowStatistics)
    {
        DrawDebugVisualization();
    }


}

// 디버그 시각화 함수
void ABSPMapGenerator02::DrawDebugVisualization()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 막다른 길을 빨간색으로 표시
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

    // 간선 시각화 - 실제 복도 경로를 따라 그리기
    for (const GraphEdge& Edge : GraphEdges)
    {
        // 경로가 있으면 경로를 따라 그리기
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

                // 복도 경로를 따라 보라색 선 그리기
                DrawDebugLine(World, StartLocation, EndLocation, FColor::Magenta, true, 10.0f, 0, 10.0f);
            }
        }
        //else if (Edge.StartNode < GraphNodes.Num() && Edge.EndNode < GraphNodes.Num())
        //{
        //    // 경로가 없으면 직선으로 (폴백)
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


// 타일이 노드인지 확인 (갈림길, 막다른 길, 교차로만 - 방은 별도 처리)
bool ABSPMapGenerator02::IsNodeTile(int32 x, int32 y)
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y)
        return false;

    ETileType02 Type = TileMap[x][y];
    return Type == ETileType02::DeadEnd ||
        Type == ETileType02::Junction ||
        Type == ETileType02::CrossRoad;
}

// 특정 방의 중심점 찾기
FIntVector ABSPMapGenerator02::FindRoomCenter(int32 RoomId)
{
    if (RoomId >= 0 && RoomId < LeafNodes.Num() && LeafNodes[RoomId]->bHasRoom)
    {
        return GetRoomCenter(LeafNodes[RoomId]);
    }
    return FIntVector::ZeroValue;
}

// 맵에서 그래프 구조 생성
void ABSPMapGenerator02::BuildGraphFromMap()
{
    GraphNodes.Empty();
    GraphEdges.Empty();
    NodePositionToIndex.Empty();

    // 1단계: 방을 노드로 추가 (각 방당 하나의 노드)
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

            // 방의 모든 타일 위치도 이 노드로 매핑
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

    // 2단계: 복도의 특별한 지점들을 노드로 추가 (갈림길, 막다른 길, 교차로)
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

    // 3단계: 노드 간 연결(간선) 찾기
    TSet<TPair<int32, int32>> ProcessedEdges; // 중복 방지

    for (int32 i = 0; i < GraphNodes.Num(); ++i)
    {
        FIntVector StartPos = GraphNodes[i].Position;

        // 시작 위치 결정 (방인 경우 가장자리 타일들에서 시작)
        TArray<FIntVector> StartPositions;

        if (GraphNodes[i].Type == ETileType02::Room)
        {
            // 방의 가장자리에서 복도와 연결된 지점 찾기
            int32 RoomId = GraphNodes[i].RoomId;
            if (RoomId >= 0 && RoomId < LeafNodes.Num())
            {
                auto& Node = LeafNodes[RoomId];

                // 방의 가장자리 타일 검사
                for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
                {
                    // 북쪽 가장자리
                    int32 y = Node->RoomMax.Y - 1;
                    if (y + 1 < MapSize.Y && IsCorridorTile(x, y + 1))
                    {
                        StartPositions.Add(FIntVector(x, y + 1, 0));
                    }
                    // 남쪽 가장자리
                    y = Node->RoomMin.Y;
                    if (y - 1 >= 0 && IsCorridorTile(x, y - 1))
                    {
                        StartPositions.Add(FIntVector(x, y - 1, 0));
                    }
                }

                for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
                {
                    // 동쪽 가장자리
                    int32 x = Node->RoomMax.X - 1;
                    if (x + 1 < MapSize.X && IsCorridorTile(x + 1, y))
                    {
                        StartPositions.Add(FIntVector(x + 1, y, 0));
                    }
                    // 서쪽 가장자리
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
            // 복도 노드는 자기 위치에서 시작
            StartPositions.Add(StartPos);
        }

        // 각 시작 위치에서 복도를 따라가며 다음 노드 찾기
        for (const FIntVector& StartCorridorPos : StartPositions)
        {
            // 복도를 따라가며 다음 노드까지 추적
            TArray<FIntVector> Path;
            Path.Add(StartCorridorPos);

            FIntVector CurrentPos = StartCorridorPos;
            FIntVector PrevPos;
            if (GraphNodes[i].Type == ETileType02::Room)
            {
                // 방에서 복도로 나가는 방향의 반대 방향 계산
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

            // 최대 100칸까지만 따라가기 (무한루프 방지)
            for (int32 Steps = 0; Steps < 100; ++Steps)
            {
                // 현재 위치에서 다음 위치 찾기
                TArray<FIntVector> Directions = {
                    FIntVector(0, 1, 0), FIntVector(0, -1, 0),
                    FIntVector(1, 0, 0), FIntVector(-1, 0, 0)
                };

                bool bFoundNext = false;
                for (const FIntVector& Dir : Directions)
                {
                    FIntVector NextPos = CurrentPos + Dir;

                    if (NextPos == PrevPos) continue; // 되돌아가지 않기

                    if (NextPos.X >= 0 && NextPos.X < MapSize.X &&
                        NextPos.Y >= 0 && NextPos.Y < MapSize.Y)
                    {
                        // 다른 노드에 도달했는지 확인
                        if (NodePositionToIndex.Contains(NextPos))
                        {
                            int32 EndNodeIndex = NodePositionToIndex[NextPos];

                            if (EndNodeIndex != i) // 자기 자신이 아닌 경우
                            {
                                // 중복 간선 체크
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
                        // 복도 타일이면 계속 진행
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

            // 복도/브릿지/문만 통과, 중간 노드(시작/끝 제외)는 차단
            auto IsPassableDirect = [&](const FIntVector& P) -> bool {
                if (!InBounds(P.X, P.Y)) return false;
                ETileType02 T = TileMap[P.X][P.Y];
                bool bPass = (T == ETileType02::Corridor ||
                    T == ETileType02::BridgeCorridor ||
                    T == ETileType02::Door);
                if (!bPass) return false;

                if (NodePositionToIndex.Contains(P)) {
                    int32 idx = NodePositionToIndex[P];
                    if (idx != StartNodeIndex && idx != EndNodeIndex) return false; // 중간 노드 차단
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

                // 목표 판정: 목표 좌표에 도달하거나, End 노드 타일에 닿으면 성공
                if (cur == Goal ||
                    (NodePositionToIndex.Contains(cur) && NodePositionToIndex[cur] == EndNodeIndex)) {
                    Reconstruct(cur);
                    return true;
                }

                for (auto d : Dirs) {
                    FIntVector nxt = cur + d;
                    if (Visited.Contains(nxt)) continue;

                    // End 노드 타일은 바로 허용
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

    // 3.5단계: 놓친 연결 찾기 (추가 복도로 인한 직접 연결)
    // 모든 노드 쌍을 확인하여 복도로 직접 연결되어 있는지 확인
    for (int32 i = 0; i < GraphNodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < GraphNodes.Num(); ++j)
        {
            // 이미 간선이 있는지 확인
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
                // BFS로 복도를 통한 직접 경로 찾기
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

    // 마지막에 추가 연결 간선들을 그래프에 추가
    for (const auto& ExtraPair : ExtraConnectionPairs)
    {
        // 방 인덱스를 노드 인덱스로 변환
        int32 NodeIndexA = -1;
        int32 NodeIndexB = -1;

        // 방 ID로 노드 인덱스 찾기
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
            // 중복 체크
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

                //// 경로 찾기 (옵션)
                //TArray<FIntVector> Path;
                //if (FindCorridorPath(GraphNodes[NodeIndexA].Position,
                //    GraphNodes[NodeIndexB].Position, Path))
                //{
                //    NewEdge.Path = Path;
                //    NewEdge.Length = Path.Num();
                //}
                //else
                //{
                //    // 경로를 못 찾으면 직선 거리 사용
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

// DFS로 연결된 컴포넌트 탐색
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

// 그래프의 연결된 컴포넌트 수 계산
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

// 순환 복잡도 계산 및 출력
void ABSPMapGenerator02::CalculateCyclomaticComplexity()
{
    // 먼저 그래프 구축
    BuildGraphFromMap();

    // 노드 타입별 카운트
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

    // 그래프 메트릭스
    int32 N = GraphNodes.Num();  // 노드 수
    int32 E = GraphEdges.Num();  // 간선 수
    int32 P = CountGraphComponents();  // 연결된 컴포넌트 수

    // 순환 복잡도 계산 (V(G) = E - N + P)
    int32 CyclomaticComplexity = E - N + P;

    // 평균 노드 차수 (Average Degree)
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

    // 평균 간선 길이 (복도 길이)
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

    // 결과 출력
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

    // 복잡도 해석
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

    // 연결성 분석
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

    // 추가 분석
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
            // 복도 계열 + 문은 통과 가능
            return T == ETileType02::Corridor
                || T == ETileType02::BridgeCorridor
                || T == ETileType02::DeadEnd
                || T == ETileType02::Junction
                || T == ETileType02::CrossRoad
                || T == ETileType02::Door;
        };

    // 시작점/도착점이 방 중심 등 복도 바깥일 수 있으니, 인접 복도 타일을 시드로 사용
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
        Prev.Add(S, FIntVector(INT32_MIN, INT32_MIN, INT32_MIN)); // 시작 표시
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

    // 경로 복원
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
    // 그래프 추적용: 복도 계열 + Door는 통과 허용 (Room은 불가)
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

    // 정확히 || 패턴이나 = 패턴만 찾아서 제거
    for (int32 x = 1; x < MapSize.X - 2; ++x)  // 여유 공간 확보
    {
        for (int32 y = 1; y < MapSize.Y - 2; ++y)
        {
            // 현재 위치가 복도인지 확인
            if (TileMap[x][y] != ETileType02::Corridor) continue;

            // 수직 평행 복도 체크 (|| 패턴)
            // 현재와 오른쪽이 모두 복도이고, 둘 다 위아래로만 연결된 경우
            if (TileMap[x + 1][y] == ETileType02::Corridor)
            {
                // 현재 타일: 위아래만 연결, 좌우는 연결 안됨
                bool currentVerticalOnly =
                    IsCorridorTile(x, y - 1) && IsCorridorTile(x, y + 1) &&  // 위아래 연결
                    !IsCorridorTile(x - 1, y) && !IsCorridorTile(x + 2, y);  // 양옆 끊김

                // 오른쪽 타일: 위아래만 연결, 좌우는 연결 안됨 (현재 제외)
                bool rightVerticalOnly =
                    IsCorridorTile(x + 1, y - 1) && IsCorridorTile(x + 1, y + 1) &&  // 위아래 연결
                    !IsCorridorTile(x + 2, y);  // 오른쪽 끊김 (왼쪽은 현재라 체크 안함)

                if (currentVerticalOnly && rightVerticalOnly)
                {
                    // 둘 중 하나 제거 (오른쪽 제거)
                    TileMap[x + 1][y] = ETileType02::Empty;
                    RemovedCount++;

                    UE_LOG(LogTemp, Verbose, TEXT("Removed vertical parallel corridor at (%d, %d)"), x + 1, y);
                }
            }

            // 수평 평행 복도 체크 (= 패턴)  
            // 현재와 위쪽이 모두 복도이고, 둘 다 좌우로만 연결된 경우
            if (TileMap[x][y + 1] == ETileType02::Corridor)
            {
                // 현재 타일: 좌우만 연결, 위아래는 연결 안됨
                bool currentHorizontalOnly =
                    IsCorridorTile(x - 1, y) && IsCorridorTile(x + 1, y) &&  // 좌우 연결
                    !IsCorridorTile(x, y - 1) && !IsCorridorTile(x, y + 2);  // 위아래 끊김

                // 위쪽 타일: 좌우만 연결, 위아래는 연결 안됨 (현재 제외)
                bool upHorizontalOnly =
                    IsCorridorTile(x - 1, y + 1) && IsCorridorTile(x + 1, y + 1) &&  // 좌우 연결
                    !IsCorridorTile(x, y + 2);  // 위쪽 끊김 (아래는 현재라 체크 안함)

                if (currentHorizontalOnly && upHorizontalOnly)
                {
                    // 둘 중 하나 제거 (위쪽 제거)
                    TileMap[x][y + 1] = ETileType02::Empty;
                    RemovedCount++;

                    UE_LOG(LogTemp, Verbose, TEXT("Removed horizontal parallel corridor at (%d, %d)"), x, y + 1);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Removed %d parallel corridor tiles"), RemovedCount);
}