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

    // Ÿ�� ����
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

void ABSPMapGenerator02::ConnectRooms()
{
    // ������ ���� ���� ����
    for (int32 i = 0; i < LeafNodes.Num() - 1; ++i)
    {
        // ���� ����� �� ã��
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
    // ���� ����, ���� ���߿�
    if (RandomStream.FRand() > 0.5f)
    {
        // ���� ����
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

        // ���� ����
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
        // ���� ����, ���� ���߿�
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
            bool bHasNorth = (y < MapSize.Y - 1 && TileMap[x][y + 1] != ETileType02::Empty);
            bool bHasSouth = (y > 0 && TileMap[x][y - 1] != ETileType02::Empty);
            bool bHasEast = (x < MapSize.X - 1 && TileMap[x + 1][y] != ETileType02::Empty);
            bool bHasWest = (x > 0 && TileMap[x - 1][y] != ETileType02::Empty);

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

// ���� ���� Ÿ�� ���� �Լ� (���� �߰��� ���� ������)
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