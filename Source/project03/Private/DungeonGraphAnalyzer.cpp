#include "DungeonGraphAnalyzer.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UDungeonGraphAnalyzer::AnalyzeDungeon(
    const TArray<TArray<EDungeonTileType>>& TileMap,
    const TArray<FRoomInfo>& RoomInfos,
    float TileSize)
{
    // 초기화
    ClearGraph();

    // 데이터 복사
    WorkingTileMap = TileMap;
    WorkingRoomInfos = RoomInfos;
    WorkingTileSize = TileSize;

    if (WorkingTileMap.Num() == 0) return;

    MapSize = FIntVector(WorkingTileMap.Num(),
        WorkingTileMap[0].Num(), 1);

    UE_LOG(LogTemp, Warning, TEXT("=== Starting Dungeon Graph Analysis ==="));
    UE_LOG(LogTemp, Warning, TEXT("Map Size: %dx%d"), MapSize.X, MapSize.Y);
    UE_LOG(LogTemp, Warning, TEXT("Room Count: %d"), WorkingRoomInfos.Num());

    // Step 1: 복도 타입 분석 (막다른길, 갈림길 찾기)
    AnalyzeCorridorTypes();

    // Step 2: 노드 추출
    ExtractNodes();

    // Step 3: 간선 추출
    ExtractEdges();

    // Step 4: 그래프 분석
    AnalyzeGraph();

    UE_LOG(LogTemp, Warning, TEXT("=== Graph Analysis Complete ==="));
    UE_LOG(LogTemp, Warning, TEXT("Nodes: %d, Edges: %d"), Nodes.Num(), Edges.Num());
}

void UDungeonGraphAnalyzer::AnalyzeDungeonBP(
    const FDungeonTileMap& TileMapWrapper,
    const TArray<FRoomInfo>& RoomInfos,
    float TileSize)
{
    // 래퍼에서 실제 타일맵 추출하여 분석
    AnalyzeDungeon(TileMapWrapper.Tiles, RoomInfos, TileSize);
}

void UDungeonGraphAnalyzer::ClearGraph()
{
    Nodes.Empty();
    Edges.Empty();
    Analysis = FDungeonGraphAnalysis();
    WorkingTileMap.Empty();
    WorkingRoomInfos.Empty();
}

void UDungeonGraphAnalyzer::AnalyzeCorridorTypes()
{
    UE_LOG(LogTemp, Warning, TEXT("Step 1: Analyzing corridor types..."));

    // 모든 복도 타일을 순회하며 타입 결정
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            if (!IsCorridorTile(x, y)) continue;

            // 이미 특수 타입이면 건너뛰기
            if (WorkingTileMap[x][y] == EDungeonTileType::DeadEnd ||
                WorkingTileMap[x][y] == EDungeonTileType::Junction ||
                WorkingTileMap[x][y] == EDungeonTileType::CrossRoad)
            {
                continue;
            }

            EDungeonTileType NewType = DetermineCorridorType(x, y);
            WorkingTileMap[x][y] = NewType;
        }
    }
}

EDungeonTileType UDungeonGraphAnalyzer::DetermineCorridorType(int32 x, int32 y) const
{
    // 복도 연결 개수 세기
    int32 Connections = CountCorridorConnections(x, y);

    // 방 인접 여부 체크
    bool AdjacentToRoom = false;
    const int32 dx[] = { 0, 0, 1, -1 };
    const int32 dy[] = { 1, -1, 0, 0 };

    for (int32 i = 0; i < 4; ++i)
    {
        int32 nx = x + dx[i];
        int32 ny = y + dy[i];
        if (nx >= 0 && nx < MapSize.X && ny >= 0 && ny < MapSize.Y)
        {
            if (WorkingTileMap[nx][ny] == EDungeonTileType::Room)
            {
                AdjacentToRoom = true;
                break;
            }
        }
    }

    // BridgeCorridor(문 앞 연결 복도)는 항상 일반 복도로 처리
    if (WorkingTileMap[x][y] == EDungeonTileType::BridgeCorridor)
        return EDungeonTileType::Corridor;

    // 방에 인접하고 연결이 1개 이하면 일반 복도 (문 연결부)
    if (AdjacentToRoom && Connections <= 1)
        return EDungeonTileType::Corridor;

    // 나란한 복도 체크 - 평행 레인 예외 처리
    if (IsPartOfWideCorrridor(x, y))
        return EDungeonTileType::Corridor;

    // 기본 규칙
    switch (Connections)
    {
    case 0:
    case 1:
        return EDungeonTileType::DeadEnd;
    case 2:
        return EDungeonTileType::Corridor;
    case 3:
        return EDungeonTileType::Junction;
    case 4:
        return EDungeonTileType::CrossRoad;
    default:
        return EDungeonTileType::Corridor;
    }
}

bool UDungeonGraphAnalyzer::IsPartOfWideCorrridor(int32 x, int32 y) const
{
    // 복도 이웃들
    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
    const bool W = (x > 0) && IsCorridorTile(x - 1, y);

    // 가로 진행(E&W) + 위/아래 한쪽만 열린 경우
    if (E && W && (N ^ S))
    {
        const int ny = N ? (y + 1) : (y - 1);
        // 위/아래 이웃이 가로 밴드면 분기 아님
        if (ny >= 0 && ny < MapSize.Y &&
            IsCorridorTile(x - 1, ny) && IsCorridorTile(x + 1, ny))
        {
            return true;
        }
    }

    // 세로 진행(N&S) + 좌/우 한쪽만 열린 경우
    if (N && S && (E ^ W))
    {
        const int nx = E ? (x + 1) : (x - 1);
        // 좌/우 이웃이 세로 밴드면 분기 아님
        if (nx >= 0 && nx < MapSize.X &&
            IsCorridorTile(nx, y - 1) && IsCorridorTile(nx, y + 1))
        {
            return true;
        }
    }

    return false;
}

void UDungeonGraphAnalyzer::ExtractNodes()
{
    UE_LOG(LogTemp, Warning, TEXT("Step 2: Extracting nodes..."));

    Nodes.Empty();
    int32 NodeIdCounter = 0;

    // 1. 방 노드 추가
    for (const FRoomInfo& Room : WorkingRoomInfos)
    {
        FDungeonGraphNode Node;
        Node.NodeId = NodeIdCounter++;
        Node.Position = Room.Center;
        Node.NodeType = EGraphNodeType::Room;
        Node.RoomId = Room.RoomId;
        Node.Bounds = FBox2D(FVector2D(Room.Min.X, Room.Min.Y),
            FVector2D(Room.Max.X, Room.Max.Y));

        Nodes.Add(Node);
    }

    // 2. 복도 특수 노드 추가 (막다른길, 갈림길)
    TSet<FIntVector> ProcessedPositions;

    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            FIntVector Pos(x, y, 0);

            // 이미 처리된 위치면 건너뛰기
            if (ProcessedPositions.Contains(Pos)) continue;

            EDungeonTileType Type = WorkingTileMap[x][y];

            if (Type == EDungeonTileType::DeadEnd ||
                Type == EDungeonTileType::Junction ||
                Type == EDungeonTileType::CrossRoad)
            {
                // 방 영역 내부가 아닌지 확인
                bool InsideRoom = false;
                for (const FRoomInfo& Room : WorkingRoomInfos)
                {
                    if (x >= Room.Min.X && x < Room.Max.X &&
                        y >= Room.Min.Y && y < Room.Max.Y)
                    {
                        InsideRoom = true;
                        break;
                    }
                }

                if (!InsideRoom)
                {
                    FDungeonGraphNode Node;
                    Node.NodeId = NodeIdCounter++;
                    Node.Position = Pos;

                    switch (Type)
                    {
                    case EDungeonTileType::DeadEnd:
                        Node.NodeType = EGraphNodeType::DeadEnd;
                        break;
                    case EDungeonTileType::Junction:
                        Node.NodeType = EGraphNodeType::Junction;
                        break;
                    case EDungeonTileType::CrossRoad:
                        Node.NodeType = EGraphNodeType::CrossRoad;
                        break;
                    }

                    Node.RoomId = -1;
                    Node.Bounds = FBox2D(FVector2D(x, y), FVector2D(x + 1, y + 1));

                    Nodes.Add(Node);
                    ProcessedPositions.Add(Pos);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Extracted %d nodes"), Nodes.Num());
}

//void UDungeonGraphAnalyzer::ExtractEdges()
//{
//    UE_LOG(LogTemp, Warning, TEXT("Step 3: Extracting edges..."));
//
//    Edges.Empty();
//    TSet<TPair<int32, int32>> ProcessedPairs;
//
//    // 모든 노드 쌍에 대해 복도 경로 찾기
//    for (int32 i = 0; i < Nodes.Num(); ++i)
//    {
//        for (int32 j = i + 1; j < Nodes.Num(); ++j)
//        {
//            // 이미 처리된 쌍이면 건너뛰기
//            TPair<int32, int32> Pair(i, j);
//            if (ProcessedPairs.Contains(Pair)) continue;
//
//            // 노드 간 복도 경로 찾기
//            TArray<FIntVector> Path;
//
//            // 다른 노드들을 피해서 경로 찾기
//            TSet<FIntVector> AvoidNodes;
//            for (int32 k = 0; k < Nodes.Num(); ++k)
//            {
//                if (k != i && k != j)
//                {
//                    // 방 노드는 전체 영역을 피해야 함
//                    if (Nodes[k].NodeType == EGraphNodeType::Room)
//                    {
//                        for (const FRoomInfo& Room : WorkingRoomInfos)
//                        {
//                            if (Room.RoomId == Nodes[k].RoomId)
//                            {
//                                for (int32 rx = Room.Min.X; rx < Room.Max.X; ++rx)
//                                {
//                                    for (int32 ry = Room.Min.Y; ry < Room.Max.Y; ++ry)
//                                    {
//                                        AvoidNodes.Add(FIntVector(rx, ry, 0));
//                                    }
//                                }
//                                break;
//                            }
//                        }
//                    }
//                    else
//                    {
//                        // 복도 노드는 해당 위치만 피함
//                        AvoidNodes.Add(Nodes[k].Position);
//                    }
//                }
//            }
//
//            if (FindCorridorPath(Nodes[i].Position, Nodes[j].Position, Path, AvoidNodes))
//            {
//                FDungeonGraphEdge Edge;
//                Edge.StartNodeId = i;
//                Edge.EndNodeId = j;
//                Edge.Path = Path;
//                Edge.Length = Path.Num();
//
//                Edges.Add(Edge);
//
//                // 노드 연결 정보 업데이트
//                Nodes[i].ConnectedNodes.Add(j);
//                Nodes[j].ConnectedNodes.Add(i);
//
//                ProcessedPairs.Add(Pair);
//            }
//        }
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("Extracted %d edges"), Edges.Num());
//}

//void UDungeonGraphAnalyzer::ExtractEdges()
//{
//    UE_LOG(LogTemp, Warning, TEXT("Step 3: Extracting edges..."));
//
//    Edges.Empty();
//    TSet<TPair<int32, int32>> ProcessedPairs;
//
//    // 각 노드에서 시작하여 인접한 노드만 찾기
//    for (int32 i = 0; i < Nodes.Num(); ++i)
//    {
//        TArray<FIntVector> StartPositions;
//
//        if (Nodes[i].NodeType == EGraphNodeType::Room)
//        {
//            // 방의 경우 가장자리 복도 타일 찾기
//            for (const FRoomInfo& Room : WorkingRoomInfos)
//            {
//                if (Room.RoomId == Nodes[i].RoomId)
//                {
//                    // 방 주변 복도 타일 수집
//                    for (int32 x = Room.Min.X - 1; x <= Room.Max.X; ++x)
//                    {
//                        for (int32 y = Room.Min.Y - 1; y <= Room.Max.Y; ++y)
//                        {
//                            if ((x == Room.Min.X - 1 || x == Room.Max.X ||
//                                y == Room.Min.Y - 1 || y == Room.Max.Y) &&
//                                x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y &&
//                                IsCorridorTile(x, y))
//                            {
//                                StartPositions.Add(FIntVector(x, y, 0));
//                            }
//                        }
//                    }
//                    break;
//                }
//            }
//        }
//        else
//        {
//            // 복도 노드는 자기 위치에서 시작
//            StartPositions.Add(Nodes[i].Position);
//        }
//
//        // 각 시작점에서 복도를 따라가며 첫 번째 노드까지만 추적
//        for (const FIntVector& StartPos : StartPositions)
//        {
//            TArray<FIntVector> Path;
//            int32 ConnectedNodeId = TraceCorridorToNextNode(StartPos, i, Path);
//
//            if (ConnectedNodeId >= 0 && ConnectedNodeId != i)
//            {
//                // 중복 체크
//                TPair<int32, int32> EdgePair(FMath::Min(i, ConnectedNodeId),
//                    FMath::Max(i, ConnectedNodeId));
//
//                if (!ProcessedPairs.Contains(EdgePair))
//                {
//                    ProcessedPairs.Add(EdgePair);
//
//                    FDungeonGraphEdge Edge;
//                    Edge.StartNodeId = i;
//                    Edge.EndNodeId = ConnectedNodeId;
//                    Edge.Path = Path;
//                    Edge.Length = Path.Num();
//
//                    Edges.Add(Edge);
//
//                    // 노드 연결 정보 업데이트
//                    Nodes[i].ConnectedNodes.AddUnique(ConnectedNodeId);
//                    Nodes[ConnectedNodeId].ConnectedNodes.AddUnique(i);
//                }
//            }
//        }
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("Extracted %d edges"), Edges.Num());
//}

void UDungeonGraphAnalyzer::ExtractEdges()
{
    UE_LOG(LogTemp, Warning, TEXT("Step 3: Extracting edges..."));

    Edges.Empty();

    // 노드 쌍별로 독립적인 경로들을 저장 (중복 간선 허용)
    TMap<TPair<int32, int32>, TArray<FDungeonGraphEdge>> NodePairEdges;

    // 각 노드에서 시작하여 모든 연결된 복도 탐색
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        TArray<FIntVector> StartPositions;
        TSet<FIntVector> ProcessedStartPositions; // 이미 처리한 시작점 추적

        if (Nodes[i].NodeType == EGraphNodeType::Room)
        {
            // 방의 경우 가장자리 복도 타일 찾기
            for (const FRoomInfo& Room : WorkingRoomInfos)
            {
                if (Room.RoomId == Nodes[i].RoomId)
                {
                    // 방 주변 복도 타일 수집
                    for (int32 x = Room.Min.X - 1; x <= Room.Max.X; ++x)
                    {
                        for (int32 y = Room.Min.Y - 1; y <= Room.Max.Y; ++y)
                        {
                            if ((x == Room.Min.X - 1 || x == Room.Max.X ||
                                y == Room.Min.Y - 1 || y == Room.Max.Y) &&
                                x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y &&
                                IsCorridorTile(x, y))
                            {
                                FIntVector StartPos(x, y, 0);
                                if (!ProcessedStartPositions.Contains(StartPos))
                                {
                                    StartPositions.Add(StartPos);
                                    ProcessedStartPositions.Add(StartPos);
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            // 복도 노드는 자기 위치에서 시작
            StartPositions.Add(Nodes[i].Position);
        }

        // 각 시작점에서 복도를 따라가며 연결된 노드 찾기
        for (const FIntVector& StartPos : StartPositions)
        {
            // 이 시작점에서 여러 방향으로 갈 수 있으므로 각 방향 탐색
            const int32 dx[] = { 0, 0, 1, -1 };
            const int32 dy[] = { 1, -1, 0, 0 };

            for (int32 dir = 0; dir < 4; ++dir)
            {
                FIntVector FirstStep(StartPos.X + dx[dir], StartPos.Y + dy[dir], 0);

                // 유효한 복도 타일인지 확인
                if (!IsValidCorridorStep(FirstStep, i))
                    continue;

                // 이 방향으로 경로 추적
                TArray<FIntVector> Path;
                int32 ConnectedNodeId = TraceCorridorPathInDirection(
                    StartPos, FirstStep, i, Path);

                if (ConnectedNodeId >= 0 && ConnectedNodeId != i)
                {
                    // 유효한 경로를 찾음
                    FDungeonGraphEdge Edge;
                    Edge.StartNodeId = i;
                    Edge.EndNodeId = ConnectedNodeId;
                    Edge.Path = Path;
                    Edge.Length = Path.Num();

                    // 노드 쌍의 키 생성 (작은 ID가 먼저)
                    TPair<int32, int32> NodePair(
                        FMath::Min(i, ConnectedNodeId),
                        FMath::Max(i, ConnectedNodeId));

                    // 이 경로가 기존 경로와 충분히 다른지 확인
                    bool bIsUniquePath = true;
                    if (NodePairEdges.Contains(NodePair))
                    {
                        for (const FDungeonGraphEdge& ExistingEdge : NodePairEdges[NodePair])
                        {
                            if (ArePathsSimilar(Edge.Path, ExistingEdge.Path))
                            {
                                bIsUniquePath = false;
                                break;
                            }
                        }
                    }

                    if (bIsUniquePath)
                    {
                        // 새로운 독립적인 경로 추가
                        if (!NodePairEdges.Contains(NodePair))
                        {
                            NodePairEdges.Add(NodePair, TArray<FDungeonGraphEdge>());
                        }
                        NodePairEdges[NodePair].Add(Edge);

                        UE_LOG(LogTemp, Warning,
                            TEXT("Found path from Node %d to Node %d (Length: %d)"),
                            i, ConnectedNodeId, Path.Num());
                    }
                }
            }
        }
    }

    // 모든 찾은 간선을 최종 Edges 배열에 추가
    for (const auto& Pair : NodePairEdges)
    {
        for (const FDungeonGraphEdge& Edge : Pair.Value)
        {
            Edges.Add(Edge);

            // 노드 연결 정보 업데이트
            Nodes[Edge.StartNodeId].ConnectedNodes.AddUnique(Edge.EndNodeId);
            Nodes[Edge.EndNodeId].ConnectedNodes.AddUnique(Edge.StartNodeId);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Extracted %d edges total"), Edges.Num());

    // 중복 경로 정보 출력
    int32 DuplicateCount = 0;
    for (const auto& Pair : NodePairEdges)
    {
        if (Pair.Value.Num() > 1)
        {
            DuplicateCount++;
            UE_LOG(LogTemp, Warning,
                TEXT("Node %d <-> Node %d has %d parallel paths"),
                Pair.Key.Key, Pair.Key.Value, Pair.Value.Num());
        }
    }
    if (DuplicateCount > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Found %d node pairs with multiple paths"), DuplicateCount);
    }
}

// 복도를 따라가며 다음 노드 찾기
int32 UDungeonGraphAnalyzer::TraceCorridorToNextNode(
    const FIntVector& StartPos,
    int32 StartNodeId,
    TArray<FIntVector>& OutPath)
{
    OutPath.Reset();

    TSet<FIntVector> Visited;
    FIntVector Current = StartPos;
    FIntVector Previous = StartPos;

    // 시작 노드가 방인 경우, 방 영역 스킵
    if (Nodes[StartNodeId].NodeType == EGraphNodeType::Room)
    {
        // 방에서 나가는 방향 결정
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[StartNodeId].RoomId)
            {
                // 방 영역을 Visited에 추가 (재진입 방지)
                for (int32 x = Room.Min.X; x < Room.Max.X; ++x)
                {
                    for (int32 y = Room.Min.Y; y < Room.Max.Y; ++y)
                    {
                        Visited.Add(FIntVector(x, y, 0));
                    }
                }
                break;
            }
        }
    }

    OutPath.Add(Current);
    Visited.Add(Current);

    // 복도를 따라가며 추적
    const int32 MaxSteps = 100;
    for (int32 Step = 0; Step < MaxSteps; ++Step)
    {
        // 4방향 확인
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        bool FoundNext = false;
        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // 범위 체크
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // 이미 방문했으면 스킵
            if (Visited.Contains(Next))
                continue;

            // 다른 노드에 도달했는지 확인
            for (int32 j = 0; j < Nodes.Num(); ++j)
            {
                if (j == StartNodeId) continue;

                bool IsAtNode = false;

                if (Nodes[j].NodeType == EGraphNodeType::Room)
                {
                    // 방 가장자리 체크
                    for (const FRoomInfo& Room : WorkingRoomInfos)
                    {
                        if (Room.RoomId == Nodes[j].RoomId)
                        {
                            // 방 바로 인접 타일인지 확인
                            if ((Next.X >= Room.Min.X - 1 && Next.X <= Room.Max.X &&
                                Next.Y >= Room.Min.Y - 1 && Next.Y <= Room.Max.Y) &&
                                !(Next.X >= Room.Min.X && Next.X < Room.Max.X &&
                                    Next.Y >= Room.Min.Y && Next.Y < Room.Max.Y))
                            {
                                IsAtNode = true;
                            }
                            break;
                        }
                    }
                }
                else
                {
                    // 복도 노드는 정확한 위치 매칭
                    if (Nodes[j].Position == Next)
                    {
                        IsAtNode = true;
                    }
                }

                if (IsAtNode)
                {
                    OutPath.Add(Next);
                    return j;  // 연결된 노드 ID 반환
                }
            }

            // 복도 타일이면 계속 진행
            if (IsCorridorTile(Next))
            {
                Previous = Current;
                Current = Next;
                OutPath.Add(Current);
                Visited.Add(Current);
                FoundNext = true;
                break;
            }
        }

        if (!FoundNext)
            break;  // 막다른 길이거나 더 이상 진행 불가
    }

    return -1;  // 연결된 노드를 찾지 못함
}

bool UDungeonGraphAnalyzer::FindCorridorPath(
    const FIntVector& Start,
    const FIntVector& End,
    TArray<FIntVector>& OutPath,
    const TSet<FIntVector>& AvoidNodes)
{
    OutPath.Reset();

    // BFS로 최단 경로 찾기
    TQueue<FIntVector> Queue;
    TSet<FIntVector> Visited;
    TMap<FIntVector, FIntVector> Parent;

    // 시작점 찾기 (방의 경우 가장자리에서 시작)
    TArray<FIntVector> StartPoints;
    if (FindRoomNodeContaining(Start) >= 0)
    {
        // 방에서 시작하는 경우, 방 가장자리의 복도 타일 찾기
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.Center == Start)
            {
                // 방 가장자리 체크
                for (int32 x = Room.Min.X - 1; x <= Room.Max.X; ++x)
                {
                    for (int32 y = Room.Min.Y - 1; y <= Room.Max.Y; ++y)
                    {
                        // 방 경계 바로 밖의 복도 타일
                        if ((x == Room.Min.X - 1 || x == Room.Max.X ||
                            y == Room.Min.Y - 1 || y == Room.Max.Y) &&
                            x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y &&
                            IsCorridorTile(x, y))
                        {
                            StartPoints.Add(FIntVector(x, y, 0));
                        }
                    }
                }
                break;
            }
        }
    }
    else
    {
        StartPoints.Add(Start);
    }

    // 도착점 설정
    TSet<FIntVector> EndPoints;
    if (FindRoomNodeContaining(End) >= 0)
    {
        // 방이 도착점인 경우, 방 가장자리 복도 타일들을 목표로
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.Center == End)
            {
                for (int32 x = Room.Min.X - 1; x <= Room.Max.X; ++x)
                {
                    for (int32 y = Room.Min.Y - 1; y <= Room.Max.Y; ++y)
                    {
                        if ((x == Room.Min.X - 1 || x == Room.Max.X ||
                            y == Room.Min.Y - 1 || y == Room.Max.Y) &&
                            x >= 0 && x < MapSize.X && y >= 0 && y < MapSize.Y &&
                            IsCorridorTile(x, y))
                        {
                            EndPoints.Add(FIntVector(x, y, 0));
                        }
                    }
                }
                break;
            }
        }
    }
    else
    {
        EndPoints.Add(End);
    }

    // BFS 시작
    for (const FIntVector& StartPoint : StartPoints)
    {
        Queue.Enqueue(StartPoint);
        Visited.Add(StartPoint);
    }

    bool bFound = false;
    FIntVector FoundEnd;

    while (!Queue.IsEmpty() && !bFound)
    {
        FIntVector Current;
        Queue.Dequeue(Current);

        // 도착점 도달 체크
        if (EndPoints.Contains(Current))
        {
            bFound = true;
            FoundEnd = Current;
            break;
        }

        // 4방향 탐색
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // 범위 체크
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // 이미 방문했으면 건너뛰기
            if (Visited.Contains(Next))
                continue;

            // 피해야 할 노드면 건너뛰기 (목적지 제외)
            if (AvoidNodes.Contains(Next) && !EndPoints.Contains(Next))
                continue;

            // 복도 타일이 아니면 건너뛰기 (목적지 제외)
            if (!IsCorridorTile(Next) && !EndPoints.Contains(Next))
                continue;

            Visited.Add(Next);
            Parent.Add(Next, Current);
            Queue.Enqueue(Next);
        }
    }

    // 경로 복원
    if (bFound)
    {
        TArray<FIntVector> ReversePath;
        FIntVector Current = FoundEnd;

        while (Parent.Contains(Current))
        {
            ReversePath.Add(Current);
            Current = Parent[Current];
        }
        ReversePath.Add(Current);

        // 역순으로 저장
        for (int32 i = ReversePath.Num() - 1; i >= 0; --i)
        {
            OutPath.Add(ReversePath[i]);
        }

        return true;
    }

    return false;
}

void UDungeonGraphAnalyzer::AnalyzeGraph()
{
    UE_LOG(LogTemp, Warning, TEXT("Step 4: Analyzing graph structure..."));

    Analysis = FDungeonGraphAnalysis();

    // 노드 수 카운트
    Analysis.NodeCount = Nodes.Num();
    Analysis.EdgeCount = Edges.Num();

    // 노드 타입별 카운트
    for (const FDungeonGraphNode& Node : Nodes)
    {
        switch (Node.NodeType)
        {
        case EGraphNodeType::Room:
            Analysis.RoomCount++;
            break;
        case EGraphNodeType::DeadEnd:
            Analysis.DeadEndCount++;
            break;
        case EGraphNodeType::Junction:
            Analysis.JunctionCount++;
            break;
        case EGraphNodeType::CrossRoad:
            Analysis.CrossRoadCount++;
            break;
        }
    }

    // 연결된 컴포넌트 수
    Analysis.ConnectedComponents = CountConnectedComponents();

    // 순환 복잡도 (Cyclomatic Complexity)
    // V(G) = E - N + P (간선수 - 노드수 + 연결성분수)
    Analysis.CyclomaticComplexity = Analysis.EdgeCount - Analysis.NodeCount + Analysis.ConnectedComponents;

    // 평균 노드 차수
    if (Analysis.NodeCount > 0)
    {
        int32 TotalDegree = 0;
        for (const FDungeonGraphNode& Node : Nodes)
        {
            TotalDegree += Node.ConnectedNodes.Num();
        }
        Analysis.AverageNodeDegree = (float)TotalDegree / (float)Analysis.NodeCount;
    }

    // 평균 경로 길이
    if (Analysis.EdgeCount > 0)
    {
        float TotalLength = 0.0f;
        for (const FDungeonGraphEdge& Edge : Edges)
        {
            TotalLength += Edge.Length;
        }
        Analysis.AveragePathLength = TotalLength / (float)Analysis.EdgeCount;
    }
}

bool UDungeonGraphAnalyzer::IsCorridorTile(int32 x, int32 y) const
{
    if (x < 0 || x >= MapSize.X || y < 0 || y >= MapSize.Y)
        return false;

    EDungeonTileType Type = WorkingTileMap[x][y];
    return Type == EDungeonTileType::Corridor ||
        Type == EDungeonTileType::BridgeCorridor ||
        Type == EDungeonTileType::DeadEnd ||
        Type == EDungeonTileType::Junction ||
        Type == EDungeonTileType::CrossRoad ||
        Type == EDungeonTileType::Door;
}

bool UDungeonGraphAnalyzer::IsCorridorTile(const FIntVector& Pos) const
{
    return IsCorridorTile(Pos.X, Pos.Y);
}

int32 UDungeonGraphAnalyzer::CountCorridorConnections(int32 x, int32 y) const
{
    int32 Count = 0;

    // 4방향 체크
    const int32 dx[] = { 0, 0, 1, -1 };
    const int32 dy[] = { 1, -1, 0, 0 };

    for (int32 i = 0; i < 4; ++i)
    {
        int32 nx = x + dx[i];
        int32 ny = y + dy[i];

        if (IsCorridorTile(nx, ny))
        {
            Count++;
        }
    }

    return Count;
}

int32 UDungeonGraphAnalyzer::FindNodeAtPosition(const FIntVector& Pos) const
{
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (Nodes[i].NodeType == EGraphNodeType::Room)
        {
            // 방 노드는 영역 체크
            const FBox2D& Bounds = Nodes[i].Bounds;
            if (Pos.X >= Bounds.Min.X && Pos.X < Bounds.Max.X &&
                Pos.Y >= Bounds.Min.Y && Pos.Y < Bounds.Max.Y)
            {
                return i;
            }
        }
        else
        {
            // 복도 노드는 정확한 위치 체크
            if (Nodes[i].Position == Pos)
            {
                return i;
            }
        }
    }
    return -1;
}

int32 UDungeonGraphAnalyzer::FindRoomNodeContaining(const FIntVector& Pos) const
{
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (Nodes[i].NodeType == EGraphNodeType::Room)
        {
            // 방 중심 좌표와 일치하는지 체크
            if (Nodes[i].Position == Pos)
            {
                return i;
            }
        }
    }
    return -1;
}

void UDungeonGraphAnalyzer::DFSComponent(int32 NodeIndex, TArray<bool>& Visited)
{
    if (NodeIndex < 0 || NodeIndex >= Nodes.Num() || Visited[NodeIndex])
        return;

    Visited[NodeIndex] = true;

    for (int32 ConnectedNode : Nodes[NodeIndex].ConnectedNodes)
    {
        DFSComponent(ConnectedNode, Visited);
    }
}

int32 UDungeonGraphAnalyzer::CountConnectedComponents()
{
    if (Nodes.Num() == 0) return 0;

    TArray<bool> Visited;
    Visited.SetNum(Nodes.Num());
    for (int32 i = 0; i < Visited.Num(); ++i)
    {
        Visited[i] = false;
    }

    int32 ComponentCount = 0;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (!Visited[i])
        {
            DFSComponent(i, Visited);
            ComponentCount++;
        }
    }

    return ComponentCount;
}

void UDungeonGraphAnalyzer::DrawDebugVisualization(UWorld* World, float Duration)
{
    if (!World) return;

    // 노드 그리기
    for (const FDungeonGraphNode& Node : Nodes)
    {
        FVector Location = FVector(Node.Position.X * WorkingTileSize,
            Node.Position.Y * WorkingTileSize,
            100.0f);
        FColor Color;
        float Radius;

        switch (Node.NodeType)
        {
        case EGraphNodeType::Room:
            Color = FColor::Blue;
            Radius = 150.0f;
            break;
        case EGraphNodeType::DeadEnd:
            Color = FColor::Red;
            Radius = 50.0f;
            break;
        case EGraphNodeType::Junction:
            Color = FColor::Yellow;
            Radius = 75.0f;
            break;
        case EGraphNodeType::CrossRoad:
            Color = FColor::Green;
            Radius = 100.0f;
            break;
        default:
            Color = FColor::White;
            Radius = 50.0f;
        }

        DrawDebugSphere(World, Location, Radius, 12, Color, true, Duration, 0, 5.0f);

        // 노드 ID 표시
        DrawDebugString(World, Location + FVector(0, 0, Radius + 20),
            FString::Printf(TEXT("N%d"), Node.NodeId),
            nullptr, Color, Duration, true, 1.5f);
    }

    // 간선 그리기
    for (const FDungeonGraphEdge& Edge : Edges)
    {
        if (Edge.Path.Num() > 1)
        {
            // 경로를 따라 선 그리기
            for (int32 i = 0; i < Edge.Path.Num() - 1; ++i)
            {
                FVector Start = FVector(Edge.Path[i].X * WorkingTileSize,
                    Edge.Path[i].Y * WorkingTileSize,
                    150.0f);
                FVector End = FVector(Edge.Path[i + 1].X * WorkingTileSize,
                    Edge.Path[i + 1].Y * WorkingTileSize,
                    150.0f);

                DrawDebugLine(World, Start, End, FColor::Magenta, true, Duration, 0, 8.0f);
            }

            // 간선 중간점에 길이 표시
            if (Edge.Path.Num() > 0)
            {
                int32 MidIndex = Edge.Path.Num() / 2;
                FVector MidPoint = FVector(Edge.Path[MidIndex].X * WorkingTileSize,
                    Edge.Path[MidIndex].Y * WorkingTileSize,
                    200.0f);

                DrawDebugString(World, MidPoint,
                    FString::Printf(TEXT("L:%d"), (int32)Edge.Length),
                    nullptr, FColor::Orange, Duration, true, 1.0f);
            }
        }
    }

    // 통계 정보 표시 (화면 좌상단)
    FVector ScreenLocation(100, 100, 500);
    DrawDebugString(World, ScreenLocation,
        FString::Printf(TEXT("Graph Stats:\nNodes: %d\nEdges: %d\nCyclomatic: %d\nComponents: %d"),
            Analysis.NodeCount, Analysis.EdgeCount,
            Analysis.CyclomaticComplexity, Analysis.ConnectedComponents),
        nullptr, FColor::White, Duration, true, 2.0f);
}

void UDungeonGraphAnalyzer::PrintStatistics()
{
    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("========== Dungeon Graph Statistics =========="));
    UE_LOG(LogTemp, Warning, TEXT(""));

    UE_LOG(LogTemp, Warning, TEXT("=== Node Breakdown ==="));
    UE_LOG(LogTemp, Warning, TEXT("Total Nodes: %d"), Analysis.NodeCount);
    UE_LOG(LogTemp, Warning, TEXT("- Rooms: %d"), Analysis.RoomCount);
    UE_LOG(LogTemp, Warning, TEXT("- Dead Ends: %d"), Analysis.DeadEndCount);
    UE_LOG(LogTemp, Warning, TEXT("- T-Junctions: %d"), Analysis.JunctionCount);
    UE_LOG(LogTemp, Warning, TEXT("- CrossRoads: %d"), Analysis.CrossRoadCount);
    UE_LOG(LogTemp, Warning, TEXT(""));

    UE_LOG(LogTemp, Warning, TEXT("=== Graph Metrics ==="));
    UE_LOG(LogTemp, Warning, TEXT("Edges: %d"), Analysis.EdgeCount);
    UE_LOG(LogTemp, Warning, TEXT("Connected Components: %d"), Analysis.ConnectedComponents);
    UE_LOG(LogTemp, Warning, TEXT("Average Node Degree: %.2f"), Analysis.AverageNodeDegree);
    UE_LOG(LogTemp, Warning, TEXT("Average Path Length: %.2f tiles"), Analysis.AveragePathLength);
    UE_LOG(LogTemp, Warning, TEXT(""));

    UE_LOG(LogTemp, Warning, TEXT("=== Cyclomatic Complexity ==="));
    UE_LOG(LogTemp, Warning, TEXT("V(G) = E - N + P = %d - %d + %d = %d"),
        Analysis.EdgeCount, Analysis.NodeCount,
        Analysis.ConnectedComponents, Analysis.CyclomaticComplexity);

    // 복잡도 해석
    if (Analysis.CyclomaticComplexity <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Tree-like (No cycles)"));
    }
    else if (Analysis.CyclomaticComplexity <= 3)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Simple with few loops"));
    }
    else if (Analysis.CyclomaticComplexity <= 7)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Moderate complexity"));
    }
    else if (Analysis.CyclomaticComplexity <= 15)
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Complex maze"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Structure: Highly complex labyrinth"));
    }

    // 연결성 분석
    if (Analysis.ConnectedComponents == 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: Fully connected dungeon"));
    }
    else if (Analysis.ConnectedComponents == Analysis.NodeCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: No connections (isolated nodes)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Connectivity: %d separate regions"), Analysis.ConnectedComponents);
        UE_LOG(LogTemp, Warning, TEXT("WARNING: Dungeon is not fully connected!"));
    }

    // 간선 상세 정보
    if (Edges.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT(""));
        UE_LOG(LogTemp, Warning, TEXT("=== Edge Details ==="));
        for (int32 i = 0; i < FMath::Min(5, Edges.Num()); ++i)
        {
            const FDungeonGraphEdge& Edge = Edges[i];
            UE_LOG(LogTemp, Warning, TEXT("Edge %d: Node %d <-> Node %d (Length: %d)"),
                i, Edge.StartNodeId, Edge.EndNodeId, (int32)Edge.Length);
        }
        if (Edges.Num() > 5)
        {
            UE_LOG(LogTemp, Warning, TEXT("... and %d more edges"), Edges.Num() - 5);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=============================================="));
}


// 특정 방향으로 복도를 따라가며 다음 노드 찾기
int32 UDungeonGraphAnalyzer::TraceCorridorPathInDirection(
    const FIntVector& StartPos,
    const FIntVector& FirstStep,
    int32 StartNodeId,
    TArray<FIntVector>& OutPath)
{
    OutPath.Reset();

    TSet<FIntVector> Visited;
    FIntVector Current = FirstStep;
    FIntVector Previous = StartPos;

    // 시작 노드가 방인 경우, 방 영역을 Visited에 추가
    if (Nodes[StartNodeId].NodeType == EGraphNodeType::Room)
    {
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[StartNodeId].RoomId)
            {
                for (int32 x = Room.Min.X; x < Room.Max.X; ++x)
                {
                    for (int32 y = Room.Min.Y; y < Room.Max.Y; ++y)
                    {
                        Visited.Add(FIntVector(x, y, 0));
                    }
                }
                break;
            }
        }
    }

    OutPath.Add(StartPos);
    OutPath.Add(Current);
    Visited.Add(StartPos);
    Visited.Add(Current);

    // 복도를 따라가며 추적
    const int32 MaxSteps = 200; // 무한 루프 방지
    for (int32 Step = 0; Step < MaxSteps; ++Step)
    {
        // 현재 위치가 다른 노드인지 확인
        for (int32 j = 0; j < Nodes.Num(); ++j)
        {
            if (j == StartNodeId) continue;

            if (IsPositionAtNode(Current, j))
            {
                return j; // 연결된 노드 ID 반환
            }
        }

        // 다음 스텝 찾기
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        bool FoundNext = false;
        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // 범위 체크
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // 이미 방문했으면 스킵
            if (Visited.Contains(Next))
                continue;

            // 역방향으로 가지 않도록
            if (Next == Previous)
                continue;

            // 복도 타일인지 확인
            if (IsCorridorTile(Next))
            {
                Previous = Current;
                Current = Next;
                OutPath.Add(Current);
                Visited.Add(Current);
                FoundNext = true;
                break;
            }
        }

        if (!FoundNext)
            break; // 막다른 길이거나 더 이상 진행 불가
    }

    return -1; // 연결된 노드를 찾지 못함
}

// 위치가 특정 노드에 있는지 확인
bool UDungeonGraphAnalyzer::IsPositionAtNode(const FIntVector& Pos, int32 NodeId) const
{
    if (NodeId < 0 || NodeId >= Nodes.Num())
        return false;

    if (Nodes[NodeId].NodeType == EGraphNodeType::Room)
    {
        // 방의 경우 가장자리 체크
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[NodeId].RoomId)
            {
                // 방 바로 인접 타일인지 확인
                if ((Pos.X >= Room.Min.X - 1 && Pos.X <= Room.Max.X &&
                    Pos.Y >= Room.Min.Y - 1 && Pos.Y <= Room.Max.Y) &&
                    !(Pos.X >= Room.Min.X && Pos.X < Room.Max.X &&
                        Pos.Y >= Room.Min.Y && Pos.Y < Room.Max.Y))
                {
                    return true;
                }
                break;
            }
        }
    }
    else
    {
        // 복도 노드는 정확한 위치 매칭
        if (Nodes[NodeId].Position == Pos)
        {
            return true;
        }
    }

    return false;
}

// 유효한 복도 이동인지 확인
bool UDungeonGraphAnalyzer::IsValidCorridorStep(const FIntVector& Pos, int32 AvoidNodeId) const
{
    // 범위 체크
    if (Pos.X < 0 || Pos.X >= MapSize.X || Pos.Y < 0 || Pos.Y >= MapSize.Y)
        return false;

    // 복도 타일인지 확인
    if (!IsCorridorTile(Pos))
        return false;

    // 시작 노드의 방 영역 안이면 제외
    if (AvoidNodeId >= 0 && Nodes[AvoidNodeId].NodeType == EGraphNodeType::Room)
    {
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[AvoidNodeId].RoomId)
            {
                if (Pos.X >= Room.Min.X && Pos.X < Room.Max.X &&
                    Pos.Y >= Room.Min.Y && Pos.Y < Room.Max.Y)
                {
                    return false;
                }
                break;
            }
        }
    }

    return true;
}

// 두 경로가 유사한지 확인 (대부분 겹치는 경로인지)
bool UDungeonGraphAnalyzer::ArePathsSimilar(
    const TArray<FIntVector>& Path1,
    const TArray<FIntVector>& Path2) const
{
    // 경로 길이가 너무 다르면 다른 경로
    if (FMath::Abs(Path1.Num() - Path2.Num()) > 5)
        return false;

    // 두 경로의 타일 집합 생성
    TSet<FIntVector> Set1(Path1);
    TSet<FIntVector> Set2(Path2);

    // 교집합 크기 계산
    TSet<FIntVector> Intersection = Set1.Intersect(Set2);

    // 70% 이상 겹치면 유사한 경로로 간주
    float OverlapRatio = (float)Intersection.Num() /
        (float)FMath::Min(Set1.Num(), Set2.Num());

    return OverlapRatio > 0.7f;
}