// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaveFunctionCollapseSubsystem02.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Editor.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "WaveFunctionCollapseModel02.h"

DEFINE_LOG_CATEGORY(LogWFC);

AActor* UWaveFunctionCollapseSubsystem02::CollapseCustom(int32 TryCount /* = 1 */, int32 RandomSeed /* = 0 */)
{
	//Resolution 값 설정
	Resolution.X = 60;
	Resolution.Y = 60;
	Resolution.Z = 1;

	if (!WFCModel)
	{
		UE_LOG(LogWFC, Error, TEXT("Invalid WFC Model"));
		return nullptr;
	}

	UE_LOG(LogWFC, Display, TEXT("Starting WFC - Model: %s, Resolution %dx%dx%d"), *WFCModel->GetFName().ToString(), Resolution.X, Resolution.Y, Resolution.Z);

	// Determinism settings
	int32 ChosenRandomSeed = (RandomSeed != 0 ? RandomSeed : FMath::RandRange(1, TNumericLimits<int32>::Max()));

	int32 ArrayReserveValue = Resolution.X * Resolution.Y * Resolution.Z;
	TArray<FWaveFunctionCollapseTileCustom> Tiles;
	TArray<int32> RemainingTiles;
	TMap<int32, FWaveFunctionCollapseQueueElementCustom> ObservationQueue;
	Tiles.Reserve(ArrayReserveValue);
	RemainingTiles.Reserve(ArrayReserveValue);

	InitializeWFC(Tiles, RemainingTiles);

	bool bSuccessfulSolve = false;

	if (TryCount > 1)
	{
		//Copy Original Initialized tiles
		TArray<FWaveFunctionCollapseTileCustom> TilesCopy = Tiles;
		TArray<int32> RemainingTilesCopy = RemainingTiles;

		int32 CurrentTry = 1;
		bSuccessfulSolve = ObservationPropagation(Tiles, RemainingTiles, ObservationQueue, ChosenRandomSeed);
		FRandomStream RandomStream(ChosenRandomSeed);
		while (!bSuccessfulSolve && CurrentTry < TryCount)
		{
			CurrentTry += 1;
			UE_LOG(LogWFC, Warning, TEXT("Failed with Seed Value: %d. Trying again.  Attempt number: %d"), ChosenRandomSeed, CurrentTry);
			ChosenRandomSeed = RandomStream.RandRange(1, TNumericLimits<int32>::Max());

			// Start from Original Initialized tiles
			Tiles = TilesCopy;
			RemainingTiles = RemainingTilesCopy;
			ObservationQueue.Empty();
			bSuccessfulSolve = ObservationPropagation(Tiles, RemainingTiles, ObservationQueue, ChosenRandomSeed);
		}
	}
	else if (TryCount == 1)
	{
		bSuccessfulSolve = ObservationPropagation(Tiles, RemainingTiles, ObservationQueue, ChosenRandomSeed);
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("Invalid TryCount on Collapse: %d"), TryCount);
		return nullptr;
	}

	// if Successful, Spawn Actor
	if (bSuccessfulSolve)
	{

		// 타일 크기 조정 로직 추가
		for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
		{
			if (Tiles[TileIndex].RemainingOptions.Num() == 1)
			{
				FWaveFunctionCollapseOptionCustom& SelectedOption = Tiles[TileIndex].RemainingOptions[0];

				// 기본 타일 크기
				float TileSize = WFCModel->TileSize;

				// 방 타일인지 확인하여 크기 조정
				if (SelectedOption.bIsRoomTile)
				{
					TileSize *= 3.0f;
				}

				FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * TileSize;

				// 스케일 계산 (기본 크기를 기준으로 조정된 크기를 반영)
				SelectedOption.BaseScale3D = FVector(TileSize / WFCModel->TileSize);

			}
		}


		AActor* SpawnedActor = SpawnActorFromTiles(Tiles);
		UE_LOG(LogWFC, Display, TEXT("Success! Seed Value: %d. Spawned Actor: %s"), ChosenRandomSeed, *SpawnedActor->GetActorLabel());

		// 테두리 블루프린트 소환 함수 호출
		SpawnBorderBlueprints();

		return SpawnedActor;
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("Failed after %d tries."), TryCount);
		return nullptr;
	}
}





void UWaveFunctionCollapseSubsystem02::SpawnBorderBlueprints()
{
	// 블루프린트 클래스 로드
	UBlueprint* LeftBorderBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t02-L.t02-L"));
	UBlueprint* RightBorderBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t02-r.t02-r"));
	UBlueprint* FrontBorderBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t02-m.t02-m"));
	UBlueprint* BackBorderBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t02-b.t02-b"));

	// 모서리 블루프린트 클래스 로드
	UBlueprint* BottomLeftCornerBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t03-back.t03-back"));
	UBlueprint* TopLeftCornerBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t03-back1.t03-back1"));
	UBlueprint* BottomRightCornerBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t03-back2.t03-back2"));
	UBlueprint* TopRightCornerBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/BP/Bedge/t03-back3.t03-back3"));

	if (!LeftBorderBlueprint || !RightBorderBlueprint || !FrontBorderBlueprint || !BackBorderBlueprint ||
		!BottomLeftCornerBlueprint || !TopLeftCornerBlueprint || !BottomRightCornerBlueprint || !TopRightCornerBlueprint)
	{
		UE_LOG(LogWFC, Error, TEXT("Failed to load one or more Blueprint assets"));
		return;
	}

	FVector Offset = FVector(WFCModel->TileSize * 0.5f); // 기본 오프셋

	for (int32 Z = 0; Z < Resolution.Z; Z++)
	{
		for (int32 Y = 0; Y <= Resolution.Y-1; Y++)
		{
			for (int32 X = 0; X <= Resolution.X-1; X++)
			{
				FIntVector Position(X, Y, Z);

				if (IsPositionInnerBorder(Position))
				{
					TSubclassOf<AActor> SelectedBPClass = nullptr;
					FVector PositionOffset = Offset; // 기본 오프셋 복사

					// 각 모서리 위치에 따라 다른 블루프린트 선택
					if (X == 0 && Y == 0) // 좌하단 모서리
					{
						SelectedBPClass = BottomLeftCornerBlueprint->GeneratedClass;
						PositionOffset.X -= WFCModel->TileSize;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (X == 0 && Y == Resolution.Y - 1) // 좌상단 모서리
					{
						SelectedBPClass = BottomRightCornerBlueprint->GeneratedClass;
						PositionOffset.X -= WFCModel->TileSize;
						PositionOffset.Y += WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1 && Y == 0) // 우하단 모서리
					{
						SelectedBPClass = TopLeftCornerBlueprint->GeneratedClass;
						PositionOffset.X += WFCModel->TileSize;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1 && Y == Resolution.Y - 1) // 우상단 모서리
					{
						SelectedBPClass = TopRightCornerBlueprint->GeneratedClass;
						PositionOffset.X += WFCModel->TileSize;
						PositionOffset.Y += WFCModel->TileSize;
					}
					else if (X == 0) // 왼쪽 테두리
					{
						SelectedBPClass = BackBorderBlueprint->GeneratedClass;
						PositionOffset.X -= WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1) // 오른쪽 테두리
					{
						SelectedBPClass = FrontBorderBlueprint->GeneratedClass;
						PositionOffset.X += WFCModel->TileSize;
					}
					else if (Y == 0) // 앞쪽 테두리
					{
						SelectedBPClass = LeftBorderBlueprint->GeneratedClass;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (Y == Resolution.Y - 1) // 뒤쪽 테두리
					{
						SelectedBPClass = RightBorderBlueprint->GeneratedClass;
						PositionOffset.Y += WFCModel->TileSize;
					}

					// 조정된 위치에 블루프린트 액터를 스폰
					if (SelectedBPClass && GetWorld())
					{
						FVector SpawnLocation = FVector(Position) * WFCModel->TileSize + PositionOffset;
						FTransform SpawnTransform = FTransform(SpawnLocation);

						AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(SelectedBPClass, SpawnTransform);
						if (SpawnedActor)
						{
							UE_LOG(LogWFC, Display, TEXT("Spawned Blueprint Actor at (%d, %d, %d) with offset"), X, Y, Z);
							SpawnedActor->Tags.Add(FName("WFCGenerated"));


						}
						else
						{
							UE_LOG(LogWFC, Error, TEXT("Failed to spawn Blueprint Actor at (%d, %d, %d)"), X, Y, Z);
						}
					}
					else
					{
						UE_LOG(LogWFC, Error, TEXT("Failed to spawn due to invalid Blueprint class or world context at (%d, %d, %d)"), X, Y, Z);
					}
				}
			}
		}
	}
}










void UWaveFunctionCollapseSubsystem02::InitializeWFC(TArray<FWaveFunctionCollapseTileCustom>& Tiles, TArray<int32>& RemainingTiles)
{
	FWaveFunctionCollapseTileCustom InitialTile;
	int32 SwapIndex = 0;




	if (BuildInitialTile(InitialTile))
	{
		float MinEntropy = InitialTile.ShannonEntropy;
		for (int32 Z = 0; Z < Resolution.Z; Z++)
		{
			for (int32 Y = 0; Y < Resolution.Y; Y++)
			{
				for (int32 X = 0; X < Resolution.X; X++)
				{
					// 첫 타일(0,0,0) 고정 로직 추가
					if (X == 0 && Y == 0 && Z == 0)
					{
						// 고정 옵션 설정
						FWaveFunctionCollapseOptionCustom FixedOption(TEXT("/Game/BP/t01-01.t01-01"));
						FWaveFunctionCollapseTileCustom FixedTile;
						FixedTile.RemainingOptions.Add(FixedOption);

						// Shannon 엔트로피 고정 (필요시 다른 값 설정 가능)
						FixedTile.ShannonEntropy = 0.0f;

						Tiles.Add(FixedTile);
						RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));

						// ObservationPropagation 단계에서 수정되지 않도록 RemainingTiles에서 제거
						RemainingTiles.RemoveAt(RemainingTiles.Num() - 1, 1, false);
						continue;
					}

					// Pre-populate with starter tiles
					if (FWaveFunctionCollapseOptionCustom* StarterOption = StarterOptions.Find(FIntVector(X, Y, Z)))
					{
						FWaveFunctionCollapseTileCustom StarterTile;
						StarterTile.RemainingOptions.Add(*StarterOption);
						StarterTile.ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(StarterTile.RemainingOptions, WFCModel);
						Tiles.Add(StarterTile);
						RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));

						// swap lower entropy tile to the beginning of RemainingTiles
						if (StarterTile.ShannonEntropy < MinEntropy)
						{
							RemainingTiles.Swap(0, RemainingTiles.Num() - 1);
							MinEntropy = StarterTile.ShannonEntropy;
							SwapIndex = 0;
						}
						else if (StarterTile.ShannonEntropy == MinEntropy && StarterTile.ShannonEntropy != InitialTile.ShannonEntropy)
						{
							SwapIndex += 1;
							RemainingTiles.Swap(SwapIndex, RemainingTiles.Num() - 1);
						}
					}

					// Pre-populate with border tiles
					else if (IsPositionInnerBorder(FIntVector(X, Y, Z))
						&& (bUseEmptyBorder || WFCModel->Constraints.Contains(FWaveFunctionCollapseOptionCustom::BorderOption)))
					{
						FWaveFunctionCollapseTileCustom BorderTile;
						BorderTile.RemainingOptions = GetInnerBorderOptions(FIntVector(X, Y, Z), InitialTile.RemainingOptions);
						BorderTile.ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(BorderTile.RemainingOptions, WFCModel);
						Tiles.Add(BorderTile);
						RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));

						// swap lower entropy tile to the beginning of RemainingTiles
						if (BorderTile.ShannonEntropy < MinEntropy)
						{
							RemainingTiles.Swap(0, RemainingTiles.Num() - 1);
							MinEntropy = BorderTile.ShannonEntropy;
							SwapIndex = 0;
						}
						else if (BorderTile.ShannonEntropy == MinEntropy && BorderTile.ShannonEntropy != InitialTile.ShannonEntropy)
						{
							SwapIndex += 1;
							RemainingTiles.Swap(SwapIndex, RemainingTiles.Num() - 1);
						}
					}

					// Fill the rest with initial tiles
					else
					{
						Tiles.Add(InitialTile);
						RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));
					}
				}
			}
		}
		StarterOptions.Empty();
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("Could not create Initial Tile from Model"));
	}
}

bool UWaveFunctionCollapseSubsystem02::BuildInitialTile(FWaveFunctionCollapseTileCustom& InitialTile)
{
	TArray<FWaveFunctionCollapseOptionCustom> InitialOptions;
	for (const TPair< FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : WFCModel->Constraints)
	{
		if (Constraint.Key.BaseObject != FWaveFunctionCollapseOptionCustom::BorderOption.BaseObject)
		{
			InitialOptions.Add(Constraint.Key);
		}
	}

	if (!InitialOptions.IsEmpty())
	{
		InitialTile.RemainingOptions = InitialOptions;
		InitialTile.ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(InitialOptions, WFCModel);
		return true;
	}
	else
	{
		return false;
	}
}

TArray<FWaveFunctionCollapseOptionCustom> UWaveFunctionCollapseSubsystem02::GetInnerBorderOptions(FIntVector Position, const TArray<FWaveFunctionCollapseOptionCustom>& InitialOptions)
{
	TArray<FWaveFunctionCollapseOptionCustom> InnerBorderOptions;
	TArray<FWaveFunctionCollapseOptionCustom> InnerBorderOptionsToRemove;
	InnerBorderOptions = InitialOptions;

	// gather options to remove
	if (Position.X == 0)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Front, InnerBorderOptions, InnerBorderOptionsToRemove);
	}
	if (Position.X == Resolution.X - 1)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Back, InnerBorderOptions, InnerBorderOptionsToRemove);
	}
	if (Position.Y == 0)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Right, InnerBorderOptions, InnerBorderOptionsToRemove);
	}
	if (Position.Y == Resolution.Y - 1)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Left, InnerBorderOptions, InnerBorderOptionsToRemove);
	}
	if (Position.Z == 0)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Up, InnerBorderOptions, InnerBorderOptionsToRemove);
	}
	if (Position.Z == Resolution.Z - 1)
	{
		GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom::Down, InnerBorderOptions, InnerBorderOptionsToRemove);
	}

	//remove options
	if (!InnerBorderOptionsToRemove.IsEmpty())
	{
		for (FWaveFunctionCollapseOptionCustom& RemoveThisOption : InnerBorderOptionsToRemove)
		{
			InnerBorderOptions.RemoveSingleSwap(RemoveThisOption, EAllowShrinking::No);
		}
		InnerBorderOptions.Shrink();
	}

	return InnerBorderOptions;
}

void UWaveFunctionCollapseSubsystem02::GatherInnerBorderOptionsToRemove(EWaveFunctionCollapseAdjacencyCustom Adjacency, const TArray<FWaveFunctionCollapseOptionCustom>& InitialOptions, TArray<FWaveFunctionCollapseOptionCustom>& OutBorderOptionsToRemove)
{
	bool bFoundBorderOptions = false;
	for (const FWaveFunctionCollapseOptionCustom& InitialOption : InitialOptions)
	{
		// if border option exists in the model, use it
		if (FWaveFunctionCollapseAdjacencyToOptionsMapCustom* FoundBorderAdjacencyToOptionsMap = WFCModel->Constraints.Find(FWaveFunctionCollapseOptionCustom::BorderOption))
		{
			if (FWaveFunctionCollapseOptionsCustom* FoundBorderOptions = FoundBorderAdjacencyToOptionsMap->AdjacencyToOptionsMap.Find(Adjacency))
			{
				if (!FoundBorderOptions->Options.Contains(InitialOption))
				{
					OutBorderOptionsToRemove.AddUnique(InitialOption);
					bFoundBorderOptions = true;
				}
			}
		}

		// else, if useEmptyBorder, use empty option
		if (bUseEmptyBorder && !bFoundBorderOptions)
		{
			if (FWaveFunctionCollapseAdjacencyToOptionsMapCustom* FoundEmptyAdjacencyToOptionsMap = WFCModel->Constraints.Find(FWaveFunctionCollapseOptionCustom::EmptyOption))
			{
				if (FWaveFunctionCollapseOptionsCustom* FoundEmptyOptions = FoundEmptyAdjacencyToOptionsMap->AdjacencyToOptionsMap.Find(Adjacency))
				{
					if (!FoundEmptyOptions->Options.Contains(InitialOption))
					{
						OutBorderOptionsToRemove.AddUnique(InitialOption);
					}
				}
			}
		}
	}
}

bool UWaveFunctionCollapseSubsystem02::IsPositionInnerBorder(FIntVector Position)
{
	return (Position.X == 0
		|| Position.Y == 0
		|| Position.Z == 0
		|| Position.X == Resolution.X - 1
		|| Position.Y == Resolution.Y - 1
		|| Position.Z == Resolution.Z - 1);
}

bool UWaveFunctionCollapseSubsystem02::Observe(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
	TArray<int32>& RemainingTiles,
	TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
	int32 RandomSeed)
{
	// RemainingTiles 배열이 비어있는지 확인
	if (RemainingTiles.Num() == 0)
	{
		UE_LOG(LogWFC, Error, TEXT("RemainingTiles array is empty!"));
		return false;
	}

	float MinEntropy = 0;
	int32 LastSameMinEntropyIndex = 0;
	int32 SelectedMinEntropyIndex = 0;
	int32 MinEntropyIndex = 0;
	FRandomStream RandomStream(RandomSeed);

	// Find MinEntropy Tile Indices
	if (RemainingTiles.Num() > 1)
	{
		for (int32 index = 0; index < RemainingTiles.Num(); index++)
		{
			if (index == 0)
			{
				MinEntropy = Tiles[RemainingTiles[index]].ShannonEntropy;
			}
			else
			{
				if (Tiles[RemainingTiles[index]].ShannonEntropy > MinEntropy)
				{
					break;
				}
				else
				{
					LastSameMinEntropyIndex += 1;
				}
			}
		}
		SelectedMinEntropyIndex = RandomStream.RandRange(0, LastSameMinEntropyIndex);
		MinEntropyIndex = RemainingTiles[SelectedMinEntropyIndex];
	}
	else
	{
		MinEntropyIndex = RemainingTiles[0];
	}

	// Rand Selection of Weighted Options using Cumulative Density
	TArray<float> CumulativeDensity;
	CumulativeDensity.Reserve(Tiles[MinEntropyIndex].RemainingOptions.Num());
	float CumulativeWeight = 0;
	for (FWaveFunctionCollapseOptionCustom& Option : Tiles[MinEntropyIndex].RemainingOptions)
	{
		CumulativeWeight += WFCModel->Constraints.Find(Option)->Weight;
		CumulativeDensity.Add(CumulativeWeight);
	}

	int32 SelectedOptionIndex = 0;
	float RandomDensity = RandomStream.FRandRange(0.0f, CumulativeDensity.Last());
	for (int32 Index = 0; Index < CumulativeDensity.Num(); Index++)
	{
		if (CumulativeDensity[Index] > RandomDensity)
		{
			SelectedOptionIndex = Index;
			break;
		}
	}

	// Make Selection
	Tiles[MinEntropyIndex] = FWaveFunctionCollapseTileCustom(Tiles[MinEntropyIndex].RemainingOptions[SelectedOptionIndex], TNumericLimits<float>::Max());



	if (SelectedMinEntropyIndex != LastSameMinEntropyIndex)
	{
		RemainingTiles.Swap(SelectedMinEntropyIndex, LastSameMinEntropyIndex);
	}
	RemainingTiles.RemoveAtSwap(LastSameMinEntropyIndex);

	if (!RemainingTiles.IsEmpty())
	{
		// if MinEntropy has changed after removal, find new MinEntropy and swap to front of array
		if (Tiles[RemainingTiles[0]].ShannonEntropy != MinEntropy)
		{
			int32 SwapToIndex = 0;
			for (int32 index = 0; index < RemainingTiles.Num(); index++)
			{
				if (index == 0)
				{
					MinEntropy = Tiles[RemainingTiles[index]].ShannonEntropy;
				}
				else
				{
					if (Tiles[RemainingTiles[index]].ShannonEntropy < MinEntropy)
					{
						SwapToIndex = 0;
						MinEntropy = Tiles[RemainingTiles[index]].ShannonEntropy;
						RemainingTiles.Swap(SwapToIndex, index);
					}
					else if (Tiles[RemainingTiles[index]].ShannonEntropy == MinEntropy)
					{
						SwapToIndex += 1;
						RemainingTiles.Swap(SwapToIndex, index);
					}
				}
			}
		}

		// Add Adjacent Tile Indices to Queue
		AddAdjacentIndicesToQueue(MinEntropyIndex, RemainingTiles, ObservationQueue);

		// Continue To Propagation
		return true;
	}
	else
	{
		// Do Not Continue to Propagation
		return false;
	}
}

void UWaveFunctionCollapseSubsystem02::AddAdjacentIndicesToQueue(int32 CenterIndex, const TArray<int32>& RemainingTiles, TMap<int32, FWaveFunctionCollapseQueueElementCustom>& OutQueue)
{
	FIntVector Position = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(CenterIndex, Resolution);
	if (Position.X + 1 < Resolution.X && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(1, 0, 0), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(1, 0, 0), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Front));
	}
	if (Position.X - 1 >= 0 && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(-1, 0, 0), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(-1, 0, 0), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Back));
	}
	if (Position.Y + 1 < Resolution.Y && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 1, 0), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 1, 0), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Right));
	}
	if (Position.Y - 1 >= 0 && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, -1, 0), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, -1, 0), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Left));
	}
	if (Position.Z + 1 < Resolution.Z && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 0, 1), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 0, 1), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Up));
	}
	if (Position.Z - 1 >= 0 && RemainingTiles.Contains(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 0, -1), Resolution)))
	{
		OutQueue.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Position + FIntVector(0, 0, -1), Resolution),
			FWaveFunctionCollapseQueueElementCustom(CenterIndex, EWaveFunctionCollapseAdjacencyCustom::Down));
	}
}

bool UWaveFunctionCollapseSubsystem02::Propagate(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
	TArray<int32>& RemainingTiles,
	TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
	int32& PropagationCount)
{
	TMap<int32, FWaveFunctionCollapseQueueElementCustom> PropagationQueue;

	while (!ObservationQueue.IsEmpty())
	{
		for (TPair<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationAdjacenctElement : ObservationQueue)
		{
			// Make sure the tile to check is still a valid remaining tile
			if (!RemainingTiles.Contains(ObservationAdjacenctElement.Key))
			{
				continue;
			}

			TArray<FWaveFunctionCollapseOptionCustom>& ObservationRemainingOptions = Tiles[ObservationAdjacenctElement.Key].RemainingOptions;

			// Get check against options
			TArray<FWaveFunctionCollapseOptionCustom> OptionsToCheckAgainst;
			OptionsToCheckAgainst.Reserve(WFCModel->Constraints.Num());
			for (FWaveFunctionCollapseOptionCustom& CenterOption : Tiles[ObservationAdjacenctElement.Value.CenterObjectIndex].RemainingOptions)
			{
				for (FWaveFunctionCollapseOptionCustom& Option : WFCModel->Constraints.FindRef(CenterOption).AdjacencyToOptionsMap.FindRef(ObservationAdjacenctElement.Value.Adjacency).Options)
				{
					OptionsToCheckAgainst.AddUnique(Option);
				}
			}

			// Accumulate New Remaining Options
			bool bAddToPropagationQueue = false;
			TArray<FWaveFunctionCollapseOptionCustom> TmpRemainingOptionsAccumulation;
			TmpRemainingOptionsAccumulation.Reserve(ObservationRemainingOptions.Num());
			for (FWaveFunctionCollapseOptionCustom& ObservationRemainingOption : ObservationRemainingOptions)
			{
				if (OptionsToCheckAgainst.Contains(ObservationRemainingOption))
				{
					TmpRemainingOptionsAccumulation.AddUnique(ObservationRemainingOption);
				}
				else
				{
					bAddToPropagationQueue = true;
				}
			}

			// If Remaining Options have changed
			if (bAddToPropagationQueue)
			{
				if (!TmpRemainingOptionsAccumulation.IsEmpty())
				{
					AddAdjacentIndicesToQueue(ObservationAdjacenctElement.Key, RemainingTiles, PropagationQueue);

					// Update Tile with new options
					float MinEntropy = Tiles[RemainingTiles[0]].ShannonEntropy;
					float NewEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(TmpRemainingOptionsAccumulation, WFCModel);
					int32 CurrentRemainingTileIndex;

					// If NewEntropy is <= MinEntropy, add to front of Remaining Tiles
					if (NewEntropy < MinEntropy)
					{
						MinEntropy = NewEntropy;
						CurrentRemainingTileIndex = RemainingTiles.Find(ObservationAdjacenctElement.Key);
						if (CurrentRemainingTileIndex != 0)
						{
							RemainingTiles.Swap(0, CurrentRemainingTileIndex);
						}
					}
					else if (NewEntropy == MinEntropy)
					{
						CurrentRemainingTileIndex = RemainingTiles.Find(ObservationAdjacenctElement.Key);
						for (int32 Index = 1; Index < RemainingTiles.Num(); Index++)
						{
							if (MinEntropy != Tiles[RemainingTiles[Index]].ShannonEntropy)
							{
								if (CurrentRemainingTileIndex != Index)
								{
									RemainingTiles.Swap(Index, CurrentRemainingTileIndex);
								}
								break;
							}
						}
					}

					Tiles[ObservationAdjacenctElement.Key] = FWaveFunctionCollapseTileCustom(TmpRemainingOptionsAccumulation, NewEntropy);
				}
				else
				{
					// Encountered Contradiction
					UE_LOG(LogWFC, Error, TEXT("Encountered Contradiction on Index %d, Tile: %s"), ObservationAdjacenctElement.Key, *Tiles[ObservationAdjacenctElement.Key].RemainingOptions[0].BaseObject.ToString());
					return false;
				}
			}
		}

		ObservationQueue = PropagationQueue;
		if (!PropagationQueue.IsEmpty())
		{
			PropagationCount += 1;
			PropagationQueue.Reset();
		}
	}

	return true;
}

bool UWaveFunctionCollapseSubsystem02::ObservationPropagation(TArray<FWaveFunctionCollapseTileCustom>& Tiles,
	TArray<int32>& RemainingTiles,
	TMap<int32, FWaveFunctionCollapseQueueElementCustom>& ObservationQueue,
	int32 RandomSeed)
{
	int32 PropagationCount = 1;
	int32 MutatedRandomSeed = RandomSeed;

	while (Observe(Tiles, RemainingTiles, ObservationQueue, MutatedRandomSeed))
	{
		if (!Propagate(Tiles, RemainingTiles, ObservationQueue, PropagationCount))
		{
			return false;
		}

		// Mutate Seed
		MutatedRandomSeed--;
	}

	// Check if all tiles in the solve are non-spawnable
	return !AreAllTilesNonSpawnable(Tiles);
}

UActorComponent* UWaveFunctionCollapseSubsystem02::AddNamedInstanceComponent(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName)
{
	Actor->Modify();
	// Assign Unique Name
	int32 Counter = 1;
	FName ComponentInstanceName = ComponentName;
	while (!FComponentEditorUtils::IsComponentNameAvailable(ComponentInstanceName.ToString(), Actor))
	{
		ComponentInstanceName = FName(*FString::Printf(TEXT("%s_%d"), *ComponentName.ToString(), Counter++));
	}
	UActorComponent* InstanceComponent = NewObject<UActorComponent>(Actor, ComponentClass, ComponentInstanceName, RF_Transactional);
	if (InstanceComponent)
	{
		Actor->AddInstanceComponent(InstanceComponent);
		Actor->FinishAddComponent(InstanceComponent, false, FTransform::Identity);
		Actor->RerunConstructionScripts();
	}
	return InstanceComponent;
}

AActor* UWaveFunctionCollapseSubsystem02::SpawnActorFromTiles(const TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	// UEditorActorSubsystem 대신 UWorld를 사용하여 게임 환경에서 액터 스폰
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogWFC, Error, TEXT("World is null, cannot spawn actors."));
		return nullptr;
	}

	// 스폰할 액터 설정
	FActorSpawnParameters SpawnParams;
	AActor* SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), OriginLocation, Orientation, SpawnParams);
	if (!SpawnedActor)
	{
		UE_LOG(LogWFC, Error, TEXT("Failed to spawn actor."));
		return nullptr;
	}

	

	// WFCModel 체크
	if (WFCModel)
	{
		FActorLabelUtilities::SetActorLabelUnique(SpawnedActor, WFCModel->GetFName().ToString());
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("WFCModel is null, cannot set actor label."));
	}

	// Components 생성
	TMap<FSoftObjectPath, UInstancedStaticMeshComponent*> BaseObjectToISM;
	for (int32 index = 0; index < Tiles.Num(); index++)
	{
		if (Tiles[index].RemainingOptions.Num() != 1)
		{
			continue;
		}

		// 빈 옵션 또는 비어있는 옵션 확인
		FSoftObjectPath BaseObject = Tiles[index].RemainingOptions[0].BaseObject;
		if (BaseObject == FWaveFunctionCollapseOptionCustom::EmptyOption.BaseObject
			|| BaseObject == FWaveFunctionCollapseOptionCustom::VoidOption.BaseObject)
		{
			continue;
		}

		// SpawnExclusion 배열 확인
		if (WFCModel->SpawnExclusion.Contains(BaseObject))
		{
			continue;
		}

		// LoadedObject의 널 체크
		UObject* LoadedObject = BaseObject.TryLoad();
		if (LoadedObject)
		{
			FRotator BaseRotator = Tiles[index].RemainingOptions[0].BaseRotator;
			FVector BaseScale3D = Tiles[index].RemainingOptions[0].BaseScale3D;
			FVector PositionOffset = FVector(WFCModel->TileSize * 0.5f);
			FVector TilePosition = (FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(index, Resolution)) * WFCModel->TileSize) + PositionOffset;

			// StaticMesh 처리 (InstancedStaticMeshComponent 사용)
			
			// 블루프린트 처리 (ChildActorComponent 사용)
			if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(LoadedObject))
			{
				if (LoadedBlueprint->GeneratedClass && LoadedBlueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
				{
					TSubclassOf<AActor> ActorClass = *LoadedBlueprint->GeneratedClass;

					// 고유 이름 생성
					FString UniqueName = FString::Printf(TEXT("%s_%d"), *LoadedObject->GetFName().ToString(), index);  // 'index'는 타일의 인덱스 등 고유한 값을 사용
					FName ComponentName(*UniqueName);

					// ChildActorComponent 생성
					UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(SpawnedActor, UChildActorComponent::StaticClass(), ComponentName);
					if (ChildActorComponent)
					{
						// 부모 컴포넌트 설정
						ChildActorComponent->SetupAttachment(SpawnedActor->GetRootComponent());

						// 컴포넌트 등록
						ChildActorComponent->RegisterComponent();

						// 자식 액터 클래스 설정
						ChildActorComponent->SetChildActorClass(ActorClass);

						// 위치, 회전, 크기 설정
						ChildActorComponent->SetRelativeLocation(TilePosition);
						ChildActorComponent->SetRelativeRotation(BaseRotator);
						ChildActorComponent->SetRelativeScale3D(BaseScale3D);

						// 액터에 컴포넌트 추가 및 초기화
						SpawnedActor->AddInstanceComponent(ChildActorComponent);
						SpawnedActor->RerunConstructionScripts();

						UE_LOG(LogWFC, Warning, TEXT("Successfully added ChildActorComponent for Blueprint: %s, Component Name: %s"), *LoadedBlueprint->GetName(), *ComponentName.ToString());
					}
					else
					{
						UE_LOG(LogWFC, Error, TEXT("Failed to create ChildActorComponent for Blueprint: %s"), *LoadedBlueprint->GetName());
					}
				}
			}
			else if (UStaticMesh* LoadedStaticMesh = Cast<UStaticMesh>(LoadedObject))
			{
				UInstancedStaticMeshComponent* ISMComponent;
				if (UInstancedStaticMeshComponent** FoundISMComponentPtr = BaseObjectToISM.Find(BaseObject))
				{
					ISMComponent = *FoundISMComponentPtr;
				}
				else
				{
					ISMComponent = Cast<UInstancedStaticMeshComponent>(UWaveFunctionCollapseSubsystem02::AddNamedInstanceComponent(SpawnedActor, UInstancedStaticMeshComponent::StaticClass(), LoadedObject->GetFName()));
					BaseObjectToISM.Add(BaseObject, ISMComponent);
				}
				ISMComponent->SetStaticMesh(LoadedStaticMesh);
				ISMComponent->SetMobility(EComponentMobility::Static);
				ISMComponent->AddInstance(FTransform(BaseRotator, TilePosition, BaseScale3D));
			}
			else
			{
				UE_LOG(LogWFC, Warning, TEXT("Invalid Type, skipping: %s"), *BaseObject.ToString());
			}
		}
		else
		{
			UE_LOG(LogWFC, Warning, TEXT("Unable to load object, skipping: %s"), *BaseObject.ToString());
		}
	}

	return SpawnedActor;
}


bool UWaveFunctionCollapseSubsystem02::AreAllTilesNonSpawnable(const TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	bool bAllTilesAreNonSpawnable = true;
	for (int32 index = 0; index < Tiles.Num(); index++)
	{
		if (Tiles[index].RemainingOptions.Num() == 1)
		{
			FSoftObjectPath BaseObject = Tiles[index].RemainingOptions[0].BaseObject;
			if (!(BaseObject == FWaveFunctionCollapseOptionCustom::EmptyOption.BaseObject
				|| BaseObject == FWaveFunctionCollapseOptionCustom::VoidOption.BaseObject
				|| WFCModel->SpawnExclusion.Contains(BaseObject)))
			{
				bAllTilesAreNonSpawnable = false;
				break;
			}
		}
	}
	return bAllTilesAreNonSpawnable;
}

void UWaveFunctionCollapseSubsystem02::DeriveGridFromTransformBounds(const TArray<FTransform>& Transforms)
{
	if (Transforms.IsEmpty())
	{
		UE_LOG(LogWFC, Error, TEXT("Empty Transform Array."));
		return;
	}

	FVector TransformPivot;
	FVector AxisAlignedPoint = FVector::ZeroVector;
	FVector MinBound = FVector::ZeroVector;
	FVector MaxBound = FVector::ZeroVector;
	TMap<FIntVector, int32> PositionToPointCount;
	float TransformOrientation = Transforms[0].GetRotation().Rotator().Yaw;

	float CosOrientation = FMath::Cos(PI / (180.f) * -TransformOrientation);
	float SinOrientation = FMath::Sin(PI / (180.f) * -TransformOrientation);
	float UnitScale = 1.0f;

	for (int32 index = 0; index < Transforms.Num(); index++)
	{
		if (index == 0)
		{
			TransformPivot = Transforms[index].GetLocation() * UnitScale;
		}
		else
		{
			FVector OffsetPoint = Transforms[index].GetLocation() * UnitScale - TransformPivot;
			AxisAlignedPoint.X = (OffsetPoint.X * CosOrientation) - (OffsetPoint.Y * SinOrientation);
			AxisAlignedPoint.Y = (OffsetPoint.X * SinOrientation) + (OffsetPoint.Y * CosOrientation);
			AxisAlignedPoint.Z = OffsetPoint.Z;
			MinBound.X = FMath::Min(AxisAlignedPoint.X, MinBound.X);
			MinBound.Y = FMath::Min(AxisAlignedPoint.Y, MinBound.Y);
			MinBound.Z = FMath::Min(AxisAlignedPoint.Z, MinBound.Z);
			MaxBound.X = FMath::Max(AxisAlignedPoint.X, MaxBound.X);
			MaxBound.Y = FMath::Max(AxisAlignedPoint.Y, MaxBound.Y);
			MaxBound.Z = FMath::Max(AxisAlignedPoint.Z, MaxBound.Z);
		}

		FIntVector PointPosition;
		PointPosition.X = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.X / WFCModel->TileSize);
		PointPosition.Y = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.Y / WFCModel->TileSize);
		PointPosition.Z = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.Z / WFCModel->TileSize);
		if (int32* PointCount = PositionToPointCount.Find(PointPosition))
		{
			PositionToPointCount.Add(PointPosition, *PointCount + 1);
		}
		else
		{
			PositionToPointCount.Add(PointPosition, 1);
		}
	}

	// Set WFC Resolution
	Resolution.X = (int32)FMath::RoundHalfFromZero(MaxBound.X / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.X / WFCModel->TileSize) + 1;
	Resolution.Y = (int32)FMath::RoundHalfFromZero(MaxBound.Y / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.Y / WFCModel->TileSize) + 1;
	Resolution.Z = (int32)FMath::RoundHalfFromZero(MaxBound.Z / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.Z / WFCModel->TileSize) + 1;

	// Set WFC OriginLocation
	FVector ReorientedMinPoint;
	ReorientedMinPoint.X = ((MinBound.X - (WFCModel->TileSize * 0.5f)) * CosOrientation) - ((MinBound.Y - (WFCModel->TileSize * 0.5f)) * -SinOrientation);
	ReorientedMinPoint.Y = ((MinBound.X - (WFCModel->TileSize * 0.5f)) * -SinOrientation) + ((MinBound.Y - (WFCModel->TileSize * 0.5f)) * CosOrientation);
	ReorientedMinPoint.Z = MinBound.Z - (WFCModel->TileSize * 0.5f);
	OriginLocation = ReorientedMinPoint + TransformPivot;

	// Set WFC Orientation
	Orientation = FRotator(0, TransformOrientation, 0);

	// Set WFC Starter Options
	FIntVector PositionOffset;
	PositionOffset.X = (int32)FMath::RoundHalfFromZero(MinBound.X / WFCModel->TileSize);
	PositionOffset.Y = (int32)FMath::RoundHalfFromZero(MinBound.Y / WFCModel->TileSize);
	PositionOffset.Z = (int32)FMath::RoundHalfFromZero(MinBound.Z / WFCModel->TileSize);
	StarterOptions.Empty();
	for (int32 z = 0; z < Resolution.Z; z++)
	{
		for (int32 y = 0; y < Resolution.Y; y++)
		{
			for (int32 x = 0; x < Resolution.X; x++)
			{
				if (!PositionToPointCount.Contains(FIntVector(x + PositionOffset.X, y + PositionOffset.Y, z + PositionOffset.Z)))
				{
					StarterOptions.Add(FIntVector(x, y, z), FWaveFunctionCollapseOptionCustom::EmptyOption);
				}
			}
		}
	}
}

void UWaveFunctionCollapseSubsystem02::DeriveGridFromTransforms(const TArray<FTransform>& Transforms)
{
	if (Transforms.IsEmpty())
	{
		UE_LOG(LogWFC, Error, TEXT("Empty Transform Array."));
		return;
	}

	FVector TransformPivot;
	FVector AxisAlignedPoint = FVector::ZeroVector;
	FVector MinBound = FVector::ZeroVector;
	FVector MaxBound = FVector::ZeroVector;
	TArray<FIntVector> PointPositions;
	float TransformOrientation = Transforms[0].GetRotation().Rotator().Yaw;

	float CosOrientation = FMath::Cos(PI / (180.f) * -TransformOrientation);
	float SinOrientation = FMath::Sin(PI / (180.f) * -TransformOrientation);
	float UnitScale = 1.0f;

	for (int32 index = 0; index < Transforms.Num(); index++)
	{
		if (index == 0)
		{
			TransformPivot = (Transforms[index].GetLocation() * UnitScale);
		}
		else
		{
			FVector LocalPoint = Transforms[index].GetLocation() * UnitScale - TransformPivot;
			AxisAlignedPoint.X = (LocalPoint.X * CosOrientation) - (LocalPoint.Y * SinOrientation);
			AxisAlignedPoint.Y = (LocalPoint.X * SinOrientation) + (LocalPoint.Y * CosOrientation);
			AxisAlignedPoint.Z = LocalPoint.Z;
			MinBound.X = FMath::Min(AxisAlignedPoint.X, MinBound.X);
			MinBound.Y = FMath::Min(AxisAlignedPoint.Y, MinBound.Y);
			MinBound.Z = FMath::Min(AxisAlignedPoint.Z, MinBound.Z);
			MaxBound.X = FMath::Max(AxisAlignedPoint.X, MaxBound.X);
			MaxBound.Y = FMath::Max(AxisAlignedPoint.Y, MaxBound.Y);
			MaxBound.Z = FMath::Max(AxisAlignedPoint.Z, MaxBound.Z);
		}

		FIntVector PointPosition;
		PointPosition.X = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.X / WFCModel->TileSize);
		PointPosition.Y = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.Y / WFCModel->TileSize);
		PointPosition.Z = (int32)FMath::RoundHalfFromZero(AxisAlignedPoint.Z / WFCModel->TileSize);
		PointPositions.AddUnique(PointPosition);
	}

	// Set WFC Resolution
	Resolution.X = (int32)FMath::RoundHalfFromZero(MaxBound.X / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.X / WFCModel->TileSize) + 1;
	Resolution.Y = (int32)FMath::RoundHalfFromZero(MaxBound.Y / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.Y / WFCModel->TileSize) + 1;
	Resolution.Z = (int32)FMath::RoundHalfFromZero(MaxBound.Z / WFCModel->TileSize) - (int32)FMath::RoundHalfFromZero(MinBound.Z / WFCModel->TileSize) + 1;

	// Set WFC OriginLocation
	FVector ReorientedMinPoint;
	ReorientedMinPoint.X = ((MinBound.X - (WFCModel->TileSize * 0.5f)) * CosOrientation) - ((MinBound.Y - (WFCModel->TileSize * 0.5f)) * -SinOrientation);
	ReorientedMinPoint.Y = ((MinBound.X - (WFCModel->TileSize * 0.5f)) * -SinOrientation) + ((MinBound.Y - (WFCModel->TileSize * 0.5f)) * CosOrientation);
	ReorientedMinPoint.Z = MinBound.Z - (WFCModel->TileSize * 0.5f);
	OriginLocation = ReorientedMinPoint + TransformPivot;

	// Set WFC Orientation
	Orientation = FRotator(0, TransformOrientation, 0);

	// Set WFC Starter Options
	FIntVector PositionOffset;
	PositionOffset.X = (int32)FMath::RoundHalfFromZero(MinBound.X / WFCModel->TileSize);
	PositionOffset.Y = (int32)FMath::RoundHalfFromZero(MinBound.Y / WFCModel->TileSize);
	PositionOffset.Z = (int32)FMath::RoundHalfFromZero(MinBound.Z / WFCModel->TileSize);
	StarterOptions.Empty();
	for (int32 z = 0; z < Resolution.Z; z++)
	{
		for (int32 y = 0; y < Resolution.Y; y++)
		{
			for (int32 x = 0; x < Resolution.X; x++)
			{
				if (!PointPositions.Contains(FIntVector(x + PositionOffset.X, y + PositionOffset.Y, z + PositionOffset.Z)))
				{
					StarterOptions.Add(FIntVector(x, y, z), FWaveFunctionCollapseOptionCustom::EmptyOption);
				}
			}
		}
	}
	for (TPair<FIntVector, FWaveFunctionCollapseOptionCustom> StarterOption : StarterOptions)
	{
		UE_LOG(LogWFC, Display, TEXT("StartOption at: %d,%d,%d"), StarterOption.Key.X, StarterOption.Key.Y, StarterOption.Key.Z);
	}
}

void UWaveFunctionCollapseSubsystem02::ExecuteWFC(int32 TryCount, int32 RandomSeed)
{
	

	OriginLocation = FVector(0.0f, 0.0f, 0.0f);
	Orientation = FRotator(0.0f, 0.0f, 0.0f);
	bUseEmptyBorder = false;

	// WFC Collapse 실행
	AActor* ResultActor = CollapseCustom(TryCount, RandomSeed);
	if (ResultActor)
	{
		UE_LOG(LogWFC, Display, TEXT("Successfully collapsed WFC and spawned actor: %s"), *ResultActor->GetName());
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("WFC collapse failed."));
	}
}

void UWaveFunctionCollapseSubsystem02::SetWFCModel()
{
	// zxzx29 모델의 경로
	const FStringAssetReference ModelPath(TEXT("/Game/WFCCORE/zxzx29.zxzx29"));

	// 모델 로드
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	UWaveFunctionCollapseModel02* LoadedModel = Cast<UWaveFunctionCollapseModel02>(StreamableManager.LoadSynchronous(ModelPath));

	if (LoadedModel)
	{
		WFCModel = LoadedModel;
		UE_LOG(LogWFC, Log, TEXT("WFCModel이 zxzx29로 설정되었습니다."));
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("WFCModel 로드 실패: %s"), *ModelPath.ToString());
	}
}

void UWaveFunctionCollapseSubsystem02::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// zxzx29 모델 설정
	SetWFCModel();
}