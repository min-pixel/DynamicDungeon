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

    // 타일 스폰
    SpawnTiles();

    UE_LOG(LogTemp, Warning, TEXT("BSP Map Generation Completed. Created %d rooms"), LeafNodes.Num());
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

// 단일 복도 타일 스폰 함수 (새로 추가된 연결 복도용)
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