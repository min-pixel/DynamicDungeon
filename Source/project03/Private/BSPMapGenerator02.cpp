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


    // 타일 스폰
    SpawnTiles();

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed. Created %d rooms"), LeafNodes.Num());

    // 맵 통계 분석 및 출력
    if (bShowStatistics)
    {
        PrintMapStatistics();
    }

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

void ABSPMapGenerator02::ConnectRooms()
{
    // 인접한 리프 노드들 연결
    for (int32 i = 0; i < LeafNodes.Num() - 1; ++i)
    {
        // 가장 가까운 방 찾기
        int32 NearestIndex = -1;
        float MinDistance = FLT_MAX;

        FIntVector CenterA = GetRoomCenter(LeafNodes[i]);

        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
        {
            FIntVector CenterB = GetRoomCenter(LeafNodes[j]);
            float Distance = FVector::Dist(FVector(CenterA), FVector(CenterB));

            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                NearestIndex = j;
            }
        }

        if (NearestIndex != -1)
        {
            CreateCorridor(LeafNodes[i], LeafNodes[NearestIndex]);
        }
    }
}

void ABSPMapGenerator02::CreateCorridor(TSharedPtr<FBSPNode02> NodeA, TSharedPtr<FBSPNode02> NodeB)
{
    if (!NodeA->bHasRoom || !NodeB->bHasRoom) return;

    FIntVector CenterA = GetRoomCenter(NodeA);
    FIntVector CenterB = GetRoomCenter(NodeB);

    CreateLShapedCorridor(CenterA, CenterB);
}

void ABSPMapGenerator02::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
{
    // 수평 먼저, 수직 나중에
    if (RandomStream.FRand() > 0.5f)
    {
        // 수평 복도
        int32 StartX = FMath::Min(Start.X, End.X);
        int32 EndX = FMath::Max(Start.X, End.X);

        for (int32 x = StartX; x <= EndX; ++x)
        {
            if (x >= 0 && x < MapSize.X && Start.Y >= 0 && Start.Y < MapSize.Y)
            {
                if (TileMap[x][Start.Y] == ETileType02::Empty)
                {
                    TileMap[x][Start.Y] = ETileType02::Corridor;
                }
            }
        }

        // 수직 복도
        int32 StartY = FMath::Min(Start.Y, End.Y);
        int32 EndY = FMath::Max(Start.Y, End.Y);

        for (int32 y = StartY; y <= EndY; ++y)
        {
            if (End.X >= 0 && End.X < MapSize.X && y >= 0 && y < MapSize.Y)
            {
                if (TileMap[End.X][y] == ETileType02::Empty)
                {
                    TileMap[End.X][y] = ETileType02::Corridor;
                }
            }
        }
    }
    else
    {
        // 수직 먼저, 수평 나중에
        int32 StartY = FMath::Min(Start.Y, End.Y);
        int32 EndY = FMath::Max(Start.Y, End.Y);

        for (int32 y = StartY; y <= EndY; ++y)
        {
            if (Start.X >= 0 && Start.X < MapSize.X && y >= 0 && y < MapSize.Y)
            {
                if (TileMap[Start.X][y] == ETileType02::Empty)
                {
                    TileMap[Start.X][y] = ETileType02::Corridor;
                }
            }
        }

        int32 StartX = FMath::Min(Start.X, End.X);
        int32 EndX = FMath::Max(Start.X, End.X);

        for (int32 x = StartX; x <= EndX; ++x)
        {
            if (x >= 0 && x < MapSize.X && End.Y >= 0 && End.Y < MapSize.Y)
            {
                if (TileMap[x][End.Y] == ETileType02::Empty)
                {
                    TileMap[x][End.Y] = ETileType02::Corridor;
                }
            }
        }
    }
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
            bool bHasNorth = (y < MapSize.Y - 1 && TileMap[x][y + 1] != ETileType02::Empty);
            bool bHasSouth = (y > 0 && TileMap[x][y - 1] != ETileType02::Empty);
            bool bHasEast = (x < MapSize.X - 1 && TileMap[x + 1][y] != ETileType02::Empty);
            bool bHasWest = (x > 0 && TileMap[x - 1][y] != ETileType02::Empty);

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

void ABSPMapGenerator02::CreateExtraConnections()
{
    if (LeafNodes.Num() < 3) return;  // 방이 3개 미만이면 추가 연결 불필요

    int32 ConnectionsAdded = 0;
    int32 TargetConnections = RandomStream.RandRange(MinExtraConnections, MaxExtraConnections);

    UE_LOG(LogTemp, Warning, TEXT("Creating extra connections. Target: %d"), TargetConnections);

    // 모든 방 쌍에 대해 검토
    TArray<TPair<int32, int32>> PotentialConnections;

    for (int32 i = 0; i < LeafNodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < LeafNodes.Num(); ++j)
        {
            // 거리가 적절한 방들만 후보로 추가
            if (IsValidConnectionDistance(i, j))
            {
                PotentialConnections.Add(TPair<int32, int32>(i, j));
            }
        }
    }

    // 후보들을 섞어서 랜덤하게 선택
    for (int32 i = PotentialConnections.Num() - 1; i > 0; --i)
    {
        int32 j = RandomStream.RandRange(0, i);
        PotentialConnections.Swap(i, j);
    }

    // 추가 연결 생성
    for (const auto& Connection : PotentialConnections)
    {
        if (ConnectionsAdded >= TargetConnections)
        {
            break;
        }

        // 확률적으로 연결 생성
        if (RandomStream.FRand() < ExtraConnectionChance)
        {
            TSharedPtr<FBSPNode02> NodeA = LeafNodes[Connection.Key];
            TSharedPtr<FBSPNode02> NodeB = LeafNodes[Connection.Value];

            FIntVector CenterA = GetRoomCenter(NodeA);
            FIntVector CenterB = GetRoomCenter(NodeB);

            // 이미 복도가 존재하는지 확인
            if (!CorridorExists(CenterA, CenterB))
            {
                // 다양한 복도 스타일 중 랜덤 선택
                float CorridorStyle = RandomStream.FRand();

                if (CorridorStyle < 0.5f)
                {
                    // L자 복도 (기존 스타일)
                    CreateLShapedCorridor(CenterA, CenterB);
                }
                else if (CorridorStyle < 0.8f)
                {
                    // 역 L자 복도 (반대 방향)
                    CreateLShapedCorridor(CenterB, CenterA);
                }
                else
                {
                    // 지그재그 복도 (더 복잡한 경로)
                    CreateZigzagCorridor(CenterA, CenterB);
                }

                ConnectionsAdded++;

                UE_LOG(LogTemp, Verbose, TEXT("Added extra connection %d between room %d and %d"),
                    ConnectionsAdded, Connection.Key, Connection.Value);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Created %d extra connections for maze complexity"), ConnectionsAdded);
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
}