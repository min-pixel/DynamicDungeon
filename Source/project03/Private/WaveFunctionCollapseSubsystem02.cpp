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

		


		// 루프 내부에서 SelectedOption 및 TileIndex를 참조
		for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
		{
			if (Tiles[TileIndex].RemainingOptions.Num() == 1)
			{
				FWaveFunctionCollapseOptionCustom& SelectedOption = Tiles[TileIndex].RemainingOptions[0];

				// 방 타일 처리
				if (SelectedOption.bIsRoomTile)
				{
					float TileSize = WFCModel->TileSize * 3.0f; // 방 타일 크기
					FVector RoomTilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * WFCModel->TileSize;

					// **겹침 여부 확인**
					bool bOverlapsOtherRoomTile = false;
					for (const FVector& ExistingRoomTilePosition : RoomTilePositions)
					{
						// 이미 존재하는 방 타일과 겹치는 경우
						if (FVector::DistSquared(RoomTilePosition, ExistingRoomTilePosition) < FMath::Square(TileSize))
						{
							bOverlapsOtherRoomTile = true;
							break;
						}
					}

					// 겹치는 경우 해당 방 타일만 제거하고 다른 타일 유지
					if (bOverlapsOtherRoomTile)
					{
						UE_LOG(LogWFC, Display, TEXT("Room tile at (%s) overlaps with existing room, removing room tile but keeping other options."),
							*RoomTilePosition.ToString());

						// 대체 타일 설정
						FWaveFunctionCollapseOptionCustom AlternativeTileOption(TEXT("/Game/BP/t03-back.t03-back")); // 대체 타일 경로
						Tiles[TileIndex].RemainingOptions.Empty();
						Tiles[TileIndex].RemainingOptions.Add(AlternativeTileOption);

						// ShannonEntropy 재계산
						Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
							Tiles[TileIndex].RemainingOptions,
							WFCModel
						);

						continue; // 다음 타일로 이동
					}

					// **겹치지 않는 경우 처리**
					RoomTilePositions.Add(RoomTilePosition); // 방 타일 위치 저장

					// 방 타일의 경계 계산
					FVector MinBound = RoomTilePosition - FVector(TileSize * 1.5f, TileSize * 1.5f, TileSize * 1.5f);
					FVector MaxBound = RoomTilePosition + FVector(TileSize * 0.4f, TileSize * 0.4f, TileSize * 0.4f);

					// 모든 인접 타일 탐색 및 삭제
					TArray<int32> AdjacentIndices = GetAdjacentIndices(TileIndex, Resolution);
					for (int32 AdjacentIndex : AdjacentIndices)
					{
						FVector AdjacentTilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(AdjacentIndex, Resolution)) * WFCModel->TileSize;

						// 타일이 방 타일의 경계 내부에 있는지 확인
						if (AdjacentTilePosition.X >= MinBound.X && AdjacentTilePosition.X <= MaxBound.X &&
							AdjacentTilePosition.Y >= MinBound.Y && AdjacentTilePosition.Y <= MaxBound.Y &&
							AdjacentTilePosition.Z >= MinBound.Z && AdjacentTilePosition.Z <= MaxBound.Z)
						{
							// 경계 내부에 있는 타일만 삭제
							Tiles[AdjacentIndex].RemainingOptions.Empty();
							Tiles[AdjacentIndex].ShannonEntropy = 0.0f;

							UE_LOG(LogWFC, Display, TEXT("Removed overlapping tile inside room boundary at index: %d"), AdjacentIndex);
						}
					}

				

					// 방 타일 크기 업데이트
					SelectedOption.BaseScale3D = FVector(3.0f); // 스케일 반영
				}
			}
		}
		
		
		// 고립된 타일 제거
		RemoveIsolatedCorridorTiles(Tiles);

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

	// 테두리 위치 오프셋 설정
	FVector Offset = FVector(0.0f, 0.0f, 0.0f); // 가로, 세로, 높이 조정

	for (int32 Z = 0; Z < Resolution.Z; Z++)
	{
		for (int32 Y = 0; Y <= Resolution.Y-1; Y++)
		{
			for (int32 X = 0; X <= Resolution.X-1; X++)
			{
				FIntVector Position(X, Y, Z);

				FVector SpawnLocation = FVector(Position) * WFCModel->TileSize;

				// 방 타일과 겹치는지 확인
				bool bOverlapsRoomTile = false;
				for (const FVector& RoomPosition : RoomTilePositions)
				{
					if (FVector::DistSquared(SpawnLocation, RoomPosition) < FMath::Square(WFCModel->TileSize * 2.0f))
					{
						bOverlapsRoomTile = true;
						break;
					}
				}

				// 겹치면 스폰을 건너뜀
				if (bOverlapsRoomTile)
				{
					UE_LOG(LogWFC, Display, TEXT("Skipping Border Actor spawn due to overlap with Room Tile at (%d, %d, %d)"), X, Y, Z);
					continue;
				}



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

					// 기존 SpawnLocation 값을 수정
					SpawnLocation += PositionOffset;

					// 조정된 위치에 블루프린트 액터를 스폰
					if (SelectedBPClass && GetWorld())
					{
						//FVector SpawnLocation = FVector(Position) * WFCModel->TileSize + PositionOffset;
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
					if (X == 0 && Y == 0 && Z == 0 || (X == 0 && Y == 1 && Z == 0) || (X == 0 && Y == 2 && Z == 0))
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


	// UWorld 참조 확인
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogWFC, Error, TEXT("World is null, cannot spawn actors."));
		return nullptr;
	}

	// 최상위 부모 액터 생성
	FActorSpawnParameters SpawnParams;
	AActor* SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), OriginLocation, Orientation, SpawnParams);
	if (!SpawnedActor)
	{
		UE_LOG(LogWFC, Error, TEXT("Failed to spawn actor."));
		return nullptr;
	}

	// 부모 액터 이름 설정
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
	for (int32 Index = 0; Index < Tiles.Num(); Index++)
	{
		// 빈 타일 무시
		if (Tiles[Index].RemainingOptions.IsEmpty())
		{
			UE_LOG(LogWFC, Display, TEXT("Skipped empty tile at index: %d"), Index);
			continue;
		}

		const FWaveFunctionCollapseOptionCustom& Option = Tiles[Index].RemainingOptions[0];

		// Empty, Void 타일 및 제외된 타일 무시
		if (Option.BaseObject == FWaveFunctionCollapseOptionCustom::EmptyOption.BaseObject
			|| Option.BaseObject == FWaveFunctionCollapseOptionCustom::VoidOption.BaseObject
			|| WFCModel->SpawnExclusion.Contains(Option.BaseObject))
		{
			continue;
		}
		
		UObject* LoadedObject = Option.BaseObject.TryLoad();
		if (LoadedObject)
		{
			FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution)) * WFCModel->TileSize;
			FRotator TileRotation = Option.BaseRotator;
			FVector TileScale = Option.BaseScale3D;

			// Static Mesh 처리
			if (UStaticMesh* LoadedStaticMesh = Cast<UStaticMesh>(LoadedObject))
			{
				UInstancedStaticMeshComponent* ISMComponent;
				if (UInstancedStaticMeshComponent** FoundISMComponentPtr = BaseObjectToISM.Find(Option.BaseObject))
				{
					ISMComponent = *FoundISMComponentPtr;
				}
				else
				{
					ISMComponent = Cast<UInstancedStaticMeshComponent>(
						AddNamedInstanceComponent(SpawnedActor, UInstancedStaticMeshComponent::StaticClass(), LoadedObject->GetFName()));
					BaseObjectToISM.Add(Option.BaseObject, ISMComponent);
				}
				ISMComponent->SetStaticMesh(LoadedStaticMesh);
				ISMComponent->AddInstance(FTransform(TileRotation, TilePosition, TileScale));
			}
			// Blueprint 처리
			else if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(LoadedObject))
			{
				TSubclassOf<AActor> ActorClass = *LoadedBlueprint->GeneratedClass;

				UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(SpawnedActor, UChildActorComponent::StaticClass());
				ChildActorComponent->SetupAttachment(SpawnedActor->GetRootComponent());
				ChildActorComponent->RegisterComponent();
				ChildActorComponent->SetChildActorClass(ActorClass);
				ChildActorComponent->SetRelativeLocation(TilePosition);
				ChildActorComponent->SetRelativeRotation(TileRotation);
				ChildActorComponent->SetRelativeScale3D(TileScale);
				SpawnedActor->AddInstanceComponent(ChildActorComponent);
			}
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
	// zxzx34 모델의 경로
	const FStringAssetReference ModelPath(TEXT("/Game/WFCCORE/zxzx34.zxzx34"));

	// 모델 로드
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	UWaveFunctionCollapseModel02* LoadedModel = Cast<UWaveFunctionCollapseModel02>(StreamableManager.LoadSynchronous(ModelPath));

	if (LoadedModel)
	{
		WFCModel = LoadedModel;
		UE_LOG(LogWFC, Log, TEXT("WFCModel이 zxzx34로 설정되었습니다."));
	}
	else
	{
		UE_LOG(LogWFC, Error, TEXT("WFCModel 로드 실패: %s"), *ModelPath.ToString());
	}
}

void UWaveFunctionCollapseSubsystem02::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// zxzx34 모델 설정
	SetWFCModel();
}

TArray<int32> UWaveFunctionCollapseSubsystem02::GetAdjacentIndices(int32 TileIndex, FIntVector GridResolution)
{
	TArray<int32> AdjacentIndices;

	FIntVector Position = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, this->Resolution);

	// 모든 방향의 오프셋 (3D 격자)
	TArray<FIntVector> Offsets = {
		FIntVector(-1, 0, 0), FIntVector(1, 0, 0),  // X축
		FIntVector(0, -1, 0), FIntVector(0, 1, 0),  // Y축
		FIntVector(-1, -1, 0), FIntVector(1, 1, 0), // 대각선
		FIntVector(-1, 1, 0), FIntVector(1, -1, 0), // 대각선
	};

	for (const FIntVector& Offset : Offsets)
	{
		FIntVector NeighborPosition = Position + Offset;
		if (NeighborPosition.X >= 0 && NeighborPosition.X < this->Resolution.X &&
			NeighborPosition.Y >= 0 && NeighborPosition.Y < this->Resolution.Y &&
			NeighborPosition.Z >= 0 && NeighborPosition.Z < this->Resolution.Z)
		{
			AdjacentIndices.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPosition, this->Resolution));
		}
	}

	return AdjacentIndices;
}


void UWaveFunctionCollapseSubsystem02::RemoveIsolatedCorridorTiles(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		// 현재 타일이 비어있으면 건너뜀
		if (Tiles[TileIndex].RemainingOptions.IsEmpty())
		{
			continue;
		}

		// 현재 타일의 옵션 가져오기
		const FWaveFunctionCollapseOptionCustom& CurrentOption = Tiles[TileIndex].RemainingOptions[0];

		// 방 타일은 삭제 대상에서 제외
		if (CurrentOption.bIsRoomTile)
		{
			continue;
		}

		// 4방향 인접 타일 검사
		TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, this->Resolution);
		bool bAllAdjacentAreNotCorridor = true; // 인접 타일이 모두 복도 타일이 아닌지 확인

		for (int32 AdjacentIndex : AdjacentIndices)
		{
			// 인접 타일이 유효한지 확인
			if (!Tiles.IsValidIndex(AdjacentIndex))
			{
				continue;
			}

			// 인접 타일의 RemainingOptions이 비어있지 않은지 확인
			if (Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
			{
				continue;
			}

			// 인접 타일의 옵션 가져오기
			const FWaveFunctionCollapseOptionCustom& AdjacentOption = Tiles[AdjacentIndex].RemainingOptions[0];

			// 복도 타일이 하나라도 있으면 삭제 대상이 아님
			if (AdjacentOption.bIsCorridorTile)
			{
				bAllAdjacentAreNotCorridor = false;
				break;
			}
		}

		// 인접 타일이 모두 복도 타일이 아닌 경우 현재 타일 삭제
		if (bAllAdjacentAreNotCorridor)
		{
			Tiles[TileIndex].RemainingOptions.Empty();
			Tiles[TileIndex].ShannonEntropy = 0.0f;

			UE_LOG(LogWFC, Display, TEXT("Removed isolated corridor tile at index: %d"), TileIndex);
		}
	}
}

TArray<int32> UWaveFunctionCollapseSubsystem02::GetCardinalAdjacentIndices(int32 TileIndex, FIntVector GridResolution)
{
	TArray<int32> AdjacentIndices;

	FIntVector Position = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, GridResolution);

	// 4방향 오프셋
	TArray<FIntVector> Offsets = {
		FIntVector(-1, 0, 0), // 왼쪽
		FIntVector(1, 0, 0),  // 오른쪽
		FIntVector(0, -1, 0), // 아래
		FIntVector(0, 1, 0)   // 위
	};

	for (const FIntVector& Offset : Offsets)
	{
		FIntVector NeighborPosition = Position + Offset;

		if (NeighborPosition.X >= 0 && NeighborPosition.X < GridResolution.X &&
			NeighborPosition.Y >= 0 && NeighborPosition.Y < GridResolution.Y &&
			NeighborPosition.Z >= 0 && NeighborPosition.Z < GridResolution.Z)
		{
			AdjacentIndices.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPosition, GridResolution));
		}
	}

	return AdjacentIndices;
}
 


