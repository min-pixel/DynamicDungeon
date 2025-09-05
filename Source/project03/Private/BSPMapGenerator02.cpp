#include "BSPMapGenerator02.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h" 

#include "CoreMinimal.h"
DEFINE_LOG_CATEGORY_STATIC(LogBSP, Log, All);

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

//void ABSPMapGenerator02::GenerateBSPMap()
//{
//    // 기존 맵 정리
//    ClearMap();
//
//    // 랜덤 시드 설정
//    if (RandomSeed == 0)
//    {
//        RandomSeed = FMath::RandRange(1, INT32_MAX);
//    }
//    RandomStream.Initialize(RandomSeed);
//
//    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Started with Seed: %d"), RandomSeed);
//
//    // 타일 맵 초기화
//    InitializeTileMap();
//
//    // BSP 트리 생성
//    RootNode = CreateBSPTree(FIntVector(0, 0, 0), MapSize, 0);
//
//    // 리프 노드 수집
//    LeafNodes.Empty();
//    CollectLeafNodes(RootNode);
//
//    // 방 생성
//    CreateRooms();
//
//    // 방 연결
//    ConnectRooms();
//
//    CleanupParallelCorridors();
//
//    // 추가 연결 생성 (미로 복잡도 증가)
//    if (bCreateLoops)
//    {
//        CreateExtraConnections();
//    }
//
//    CleanupParallelCorridors();
//
//    CollapseThickCorridorBlobsFavorDoor();
//    PatchCorridorSingleTileGaps();
//    // 타일 스폰
//    SpawnTiles();
//
//    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed. Created %d rooms"), LeafNodes.Num());
//
//    // ===== 그래프 분석 시작 =====
//    if (GraphAnalyzer)
//    {
//        // TileMap을 EDungeonTileType으로 변환
//        TArray<TArray<EDungeonTileType>> ConvertedTileMap;
//        ConvertedTileMap.SetNum(MapSize.X);
//
//        for (int32 x = 0; x < MapSize.X; ++x)
//        {
//            ConvertedTileMap[x].SetNum(MapSize.Y);
//            for (int32 y = 0; y < MapSize.Y; ++y)
//            {
//                // ETileType02를 EDungeonTileType으로 변환
//                switch (TileMap[x][y])
//                {
//                case ETileType02::Empty:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Empty;
//                    break;
//                case ETileType02::Room:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Room;
//                    break;
//                case ETileType02::Corridor:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Corridor;
//                    break;
//                case ETileType02::BridgeCorridor:
//                    ConvertedTileMap[x][y] = EDungeonTileType::BridgeCorridor;
//                    break;
//                case ETileType02::Door:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Door;
//                    break;
//                case ETileType02::DeadEnd:
//                    ConvertedTileMap[x][y] = EDungeonTileType::DeadEnd;
//                    break;
//                case ETileType02::Junction:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Junction;
//                    break;
//                case ETileType02::CrossRoad:
//                    ConvertedTileMap[x][y] = EDungeonTileType::CrossRoad;
//                    break;
//                default:
//                    ConvertedTileMap[x][y] = EDungeonTileType::Empty;
//                    break;
//                }
//            }
//        }
//
//        // 방 정보 준비
//        TArray<FRoomInfo> RoomInfos;
//        for (int32 i = 0; i < LeafNodes.Num(); ++i)
//        {
//            if (LeafNodes[i]->bHasRoom)
//            {
//                FRoomInfo Info;
//                Info.RoomId = i;
//                Info.Center = GetRoomCenter(LeafNodes[i]);
//                Info.Min = LeafNodes[i]->RoomMin;
//                Info.Max = LeafNodes[i]->RoomMax;
//                RoomInfos.Add(Info);
//            }
//        }
//
//        // 그래프 분석 실행 (C++ 함수 직접 호출)
//        GraphAnalyzer->AnalyzeDungeon(ConvertedTileMap, RoomInfos, TileSize);
//
//        // 통계 출력
//        if (bShowStatistics)
//        {
//            GraphAnalyzer->PrintStatistics();
//
//            // 디버그 시각화
//            GraphAnalyzer->DrawDebugVisualization(GetWorld(), -1.0f);
//        }
//
//        // 분석 결과를 MapStats에 반영 (선택사항)
//        FDungeonGraphAnalysis GraphAnalysis = GraphAnalyzer->GetAnalysis();
//        MapStats.RoomCount = GraphAnalysis.RoomCount;
//        MapStats.DeadEndCount = GraphAnalysis.DeadEndCount;
//        MapStats.JunctionCount = GraphAnalysis.JunctionCount;
//        MapStats.CrossRoadCount = GraphAnalysis.CrossRoadCount;
//
//        // 간선 수 로그
//        UE_LOG(LogTemp, Warning, TEXT("Graph Analysis: Found %d nodes and %d edges"),
//            GraphAnalysis.NodeCount, GraphAnalysis.EdgeCount);
//    }
//
//    //// 맵 통계 분석 및 출력
//    //if (bShowStatistics)
//    //{
//    //    PrintMapStatistics();
//    //}
//
//}

void ABSPMapGenerator02::GenerateBSPMap()
{
    // 기존 맵(액터) 정리: 한 번만
    
    //실행 시간 측정 시작
    double Start = FPlatformTime::Seconds();
   

    // 시드 초기화(첫 시도용)
    if (RandomSeed == 0)
        RandomSeed = FMath::RandRange(1, INT32_MAX);

    const int32 BaseSeed = RandomSeed;

    bool bOk = false;
    FIntVector BadTL(-1, -1, 0);

    for (int32 Attempt = 1; Attempt <= FMath::Max(1, MaxGenerateAttempts); ++Attempt)
    {
        // ── 시드 결정 ──
        if (Attempt == 1)
            RandomStream.Initialize(BaseSeed);
        else
        {
            if (bReseedOnRetry)
                RandomSeed = FMath::RandRange(1, INT32_MAX);
            else
                ++RandomSeed; // 같은 값 반복 방지
            RandomStream.Initialize(RandomSeed);
        }

        UE_LOG(LogTemp, Warning, TEXT("[BSP] Attempt %d/%d (Seed=%d)"),
            Attempt, MaxGenerateAttempts, RandomSeed);

        ClearMap();

        // ── 타일맵 생성 파이프라인 (스폰/그래프분석 제외) ──
        InitializeTileMap();

        RootNode = CreateBSPTree(FIntVector(0, 0, 0), MapSize, 0);

        LeafNodes.Empty();
        CollectLeafNodes(RootNode);

        CreateRooms();
        ConnectRooms();

        CleanupParallelCorridors();

        if (bCreateLoops)
            CreateExtraConnections();

        CleanupParallelCorridors();

        // 두꺼운 복도 덩어리/1칸 갭 정리
        CollapseThickCorridorBlobsFavorDoor();
        PatchCorridorSingleTileGaps();

        SpawnTiles();

        // ── 검증: 2x2 복도 덩어리 존재? ──
        if (!Has2x2CorridorBlob(BadTL))
        {
            bOk = true;
            break; // 성공 → 루프 탈출
        }

        // 실패 로그 & 디버그 표시
#if WITH_EDITOR
        if (bMarkRejected2x2 && GetWorld())
        {
            const FVector Center = FVector((BadTL.X + 1.f) * TileSize, (BadTL.Y + 1.f) * TileSize, 0.f);
            DrawDebugBox(GetWorld(), Center, FVector(TileSize, TileSize, 20.f),
                FColor::Red, false, 2.0f, 0, 5.0f);
        }
#endif
        UE_LOG(LogTemp, Warning, TEXT("[BSP] Rejected: 2x2 corridor blob at (%d,%d)"),
            BadTL.X, BadTL.Y);
    }

    if (!bOk)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BSP] Max attempts reached. Using last result."));
    }

    //실행 시간 측정 끝
    double End = FPlatformTime::Seconds();
    UE_LOG(LogBSP, Warning, TEXT("BSP took: %.3f seconds"), End - Start);

    // ── 최종 1회만 스폰 ──
    
    // ── 그리고 마지막에 그래프 분석 ──
    RunGraphAnalysis();
    if (GraphAnalyzer)
    {
        const FDungeonGraphAnalysis GA = GraphAnalyzer->GetAnalysis();
        UE_LOG(LogBSP, Warning, TEXT("BSP Cyclo %d"), GA.CyclomaticComplexity);
    }

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed (Seed=%d)."), RandomSeed);
   
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

    // 좌우 이웃 + Y 구간 겹침 → 수평 직선 시도
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

            // 직선 연결도 평행 체크
            FIntVector StraightStart(Left->RoomMax.X, Y, 0);
            FIntVector StraightEnd(Right->RoomMin.X, Y, 0);

            if (!WouldCreateParallelCorridor(StraightStart, StraightEnd, 1.5f))
            {
                CarveHorizontal(Y, Left->RoomMax.X, Right->RoomMin.X);
                return; // 직선 성공
            }
        }
    }

    // 상하 이웃 + X 구간 겹침 → 수직 직선 시도
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

            // 직선 연결도 평행 체크
            FIntVector StraightStart(X, Bottom->RoomMax.Y, 0);
            FIntVector StraightEnd(X, Top->RoomMin.Y, 0);

            if (!WouldCreateParallelCorridor(StraightStart, StraightEnd, 1.5f))
            {
                CarveVertical(X, Bottom->RoomMax.Y, Top->RoomMin.Y);
                return; // 직선 성공
            }
        }
    }

    // 직선 불가능 또는 평행 위험 → L자 (개선된 버전 사용)
    FIntVector CenterA = GetRoomCenter(NodeA);
    FIntVector CenterB = GetRoomCenter(NodeB);

    // L자도 평행 체크
    if (WouldCreateParallelCorridor(CenterA, CenterB, 2.0f))
    {
        UE_LOG(LogTemp, Warning, TEXT("Skipping corridor due to high parallel risk between rooms"));
        return; // 연결 포기
    }

    CreateLShapedCorridorSafe(CenterA, CenterB);
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

    // 두 서브트리 사이에서 가장 가까운 방 쌍을 골라 연결 (평행 체크 포함)
    auto ConnectClosestBetween = [&](TArray<TSharedPtr<FBSPNode02>>& Ls, TArray<TSharedPtr<FBSPNode02>>& Rs)
        {
            if (Ls.Num() == 0 || Rs.Num() == 0) return;

            float Best = FLT_MAX;
            TSharedPtr<FBSPNode02> BestA, BestB;
            bool bFoundSafeConnection = false;

            // 1차: 평행 위험이 없는 가장 가까운 연결 찾기
            for (auto& A : Ls)
            {
                const FIntVector CA = GetRoomCenter(A);
                for (auto& B : Rs)
                {
                    const FIntVector CB = GetRoomCenter(B);
                    const float D = FVector::Dist(FVector(CA), FVector(CB));

                    // 평행 위험 체크
                    bool bSafe = !WouldCreateParallelCorridor(CA, CB, 2.0f);

                    if (bSafe && D < Best)
                    {
                        Best = D;
                        BestA = A;
                        BestB = B;
                        bFoundSafeConnection = true;
                    }
                }
            }

            // 안전한 연결이 없으면 어쩔 수 없이 가장 가까운 것 사용
            if (!bFoundSafeConnection)
            {
                UE_LOG(LogTemp, Warning, TEXT("No safe BSP connection found, using closest with parallel risk"));

                Best = FLT_MAX;
                for (auto& A : Ls)
                {
                    const FIntVector CA = GetRoomCenter(A);
                    for (auto& B : Rs)
                    {
                        const FIntVector CB = GetRoomCenter(B);
                        const float D = FVector::Dist(FVector(CA), FVector(CB));
                        if (D < Best)
                        {
                            Best = D;
                            BestA = A;
                            BestB = B;
                        }
                    }
                }
            }

            if (BestA && BestB)
            {
                if (bFoundSafeConnection)
                {
                    UE_LOG(LogTemp, Verbose, TEXT("Creating safe BSP connection"));
                }
                CreateCorridor(BestA, BestB);
            }
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
                TileClass =  CorridorVerticalClass;
            }
            else if (bHasEast || bHasWest)
            {
                // X축 방향 연결 = 가로 복도
                TileClass = CorridorHorizontalClass;
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


void ABSPMapGenerator02::EnsureCorridorConnection(const FIntVector& DoorPos,
    const FIntVector& Direction,
    TSharedPtr<FBSPNode02> /*Node*/)
{
    const FIntVector P = DoorPos + Direction;

    // 경계 체크
    if (!IsValidPosition(P.X, P.Y)) return;

    // 이미 복도 계열이면 그대로 둠(다시 덮지 않음)
    if (IsCorridorTile(P.X, P.Y)) return;

    // 빈칸일 때만 1칸 다리 생성 (※ 평행/대각 선검사 없음)
    if (TileMap[P.X][P.Y] == ETileType02::Empty)
    {
        TileMap[P.X][P.Y] = ETileType02::BridgeCorridor;
        SpawnSingleCorridorTile(P);
    }
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



// 후보 경로가 기존 복도에 "바짝 평행"하게 붙을 위험이 큰지 평가
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
    int32 RejectedCount = 0; // 거부된 연결 수 카운트

    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);

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

    TArray<TPair<int32, int32>> ExtraEdges;

    for (const auto& Conn : PotentialConnections)
    {
        if (ConnectionsAdded >= TargetConnections) break;

        if (RandomStream.FRand() >= ExtraConnectionChance) continue;

        TSharedPtr<FBSPNode02> NodeA = LeafNodes[Conn.Key];
        TSharedPtr<FBSPNode02> NodeB = LeafNodes[Conn.Value];

        FIntVector CenterA = GetRoomCenter(NodeA);
        FIntVector CenterB = GetRoomCenter(NodeB);

        // 1) 기존 중복 체크
        if (CorridorExists(CenterA, CenterB))
        {
            RejectedCount++;
            continue;
        }

        // 2) 개선된 평행 복도 체크 - 더 엄격하게
        if (WouldCreateParallelCorridor(CenterA, CenterB, 2.0f)) // 2칸 이내 평행 금지
        {
            RejectedCount++;
            UE_LOG(LogTemp, Verbose, TEXT("Rejected connection between room %d and %d due to parallel corridor risk"),
                Conn.Key, Conn.Value);
            continue;
        }

        // 3) 통과하면 복도 생성
        CreateCorridor(NodeA, NodeB);
        //CreateCorridorStopAtContact(NodeA, NodeB);
        ExtraEdges.Add(TPair<int32, int32>(Conn.Key, Conn.Value));
        ++ConnectionsAdded;

        UE_LOG(LogTemp, Verbose, TEXT("Added extra connection %d between room %d and %d"),
            ConnectionsAdded, Conn.Key, Conn.Value);
    }

    ExtraConnectionPairs = ExtraEdges;

    UE_LOG(LogTemp, Warning, TEXT("Created %d extra connections, rejected %d due to parallel risk"),
        ConnectionsAdded, RejectedCount);
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

void ABSPMapGenerator02::CreateZigzagCorridor(const FIntVector& Start, const FIntVector& End)
{
    auto ClampIn = [&](int32& v, int32 lo, int32 hi) { v = FMath::Clamp(v, lo, hi); };
    auto HasDiag = [&](int x, int y) {
        auto C = [&](int cx, int cy) { return IsValidPosition(cx, cy) && IsCorridorTile(cx, cy); };
        return C(x - 1, y - 1) || C(x + 1, y - 1) || C(x - 1, y + 1) || C(x + 1, y + 1);
        };

    // L-경로 한 구간(수평/수직)에 대해 "대각 접촉"이 나는지 빠르게 검사
    auto LDiagUnsafe = [&](const FIntVector& A, const FIntVector& B, bool bHorizontalFirst) {
        // 수평/수직 진행 순서에 맞춰 경로를 미리 스캔(타일을 아직 쓰지 않음)
        if (bHorizontalFirst) {
            int stepX = (A.X <= B.X) ? 1 : -1;
            for (int x = A.X; x != B.X + stepX; x += stepX)
                if (HasDiag(x, A.Y)) return true;
            int stepY = (A.Y <= B.Y) ? 1 : -1;
            for (int y = A.Y; y != B.Y + stepY; y += stepY)
                if (HasDiag(B.X, y)) return true;
        }
        else {
            int stepY = (A.Y <= B.Y) ? 1 : -1;
            for (int y = A.Y; y != B.Y + stepY; y += stepY)
                if (HasDiag(A.X, y)) return true;
            int stepX = (A.X <= B.X) ? 1 : -1;
            for (int x = A.X; x != B.X + stepX; x += stepX)
                if (HasDiag(x, B.Y)) return true;
        }
        return false;
        };

    // 한 구간이 "평행 위험" 또는 "대각 접촉" 없이 안전한지
    auto SegmentSafe = [&](const FIntVector& A, const FIntVector& B) {
        if (WouldCreateParallelCorridor(A, B, /*MinDistance=*/2.0f)) return false; // 평행 위험 차단
        // 두 가지 L 순서 중 하나라도 대각 접촉이 안 나면 통과
        if (LDiagUnsafe(A, B, true) && LDiagUnsafe(A, B, false)) return false;
        return true;
        };

    auto TryWithMid = [&](FIntVector Mid)->bool {
        if (!IsValidPosition(Mid.X, Mid.Y)) return false;

        // 두 구간 모두 선검사
        if (!SegmentSafe(Start, Mid)) return false;
        if (!SegmentSafe(Mid, End))   return false;

        // 둘 다 OK면 커밋(실제 쓰기)
        CreateLShapedCorridorSafe(Start, Mid);
        CreateLShapedCorridorSafe(Mid, End);
        return true;
        };

    // (1) 기본 중간점 산출 + 범위 보정
    int32 MidX = (Start.X + End.X) / 2;
    int32 MidY = (Start.Y + End.Y) / 2;
    int32 OffsetRange = 3;
    MidX += RandomStream.RandRange(-OffsetRange, OffsetRange);
    MidY += RandomStream.RandRange(-OffsetRange, OffsetRange);
    ClampIn(MidX, 0, MapSize.X - 1);
    ClampIn(MidY, 0, MapSize.Y - 1);
    FIntVector Mid(MidX, MidY, 0);

    // (2) 기본 시도 → 실패 시 여러 오프셋 대안 시도
    if (TryWithMid(Mid)) return;

    const FIntVector Alts[] = {
        FIntVector(Mid.X + 2, Mid.Y, 0), FIntVector(Mid.X - 2, Mid.Y, 0),
        FIntVector(Mid.X, Mid.Y + 2, 0), FIntVector(Mid.X, Mid.Y - 2, 0),
        FIntVector(Mid.X + 3, Mid.Y, 0), FIntVector(Mid.X - 3, Mid.Y, 0)
    };
    for (const auto& A : Alts) if (TryWithMid(A)) return;

    // (3) 지그재그가 전부 위험하면, 최후 수단으로 안전한 L-직결만 시도
    if (!WouldCreateParallelCorridor(Start, End, 2.0f)) {
        CreateLShapedCorridorSafe(Start, End);
    }
    else {
        UE_LOG(LogTemp, Verbose, TEXT("Zigzag skipped: parallel/diagonal risk"));
    }
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
        Type == ETileType02::CrossRoad ||
        Type == ETileType02::Door;
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
    auto IsIn = [this](int32 x, int32 y)
        {
            return x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y;
        };

    const int dx[4] = { 0, 0, 1, -1 };
    const int dy[4] = { 1, -1, 0, 0 };

    int Pass = 0;
    int TotalRemoved = 0;

    // 여러 군데에 흩어진 스텁을 한 번에 지우기 위해 소규모 반복(침식 방지용으로 최대 4회)
    while (Pass++ < 4)
    {
        TArray<FIntVector> ToRemove;

        for (int32 x = 0; x < MapSize.X; ++x)
        {
            for (int32 y = 0; y < MapSize.Y; ++y)
            {
                // 문 앞 1칸(BridgeCorridor)은 건드리지 않고, 순수 Corridor만 검사
                if (TileMap[x][y] != ETileType02::Corridor)
                    continue;

                // 1) 자신이 잎(연결 1개)이어야 함
                if (CountConnections(x, y) != 1)
                    continue;

                // 2) 붙어있는 이웃 찾기(복도 계열)
                int nx = 0, ny = 0;
                bool bFound = false;
                for (int k = 0; k < 4; ++k)
                {
                    int tx = x + dx[k], ty = y + dy[k];
                    if (IsIn(tx, ty) && IsCorridorTile(tx, ty))
                    {
                        nx = tx; ny = ty; bFound = true; break;
                    }
                }
                if (!bFound) continue;

                // 3) 이웃의 연결 상황 파악
                bool N = IsCorridorTile(nx, ny + 1);
                bool S = IsCorridorTile(nx, ny - 1);
                bool E = IsCorridorTile(nx + 1, ny);
                bool W = IsCorridorTile(nx - 1, ny);
                int  c = (N ? 1 : 0) + (S ? 1 : 0) + (E ? 1 : 0) + (W ? 1 : 0);

                // 이웃이 직선 통과(세로 또는 가로) + 스텁 1개 → 총 3연결이어야 함
                bool VerticalThrough = N && S;
                bool HorizontalThrough = E && W;

                if (c == 3 && (VerticalThrough ^ HorizontalThrough))
                {
                    // 세로 통과면 우리의 연결은 좌/우 방향(수직)이어야 함
                    if (VerticalThrough && x != nx && FMath::Abs(x - nx) == 1 && y == ny)
                    {
                        ToRemove.Add({ x, y, 0 });
                        continue;
                    }
                    // 가로 통과면 우리의 연결은 위/아래 방향(수직)이어야 함
                    if (HorizontalThrough && y != ny && FMath::Abs(y - ny) == 1 && x == nx)
                    {
                        ToRemove.Add({ x, y, 0 });
                        continue;
                    }
                }
            }
        }

        if (ToRemove.Num() == 0) break;

        for (const FIntVector& P : ToRemove)
        {
            TileMap[P.X][P.Y] = ETileType02::Empty;
        }
        TotalRemoved += ToRemove.Num();
    }

#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning, TEXT("CleanupParallelCorridors: removed %d side-stubs in %d passes"),
        TotalRemoved, Pass);
#endif
}


// 1. 더 엄격한 평행 감지
bool ABSPMapGenerator02::WouldCreateParallelCorridor(const FIntVector& Start, const FIntVector& End, float MinDistance = 2.0f)
{
    // L자 경로의 두 가지 가능성 모두 체크
    bool bHorizontalFirstBlocked = CheckLShapePathForParallel(Start, End, true, MinDistance);
    bool bVerticalFirstBlocked = CheckLShapePathForParallel(Start, End, false, MinDistance);

    // 둘 다 차단되면 이 연결은 포기
    return bHorizontalFirstBlocked && bVerticalFirstBlocked;
}

// 2. L자 경로별 평행 체크
bool ABSPMapGenerator02::CheckLShapePathForParallel(const FIntVector& Start, const FIntVector& End, bool bHorizontalFirst, float MinDistance)
{
    if (bHorizontalFirst)
    {
        // 수평 구간: Start.X → End.X (Y = Start.Y)
        if (HasParallelInRange(Start.X, End.X, Start.Y, true, MinDistance))
            return true;

        // 수직 구간: Start.Y → End.Y (X = End.X)  
        if (HasParallelInRange(Start.Y, End.Y, End.X, false, MinDistance))
            return true;
    }
    else
    {
        // 수직 구간: Start.Y → End.Y (X = Start.X)
        if (HasParallelInRange(Start.Y, End.Y, Start.X, false, MinDistance))
            return true;

        // 수평 구간: Start.X → End.X (Y = End.Y)
        if (HasParallelInRange(Start.X, End.X, End.Y, true, MinDistance))
            return true;
    }

    return false;
}

// 3. 특정 구간에서 평행 복도 존재 여부 체크
bool ABSPMapGenerator02::HasParallelInRange(int32 start, int32 end, int32 fixed, bool bHorizontal, float MinDistance)
{
    if (start == end) return false;

    int32 minVal = FMath::Min(start, end);
    int32 maxVal = FMath::Max(start, end);

    // 구간의 80% 이상에서 평행 복도가 감지되면 차단
    int32 totalChecks = maxVal - minVal + 1;
    int32 parallelCount = 0;
    int32 threshold = FMath::CeilToInt(totalChecks * 0.2f); // 20%만 넘어도 차단 (엄격)

    for (int32 i = minVal; i <= maxVal; ++i)
    {
        int32 checkX = bHorizontal ? i : fixed;
        int32 checkY = bHorizontal ? fixed : i;

        if (!IsValidPosition(checkX, checkY)) continue;

        // 현재 위치가 이미 복도면 평행 가능성 높음
        if (IsCorridorTile(checkX, checkY))
        {
            parallelCount++;
            continue;
        }

        // MinDistance 범위 내에 평행 복도가 있는지 체크
        bool hasNearbyParallel = false;

        for (int32 dist = 1; dist <= (int32)MinDistance; ++dist)
        {
            if (bHorizontal)
            {
                // 수평 진행 시 위아래 체크
                if ((checkY + dist < MapSize.Y && IsCorridorTile(checkX, checkY + dist)) ||
                    (checkY - dist >= 0 && IsCorridorTile(checkX, checkY - dist)))
                {
                    hasNearbyParallel = true;
                    break;
                }
            }
            else
            {
                // 수직 진행 시 좌우 체크  
                if ((checkX + dist < MapSize.X && IsCorridorTile(checkX + dist, checkY)) ||
                    (checkX - dist >= 0 && IsCorridorTile(checkX - dist, checkY)))
                {
                    hasNearbyParallel = true;
                    break;
                }
            }
        }

        if (hasNearbyParallel)
            parallelCount++;
    }

    return parallelCount > threshold;
}

bool ABSPMapGenerator02::IsValidPosition(int32 x, int32 y) const
{
    return x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y;
}

void ABSPMapGenerator02::CreateLShapedCorridorSafe(const FIntVector& Start, const FIntVector& End)
{
    // --- local helpers ------------------------------------------------------
    auto Carve = [&](int32 x, int32 y)
        {
            if (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y)
            {
                if (TileMap[x][y] == ETileType02::Empty)
                {
                    TileMap[x][y] = ETileType02::Corridor;
                }
            }
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

    auto IsHorizontalSegmentSafe = [&](int32 y, int32 x0, int32 x1, int32 maxConsecutive)->bool
        {
            int32 step = (x0 <= x1) ? 1 : -1;
            int32 consecutive = 0;
            for (int32 x = x0; x != x1 + step; x += step)
            {
                const bool up = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
                const bool down = (y > 0) && IsCorridorTile(x, y - 1);
                if (up || down)
                {
                    if (++consecutive > maxConsecutive) return false; // abort BEFORE carving
                }
                else
                {
                    consecutive = 0;
                }
            }
            return true;
        };

    auto IsVerticalSegmentSafe = [&](int32 x, int32 y0, int32 y1, int32 maxConsecutive)->bool
        {
            int32 step = (y0 <= y1) ? 1 : -1;
            int32 consecutive = 0;
            for (int32 y = y0; y != y1 + step; y += step)
            {
                const bool left = (x > 0) && IsCorridorTile(x - 1, y);
                const bool right = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
                if (left || right)
                {
                    if (++consecutive > maxConsecutive) return false; // abort BEFORE carving
                }
                else
                {
                    consecutive = 0;
                }
            }
            return true;
        };
    // ------------------------------------------------------------------------

    const int32 MaxConsecutive = 2;

    auto AttemptHorizontalFirst = [&]() -> bool
        {
            // 1) 먼저 전체 경로 안전성 검사
            if (!IsHorizontalSegmentSafe(Start.Y, Start.X, End.X, MaxConsecutive))
                return false;

            // 2) 통과 시에만 일괄 Carve (스텁 방지)
            CarveHorizontal(Start.Y, Start.X, End.X);
            CarveVertical(End.X, Start.Y, End.Y);
            return true;
        };

    auto AttemptVerticalFirst = [&]() -> bool
        {
            if (!IsVerticalSegmentSafe(Start.X, Start.Y, End.Y, MaxConsecutive))
                return false;

            CarveVertical(Start.X, Start.Y, End.Y);
            CarveHorizontal(End.Y, Start.X, End.X);
            return true;
        };

    // 기존 로직 유지: 랜덤으로 우선순위 선택, 실패 시 반대 순서 시도
    if (RandomStream.FRand() > 0.5f)
    {
        if (!AttemptHorizontalFirst()) AttemptVerticalFirst();
    }
    else
    {
        if (!AttemptVerticalFirst()) AttemptHorizontalFirst();
    }
}

// 추가 복도 전용: 진행 중 기존 복도/문/브리지를 만나면 그 직전까지 파고 종료
void ABSPMapGenerator02::CreateCorridorStopAtContact(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB)
{
    if (!NodeA.IsValid() || !NodeB.IsValid()) return;
    if (!NodeA->bHasRoom || !NodeB->bHasRoom) return;

    auto InBounds = [&](int32 x, int32 y)
        {
            return x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y;
        };
    auto IsCorridorLike = [&](int32 x, int32 y)
        {
            if (!InBounds(x, y)) return false;
            const ETileType02 t = TileMap[x][y];
            // “연결”로 인정할 것들
            return (t == ETileType02::Corridor || t == ETileType02::BridgeCorridor || t == ETileType02::Door);
        };
    auto CarveIfEmpty = [&](int32 x, int32 y)
        {
            if (InBounds(x, y) && TileMap[x][y] == ETileType02::Empty)
                TileMap[x][y] = ETileType02::Corridor;
        };
    auto ConnectedToOthers = [&](int32 x, int32 y, int32 px, int32 py)->bool
        {
            // 방금 깐 타일 (x,y)이 “이전칸(px,py)”을 제외한 주위 4방 중
            // 하나라도 기존 복도 계열이면 '연결 성립'
            const int dx[4] = { 0, 0, 1,-1 };
            const int dy[4] = { 1,-1, 0, 0 };
            for (int k = 0; k < 4; ++k)
            {
                const int nx = x + dx[k], ny = y + dy[k];
                if (nx == px && ny == py) continue; // 자기 직전칸(우리가 깐 경로)은 제외
                if (IsCorridorLike(nx, ny)) return true;
            }
            return false;
        };

    // 한 구간을 깔되, “연결”이 성립하는 즉시 true 반환(나머지 구간 스킵 신호)
    auto CarveHorizontalUntilJoin = [&](int32 y, int32 x0, int32 x1)->bool
        {
            int32 step = (x0 <= x1) ? 1 : -1;
            int32 px = x0 - step, py = y; // 이전칸(초기엔 진행 반대쪽)
            for (int32 x = x0; x != x1 + step; x += step)
            {
                CarveIfEmpty(x, y);
                if (ConnectedToOthers(x, y, px, py)) return true; // 연결 성립 → 즉시 종료
                px = x; py = y;
            }
            return false; // 끝까지 갔지만 중간 연결은 없었음
        };
    auto CarveVerticalUntilJoin = [&](int32 x, int32 y0, int32 y1)->bool
        {
            int32 step = (y0 <= y1) ? 1 : -1;
            int32 px = x, py = y0 - step;
            for (int32 y = y0; y != y1 + step; y += step)
            {
                CarveIfEmpty(x, y);
                if (ConnectedToOthers(x, y, px, py)) return true;
                px = x; py = y;
            }
            return false;
        };

    const FBSPNode02& A = *NodeA.Get();
    const FBSPNode02& B = *NodeB.Get();

    // ── 1) 좌우 이웃 + Y구간 겹치면 수평 직선 우선(기존 CreateCorridor 로직과 동일) ──
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
            CarveHorizontalUntilJoin(Y, Left->RoomMax.X, Right->RoomMin.X);
            return; // 수평 구간에서 이미 연결되면 더 이상 진행하지 않음
        }
    }

    // ── 2) 상하 이웃 + X구간 겹치면 수직 직선 ──
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
            CarveVerticalUntilJoin(X, Bottom->RoomMax.Y, Top->RoomMin.Y);
            return;
        }
    }

    // ── 3) 그 외엔 L자. 1구간에서 연결되면 2구간은 스킵 ──
    FIntVector S = GetRoomCenter(NodeA);
    FIntVector E = GetRoomCenter(NodeB);

    if (RandomStream.FRand() < 0.5f)
    {
        // H → V
        if (!CarveHorizontalUntilJoin(S.Y, S.X, E.X))
            CarveVerticalUntilJoin(E.X, S.Y, E.Y);
    }
    else
    {
        // V → H
        if (!CarveVerticalUntilJoin(S.X, S.Y, E.Y))
            CarveHorizontalUntilJoin(E.Y, S.X, E.X);
    }
}

void ABSPMapGenerator02::CollapseThickCorridorBlobsFavorDoor()
{
    auto In = [&](int32 x, int32 y) { return x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y; };

    // Corridor 계열(문 앞 한칸 포함)
    auto IsCorr = [&](int32 x, int32 y)->bool
        {
            if (!In(x, y)) return false;
            const ETileType02 t = TileMap[x][y];
            return (t == ETileType02::Corridor || t == ETileType02::BridgeCorridor);
        };
    // 문/브리지(보호 우선순위)
    auto IsDoorLike = [&](int32 x, int32 y)->bool
        {
            if (!In(x, y)) return false;
            const ETileType02 t = TileMap[x][y];
            return (t == ETileType02::Door || t == ETileType02::BridgeCorridor);
        };
    // 외부 열린면 판정(연결로 문도 포함)
    auto IsCorridorLikeOutside = [&](int32 x, int32 y)->bool
        {
            if (!In(x, y)) return false;
            const ETileType02 t = TileMap[x][y];
            return (t == ETileType02::Corridor || t == ETileType02::BridgeCorridor || t == ETileType02::Door);
        };

    struct Sides { bool L = false, R = false, T = false, B = false; };
    auto SideOpens = [&](int32 x0, int32 y0, int32 w, int32 h)->Sides
        {
            Sides s;
            for (int32 yy = y0; yy < y0 + h; ++yy) { s.L |= IsCorridorLikeOutside(x0 - 1, yy); s.R |= IsCorridorLikeOutside(x0 + w, yy); }
            for (int32 xx = x0; xx < x0 + w; ++xx) { s.B |= IsCorridorLikeOutside(xx, y0 - 1);   s.T |= IsCorridorLikeOutside(xx, y0 + h); }
            return s;
        };

    // ----- 2x2: 한 칸 제거해도 외부 열린면 연결이 유지되는지 로컬 검증 -----
    auto RemovalKeepsConnectivity_2x2 = [&](int32 x0, int32 y0, int32 remIdx)->bool
        {
            const FIntVector cell[4] = {
                FIntVector(x0,   y0,   0),
                FIntVector(x0 + 1, y0,   0),
                FIntVector(x0,   y0 + 1, 0),
                FIntVector(x0 + 1, y0 + 1, 0)
            };

            const Sides s = SideOpens(x0, y0, 2, 2);

            // 남는 3칸 수집
            TArray<FIntVector> remain; remain.Reserve(3);
            for (int32 i = 0; i < 4; ++i)
            {
                if (i == remIdx) continue;
                if (IsCorr(cell[i].X, cell[i].Y)) remain.Add(cell[i]);
            }
            if (remain.Num() == 0) return false;

            // 내부 BFS
            TSet<FIntVector> vis; TArray<FIntVector> q; q.Add(remain[0]); vis.Add(remain[0]);
            auto Push = [&](int32 ax, int32 ay) {
                if (!In(ax, ay) || !IsCorr(ax, ay))return;
                FIntVector v(ax, ay, 0); if (!vis.Contains(v)) { vis.Add(v); q.Add(v); }
                };
            for (int32 qi = 0; qi < q.Num(); ++qi) {
                const int32 x = q[qi].X, y = q[qi].Y;
                if (x - 1 >= x0 && IsCorr(x - 1, y)) Push(x - 1, y);
                if (x + 1 <= x0 + 1 && IsCorr(x + 1, y)) Push(x + 1, y);
                if (y - 1 >= y0 && IsCorr(x, y - 1)) Push(x, y - 1);
                if (y + 1 <= y0 + 1 && IsCorr(x, y + 1)) Push(x, y + 1);
            }

            auto touchesL = [&]() { return s.L && ((vis.Contains(FIntVector(x0, y0, 0)) && IsCorridorLikeOutside(x0 - 1, y0)) ||
                (vis.Contains(FIntVector(x0, y0 + 1, 0)) && IsCorridorLikeOutside(x0 - 1, y0 + 1))); };
            auto touchesR = [&]() { return s.R && ((vis.Contains(FIntVector(x0 + 1, y0, 0)) && IsCorridorLikeOutside(x0 + 2, y0)) ||
                (vis.Contains(FIntVector(x0 + 1, y0 + 1, 0)) && IsCorridorLikeOutside(x0 + 2, y0 + 1))); };
            auto touchesB = [&]() { return s.B && ((vis.Contains(FIntVector(x0, y0, 0)) && IsCorridorLikeOutside(x0, y0 - 1)) ||
                (vis.Contains(FIntVector(x0 + 1, y0, 0)) && IsCorridorLikeOutside(x0 + 1, y0 - 1))); };
            auto touchesT = [&]() { return s.T && ((vis.Contains(FIntVector(x0, y0 + 1, 0)) && IsCorridorLikeOutside(x0, y0 + 2)) ||
                (vis.Contains(FIntVector(x0 + 1, y0 + 1, 0)) && IsCorridorLikeOutside(x0 + 1, y0 + 2))); };

            if (s.L && !touchesL()) return false;
            if (s.R && !touchesR()) return false;
            if (s.B && !touchesB()) return false;
            if (s.T && !touchesT()) return false;
            return true;
        };

    // ----- w×h 박스: (줄/열 전체 제거) 연결성 검증 -----
    auto RemovalKeepsConnectivity_Box = [&](int32 x0, int32 y0, int32 w, int32 h, bool removeIsCol, int32 rmIdx)->bool
        {
            const Sides s0 = SideOpens(x0, y0, w, h);
            auto InBox = [&](int32 ax, int32 ay) {return (ax >= x0 && ax < x0 + w && ay >= y0 && ay < y0 + h); };
            auto Removed = [&](int32 ax, int32 ay) { return removeIsCol ? (ax == x0 + rmIdx) : (ay == y0 + rmIdx); };

            // 시작점
            FIntVector st(-999, -999, 0);
            for (int32 yy = y0; yy < y0 + h && st.X == -999; ++yy)
                for (int32 xx = x0; xx < x0 + w && st.X == -999; ++xx)
                    if (!Removed(xx, yy) && IsCorr(xx, yy)) st = FIntVector(xx, yy, 0);
            if (st.X == -999) return false;

            // BFS
            TSet<FIntVector> vis; TArray<FIntVector> q; q.Add(st); vis.Add(st);
            auto Push = [&](int32 ax, int32 ay) {
                if (!In(ax, ay) || !InBox(ax, ay) || Removed(ax, ay) || !IsCorr(ax, ay))return;
                FIntVector v(ax, ay, 0); if (!vis.Contains(v)) { vis.Add(v); q.Add(v); }
                };
            for (int32 qi = 0; qi < q.Num(); ++qi) {
                const int32 x = q[qi].X, y = q[qi].Y;
                if (x - 1 >= x0)     Push(x - 1, y);
                if (x + 1 < x0 + w)    Push(x + 1, y);
                if (y - 1 >= y0)     Push(x, y - 1);
                if (y + 1 < y0 + h)    Push(x, y + 1);
            }

            auto touchL = [&]() { if (!s0.L) return true; for (int32 yy = y0; yy < y0 + h; ++yy) if (vis.Contains(FIntVector(x0, yy, 0)) && IsCorridorLikeOutside(x0 - 1, yy)) return true; return false; };
            auto touchR = [&]() { if (!s0.R) return true; const int32 xr = x0 + w - 1; for (int32 yy = y0; yy < y0 + h; ++yy) if (vis.Contains(FIntVector(xr, yy, 0)) && IsCorridorLikeOutside(xr + 1, yy)) return true; return false; };
            auto touchB = [&]() { if (!s0.B) return true; for (int32 xx = x0; xx < x0 + w; ++xx) if (vis.Contains(FIntVector(xx, y0, 0)) && IsCorridorLikeOutside(xx, y0 - 1)) return true; return false; };
            auto touchT = [&]() { if (!s0.T) return true; const int32 yt = y0 + h - 1; for (int32 xx = x0; xx < x0 + w; ++xx) if (vis.Contains(FIntVector(xx, yt, 0)) && IsCorridorLikeOutside(xx, yt + 1)) return true; return false; };

            return touchL() && touchR() && touchB() && touchT();
        };

    // ----- keep-one(줄/열에서 하나 남기기) 연결성 검증 -----
    auto RemovalKeepsConnectivity_KeepOne = [&](int32 x0, int32 y0, int32 w, int32 h, bool removeIsCol, int32 rmIdx, int32 keepOffset)->bool
        {
            const Sides s0 = SideOpens(x0, y0, w, h);
            auto InBox = [&](int32 ax, int32 ay) {return (ax >= x0 && ax < x0 + w && ay >= y0 && ay < y0 + h); };
            auto Removed = [&](int32 ax, int32 ay) {
                if (removeIsCol) {
                    int32 col = ax - (x0);
                    int32 row = ay - (y0);
                    return (col == rmIdx) && (row != keepOffset);
                }
                else {
                    int32 row = ay - (y0);
                    int32 col = ax - (x0);
                    return (row == rmIdx) && (col != keepOffset);
                }
                };

            // 시작점
            FIntVector st(-999, -999, 0);
            for (int32 yy = y0; yy < y0 + h && st.X == -999; ++yy)
                for (int32 xx = x0; xx < x0 + w && st.X == -999; ++xx)
                    if (!Removed(xx, yy) && IsCorr(xx, yy)) st = FIntVector(xx, yy, 0);
            if (st.X == -999) return false;

            // BFS
            TSet<FIntVector> vis; TArray<FIntVector> q; q.Add(st); vis.Add(st);
            auto Push = [&](int32 ax, int32 ay) {
                if (!In(ax, ay) || !InBox(ax, ay) || Removed(ax, ay) || !IsCorr(ax, ay))return;
                FIntVector v(ax, ay, 0); if (!vis.Contains(v)) { vis.Add(v); q.Add(v); }
                };
            for (int32 qi = 0; qi < q.Num(); ++qi) {
                const int32 x = q[qi].X, y = q[qi].Y;
                if (x - 1 >= x0)     Push(x - 1, y);
                if (x + 1 < x0 + w)    Push(x + 1, y);
                if (y - 1 >= y0)     Push(x, y - 1);
                if (y + 1 < y0 + h)    Push(x, y + 1);
            }

            auto touchL = [&]() { if (!s0.L) return true; for (int32 yy = y0; yy < y0 + h; ++yy) if (vis.Contains(FIntVector(x0, yy, 0)) && IsCorridorLikeOutside(x0 - 1, yy)) return true; return false; };
            auto touchR = [&]() { if (!s0.R) return true; const int32 xr = x0 + w - 1; for (int32 yy = y0; yy < y0 + h; ++yy) if (vis.Contains(FIntVector(xr, yy, 0)) && IsCorridorLikeOutside(xr + 1, yy)) return true; return false; };
            auto touchB = [&]() { if (!s0.B) return true; for (int32 xx = x0; xx < x0 + w; ++xx) if (vis.Contains(FIntVector(xx, y0, 0)) && IsCorridorLikeOutside(xx, y0 - 1)) return true; return false; };
            auto touchT = [&]() { if (!s0.T) return true; const int32 yt = y0 + h - 1; for (int32 xx = x0; xx < x0 + w; ++xx) if (vis.Contains(FIntVector(xx, yt, 0)) && IsCorridorLikeOutside(xx, yt + 1)) return true; return false; };

            return touchL() && touchR() && touchB() && touchT();
        };

    auto ScoreRemoval_2x2 = [&](int32 x0, int32 y0, int32 remIdx)->int32
        {
            const FIntVector cell[4] = {
                FIntVector(x0,   y0,   0),
                FIntVector(x0 + 1, y0,   0),
                FIntVector(x0,   y0 + 1, 0),
                FIntVector(x0 + 1, y0 + 1, 0)
            };
            const int32 rx = cell[remIdx].X, ry = cell[remIdx].Y;

            int32 doorDist = 0;
            doorDist += IsDoorLike(rx - 1, ry) ? 0 : 1;
            doorDist += IsDoorLike(rx + 1, ry) ? 0 : 1;
            doorDist += IsDoorLike(rx, ry - 1) ? 0 : 1;
            doorDist += IsDoorLike(rx, ry + 1) ? 0 : 1;

            int32 openings = 0;
            openings += IsCorridorLikeOutside(rx - 1, ry) ? 1 : 0;
            openings += IsCorridorLikeOutside(rx + 1, ry) ? 1 : 0;
            openings += IsCorridorLikeOutside(rx, ry - 1) ? 1 : 0;
            openings += IsCorridorLikeOutside(rx, ry + 1) ? 1 : 0;

            return openings * 4 + doorDist; // 낮을수록 제거 선호
        };

    auto SideSum = [&](int32 x0, int32 y0, int32 w, int32 h, bool horizontal)->int32
        {
            int32 s = 0;
            if (horizontal) for (int32 xx = x0; xx < x0 + w; ++xx) s += IsCorridorLikeOutside(xx, y0) ? 1 : 0;
            else           for (int32 yy = y0; yy < y0 + h; ++yy) s += IsCorridorLikeOutside(x0, yy) ? 1 : 0;
            return s;
        };

    // 제거 예약 집합
    TSet<FIntVector> toRemove;
    auto IsMarked = [&](int32 ax, int32 ay)->bool { return toRemove.Contains(FIntVector(ax, ay, 0)); };

    // ===== 2×2 처리 =====
    for (int32 x = 0; x < MapSize.X - 1; ++x)
        for (int32 y = 0; y < MapSize.Y - 1; ++y)
        {
            if (!(IsCorr(x, y) && IsCorr(x + 1, y) && IsCorr(x, y + 1) && IsCorr(x + 1, y + 1))) continue;

            const ETileType02 t00 = TileMap[x][y], t10 = TileMap[x + 1][y], t01 = TileMap[x][y + 1], t11 = TileMap[x + 1][y + 1];

            int32 bestIdx = -1, bestScore = INT32_MAX;
            for (int32 idx = 0; idx < 4; ++idx)
            {
                const ETileType02 tt = (idx == 0 ? t00 : idx == 1 ? t10 : idx == 2 ? t01 : t11);
                if (tt == ETileType02::BridgeCorridor) continue;        // 보호
                if (!RemovalKeepsConnectivity_2x2(x, y, idx)) continue; // 연결 보존

                const int32 sc = ScoreRemoval_2x2(x, y, idx);
                if (sc < bestScore) { bestScore = sc; bestIdx = idx; }
            }
            if (bestIdx != -1)
            {
                static const FIntVector mapIdx[4] = { FIntVector(0,0,0),FIntVector(1,0,0),FIntVector(0,1,0),FIntVector(1,1,0) };
                toRemove.Add(FIntVector(x + mapIdx[bestIdx].X, y + mapIdx[bestIdx].Y, 0));
            }
        }

    // ===== 2×3(세로) : 열 하나 얇게 (한 칸은 남김) =====
    for (int32 x = 0; x < MapSize.X - 1; ++x)
        for (int32 y = 0; y < MapSize.Y - 2; ++y)
        {
            if (!(IsCorr(x, y) && IsCorr(x + 1, y) && IsCorr(x, y + 1) && IsCorr(x + 1, y + 1) && IsCorr(x, y + 2) && IsCorr(x + 1, y + 2))) continue;

            const bool doorL = IsDoorLike(x - 1, y) || IsDoorLike(x - 1, y + 1) || IsDoorLike(x - 1, y + 2);
            const bool doorR = IsDoorLike(x + 2, y) || IsDoorLike(x + 2, y + 1) || IsDoorLike(x + 2, y + 2);
            const int32 keepCol = (doorL && !doorR) ? 0 : ((!doorL && doorR) ? 1 : (SideSum(x - 1, y, 1, 3, false) > SideSum(x + 2, y, 1, 3, false) ? 0 : 1));
            const int32 rmCol = 1 - keepCol;
            const int32 rx = x + rmCol;

            // BridgeCorridor 보호
            bool can = true;
            for (int32 yy = y; yy <= y + 2; ++yy) if (TileMap[rx][yy] == ETileType02::BridgeCorridor) { can = false; break; }
            if (!can) continue;

            // 충돌 방지: 보존 열에 이미 제거 예약이 있으면 스킵
            bool conflict = false;
            for (int32 yy = y; yy <= y + 2; ++yy) if (IsMarked(x + keepCol, yy)) { conflict = true; break; }
            if (conflict) continue;

            // (선택) 줄 전체 제거 연결성 체크 -> 실패해도 keep-one으로 시도
            bool ok_full = RemovalKeepsConnectivity_Box(x, y, 2, 3, true/*col*/, rmCol);

            // 남길 한 칸 선택: 문 우선 -> 외부열림 합 -> 중앙
            auto ScoreKeepCol = [&](int32 yy)->int32
                {
                    int32 s = 0;
                    // 문/브리지에 붙으면 가점(낮은 점수)
                    bool nearDoor = IsDoorLike(rx - 1, yy) || IsDoorLike(rx + 1, yy);
                    s += nearDoor ? 0 : 4;
                    // 외부열림(좌/우) 많이 맞닿을수록 가점
                    int32 open = (IsCorridorLikeOutside(rx - 1, yy) ? 1 : 0) + (IsCorridorLikeOutside(rx + 1, yy) ? 1 : 0);
                    s += (2 - open) * 2;
                    // 중앙 선호
                    s += FMath::Abs((y + 1) - yy);
                    return s;
                };
            int32 keepYY = y; int32 bestS = INT32_MAX;
            for (int32 yy = y; yy <= y + 2; ++yy) { int32 sc = ScoreKeepCol(yy); if (sc < bestS) { bestS = sc; keepYY = yy; } }

            // keep-one 연결성 체크
            if (!RemovalKeepsConnectivity_KeepOne(x, y, 2, 3, true/*col*/, rmCol, keepYY - y))
            {
                // keep-one도 연결성 깨면 전체 스킵
                continue;
            }

            // 적용: rmCol 열에서 keepYY만 남기고 나머지 2칸 제거
            for (int32 yy = y; yy <= y + 2; ++yy)
                if (yy != keepYY) toRemove.Add(FIntVector(rx, yy, 0));
        }

    // ===== 3×2(가로) : 행 하나 얇게 (한 칸은 남김) =====
    for (int32 x = 0; x < MapSize.X - 2; ++x)
        for (int32 y = 0; y < MapSize.Y - 1; ++y)
        {
            if (!(IsCorr(x, y) && IsCorr(x + 1, y) && IsCorr(x + 2, y) && IsCorr(x, y + 1) && IsCorr(x + 1, y + 1) && IsCorr(x + 2, y + 1))) continue;

            const bool doorB = IsDoorLike(x, y - 1) || IsDoorLike(x + 1, y - 1) || IsDoorLike(x + 2, y - 1);
            const bool doorT = IsDoorLike(x, y + 2) || IsDoorLike(x + 1, y + 2) || IsDoorLike(x + 2, y + 2);
            const int32 keepRow = (doorB && !doorT) ? 0 : ((!doorB && doorT) ? 1 : (SideSum(x, y - 1, 3, 1, true) > SideSum(x, y + 2, 3, 1, true) ? 0 : 1));
            const int32 rmRow = 1 - keepRow;
            const int32 ry = y + rmRow;

            // BridgeCorridor 보호
            bool can = true;
            for (int32 xx = x; xx <= x + 2; ++xx) if (TileMap[xx][ry] == ETileType02::BridgeCorridor) { can = false; break; }
            if (!can) continue;

            // 충돌 방지: 보존 행에 이미 제거 예약 있으면 스킵
            bool conflict = false;
            for (int32 xx = x; xx <= x + 2; ++xx) if (IsMarked(xx, y + keepRow)) { conflict = true; break; }
            if (conflict) continue;

            //// (선택) 줄 전체 제거 연결성 체크 -> 실패해도 keep-one으로 시도
            //bool ok_full = RemovalKeepsConnectivity_Box(x, y, 3, 2, false/*row*/, rmRow);

            // 남길 한 칸 선택: 문 우선 -> 외부열림 합 -> 중앙
            auto ScoreKeepRow = [&](int32 xx)->int32
                {
                    int32 s = 0;
                    bool nearDoor = IsDoorLike(xx, ry - 1) || IsDoorLike(xx, ry + 1);
                    s += nearDoor ? 0 : 4;
                    int32 open = (IsCorridorLikeOutside(xx, ry - 1) ? 1 : 0) + (IsCorridorLikeOutside(xx, ry + 1) ? 1 : 0);
                    s += (2 - open) * 2;
                    s += FMath::Abs((x + 1) - xx);
                    return s;
                };
            int32 keepXX = x; int32 bestS = INT32_MAX;
            for (int32 xx = x; xx <= x + 2; ++xx) { int32 sc = ScoreKeepRow(xx); if (sc < bestS) { bestS = sc; keepXX = xx; } }

            // keep-one 연결성 체크
            if (!RemovalKeepsConnectivity_KeepOne(x, y, 3, 2, false/*row*/, rmRow, keepXX - x))
            {
                // keep-one도 연결성 깨면 전체 스킵
                continue;
            }

            // 적용: rmRow 행에서 keepXX만 남기고 나머지 2칸 제거
            for (int32 xx = x; xx <= x + 2; ++xx)
                if (xx != keepXX) toRemove.Add(FIntVector(xx, ry, 0));
        }

    auto WouldBreakLocalBridge = [&](int32 cx, int32 cy)->bool
        {
            auto Pass = [&](int32 ax, int32 ay)->bool
                {
                    if (!In(ax, ay)) return false;
                    if (ax == cx && ay == cy) return false; // 중심(지울 칸)은 제외
                    // 이미 다른 후보가 제거 예정이면 통과 불가로 취급
                    if (toRemove.Contains(FIntVector(ax, ay, 0))) return false;

                    ETileType02 t = TileMap[ax][ay];
                    // 문/브릿지도 통행 가능으로 봄
                    return (t == ETileType02::Corridor || t == ETileType02::Door || t == ETileType02::BridgeCorridor);
                };

            // 3x3(센터 제외) 안에서 통행 가능한 씨드 수집
            TArray<FIntVector> seeds;
            for (int32 y = cy - 1; y <= cy + 1; ++y)
                for (int32 x = cx - 1; x <= cx + 1; ++x)
                    if (!(x == cx && y == cy) && Pass(x, y))
                        seeds.Add(FIntVector(x, y, 0));

            if (seeds.Num() <= 1) return false; // 연결할 대상이 0/1개면 단절 문제 없음

            // 로컬 BFS로 연결성 확인(센터 없이 서로 도달 가능한지)
            TSet<FIntVector> vis; TArray<FIntVector> q;
            auto Push = [&](int32 ax, int32 ay)
                {
                    if (!Pass(ax, ay)) return;
                    FIntVector v(ax, ay, 0);
                    if (!vis.Contains(v)) { vis.Add(v); q.Add(v); }
                };
            Push(seeds[0].X, seeds[0].Y);
            for (int32 i = 0; i < q.Num(); ++i)
            {
                int32 x = q[i].X, y = q[i].Y;
                Push(x - 1, y); Push(x + 1, y); Push(x, y - 1); Push(x, y + 1);
            }

            // 씨드 중 일부라도 서로 닿지 못한다면, 이 칸은 '브리지' 역할 → 제거 금지
            return vis.Num() < seeds.Num();
        };


    // ===== 적용 =====
    for (const FIntVector& p : toRemove)
    {
        if (!In(p.X, p.Y)) continue;
        if (TileMap[p.X][p.Y] == ETileType02::BridgeCorridor) continue; // 보호
        if (WouldBreakLocalBridge(p.X, p.Y)) continue;
        if (TileMap[p.X][p.Y] == ETileType02::Corridor) TileMap[p.X][p.Y] = ETileType02::Empty;
    }

#if !UE_BUILD_SHIPPING
    if (toRemove.Num() > 0)
        UE_LOG(LogTemp, Warning, TEXT("CollapseThickCorridorBlobsFavorDoor v2.4: removed %d tiles (keep-one bridge)"), toRemove.Num());
#endif
}

void ABSPMapGenerator02::PatchCorridorSingleTileGaps()
{
    auto InBounds = [&](int32 x, int32 y)
        {
            return (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y);
        };

    auto IsCorr = [&](int32 x, int32 y)
        {
            return InBounds(x, y) && IsCorridorTile(x, y);
        };

    auto IsEmpty = [&](int32 x, int32 y)
        {
            return InBounds(x, y) && TileMap[x][y] == ETileType02::Empty;
        };

    TArray<FIntVector> ToFill;

    // ── Pass A: 좌우/상하로 '정확히 1칸 갭' 메우기 ──
    for (int32 y = 1; y < MapSize.Y - 1; ++y)
    {
        for (int32 x = 1; x < MapSize.X - 1; ++x)
        {
            if (!IsEmpty(x, y)) continue;

            const bool L = IsCorr(x - 1, y);
            const bool R = IsCorr(x + 1, y);
            const bool U = IsCorr(x, y + 1);
            const bool D = IsCorr(x, y - 1);

            // 수평 1칸 갭
            if (L && R && !U && !D)
            {
                ToFill.Add(FIntVector(x, y, 0));
                continue;
            }
            // 수직 1칸 갭
            if (U && D && !L && !R)
            {
                ToFill.Add(FIntVector(x, y, 0));
                continue;
            }
        }
    }

    auto WouldCreate2x2At = [&](int32 fx, int32 fy)->bool
        {
            auto Passable = [&](int32 x, int32 y)->bool
                {
                    if (x == fx && y == fy) return true; // 지금 막 메운다고 가정
                    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y) return false;
                    const ETileType02 t = TileMap[x][y];
                    // 통로 취급 타일 (Door 포함)
                    return t == ETileType02::Corridor
                        || t == ETileType02::BridgeCorridor
                        || t == ETileType02::DeadEnd
                        || t == ETileType02::Junction
                        || t == ETileType02::CrossRoad
                        || t == ETileType02::Door;
                };

            // (fx,fy)를 포함하는 2x2 네 블록을 모두 검사
            // 좌상단이 (fx-1,fy-1)~(fx,fy)인 2x2, 총 4가지 원근
            for (int ox = -1; ox <= 0; ++ox)
            {
                for (int oy = -1; oy <= 0; ++oy)
                {
                    const int x0 = fx + ox, y0 = fy + oy;
                    if (x0 < 0 || y0 < 0 || x0 + 1 >= MapSize.X || y0 + 1 >= MapSize.Y) continue;

                    const bool a = Passable(x0, y0);
                    const bool b = Passable(x0 + 1, y0);
                    const bool c = Passable(x0, y0 + 1);
                    const bool d = Passable(x0 + 1, y0 + 1);

                    if (a && b && c && d) return true; // 진짜 2x2 형성
                }
            }
            return false;
        };

    // ── Pass B: 데드엔드가 '앞으로 한 칸'만 더 가면 자연스레 이어지는 경우 ──
    for (int32 y = 0; y < MapSize.Y; ++y)
    {
        for (int32 x = 0; x < MapSize.X; ++x)
        {
            if (!IsCorr(x, y)) continue;

            // 기존 유틸: 연결 개수 1개면 데드엔드
            if (CountConnections(x, y) != 1) continue;

            // 이웃 복도(유일한 연결) 찾아서 그 반대 방향으로 1칸 연장
            FIntVector Dir(0, 0, 0);
            if (IsCorr(x + 1, y)) Dir = FIntVector(-1, 0, 0);
            else if (IsCorr(x - 1, y)) Dir = FIntVector(1, 0, 0);
            else if (IsCorr(x, y + 1)) Dir = FIntVector(0, -1, 0);
            else if (IsCorr(x, y - 1)) Dir = FIntVector(0, 1, 0);

            if (Dir == FIntVector::ZeroValue) continue;

            const int32 tx = x + Dir.X;
            const int32 ty = y + Dir.Y;
            if (!InBounds(tx, ty) || !IsEmpty(tx, ty)) continue;

            // 2x2 두꺼운 블록 방지: 연장 방향의 측면에 기존 복도가 붙어 있으면 스킵
            const bool IsHorizontal = (Dir.X != 0);
            //if (IsHorizontal)
            //{
            //    if (IsCorr(tx, ty + 1) || IsCorr(tx, ty - 1)) continue;
            //}
            //else // Vertical
            //{
            //    if (IsCorr(tx + 1, ty) || IsCorr(tx - 1, ty)) continue;
            //}

            if (WouldCreate2x2At(tx, ty)) continue;

            ToFill.AddUnique(FIntVector(tx, ty, 0));
        }
    }

    // ── 적용: 맵만 갱신 (스폰은 기존 SpawnTiles 단계에서 일괄 처리) ──
    for (const FIntVector& P : ToFill)
    {
        TileMap[P.X][P.Y] = ETileType02::Corridor; // 또는 BridgeCorridor로 표기하고 싶으면 변경
    }

#if !UE_BUILD_SHIPPING
    if (ToFill.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("PatchCorridorSingleTileGaps: filled %d single-tile gaps"), ToFill.Num());
    }
#endif
}

bool ABSPMapGenerator02::Has2x2CorridorBlob(FIntVector& OutTopLeft)
{
    auto InBounds = [&](int32 x, int32 y) {
        return (x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y);
        };

    // 2x2 블록 검사용 전용 판정자: Door도 '통로'로 간주
    auto IsPassableForBlob = [&](int32 x, int32 y)->bool {
        if (!InBounds(x, y)) return false;
        const ETileType02 t = TileMap[x][y];
        return  t == ETileType02::Corridor
            || t == ETileType02::BridgeCorridor
            || t == ETileType02::DeadEnd
            || t == ETileType02::Junction
            || t == ETileType02::CrossRoad
            || t == ETileType02::Door;          // ← 추가 포인트
        };

    for (int32 x = 0; x <= MapSize.X - 2; ++x)
    {
        for (int32 y = 0; y <= MapSize.Y - 2; ++y)
        {
            const bool c00 = IsPassableForBlob(x, y);
            const bool c10 = IsPassableForBlob(x + 1, y);
            const bool c01 = IsPassableForBlob(x, y + 1);
            const bool c11 = IsPassableForBlob(x + 1, y + 1);

            if (c00 && c10 && c01 && c11)
            {
                OutTopLeft = FIntVector(x, y, 0);
                return true;
            }
        }
    }
    OutTopLeft = FIntVector(-1, -1, 0);
    return false;
}

// 기존 GenerateBSPMap 안에 있던 "ConvertedTileMap 만들고 AnalyzeDungeon" 블록을 그대로 옮김
void ABSPMapGenerator02::RunGraphAnalysis()
{
    if (!GraphAnalyzer) return;

    // 1) TileMap -> EDungeonTileType 변환
    TArray<TArray<EDungeonTileType>> ConvertedTileMap;
    ConvertedTileMap.SetNum(MapSize.X);
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        ConvertedTileMap[x].SetNum(MapSize.Y);
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            switch (TileMap[x][y])
            {
            case ETileType02::Empty:          ConvertedTileMap[x][y] = EDungeonTileType::Empty; break;
            case ETileType02::Room:           ConvertedTileMap[x][y] = EDungeonTileType::Room; break;
            case ETileType02::Corridor:       ConvertedTileMap[x][y] = EDungeonTileType::Corridor; break;
            case ETileType02::BridgeCorridor: ConvertedTileMap[x][y] = EDungeonTileType::BridgeCorridor; break;
            case ETileType02::Door:           ConvertedTileMap[x][y] = EDungeonTileType::Door; break;
            case ETileType02::DeadEnd:        ConvertedTileMap[x][y] = EDungeonTileType::DeadEnd; break;
            case ETileType02::Junction:       ConvertedTileMap[x][y] = EDungeonTileType::Junction; break;
            case ETileType02::CrossRoad:      ConvertedTileMap[x][y] = EDungeonTileType::CrossRoad; break;
            default:                          ConvertedTileMap[x][y] = EDungeonTileType::Empty; break;
            }
        }
    }

    // 2) 방 정보 수집
    TArray<FRoomInfo> RoomInfos;
    RoomInfos.Reserve(LeafNodes.Num());
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

    // 3) 분석 실행 & 출력
    GraphAnalyzer->AnalyzeDungeon(ConvertedTileMap, RoomInfos, TileSize);

    if (bShowStatistics)
    {
        GraphAnalyzer->PrintStatistics();
        GraphAnalyzer->DrawDebugVisualization(GetWorld(), -1.0f);
    }

    // 4) 결과 일부를 MapStats에 반영 (선택)
    const FDungeonGraphAnalysis GA = GraphAnalyzer->GetAnalysis();
    MapStats.RoomCount = GA.RoomCount;
    MapStats.DeadEndCount = GA.DeadEndCount;
    MapStats.JunctionCount = GA.JunctionCount;
    MapStats.CrossRoadCount = GA.CrossRoadCount;

    UE_LOG(LogTemp, Warning, TEXT("Graph Analysis: Found %d nodes and %d edges"),
        GA.NodeCount, GA.EdgeCount);
}
