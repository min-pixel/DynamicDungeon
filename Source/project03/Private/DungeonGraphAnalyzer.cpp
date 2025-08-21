#include "DungeonGraphAnalyzer.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UDungeonGraphAnalyzer::AnalyzeDungeon(
    const TArray<TArray<EDungeonTileType>>& TileMap,
    const TArray<FRoomInfo>& RoomInfos,
    float TileSize)
{
    // �ʱ�ȭ
    ClearGraph();

    // ������ ����
    WorkingTileMap = TileMap;
    WorkingRoomInfos = RoomInfos;
    WorkingTileSize = TileSize;

    if (WorkingTileMap.Num() == 0) return;

    MapSize = FIntVector(WorkingTileMap.Num(),
        WorkingTileMap[0].Num(), 1);

    UE_LOG(LogTemp, Warning, TEXT("=== Starting Dungeon Graph Analysis ==="));
    UE_LOG(LogTemp, Warning, TEXT("Map Size: %dx%d"), MapSize.X, MapSize.Y);
    UE_LOG(LogTemp, Warning, TEXT("Room Count: %d"), WorkingRoomInfos.Num());

    // Step 1: ���� Ÿ�� �м� (���ٸ���, ������ ã��)
    AnalyzeCorridorTypes();

    // Step 2: ��� ����
    ExtractNodes();

    // Step 3: ���� ����
    ExtractEdges();

    // Step 4: �׷��� �м�
    AnalyzeGraph();

    UE_LOG(LogTemp, Warning, TEXT("=== Graph Analysis Complete ==="));
    UE_LOG(LogTemp, Warning, TEXT("Nodes: %d, Edges: %d"), Nodes.Num(), Edges.Num());
}

void UDungeonGraphAnalyzer::AnalyzeDungeonBP(
    const FDungeonTileMap& TileMapWrapper,
    const TArray<FRoomInfo>& RoomInfos,
    float TileSize)
{
    // ���ۿ��� ���� Ÿ�ϸ� �����Ͽ� �м�
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

    // ��� ���� Ÿ���� ��ȸ�ϸ� Ÿ�� ����
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            if (!IsCorridorTile(x, y)) continue;

            // �̹� Ư�� Ÿ���̸� �ǳʶٱ�
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
    // ���� ���� ���� ����
    int32 Connections = CountCorridorConnections(x, y);

    // �� ���� ���� üũ
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

    // BridgeCorridor(�� �� ���� ����)�� �׻� �Ϲ� ������ ó��
    if (WorkingTileMap[x][y] == EDungeonTileType::BridgeCorridor)
        return EDungeonTileType::Corridor;

    // �濡 �����ϰ� ������ 1�� ���ϸ� �Ϲ� ���� (�� �����)
    if (AdjacentToRoom && Connections <= 1)
        return EDungeonTileType::Corridor;

    // ������ ���� üũ - ���� ���� ���� ó��
    if (IsPartOfWideCorrridor(x, y))
        return EDungeonTileType::Corridor;

    // �⺻ ��Ģ
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
    // ���� �̿���
    const bool N = (y < MapSize.Y - 1) && IsCorridorTile(x, y + 1);
    const bool S = (y > 0) && IsCorridorTile(x, y - 1);
    const bool E = (x < MapSize.X - 1) && IsCorridorTile(x + 1, y);
    const bool W = (x > 0) && IsCorridorTile(x - 1, y);

    // ���� ����(E&W) + ��/�Ʒ� ���ʸ� ���� ���
    if (E && W && (N ^ S))
    {
        const int ny = N ? (y + 1) : (y - 1);
        // ��/�Ʒ� �̿��� ���� ���� �б� �ƴ�
        if (ny >= 0 && ny < MapSize.Y &&
            IsCorridorTile(x - 1, ny) && IsCorridorTile(x + 1, ny))
        {
            return true;
        }
    }

    // ���� ����(N&S) + ��/�� ���ʸ� ���� ���
    if (N && S && (E ^ W))
    {
        const int nx = E ? (x + 1) : (x - 1);
        // ��/�� �̿��� ���� ���� �б� �ƴ�
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

    // 1. �� ��� �߰�
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

    // 2. ���� Ư�� ��� �߰� (���ٸ���, ������)
    TSet<FIntVector> ProcessedPositions;

    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            FIntVector Pos(x, y, 0);

            // �̹� ó���� ��ġ�� �ǳʶٱ�
            if (ProcessedPositions.Contains(Pos)) continue;

            EDungeonTileType Type = WorkingTileMap[x][y];

            if (Type == EDungeonTileType::DeadEnd ||
                Type == EDungeonTileType::Junction ||
                Type == EDungeonTileType::CrossRoad)
            {
                // �� ���� ���ΰ� �ƴ��� Ȯ��
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
//    // ��� ��� �ֿ� ���� ���� ��� ã��
//    for (int32 i = 0; i < Nodes.Num(); ++i)
//    {
//        for (int32 j = i + 1; j < Nodes.Num(); ++j)
//        {
//            // �̹� ó���� ���̸� �ǳʶٱ�
//            TPair<int32, int32> Pair(i, j);
//            if (ProcessedPairs.Contains(Pair)) continue;
//
//            // ��� �� ���� ��� ã��
//            TArray<FIntVector> Path;
//
//            // �ٸ� ������ ���ؼ� ��� ã��
//            TSet<FIntVector> AvoidNodes;
//            for (int32 k = 0; k < Nodes.Num(); ++k)
//            {
//                if (k != i && k != j)
//                {
//                    // �� ���� ��ü ������ ���ؾ� ��
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
//                        // ���� ���� �ش� ��ġ�� ����
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
//                // ��� ���� ���� ������Ʈ
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
//    // �� ��忡�� �����Ͽ� ������ ��常 ã��
//    for (int32 i = 0; i < Nodes.Num(); ++i)
//    {
//        TArray<FIntVector> StartPositions;
//
//        if (Nodes[i].NodeType == EGraphNodeType::Room)
//        {
//            // ���� ��� �����ڸ� ���� Ÿ�� ã��
//            for (const FRoomInfo& Room : WorkingRoomInfos)
//            {
//                if (Room.RoomId == Nodes[i].RoomId)
//                {
//                    // �� �ֺ� ���� Ÿ�� ����
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
//            // ���� ���� �ڱ� ��ġ���� ����
//            StartPositions.Add(Nodes[i].Position);
//        }
//
//        // �� ���������� ������ ���󰡸� ù ��° �������� ����
//        for (const FIntVector& StartPos : StartPositions)
//        {
//            TArray<FIntVector> Path;
//            int32 ConnectedNodeId = TraceCorridorToNextNode(StartPos, i, Path);
//
//            if (ConnectedNodeId >= 0 && ConnectedNodeId != i)
//            {
//                // �ߺ� üũ
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
//                    // ��� ���� ���� ������Ʈ
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

    // ��� �ֺ��� �������� ��ε��� ���� (�ߺ� ���� ���)
    TMap<TPair<int32, int32>, TArray<FDungeonGraphEdge>> NodePairEdges;

    // �� ��忡�� �����Ͽ� ��� ����� ���� Ž��
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        TArray<FIntVector> StartPositions;
        TSet<FIntVector> ProcessedStartPositions; // �̹� ó���� ������ ����

        if (Nodes[i].NodeType == EGraphNodeType::Room)
        {
            // ���� ��� �����ڸ� ���� Ÿ�� ã��
            for (const FRoomInfo& Room : WorkingRoomInfos)
            {
                if (Room.RoomId == Nodes[i].RoomId)
                {
                    // �� �ֺ� ���� Ÿ�� ����
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
            // ���� ���� �ڱ� ��ġ���� ����
            StartPositions.Add(Nodes[i].Position);
        }

        // �� ���������� ������ ���󰡸� ����� ��� ã��
        for (const FIntVector& StartPos : StartPositions)
        {
            // �� ���������� ���� �������� �� �� �����Ƿ� �� ���� Ž��
            const int32 dx[] = { 0, 0, 1, -1 };
            const int32 dy[] = { 1, -1, 0, 0 };

            for (int32 dir = 0; dir < 4; ++dir)
            {
                FIntVector FirstStep(StartPos.X + dx[dir], StartPos.Y + dy[dir], 0);

                // ��ȿ�� ���� Ÿ������ Ȯ��
                if (!IsValidCorridorStep(FirstStep, i))
                    continue;

                // �� �������� ��� ����
                TArray<FIntVector> Path;
                int32 ConnectedNodeId = TraceCorridorPathInDirection(
                    StartPos, FirstStep, i, Path);

                if (ConnectedNodeId >= 0 && ConnectedNodeId != i)
                {
                    // ��ȿ�� ��θ� ã��
                    FDungeonGraphEdge Edge;
                    Edge.StartNodeId = i;
                    Edge.EndNodeId = ConnectedNodeId;
                    Edge.Path = Path;
                    Edge.Length = Path.Num();

                    // ��� ���� Ű ���� (���� ID�� ����)
                    TPair<int32, int32> NodePair(
                        FMath::Min(i, ConnectedNodeId),
                        FMath::Max(i, ConnectedNodeId));

                    // �� ��ΰ� ���� ��ο� ����� �ٸ��� Ȯ��
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
                        // ���ο� �������� ��� �߰�
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

    // ��� ã�� ������ ���� Edges �迭�� �߰�
    for (const auto& Pair : NodePairEdges)
    {
        for (const FDungeonGraphEdge& Edge : Pair.Value)
        {
            Edges.Add(Edge);

            // ��� ���� ���� ������Ʈ
            Nodes[Edge.StartNodeId].ConnectedNodes.AddUnique(Edge.EndNodeId);
            Nodes[Edge.EndNodeId].ConnectedNodes.AddUnique(Edge.StartNodeId);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Extracted %d edges total"), Edges.Num());

    // �ߺ� ��� ���� ���
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

// ������ ���󰡸� ���� ��� ã��
int32 UDungeonGraphAnalyzer::TraceCorridorToNextNode(
    const FIntVector& StartPos,
    int32 StartNodeId,
    TArray<FIntVector>& OutPath)
{
    OutPath.Reset();

    TSet<FIntVector> Visited;
    FIntVector Current = StartPos;
    FIntVector Previous = StartPos;

    // ���� ��尡 ���� ���, �� ���� ��ŵ
    if (Nodes[StartNodeId].NodeType == EGraphNodeType::Room)
    {
        // �濡�� ������ ���� ����
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[StartNodeId].RoomId)
            {
                // �� ������ Visited�� �߰� (������ ����)
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

    // ������ ���󰡸� ����
    const int32 MaxSteps = 100;
    for (int32 Step = 0; Step < MaxSteps; ++Step)
    {
        // 4���� Ȯ��
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        bool FoundNext = false;
        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // ���� üũ
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // �̹� �湮������ ��ŵ
            if (Visited.Contains(Next))
                continue;

            // �ٸ� ��忡 �����ߴ��� Ȯ��
            for (int32 j = 0; j < Nodes.Num(); ++j)
            {
                if (j == StartNodeId) continue;

                bool IsAtNode = false;

                if (Nodes[j].NodeType == EGraphNodeType::Room)
                {
                    // �� �����ڸ� üũ
                    for (const FRoomInfo& Room : WorkingRoomInfos)
                    {
                        if (Room.RoomId == Nodes[j].RoomId)
                        {
                            // �� �ٷ� ���� Ÿ������ Ȯ��
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
                    // ���� ���� ��Ȯ�� ��ġ ��Ī
                    if (Nodes[j].Position == Next)
                    {
                        IsAtNode = true;
                    }
                }

                if (IsAtNode)
                {
                    OutPath.Add(Next);
                    return j;  // ����� ��� ID ��ȯ
                }
            }

            // ���� Ÿ���̸� ��� ����
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
            break;  // ���ٸ� ���̰ų� �� �̻� ���� �Ұ�
    }

    return -1;  // ����� ��带 ã�� ����
}

bool UDungeonGraphAnalyzer::FindCorridorPath(
    const FIntVector& Start,
    const FIntVector& End,
    TArray<FIntVector>& OutPath,
    const TSet<FIntVector>& AvoidNodes)
{
    OutPath.Reset();

    // BFS�� �ִ� ��� ã��
    TQueue<FIntVector> Queue;
    TSet<FIntVector> Visited;
    TMap<FIntVector, FIntVector> Parent;

    // ������ ã�� (���� ��� �����ڸ����� ����)
    TArray<FIntVector> StartPoints;
    if (FindRoomNodeContaining(Start) >= 0)
    {
        // �濡�� �����ϴ� ���, �� �����ڸ��� ���� Ÿ�� ã��
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.Center == Start)
            {
                // �� �����ڸ� üũ
                for (int32 x = Room.Min.X - 1; x <= Room.Max.X; ++x)
                {
                    for (int32 y = Room.Min.Y - 1; y <= Room.Max.Y; ++y)
                    {
                        // �� ��� �ٷ� ���� ���� Ÿ��
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

    // ������ ����
    TSet<FIntVector> EndPoints;
    if (FindRoomNodeContaining(End) >= 0)
    {
        // ���� �������� ���, �� �����ڸ� ���� Ÿ�ϵ��� ��ǥ��
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

    // BFS ����
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

        // ������ ���� üũ
        if (EndPoints.Contains(Current))
        {
            bFound = true;
            FoundEnd = Current;
            break;
        }

        // 4���� Ž��
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // ���� üũ
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // �̹� �湮������ �ǳʶٱ�
            if (Visited.Contains(Next))
                continue;

            // ���ؾ� �� ���� �ǳʶٱ� (������ ����)
            if (AvoidNodes.Contains(Next) && !EndPoints.Contains(Next))
                continue;

            // ���� Ÿ���� �ƴϸ� �ǳʶٱ� (������ ����)
            if (!IsCorridorTile(Next) && !EndPoints.Contains(Next))
                continue;

            Visited.Add(Next);
            Parent.Add(Next, Current);
            Queue.Enqueue(Next);
        }
    }

    // ��� ����
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

        // �������� ����
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

    // ��� �� ī��Ʈ
    Analysis.NodeCount = Nodes.Num();
    Analysis.EdgeCount = Edges.Num();

    // ��� Ÿ�Ժ� ī��Ʈ
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

    // ����� ������Ʈ ��
    Analysis.ConnectedComponents = CountConnectedComponents();

    // ��ȯ ���⵵ (Cyclomatic Complexity)
    // V(G) = E - N + P (������ - ���� + ���Ἲ�м�)
    Analysis.CyclomaticComplexity = Analysis.EdgeCount - Analysis.NodeCount + Analysis.ConnectedComponents;

    // ��� ��� ����
    if (Analysis.NodeCount > 0)
    {
        int32 TotalDegree = 0;
        for (const FDungeonGraphNode& Node : Nodes)
        {
            TotalDegree += Node.ConnectedNodes.Num();
        }
        Analysis.AverageNodeDegree = (float)TotalDegree / (float)Analysis.NodeCount;
    }

    // ��� ��� ����
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

    // 4���� üũ
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
            // �� ���� ���� üũ
            const FBox2D& Bounds = Nodes[i].Bounds;
            if (Pos.X >= Bounds.Min.X && Pos.X < Bounds.Max.X &&
                Pos.Y >= Bounds.Min.Y && Pos.Y < Bounds.Max.Y)
            {
                return i;
            }
        }
        else
        {
            // ���� ���� ��Ȯ�� ��ġ üũ
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
            // �� �߽� ��ǥ�� ��ġ�ϴ��� üũ
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

    // ��� �׸���
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

        // ��� ID ǥ��
        DrawDebugString(World, Location + FVector(0, 0, Radius + 20),
            FString::Printf(TEXT("N%d"), Node.NodeId),
            nullptr, Color, Duration, true, 1.5f);
    }

    // ���� �׸���
    for (const FDungeonGraphEdge& Edge : Edges)
    {
        if (Edge.Path.Num() > 1)
        {
            // ��θ� ���� �� �׸���
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

            // ���� �߰����� ���� ǥ��
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

    // ��� ���� ǥ�� (ȭ�� �»��)
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

    // ���⵵ �ؼ�
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

    // ���Ἲ �м�
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

    // ���� �� ����
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


// Ư�� �������� ������ ���󰡸� ���� ��� ã��
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

    // ���� ��尡 ���� ���, �� ������ Visited�� �߰�
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

    // ������ ���󰡸� ����
    const int32 MaxSteps = 200; // ���� ���� ����
    for (int32 Step = 0; Step < MaxSteps; ++Step)
    {
        // ���� ��ġ�� �ٸ� ������� Ȯ��
        for (int32 j = 0; j < Nodes.Num(); ++j)
        {
            if (j == StartNodeId) continue;

            if (IsPositionAtNode(Current, j))
            {
                return j; // ����� ��� ID ��ȯ
            }
        }

        // ���� ���� ã��
        const int32 dx[] = { 0, 0, 1, -1 };
        const int32 dy[] = { 1, -1, 0, 0 };

        bool FoundNext = false;
        for (int32 i = 0; i < 4; ++i)
        {
            FIntVector Next(Current.X + dx[i], Current.Y + dy[i], 0);

            // ���� üũ
            if (Next.X < 0 || Next.X >= MapSize.X ||
                Next.Y < 0 || Next.Y >= MapSize.Y)
                continue;

            // �̹� �湮������ ��ŵ
            if (Visited.Contains(Next))
                continue;

            // ���������� ���� �ʵ���
            if (Next == Previous)
                continue;

            // ���� Ÿ������ Ȯ��
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
            break; // ���ٸ� ���̰ų� �� �̻� ���� �Ұ�
    }

    return -1; // ����� ��带 ã�� ����
}

// ��ġ�� Ư�� ��忡 �ִ��� Ȯ��
bool UDungeonGraphAnalyzer::IsPositionAtNode(const FIntVector& Pos, int32 NodeId) const
{
    if (NodeId < 0 || NodeId >= Nodes.Num())
        return false;

    if (Nodes[NodeId].NodeType == EGraphNodeType::Room)
    {
        // ���� ��� �����ڸ� üũ
        for (const FRoomInfo& Room : WorkingRoomInfos)
        {
            if (Room.RoomId == Nodes[NodeId].RoomId)
            {
                // �� �ٷ� ���� Ÿ������ Ȯ��
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
        // ���� ���� ��Ȯ�� ��ġ ��Ī
        if (Nodes[NodeId].Position == Pos)
        {
            return true;
        }
    }

    return false;
}

// ��ȿ�� ���� �̵����� Ȯ��
bool UDungeonGraphAnalyzer::IsValidCorridorStep(const FIntVector& Pos, int32 AvoidNodeId) const
{
    // ���� üũ
    if (Pos.X < 0 || Pos.X >= MapSize.X || Pos.Y < 0 || Pos.Y >= MapSize.Y)
        return false;

    // ���� Ÿ������ Ȯ��
    if (!IsCorridorTile(Pos))
        return false;

    // ���� ����� �� ���� ���̸� ����
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

// �� ��ΰ� �������� Ȯ�� (��κ� ��ġ�� �������)
bool UDungeonGraphAnalyzer::ArePathsSimilar(
    const TArray<FIntVector>& Path1,
    const TArray<FIntVector>& Path2) const
{
    // ��� ���̰� �ʹ� �ٸ��� �ٸ� ���
    if (FMath::Abs(Path1.Num() - Path2.Num()) > 5)
        return false;

    // �� ����� Ÿ�� ���� ����
    TSet<FIntVector> Set1(Path1);
    TSet<FIntVector> Set2(Path2);

    // ������ ũ�� ���
    TSet<FIntVector> Intersection = Set1.Intersect(Set2);

    // 70% �̻� ��ġ�� ������ ��η� ����
    float OverlapRatio = (float)Intersection.Num() /
        (float)FMath::Min(Set1.Num(), Set2.Num());

    return OverlapRatio > 0.7f;
}