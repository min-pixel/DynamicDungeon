#include "BSPMapGenerator.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h" 
#include "Components/InstancedStaticMeshComponent.h"

ABSPMapGenerator::ABSPMapGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    // �⺻ Ÿ�� ��� ����
    static ConstructorHelpers::FClassFinder<AActor> RoomNorth(TEXT("/Game/BP/romm"));
    static ConstructorHelpers::FClassFinder<AActor> RoomSouth(TEXT("/Game/BP/rommBack"));
    static ConstructorHelpers::FClassFinder<AActor> RoomWest(TEXT("/Game/BP/rommLEFT"));
    static ConstructorHelpers::FClassFinder<AActor> RoomEast(TEXT("/Game/BP/rommRIGHT"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorH(TEXT("/Game/BP/t01-01"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorV(TEXT("/Game/BP/t01"));
    static ConstructorHelpers::FClassFinder<AActor> CorridorCorner(TEXT("/Game/BP/goalt01"));

    if (RoomNorth.Succeeded()) RoomNorthClass = RoomNorth.Class;
    if (RoomSouth.Succeeded()) RoomSouthClass = RoomSouth.Class;
    if (RoomWest.Succeeded()) RoomWestClass = RoomWest.Class;
    if (RoomEast.Succeeded()) RoomEastClass = RoomEast.Class;
    if (CorridorH.Succeeded()) CorridorHorizontalClass = CorridorH.Class;
    if (CorridorV.Succeeded()) CorridorVerticalClass = CorridorV.Class;
    if (CorridorCorner.Succeeded()) CorridorCornerClass = CorridorCorner.Class;
}

void ABSPMapGenerator::BeginPlay()
{
    Super::BeginPlay();


    GenerateBSPMap();
}

void ABSPMapGenerator::GenerateBSPMap()
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

void ABSPMapGenerator::ClearMap()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), "BSPGenerated", FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }
}

void ABSPMapGenerator::InitializeTileMap()
{
    TileMap.SetNum(MapSize.X);
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        TileMap[x].SetNum(MapSize.Y);
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            TileMap[x][y] = ETileType::Empty;
        }
    }
}

TSharedPtr<FBSPNode> ABSPMapGenerator::CreateBSPTree(const FIntVector& Min, const FIntVector& Max, int32 Depth)
{
    TSharedPtr<FBSPNode> Node = MakeShareable(new FBSPNode(Min, Max));

    // �ִ� ���� ���� �Ǵ� �ּ� ũ�� ����
    if (Depth >= MaxDepth || !SplitNode(Node, Depth))
    {
        Node->bIsLeaf = true;
        return Node;
    }

    return Node;
}

bool ABSPMapGenerator::SplitNode(TSharedPtr<FBSPNode> Node, int32 Depth)
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

void ABSPMapGenerator::CreateRooms()
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
                    TileMap[x][y] = ETileType::Room;
                }
            }
        }
    }
}



void ABSPMapGenerator::ConnectRooms()
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

void ABSPMapGenerator::CreateCorridor(TSharedPtr<FBSPNode> NodeA, TSharedPtr<FBSPNode> NodeB)
{
    if (!NodeA->bHasRoom || !NodeB->bHasRoom) return;

    FIntVector CenterA = GetRoomCenter(NodeA);
    FIntVector CenterB = GetRoomCenter(NodeB);

    CreateLShapedCorridor(CenterA, CenterB);
}

void ABSPMapGenerator::CreateLShapedCorridor(const FIntVector& Start, const FIntVector& End)
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
                if (TileMap[x][Start.Y] == ETileType::Empty)
                {
                    TileMap[x][Start.Y] = ETileType::Corridor;
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
                if (TileMap[End.X][y] == ETileType::Empty)
                {
                    TileMap[End.X][y] = ETileType::Corridor;
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
                if (TileMap[Start.X][y] == ETileType::Empty)
                {
                    TileMap[Start.X][y] = ETileType::Corridor;
                }
            }
        }

        int32 StartX = FMath::Min(Start.X, End.X);
        int32 EndX = FMath::Max(Start.X, End.X);

        for (int32 x = StartX; x <= EndX; ++x)
        {
            if (x >= 0 && x < MapSize.X && End.Y >= 0 && End.Y < MapSize.Y)
            {
                if (TileMap[x][End.Y] == ETileType::Empty)
                {
                    TileMap[x][End.Y] = ETileType::Corridor;
                }
            }
        }
    }
}

//void ABSPMapGenerator::SpawnTiles()
//{
//    UWorld* World = GetWorld();
//    if (!World)
//    {
//        UE_LOG(LogTemp, Error, TEXT("World is null!"));
//        return;
//    }
//
//    int32 SpawnedCount = 0;
//
//    // ���� ����� ó��
//    TSet<FIntVector> ProcessedRoomTiles;
//
//    for (auto& LeafNode : LeafNodes)
//    {
//        if (!LeafNode->bHasRoom) continue;
//
//        // ���� ���� ũ�� ���
//        FIntVector RoomSize = LeafNode->RoomMax - LeafNode->RoomMin;
//        FIntVector RoomCenter = GetRoomCenter(LeafNode);
//
//        // �̹� ó���� �������� Ȯ��
//        bool bAlreadyProcessed = false;
//        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
//        {
//            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
//            {
//                if (ProcessedRoomTiles.Contains(FIntVector(x, y, 0)))
//                {
//                    bAlreadyProcessed = true;
//                    break;
//                }
//            }
//            if (bAlreadyProcessed) break;
//        }
//
//        if (bAlreadyProcessed) continue;
//
//        // �� ���� ��ü�� ó�������� ǥ��
//        for (int32 x = LeafNode->RoomMin.X; x < LeafNode->RoomMax.X; ++x)
//        {
//            for (int32 y = LeafNode->RoomMin.Y; y < LeafNode->RoomMax.Y; ++y)
//            {
//                ProcessedRoomTiles.Add(FIntVector(x, y, 0));
//            }
//        }
//
//        // �� ���� ���� (���� ũ��)
//        FVector SpawnLocation = FVector(RoomCenter.X * TileSize, RoomCenter.Y * TileSize, 0);
//        FRotator SpawnRotation = FRotator::ZeroRotator;
//
//        // �� ���� ���� (���� �� ��� ����)
//        FString DoorDir = GetRoomDoorDirectionForNode(LeafNode);
//        TSubclassOf<AActor> RoomClass = nullptr;
//
//        if (DoorDir == "North" && RoomNorthClass)
//        {
//            RoomClass = RoomNorthClass;
//            SpawnRotation = FRotator(0, 0, 0);
//        }
//        else if (DoorDir == "South" && RoomSouthClass)
//        {
//            RoomClass = RoomSouthClass;
//            SpawnRotation = FRotator(0, 180, 0);
//        }
//        else if (DoorDir == "East" && RoomEastClass)
//        {
//            RoomClass = RoomEastClass;
//            SpawnRotation = FRotator(0, 90, 0);
//        }
//        else if (DoorDir == "West" && RoomWestClass)
//        {
//            RoomClass = RoomWestClass;
//            SpawnRotation = FRotator(0, -90, 0);
//        }
//
//        if (RoomClass)
//        {
//            FActorSpawnParameters SpawnParams;
//            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//
//            AActor* SpawnedRoom = World->SpawnActor<AActor>(RoomClass, SpawnLocation, SpawnRotation, SpawnParams);
//            if (SpawnedRoom)
//            {
//                SpawnedRoom->Tags.Add("BSPGenerated");
//
//                // �� ũ�⿡ �°� ������ ����
//                FVector RoomScale = FVector(
//                    (float)RoomSize.X,  // X ũ��
//                    (float)RoomSize.Y,  // Y ũ��
//                    1.0f                // Z�� �״��
//                );
//                SpawnedRoom->SetActorScale3D(RoomScale);
//
//                SpawnedCount++;
//
//                UE_LOG(LogTemp, Warning, TEXT("Spawned room at (%d, %d) with size (%d x %d), door facing %s"),
//                    RoomCenter.X, RoomCenter.Y, RoomSize.X, RoomSize.Y, *DoorDir);
//            }
//        }
//    }
//
//    // ���� Ÿ�� ����
//    for (int32 x = 0; x < MapSize.X; ++x)
//    {
//        for (int32 y = 0; y < MapSize.Y; ++y)
//        {
//            // �� Ÿ���� �̹� ó�������Ƿ� �ǳʶ�
//            if (TileMap[x][y] != ETileType::Corridor) continue;
//            if (ProcessedRoomTiles.Contains(FIntVector(x, y, 0))) continue;
//
//            FVector SpawnLocation = FVector(x * TileSize, y * TileSize, 0);
//            FRotator SpawnRotation = FRotator::ZeroRotator;
//            TSubclassOf<AActor> TileClass = nullptr;
//
//            // ���� Ÿ�� ���� Ȯ�� (X/Y �ݴ��)
//            bool bHasNorth = (y < MapSize.Y - 1 && TileMap[x][y + 1] != ETileType::Empty);
//            bool bHasSouth = (y > 0 && TileMap[x][y - 1] != ETileType::Empty);
//            bool bHasEast = (x < MapSize.X - 1 && TileMap[x + 1][y] != ETileType::Empty);
//            bool bHasWest = (x > 0 && TileMap[x - 1][y] != ETileType::Empty);
//
//            int32 ConnectionCount = (bHasNorth ? 1 : 0) + (bHasSouth ? 1 : 0) +
//                (bHasEast ? 1 : 0) + (bHasWest ? 1 : 0);
//
//            // �ڳʳ� ������
//            if (ConnectionCount > 2 ||
//                (bHasNorth && bHasEast) || (bHasNorth && bHasWest) ||
//                (bHasSouth && bHasEast) || (bHasSouth && bHasWest))
//            {
//                TileClass = CorridorCornerClass;
//            }
//            else if (bHasNorth || bHasSouth)
//            {
//                // Y�� ���� ���� = ���� ���� (������ X/Y �ݴ��̹Ƿ� ���� Ÿ�� ���)
//                TileClass = CorridorHorizontalClass;
//            }
//            else if (bHasEast || bHasWest)
//            {
//                // X�� ���� ���� = ���� ���� (������ X/Y �ݴ��̹Ƿ� ���� Ÿ�� ���)
//                TileClass = CorridorVerticalClass;
//            }
//
//            if (TileClass)
//            {
//                FActorSpawnParameters SpawnParams;
//                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//
//                AActor* SpawnedTile = World->SpawnActor<AActor>(TileClass, SpawnLocation, SpawnRotation, SpawnParams);
//                if (SpawnedTile)
//                {
//                    SpawnedTile->Tags.Add("BSPGenerated");
//                    SpawnedCount++;
//                }
//            }
//        }
//    }
//
//    UE_LOG(LogTemp, Warning, TEXT("Spawned %d tiles total"), SpawnedCount);
//}

void ABSPMapGenerator::SpawnTiles()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("World is null!"));
        return;
    }

    int32 SpawnedCount = 0;

    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            if (TileMap[x][y] == ETileType::Empty) continue;

            FVector SpawnLocation = FVector(x * TileSize, y * TileSize, 0);
            FRotator SpawnRotation = FRotator::ZeroRotator;
            TSubclassOf<AActor> TileClass = nullptr;

            // �浵 ���� Ÿ�Ϸ� ó��
            if (TileMap[x][y] == ETileType::Room || TileMap[x][y] == ETileType::Corridor)
            {
                // Ÿ�� ���� Ȯ��
                bool bHasNorth = (y > 0 && TileMap[x][y - 1] != ETileType::Empty);
                bool bHasSouth = (y < MapSize.Y - 1 && TileMap[x][y + 1] != ETileType::Empty);
                bool bHasEast = (x < MapSize.X - 1 && TileMap[x + 1][y] != ETileType::Empty);
                bool bHasWest = (x > 0 && TileMap[x - 1][y] != ETileType::Empty);

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
                    // Y�� ���� ���� = ���� (������ X/Y �ݴ��̹Ƿ� ���� Ÿ��)
                    TileClass = CorridorHorizontalClass;
                }
                else if (bHasEast || bHasWest)
                {
                    // X�� ���� ���� = ���� (������ X/Y �ݴ��̹Ƿ� ���� Ÿ��)
                    TileClass = CorridorVerticalClass;
                }
                else
                {
                    // ������ ���ų� �ϳ��� �ִ� ��� - �⺻������ �ڳ� Ÿ�� ���
                    TileClass = CorridorCornerClass;
                }
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
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to spawn tile at (%d, %d)"), x, y);
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned %d tiles"), SpawnedCount);
}


FString ABSPMapGenerator::GetRoomDoorDirection(const FIntVector& RoomCenter)
{
    // �� �ֺ��� ���� Ȯ�� (3x3 �� ����)
    TArray<FString> PossibleDirections;

    // ���� Ȯ�� (Y+)
    for (int32 dx = -1; dx <= 1; ++dx)
    {
        int32 checkX = RoomCenter.X + dx;
        int32 checkY = RoomCenter.Y + 2;
        if (checkX >= 0 && checkX < MapSize.X && checkY < MapSize.Y &&
            TileMap[checkX][checkY] == ETileType::Corridor)
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
            TileMap[checkX][checkY] == ETileType::Corridor)
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
            TileMap[checkX][checkY] == ETileType::Corridor)
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
            TileMap[checkX][checkY] == ETileType::Corridor)
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

// ��� �������� �� ���� ����
FString ABSPMapGenerator::GetRoomDoorDirectionForNode(TSharedPtr<FBSPNode> Node)
{
    if (!Node || !Node->bHasRoom) return "North";

    TArray<FString> PossibleDirections;

    // ���� Ȯ�� (Y+ ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMax.Y;
        if (checkY < MapSize.Y && TileMap[x][checkY] == ETileType::Corridor)
        {
            PossibleDirections.AddUnique("North");
            break;
        }
    }

    // ���� Ȯ�� (Y- ����)
    for (int32 x = Node->RoomMin.X; x < Node->RoomMax.X; ++x)
    {
        int32 checkY = Node->RoomMin.Y - 1;
        if (checkY >= 0 && TileMap[x][checkY] == ETileType::Corridor)
        {
            PossibleDirections.AddUnique("South");
            break;
        }
    }

    // ���� Ȯ�� (X+ ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMax.X;
        if (checkX < MapSize.X && TileMap[checkX][y] == ETileType::Corridor)
        {
            PossibleDirections.AddUnique("East");
            break;
        }
    }

    // ���� Ȯ�� (X- ����)
    for (int32 y = Node->RoomMin.Y; y < Node->RoomMax.Y; ++y)
    {
        int32 checkX = Node->RoomMin.X - 1;
        if (checkX >= 0 && TileMap[checkX][y] == ETileType::Corridor)
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

void ABSPMapGenerator::CollectLeafNodes(TSharedPtr<FBSPNode> Node)
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

FIntVector ABSPMapGenerator::GetRoomCenter(TSharedPtr<FBSPNode> Node)
{
    if (!Node->bHasRoom) return FIntVector::ZeroValue;

    return FIntVector(
        (Node->RoomMin.X + Node->RoomMax.X) / 2,
        (Node->RoomMin.Y + Node->RoomMax.Y) / 2,
        0
    );
}

bool ABSPMapGenerator::CanPlaceTile(const FIntVector& Pos)
{
    return Pos.X >= 0 && Pos.X < MapSize.X &&
        Pos.Y >= 0 && Pos.Y < MapSize.Y;
}