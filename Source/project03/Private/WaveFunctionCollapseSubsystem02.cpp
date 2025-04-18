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
#include <queue>

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

	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions.Num(): %d"), UserFixedOptions.Num());

	// StarterOptions 적용  20250417
	//for (const auto& Entry : UserFixedOptions)
	//{
	//	const FIntVector& Coord = Entry.Key;
	//	const FWaveFunctionCollapseOptionCustom& FixedOption = Entry.Value;

	//	int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

	//	if (Tiles.IsValidIndex(Index))
	//	{
	//		Tiles[Index].RemainingOptions.Empty();
	//		Tiles[Index].RemainingOptions.Add(FixedOption);

	//		// ShannonEntropy 재계산
	//		Tiles[Index].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
	//			Tiles[Index].RemainingOptions,
	//			WFCModel
	//		);

	//		UE_LOG(LogTemp, Warning, TEXT("StarterOption applied at (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Invalid StarterOption index for (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);
	//	}
	//}


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
		RemoveIsolatedCorridorTiles(Tiles);
		RemoveDisconnectedCorridors(Tiles);
		
		
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
						if (FVector::DistSquared(RoomTilePosition, ExistingRoomTilePosition) < FMath::Square(TileSize*1.5f))
						{
							bOverlapsOtherRoomTile = true;
							break;
						}
					}
					//20250418
					FIntVector GridCoord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
					bool bIsUserFixed = UserFixedOptions.Contains(GridCoord);

					if (bOverlapsOtherRoomTile && !bIsUserFixed)
					{
						// 원래 코드 유지: goalt01로 바꾸고 고립 검사까지
						UE_LOG(LogWFC, Display, TEXT("Room tile at (%s) overlaps with existing room, removing room tile but keeping other options."),
							*RoomTilePosition.ToString());

						// 대체 타일 설정
						FWaveFunctionCollapseOptionCustom AlternativeTileOption(TEXT("/Game/BP/goalt01.goalt01")); // 대체 타일 경로
						Tiles[TileIndex].RemainingOptions.Empty();
						Tiles[TileIndex].RemainingOptions.Add(AlternativeTileOption);



						// ShannonEntropy 재계산
						Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
							Tiles[TileIndex].RemainingOptions,
							WFCModel
						);

						// **여기서 goalt01 타일이 고립되었는지 검사!**
						if (IsIsolatedTile(TileIndex, Tiles))  // 함수로 고립 체크
						{
							UE_LOG(LogWFC, Display, TEXT("Isolated goalt01 tile at (%s), replacing or removing."), *RoomTilePosition.ToString());

							// 삭제할 경우
							 /*Tiles[TileIndex].RemainingOptions.Empty();
							 Tiles[TileIndex].ShannonEntropy = 0.0f; */

							 // 다른 타일로 대체할 경우 (예: /Game/WFCCORE/wfc/SpecialOption/Option_Empty 타일)
							FWaveFunctionCollapseOptionCustom ReplacementTile(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
							Tiles[TileIndex].RemainingOptions.Empty();
							Tiles[TileIndex].RemainingOptions.Add(ReplacementTile);

							// ShannonEntropy 다시 계산
							Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
								Tiles[TileIndex].RemainingOptions,
								WFCModel
							);
						}



						continue; // 다음 타일로 이동

					}
					//20250418

					// //겹치는 경우 해당 방 타일만 제거하고 다른 타일 유지
					//if (bOverlapsOtherRoomTile)
					//{
					//	UE_LOG(LogWFC, Display, TEXT("Room tile at (%s) overlaps with existing room, removing room tile but keeping other options."),
					//		*RoomTilePosition.ToString());

					//	// 대체 타일 설정
					//	FWaveFunctionCollapseOptionCustom AlternativeTileOption(TEXT("/Game/BP/goalt01.goalt01")); // 대체 타일 경로
					//	Tiles[TileIndex].RemainingOptions.Empty();
					//	Tiles[TileIndex].RemainingOptions.Add(AlternativeTileOption);



					//	// ShannonEntropy 재계산
					//	Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
					//		Tiles[TileIndex].RemainingOptions,
					//		WFCModel
					//	);

					//	// **여기서 goalt01 타일이 고립되었는지 검사!**
					//	if (IsIsolatedTile(TileIndex, Tiles))  // 함수로 고립 체크
					//	{
					//		UE_LOG(LogWFC, Display, TEXT("Isolated goalt01 tile at (%s), replacing or removing."), *RoomTilePosition.ToString());

					//		// 삭제할 경우
					//		 /*Tiles[TileIndex].RemainingOptions.Empty();
					//		 Tiles[TileIndex].ShannonEntropy = 0.0f; */

					//		// 다른 타일로 대체할 경우 (예: /Game/WFCCORE/wfc/SpecialOption/Option_Empty 타일)
					//		FWaveFunctionCollapseOptionCustom ReplacementTile(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
					//		Tiles[TileIndex].RemainingOptions.Empty();
					//		Tiles[TileIndex].RemainingOptions.Add(ReplacementTile);

					//		// ShannonEntropy 다시 계산
					//		Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
					//			Tiles[TileIndex].RemainingOptions,
					//			WFCModel
					//		);
					//	}



					//	continue; // 다음 타일로 이동
					//}
					  
					

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
				
					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);

					ConnectIsolatedRooms(Tiles);

					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);

					// 방 타일 앞에 goalt01을 배치하는 함수 실행
					PlaceGoalTileInFrontOfRoom(TileIndex, Tiles);

					
				}
			}
		}
		

		// 성공한 타일 데이터를 저장
		LastCollapsedTiles = Tiles;

		AActor* SpawnedActor = SpawnActorFromTiles(Tiles);

		if (SpawnedActor)
		{
			SpawnedActor->Tags.Add(FName("WFCGenerated"));
		}

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

const TArray<FWaveFunctionCollapseTileCustom>& UWaveFunctionCollapseSubsystem02::GetLastCollapsedTiles() const
{
	return LastCollapsedTiles;
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
					//// 첫 타일(0,0,0) 고정 로직 추가, 02/12 테스트를 위해 추가함.
					//if (X == 1 && Y == 0 && Z == 0)
					//{
					//	// 고정 옵션 설정
					//	FWaveFunctionCollapseOptionCustom FixedOption(TEXT("/Game/BP/romm.romm"));
					//	FWaveFunctionCollapseTileCustom FixedTile;
					//	FixedTile.RemainingOptions.Add(FixedOption);

					//	// Shannon 엔트로피 고정 (필요시 다른 값 설정 가능)
					//	FixedTile.ShannonEntropy = 0.0f;

					//	Tiles.Add(FixedTile);
					//	RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));

					//	// ObservationPropagation 단계에서 수정되지 않도록 RemainingTiles에서 제거
					//	RemainingTiles.RemoveAt(RemainingTiles.Num() - 1, 1, false);
					//	continue;
					//}

					//// 첫 타일(0,0,0) 고정 로직 추가
					//if (X == 2 && Y == 0 && Z == 0 || X == 0 && Y == 2 && Z == 0 || X == 3 && Y == 0 && Z == 0 || X == 0 && Y == 3 && Z == 0 || X == 0 && Y == 0 && Z == 0 || X == 0 && Y == 1 && Z == 0 || X == 1 && Y == 1 && Z == 0)
					//{
					//	// 고정 옵션 설정
					//	FWaveFunctionCollapseOptionCustom FixedOption(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
					//	FWaveFunctionCollapseTileCustom FixedTile;
					//	FixedTile.RemainingOptions.Add(FixedOption);

					//	// Shannon 엔트로피 고정 (필요시 다른 값 설정 가능)
					//	FixedTile.ShannonEntropy = 0.0f;

					//	Tiles.Add(FixedTile);
					//	RemainingTiles.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector(X, Y, Z), Resolution));

					//	// ObservationPropagation 단계에서 수정되지 않도록 RemainingTiles에서 제거
					//	RemainingTiles.RemoveAt(RemainingTiles.Num() - 1, 1, false);
					//	continue;
					//}



					// Pre-populate with starter tiles
					//20250417
					FIntVector CurrentCoord(X, Y, Z);
					if (FWaveFunctionCollapseOptionCustom* FixedOption = UserFixedOptions.Find(FIntVector(X, Y, Z)))
					{
						FWaveFunctionCollapseTileCustom FixedTile;
						FixedTile.RemainingOptions.Add(*FixedOption);
						FixedTile.ShannonEntropy = 0.0f;

						Tiles.Add(FixedTile);

						UE_LOG(LogTemp, Warning, TEXT("ioioio: (%d, %d, %d) → %s"),
							X, Y, Z, *FixedOption->BaseObject.ToString());

						// RemainingTiles에 추가 안 함 → 완전 고정
						continue;
					}
					else if (FWaveFunctionCollapseOptionCustom* StarterOption = StarterOptions.Find(FIntVector(X, Y, Z)))
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

				FIntVector LogicalCoord(x, y, z);
				FIntVector WorldCoord = LogicalCoord + PositionOffset;

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
 

void UWaveFunctionCollapseSubsystem02::RemoveDisconnectedCorridors(TArray<FWaveFunctionCollapseTileCustom>& Tiles) {
	TSet<int32> VisitedTiles;

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex) {
		if (VisitedTiles.Contains(TileIndex) || Tiles[TileIndex].RemainingOptions.IsEmpty()) {
			continue;
		}

		const FWaveFunctionCollapseOptionCustom& CurrentOption = Tiles[TileIndex].RemainingOptions[0];

		// 복도 타일이 아니면 스킵 (룸 타일 포함)
		if (!CurrentOption.bIsCorridorTile) {
			continue;
		}

		// 복도 그룹 탐색
		TSet<int32> CorridorGroup;
		FloodFillCorridors(TileIndex, Tiles, CorridorGroup, VisitedTiles);

		// 복도 그룹 크기가 5 미만인 경우 삭제
		if (CorridorGroup.Num() < 30) {
			for (int32 CorridorTileIndex : CorridorGroup) {
				const FWaveFunctionCollapseOptionCustom& TileOption = Tiles[CorridorTileIndex].RemainingOptions[0];

				// 복도 타일만 삭제 (룸 타일은 삭제하지 않음)
				if (TileOption.bIsCorridorTile) {
					Tiles[CorridorTileIndex].RemainingOptions.Empty();
					Tiles[CorridorTileIndex].ShannonEntropy = 0.0f;
				}
			}

			UE_LOG(LogWFC, Display, TEXT("Removed disconnected corridor group with %d tiles"), CorridorGroup.Num());
		}
	}
}

void UWaveFunctionCollapseSubsystem02::FloodFillCorridors(
	int32 StartIndex,
	const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
	TSet<int32>& OutGroup,
	TSet<int32>& VisitedTiles
) {
	TQueue<int32> Queue;
	Queue.Enqueue(StartIndex);

	while (!Queue.IsEmpty()) {
		int32 CurrentIndex;
		Queue.Dequeue(CurrentIndex);

		// 이미 방문한 타일은 건너뜀
		if (VisitedTiles.Contains(CurrentIndex)) {
			continue;
		}

		VisitedTiles.Add(CurrentIndex);

		const FWaveFunctionCollapseOptionCustom& CurrentOption = Tiles[CurrentIndex].RemainingOptions[0];

		// 복도 타일만 그룹에 포함
		if (CurrentOption.bIsCorridorTile) {
			OutGroup.Add(CurrentIndex);
		}
		else if (CurrentOption.bIsRoomTile) {
			continue;  // 방 타일은 탐색에서 완전히 배제
		}
		else {
			continue;  // 기타 타일도 제외
		}

		// 인접 타일 탐색
		TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(CurrentIndex, Resolution);
		for (int32 AdjacentIndex : AdjacentIndices) {
			if (!VisitedTiles.Contains(AdjacentIndex) &&
				Tiles.IsValidIndex(AdjacentIndex) &&
				!Tiles[AdjacentIndex].RemainingOptions.IsEmpty()) {

				const FWaveFunctionCollapseOptionCustom& AdjacentOption = Tiles[AdjacentIndex].RemainingOptions[0];

				// 복도 타일만 큐에 추가
				if (AdjacentOption.bIsCorridorTile) {
					Queue.Enqueue(AdjacentIndex);
				}
			}
		}
	}
}


void UWaveFunctionCollapseSubsystem02::AdjustRoomTileBasedOnCorridors(int32 TileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	if (!Tiles.IsValidIndex(TileIndex) || Tiles[TileIndex].RemainingOptions.Num() != 1)
	{
		UE_LOG(LogWFC, Warning, TEXT("Invalid TileIndex or RemainingOptions for TileIndex: %d"), TileIndex);
		return;
	}

	FWaveFunctionCollapseOptionCustom& RoomTileOption = Tiles[TileIndex].RemainingOptions[0];

	// 방 타일이 아닌 경우 스킵
	if (!RoomTileOption.bIsRoomTile)
	{
		return;
	}

	// 타일의 현재 위치 가져오기
	FIntVector TilePosition = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);

	
	bool bIsCompletelyIsolated = true;
	TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, Resolution);
	for (int32 AdjacentIndex : AdjacentIndices)
	{
		if (Tiles.IsValidIndex(AdjacentIndex) && !Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
		{
			bIsCompletelyIsolated = false;
			break;
		}
	}

	// 방 타일의 초기 문 방향 확인
	TSet<FString> InitialDoorDirections;
	if (RoomTileOption.bHasDoorNorth) InitialDoorDirections.Add(TEXT("North"));
	if (RoomTileOption.bHasDoorSouth) InitialDoorDirections.Add(TEXT("South"));
	if (RoomTileOption.bHasDoorEast) InitialDoorDirections.Add(TEXT("East"));
	if (RoomTileOption.bHasDoorWest) InitialDoorDirections.Add(TEXT("West"));

	// 방향별 우선순위 초기화
	TMap<FString, int32> DirectionPriorities = {
		{TEXT("North"), 0},
		{TEXT("South"), 0},
		{TEXT("East"), 0},
		{TEXT("West"), 0}
	};



	// 금지된 방향 설정을 위한 TSet
	TSet<FString> ForbiddenDirections;

	// "테두리 및 내부 한 줄" 처리
	bool bIsOuterBorder = (TilePosition.X == 0 || TilePosition.X == Resolution.X - 1 ||
		TilePosition.Y == 0 || TilePosition.Y == Resolution.Y - 1);

	bool bIsInnerBorder = (TilePosition.X == 1 || TilePosition.X == Resolution.X - 2 ||
		TilePosition.Y == 1 || TilePosition.Y == Resolution.Y - 2);

	// 금지된 방향을 추가
	if (bIsOuterBorder || bIsInnerBorder)
	{
		if (TilePosition.Y == Resolution.Y - 1 || TilePosition.Y == Resolution.Y - 2)
		{
			ForbiddenDirections.Add(TEXT("East"));
		}
		if (TilePosition.Y == 0 || TilePosition.Y == 1)
		{
			ForbiddenDirections.Add(TEXT("West"));
		}
		if (TilePosition.X == Resolution.X - 1 || TilePosition.X == Resolution.X - 2)
		{
			ForbiddenDirections.Add(TEXT("North"));
		}
		if (TilePosition.X == 0 || TilePosition.X == 1)
		{
			ForbiddenDirections.Add(TEXT("South"));
		}
	}


	// 방향별 오프셋
	TArray<FIntVector> DirectionOffsets = {
		FIntVector(2, 0, 0),   // 북쪽
		FIntVector(-2, 0, 0),  // 남쪽
		FIntVector(0, 2, 0),   // 동쪽
		FIntVector(0, -2, 0)   // 서쪽
	};

	TArray<FString> DirectionNames = { TEXT("North"), TEXT("South"), TEXT("East"), TEXT("West") };

	// 금지된 타일 목록
	TArray<FString> ForbiddenTiles = {
		FString(TEXT("/Game/BP/t03-back")),
		FString(TEXT("/Game/BP/t03-back_B")),
		FString(TEXT("/Game/BP/t03-back_L")),
		FString(TEXT("/Game/BP/t03-back_R")),
		FString(TEXT("/Game/BP/goalt01")),
		FString(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"))
	};


	// 1. 각 방향에 대해 검사
	for (int32 i = 0; i < DirectionOffsets.Num(); ++i)
	{
		FIntVector NeighborPosition = TilePosition + DirectionOffsets[i];
		int32 NeighborIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPosition, Resolution);

		// "아무 타일도 없는 방향" 처리
		if (!Tiles.IsValidIndex(NeighborIndex) || Tiles[NeighborIndex].RemainingOptions.IsEmpty())
		{
			// 해당 방향은 우선순위를 매우 낮게 설정하여 절대 선택되지 않게 함
			DirectionPriorities[DirectionNames[i]] = -50;
			UE_LOG(LogWFC, Verbose, TEXT("Direction %s has no valid tile for tile at (%d, %d, %d)."),
				*DirectionNames[i], TilePosition.X, TilePosition.Y, TilePosition.Z);
			continue;
		}	

		// 유효한 타일인지 확인
		if (Tiles.IsValidIndex(NeighborIndex) && !Tiles[NeighborIndex].RemainingOptions.IsEmpty())
		{
			const FWaveFunctionCollapseOptionCustom& NeighborOption = Tiles[NeighborIndex].RemainingOptions[0];

			// 금지된 방향인지 확인
			if (ForbiddenDirections.Contains(DirectionNames[i]))
			{
				DirectionPriorities[DirectionNames[i]] = -30; // 금지된 방향은 우선순위를 매우 낮게 설정
				continue;
			}

			// 1. 주변 타일이 복도 타일인가?
			if (NeighborOption.bIsCorridorTile)
			{
				DirectionPriorities[DirectionNames[i]] = 20;
				continue;
			}

			// 2. 주변 타일이 금지된 타일인가?
			if (ForbiddenTiles.Contains(NeighborOption.BaseObject.ToString()))
			{
				DirectionPriorities[DirectionNames[i]] = -30;
				continue;
			}
		}
	}
	
	// 초기 문 방향이 주변 복도와 이미 연결되어 있다면 회전하지 않음
	bool bConnectedToCorridor = false;
	for (const FString& DoorDirection : InitialDoorDirections)
	{
		if (DirectionPriorities.Contains(DoorDirection) && DirectionPriorities[DoorDirection] > 0)
		{
			bConnectedToCorridor = true;
			break;
		}
	}

	// 복도가 연결되지 않은 경우 강제 회전
	if (!bConnectedToCorridor)
	{
		UE_LOG(LogWFC, Verbose, TEXT("Room tile at (%d, %d, %d) not connected to any corridor. Forcing rotation."),
			TilePosition.X, TilePosition.Y, TilePosition.Z);
	}
	else
	{
		// 복도와 연결되어 있으면 회전하지 않음
		UE_LOG(LogWFC, Verbose, TEXT("Room tile at (%d, %d, %d) already connected to corridor."),
			TilePosition.X, TilePosition.Y, TilePosition.Z);
		return;
	}


	FString BestDirection;
	int32 MaxPriority = 0;
	TArray<FString> EqualPriorityDirections;

	for (const auto& Direction : DirectionPriorities)
	{
		// 우선순위가 더 높은 방향이 나오면 기존 리스트 초기화
		if (Direction.Value > MaxPriority)
		{
			MaxPriority = Direction.Value;
			EqualPriorityDirections.Empty(); // 기존 저장된 방향 제거
			EqualPriorityDirections.Add(Direction.Key);
		}
		// 우선순위가 같고 양수인 경우 리스트에 추가
		else if (Direction.Value == MaxPriority && MaxPriority > 0)
		{
			EqualPriorityDirections.Add(Direction.Key);
		}
	}

	// 우선순위가 양수인 방향이 없으면 회전하지 않음
	if (MaxPriority <= 0)
	{
		UE_LOG(LogWFC, Warning, TEXT("No valid positive priority direction for room tile at (%d, %d, %d). Skipping rotation."),
			TilePosition.X, TilePosition.Y, TilePosition.Z);
		return;
	}

	BestDirection = EqualPriorityDirections.Last();

	// 회전 실행
	UE_LOG(LogWFC, Verbose, TEXT("Rotating room tile at (%d, %d, %d) to best direction: %s."),
		TilePosition.X, TilePosition.Y, TilePosition.Z, *BestDirection);
	RotateRoomTile(RoomTileOption, BestDirection);

	//// 방 타일이 회전한 후, 문이 있는 방향의 위치에 "goalt01" 배치
	//FIntVector GoalTilePosition = TilePosition;

	//if (RoomTileOption.bHasDoorNorth)
	//{
	//	GoalTilePosition.X += 2; // 북쪽으로 한 칸 띄운 위치
	//}
	//else if (RoomTileOption.bHasDoorSouth)
	//{
	//	GoalTilePosition.X -= 2; // 남쪽으로 한 칸 띄운 위치
	//}
	//else if (RoomTileOption.bHasDoorEast)
	//{
	//	GoalTilePosition.Y += 2; // 동쪽으로 한 칸 띄운 위치
	//}
	//else if (RoomTileOption.bHasDoorWest)
	//{
	//	GoalTilePosition.Y -= 2; // 서쪽으로 한 칸 띄운 위치
	//}

	//// 해당 위치가 유효한지 확인
	//int32 GoalTileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(GoalTilePosition, Resolution);
	//if (Tiles.IsValidIndex(GoalTileIndex) && !Tiles[GoalTileIndex].RemainingOptions.IsEmpty())
	//{
	//	UE_LOG(LogWFC, Display, TEXT("Replacing tile at (%d, %d, %d) with goalt01"),
	//		GoalTilePosition.X, GoalTilePosition.Y, GoalTilePosition.Z);

	//	// 기존 옵션을 제거하고 goalt01을 추가
	//	Tiles[GoalTileIndex].RemainingOptions.Empty();
	//	FWaveFunctionCollapseOptionCustom GoalTileOption(TEXT("/Game/BP/goalt01.goalt01"));
	//	Tiles[GoalTileIndex].RemainingOptions.Add(GoalTileOption);

	//	// 새로운 타일의 ShannonEntropy 업데이트
	//	Tiles[GoalTileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
	//		Tiles[GoalTileIndex].RemainingOptions, WFCModel);
	//}
}



void UWaveFunctionCollapseSubsystem02::RotateRoomTile(FWaveFunctionCollapseOptionCustom& RoomTileOption, const FString& TargetDirection)
{
	FString InitialDirection;

	// 초기 문 방향 확인
	if (RoomTileOption.bHasDoorNorth)
	{
		InitialDirection = TEXT("North");
	}
	else if (RoomTileOption.bHasDoorSouth)
	{
		InitialDirection = TEXT("South");
	}
	else if (RoomTileOption.bHasDoorEast)
	{
		InitialDirection = TEXT("East");
	}
	else if (RoomTileOption.bHasDoorWest)
	{
		InitialDirection = TEXT("West");
	}
	else
	{
		// 초기 문 방향이 없는 경우 로깅
		UE_LOG(LogWFC, Warning, TEXT("RoomTileOption has no initial door direction!"));
		return;
	}

	// 초기 방향과 목표 방향의 회전 각도 계산
	TMap<FString, int32> DirectionToAngle = {
		{TEXT("North"), 0},
		{TEXT("East"), 90},
		{TEXT("South"), 180},
		{TEXT("West"), 270}
	};

	int32 InitialAngle = DirectionToAngle[InitialDirection];
	int32 TargetAngle = DirectionToAngle[TargetDirection];

	// 필요한 회전량 계산 (시계 방향 기준)
	int32 RotationAmount = (TargetAngle - InitialAngle + 360) % 360;

	// Rotator 업데이트
	RoomTileOption.BaseRotator = FRotator(0, RotationAmount, 0);

	// 문 방향 업데이트
	RoomTileOption.bHasDoorNorth = (TargetDirection == TEXT("North"));
	RoomTileOption.bHasDoorSouth = (TargetDirection == TEXT("South"));
	RoomTileOption.bHasDoorEast = (TargetDirection == TEXT("East"));
	RoomTileOption.bHasDoorWest = (TargetDirection == TEXT("West"));

	UE_LOG(LogWFC, Verbose, TEXT("Rotated room tile: Initial = %s, Target = %s, Rotation = %d degrees"),
		*InitialDirection, *TargetDirection, RotationAmount);
}
//------------------------------------------------------------------------------------------------------






int32 UWaveFunctionCollapseSubsystem02::FindClosestCorridor(int32 StartIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution)
{
	int32 ClosestCorridorIndex = -1;
	float MinDistance = FLT_MAX;

	TArray<int32> RoomBoundaryIndices = GetRoomBoundaryIndices(StartIndex, Tiles, GridResolution);

	for (int32 BoundaryIndex : RoomBoundaryIndices)
	{
		FIntVector StartPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(BoundaryIndex, GridResolution);

		for (int32 i = 0; i < Tiles.Num(); i++)
		{
			if (!Tiles[i].RemainingOptions.IsEmpty())
			{
				const FWaveFunctionCollapseOptionCustom& TileOption = Tiles[i].RemainingOptions[0];

				// 기존: 복도 타일만 탐색 → 수정: 옵션 엠티 타일도 포함
				if (TileOption.bIsCorridorTile && ( TileOption.BaseObject.ToString() == TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty") || 
					TileOption.BaseObject.ToString() == TEXT("/Game/BP/goalt01.goalt01")))
				{
					FIntVector CorridorPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(i, GridResolution);
					float Distance = FMath::Abs(StartPos.X - CorridorPos.X) + FMath::Abs(StartPos.Y - CorridorPos.Y);

					if (Distance < MinDistance)
					{
						MinDistance = Distance;
						ClosestCorridorIndex = i;
					}
				}
			}
		}
	}

	
	
	if (ClosestCorridorIndex != -1)
	{
		int32 BestCorridorIndex = ClosestCorridorIndex;
		float ShortestPathLength = FLT_MAX;

		UE_LOG(LogWFC, Display, TEXT("Checking alternative corridors for better path"));

		// 기존에 찾은 복도와 다른 복도들을 비교
		for (int32 i = 0; i < Tiles.Num(); i++)
		{
			if (!Tiles[i].RemainingOptions.IsEmpty() && Tiles[i].RemainingOptions[0].bIsCorridorTile)
			{
				FString TilePath = Tiles[i].RemainingOptions[0].BaseObject.ToString();

				
				if (TilePath == TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty") ||
					TilePath == TEXT("/Game/BP/goalt01.goalt01"))
				{
					continue;
				}

				
				TArray<int32> Path = FindPathAStar(StartIndex, i, -1, Tiles, GridResolution);
				float PathLength = Path.Num();

				
				if (PathLength > 0 && PathLength < ShortestPathLength)
				{
					
					ShortestPathLength = PathLength;
					BestCorridorIndex = i;
				}
			}
		}

		
		ClosestCorridorIndex = BestCorridorIndex;
	}

	if (ClosestCorridorIndex == -1)
	{
		UE_LOG(LogWFC, Warning, TEXT("No corridor found for Room at index %d"), StartIndex);
	}

	return ClosestCorridorIndex;
}

TArray<int32> UWaveFunctionCollapseSubsystem02::FindPathAStar(
	int32 StartIndex, int32 GoalIndex, int32 PreviousIndex,
	const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution)
{
	if (StartIndex == GoalIndex)
	{
		return {};
	}

	
	int32 IsolatedRoomIndex = -1;
	TArray<int32> RoomIndices;

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (!Tiles[TileIndex].RemainingOptions.IsEmpty() &&
			Tiles[TileIndex].RemainingOptions[0].bIsRoomTile)
		{
			bool bIsIsolated = true;
			TArray<int32> AdjacentIndices = GetTwoStepAdjacentIndices(TileIndex, GridResolution);

			for (int32 AdjacentIndex : AdjacentIndices)
			{
				if (Tiles.IsValidIndex(AdjacentIndex) && !Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
				{
					if (Tiles[AdjacentIndex].RemainingOptions[0].bIsCorridorTile)
					{
						bIsIsolated = false;
						break;
					}
				}
			}

			if (bIsIsolated)
			{
				RoomIndices.Add(TileIndex);
			}
		}
	}

	
	if (!RoomIndices.IsEmpty())
	{
		float MinDistance = FLT_MAX;
		for (int32 RoomIndex : RoomIndices)
		{
			float Distance = FVector::DistSquared(
				FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(RoomIndex, GridResolution)),
				FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(GoalIndex, GridResolution))
			);

			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				IsolatedRoomIndex = RoomIndex;
			}
		}

		if (IsolatedRoomIndex != -1)
		{
			UE_LOG(LogWFC, Display, TEXT("Using isolated room at index %d as new start index"), IsolatedRoomIndex);
			StartIndex = IsolatedRoomIndex;
		}
	}



	TMap<int32, float> CostSoFar;
	TMap<int32, int32> CameFrom;
	TMap<int32, float> Heuristic;
	TSet<int32> OpenSet;

	CostSoFar.Add(StartIndex, 0.0f);
	Heuristic.Add(StartIndex, 0.0f);
	OpenSet.Add(StartIndex);




	while (!OpenSet.IsEmpty())
	{
		int32 CurrentIndex = -1;
		float MinF = FLT_MAX;

		for (int32 Index : OpenSet)
		{
			float F = CostSoFar[Index] + Heuristic[Index];
			if (F < MinF)
			{
				MinF = F;
				CurrentIndex = Index;
			}
		}

		if (CurrentIndex == GoalIndex)
		{
			TArray<int32> Path;
			int32 TraceIndex = GoalIndex;

			
			if (!CameFrom.Contains(TraceIndex))
			{
				UE_LOG(LogWFC, Warning, TEXT("FindPathAStar: GoalIndex %d not found in CameFrom!"), GoalIndex);
				return {};  // 경로가 실패한 경우 빈 배열 반환
			}

			while (TraceIndex != StartIndex)
			{
				if (!CameFrom.Contains(TraceIndex))  
				{
					UE_LOG(LogWFC, Warning, TEXT("FindPathAStar: TraceIndex %d is missing in CameFrom!"), TraceIndex);
					return {};  // 잘못된 경로 추적 방지
				}

				Path.Add(TraceIndex);
				TraceIndex = CameFrom[TraceIndex];
			}

			Path.Add(StartIndex);
			Algo::Reverse(Path);
			return Path;
		}

		OpenSet.Remove(CurrentIndex);
		TArray<int32> Neighbors = GetCardinalAdjacentIndices(CurrentIndex, GridResolution);

		for (int32 NeighborIndex : Neighbors)
		{
			if (!Tiles.IsValidIndex(NeighborIndex))
			{
				continue;
			}

			bool bIsWalkable = (Tiles[NeighborIndex].RemainingOptions.IsEmpty() ||
				Tiles[NeighborIndex].RemainingOptions[0].bIsCorridorTile ||
				Tiles[NeighborIndex].RemainingOptions[0].BaseObject.ToString() == TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));

			if (bIsWalkable)
			{
				float NewCost = CostSoFar[CurrentIndex] + 1.0f;
				if (!CostSoFar.Contains(NeighborIndex) || NewCost < CostSoFar[NeighborIndex])
				{
					CostSoFar.Add(NeighborIndex, NewCost);
					Heuristic.Add(NeighborIndex,
						FMath::Abs(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(NeighborIndex, GridResolution).X -
							UWaveFunctionCollapseBPLibrary02::IndexAsPosition(GoalIndex, GridResolution).X) +
						FMath::Abs(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(NeighborIndex, GridResolution).Y -
							UWaveFunctionCollapseBPLibrary02::IndexAsPosition(GoalIndex, GridResolution).Y));

					CameFrom.Add(NeighborIndex, CurrentIndex);
					OpenSet.Add(NeighborIndex);
				}
			}
		}
	}

	UE_LOG(LogWFC, Warning, TEXT("FindPathAStar: No path found from %d to %d"), StartIndex, GoalIndex);
	return {};  // 경로를 찾지 못하면 빈 배열 반환
}

void UWaveFunctionCollapseSubsystem02::ConnectIsolatedRooms(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	TArray<int32> IsolatedRoomIndices;
	

	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
	{
		if (Tiles[TileIndex].RemainingOptions.IsEmpty())
		{
			continue;
		}

		const FWaveFunctionCollapseOptionCustom& CurrentOption = Tiles[TileIndex].RemainingOptions[0];

		if (!CurrentOption.bIsRoomTile)
		{
			continue;
		}

		bool bIsIsolated = true;
		TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, Resolution);

		for (int32 AdjacentIndex : AdjacentIndices)
		{
			if (Tiles.IsValidIndex(AdjacentIndex) && !Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
			{

				if (Tiles[AdjacentIndex].RemainingOptions[0].BaseObject.ToString() == TEXT("/Game/BP/goalt01.goalt01"))
				{
					continue;
				}

				bIsIsolated = false;
				break;
			}
		}

		if (bIsIsolated)
		{
			for (int32 AdjacentIndex : AdjacentIndices)
			{
				if (Tiles.IsValidIndex(AdjacentIndex) && Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
				{
					IsolatedRoomIndices.Add(AdjacentIndex);
				}
			}
		}
	}

	for (int32 RoomIndex : IsolatedRoomIndices)
	{

		bool bSurroundedByEmptySpace = true;
		TArray<int32> OffsetIndices = {
			RoomIndex + 2,  // 오른쪽 2칸
			RoomIndex - 2,  // 왼쪽 2칸
			RoomIndex + Resolution.X * 2, // 위쪽 2칸
			RoomIndex - Resolution.X * 2  // 아래쪽 2칸
		};

		for (int32 OffsetIndex : OffsetIndices)
		{
			if (Tiles.IsValidIndex(OffsetIndex) && !Tiles[OffsetIndex].RemainingOptions.IsEmpty())
			{
				bSurroundedByEmptySpace = false;
				break;
			}
		}

		if (bSurroundedByEmptySpace)
		{

			int32 ClosestCorridorIndex = FindClosestCorridor(RoomIndex, Tiles, Resolution);


			if (ClosestCorridorIndex != -1)
			{

				
				int32 PreviousIndex = -1;
				TArray<int32> RoomAdjacentIndices = GetCardinalAdjacentIndices(RoomIndex, Resolution);
				for (int32 AdjacentIndex : RoomAdjacentIndices)
				{
					if (Tiles.IsValidIndex(AdjacentIndex) &&
						!Tiles[AdjacentIndex].RemainingOptions.IsEmpty() &&
						Tiles[AdjacentIndex].RemainingOptions[0].bIsRoomTile)
					{
						PreviousIndex = AdjacentIndex;
						break;
					}
				}

				
				if (PreviousIndex == -1)
				{
					PreviousIndex = RoomIndex;
				}

				

				TArray<int32> Path = FindPathAStar(RoomIndex, ClosestCorridorIndex, PreviousIndex, Tiles, Resolution);
				

				if (Path.IsEmpty())
				{
					UE_LOG(LogWFC, Warning, TEXT("A* returned an empty path from room %d to corridor %d"), RoomIndex, ClosestCorridorIndex);
				}
				else
				{
					UE_LOG(LogWFC, Display, TEXT("A* found path from room %d to corridor %d with %d steps"), RoomIndex, ClosestCorridorIndex, Path.Num());
					FillEmptyTilesAlongPath(Path, Tiles);
				}

				FillEmptyTilesAlongPath(Path, Tiles);

				for (int32 PathIndex : Path)
				{
					if (Tiles.IsValidIndex(PathIndex) && Tiles[PathIndex].RemainingOptions.IsEmpty())
					{

						bool bIsInsideRoom = IsInsideRoom(PathIndex, Tiles, Resolution);


						FWaveFunctionCollapseOptionCustom CorridorOption(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty")); ///Game/WFCCORE/wfc/SpecialOption/Option_Empty
						Tiles[PathIndex].RemainingOptions.Add(CorridorOption);
						Tiles[PathIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
							Tiles[PathIndex].RemainingOptions, WFCModel);

						// 복도 액터 스폰
						UWorld* World = GetWorld();
						if (World)
						{
							FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(PathIndex, Resolution)) * WFCModel->TileSize;
							FRotator TileRotation = CorridorOption.BaseRotator;
							FVector TileScale = CorridorOption.BaseScale3D;

							FTransform SpawnTransform = FTransform(TileRotation, TilePosition, TileScale);
							AActor* SpawnedCorridorActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnTransform);

							if (SpawnedCorridorActor)
							{
								UE_LOG(LogWFC, Display, TEXT("sdsdsdsdsdsdsdsdsd %s for connecting isolated room at index %d"),
									*TilePosition.ToString(), RoomIndex);
							}
							else
							{
								UE_LOG(LogWFC, Error, TEXT("Failed to spawn Corridor Actor at index %d"), PathIndex);
							}
						}
					}
				}
			}
		}
	}
}

void UWaveFunctionCollapseSubsystem02::FillEmptyTilesAlongPath(
	const TArray<int32>& Path, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{


	if (Path.Num() <= 1)
	{
		return;
	}

	// 방 타일 리스트 저장
	TSet<int32> RoomTiles;
	for (int32 i = 0; i < Tiles.Num(); i++)
	{
		if (Tiles.IsValidIndex(i) && !Tiles[i].RemainingOptions.IsEmpty() && Tiles[i].RemainingOptions[0].bIsRoomTile)
		{
			RoomTiles.Add(i);
		}
	}

	// 방 내부 확장 (대각선 포함)
	TSet<int32> ExtendedRoomTiles = RoomTiles;
	for (int32 RoomIndex : RoomTiles)
	{
		TArray<int32> AdjacentIndices = GetAllAdjacentIndices(RoomIndex, Resolution);
		for (int32 AdjacentIndex : AdjacentIndices)
		{
			if (Tiles.IsValidIndex(AdjacentIndex))
			{
				ExtendedRoomTiles.Add(AdjacentIndex);
			}
		}
	}

	// 경로를 따라 액터를 배치
	for (int32 i = 1; i < Path.Num(); i++)
	{
		int32 CurrentIndex = Path[i];
		int32 PreviousIndex = Path[i - 1];

		if (!Tiles.IsValidIndex(CurrentIndex))
		{
			continue;
		}

		// 이미 복도인 경우 건너뜀
		if (!Tiles[CurrentIndex].RemainingOptions.IsEmpty() && Tiles[CurrentIndex].RemainingOptions[0].bIsCorridorTile)
		{
			UE_LOG(LogWFC, Display, TEXT("Skipping modification at index %d (Already Corridor)"), CurrentIndex);
			continue;
		}
		bool bIsInsideRoom = ExtendedRoomTiles.Contains(CurrentIndex);

		// 기본 복도 액터
		FString SelectedTilePath;

		if (bIsInsideRoom)
		{

			SelectedTilePath = TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty");

			// 타일 데이터 갱신
			Tiles[CurrentIndex].RemainingOptions.Empty();
			FWaveFunctionCollapseOptionCustom NewOption(SelectedTilePath);
			Tiles[CurrentIndex].RemainingOptions.Add(NewOption);
			Tiles[CurrentIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
				Tiles[CurrentIndex].RemainingOptions, WFCModel);

			UE_LOG(LogWFC, Display, TEXT("Skipping actor spawn at index %d (Inside Room)"), CurrentIndex);
			continue;
		}


		// 현재 타일과 이전 타일의 위치 비교
		FIntVector CurrentPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(CurrentIndex, Resolution);
		FIntVector PreviousPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(PreviousIndex, Resolution);

		// 방향 판별 (X: 세로, Y: 가로)
		if (i < Path.Num() - 1) // 다음 타일이 존재할 경우 꺾이는지 확인
		{
			int32 NextIndex = Path[i + 1];
			FIntVector NextPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(NextIndex, Resolution);

			bool bWasHorizontal = (PreviousPos.X == CurrentPos.X);
			bool bWasVertical = (PreviousPos.Y == CurrentPos.Y);
			bool bNowHorizontal = (CurrentPos.X == NextPos.X);
			bool bNowVertical = (CurrentPos.Y == NextPos.Y);

			// **직선 타일인지 먼저 체크하여 우선 적용**
			if (bWasHorizontal && bNowHorizontal)
			{
				SelectedTilePath = TEXT("/Game/BP/t01-01.t01-01");  // **가로 직선**
			}
			else if (bWasVertical && bNowVertical)
			{
				SelectedTilePath = TEXT("/Game/BP/t01.t01");  // **세로 직선**
			}
			else
			{
				// 방향 변경 감지
				if (bWasHorizontal && bNowVertical)  // → ↓ (오른쪽에서 아래)
				{
					SelectedTilePath = TEXT("/Game/BP/t04-right.t04-right");
				}
				else if (bWasHorizontal && !bNowVertical) // → ↑ (오른쪽에서 위)
				{
					SelectedTilePath = TEXT("/Game/BP/t04-right1.t04-right1");
				}
				else if (!bWasHorizontal && bNowVertical) // ← ↓ (왼쪽에서 아래)
				{
					SelectedTilePath = TEXT("/Game/BP/t05-left.t05-left");
				}
				else if (!bWasHorizontal && !bNowVertical) // ← ↑ (왼쪽에서 위)
				{
					SelectedTilePath = TEXT("/Game/BP/t05-left1.t05-left1");
				}
				else if (bWasVertical && bNowHorizontal) // ↑ → (위에서 오른쪽)
				{
					SelectedTilePath = TEXT("/Game/BP/t06-right.t06-right");
				}
				else if (bWasVertical && !bNowHorizontal) // ↑ ← (위에서 왼쪽)
				{
					SelectedTilePath = TEXT("/Game/BP/t06-left.t06-left");
				}
				else if (!bWasVertical && bNowHorizontal) // ↓ → (아래에서 오른쪽)
				{
					SelectedTilePath = TEXT("/Game/BP/t07-right.t07-right");
				}
				else if (!bWasVertical && !bNowHorizontal) // ↓ ← (아래에서 왼쪽)
				{
					SelectedTilePath = TEXT("/Game/BP/t07-left.t07-left");
				}
			}
		}
		else
		{
			// **마지막 타일이 직선일 경우 기존 직선 로직을 적용**
			if (CurrentPos.X != PreviousPos.X)  // 가로 방향
			{
				SelectedTilePath = TEXT("/Game/BP/t01-01.t01-01");
			}
			else if (CurrentPos.Y != PreviousPos.Y) // 세로 방향
			{
				SelectedTilePath = TEXT("/Game/BP/t01.t01");
			}
		}

		// 타일 데이터 갱신
		Tiles[CurrentIndex].RemainingOptions.Empty();
		FWaveFunctionCollapseOptionCustom NewOption(SelectedTilePath);
		Tiles[CurrentIndex].RemainingOptions.Add(NewOption);
		Tiles[CurrentIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
			Tiles[CurrentIndex].RemainingOptions, WFCModel);

		UE_LOG(LogWFC, Display, TEXT("Filled path at index %d with %s"), CurrentIndex, *SelectedTilePath);


		// 액터 스폰
		UWorld* World = GetWorld();
		if (World)
		{
			FVector TilePosition = FVector(CurrentPos) * WFCModel->TileSize;
			FRotator TileRotation = NewOption.BaseRotator;
			FVector TileScale = NewOption.BaseScale3D;

			FTransform SpawnTransform = FTransform(TileRotation, TilePosition, TileScale);
			AActor* SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnTransform);

			if (SpawnedActor)
			{
				UE_LOG(LogWFC, Display, TEXT("Spawned %s for path at index %d"), *SelectedTilePath, CurrentIndex);
			}
		}

		if (!Path.IsEmpty())
		{
			int32 GoalTileIndex = Path.Last();
			ReplaceGoalTileWithCustomTile(GoalTileIndex, Tiles);
		}

	}
}


bool UWaveFunctionCollapseSubsystem02::IsInsideRoom(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution)
{
	// 타일 인덱스가 유효한지 확인
	if (!Tiles.IsValidIndex(TileIndex) || Tiles[TileIndex].RemainingOptions.IsEmpty())
	{
		return false;
	}

	// 방 타일인지 확인
	if (!Tiles[TileIndex].RemainingOptions[0].bIsRoomTile)
	{
		return false;
	}

	// 방의 경계 타일인지 확인 (경계는 내부로 간주하지 않음)
	TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, GridResolution);
	for (int32 AdjacentIndex : AdjacentIndices)
	{
		// 인접한 타일이 비어있다면 방의 경계로 간주
		if (Tiles.IsValidIndex(AdjacentIndex) && Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
		{
			return false;
		}
	}

	// 방의 경계가 아니라면 내부 타일
	return true;
}

TArray<int32> UWaveFunctionCollapseSubsystem02::GetRoomBoundaryIndices(int32 RoomIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution)
{
	TArray<int32> BoundaryIndices;
	TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(RoomIndex, GridResolution);

	for (int32 AdjacentIndex : AdjacentIndices)
	{
		if (Tiles.IsValidIndex(AdjacentIndex) && Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
		{
			BoundaryIndices.Add(AdjacentIndex);
		}
	}

	return BoundaryIndices;
}

bool UWaveFunctionCollapseSubsystem02::IsRoomBoundary(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles, FIntVector GridResolution)
{
	TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, GridResolution);
	for (int32 AdjacentIndex : AdjacentIndices)
	{
		if (Tiles.IsValidIndex(AdjacentIndex) && !Tiles[AdjacentIndex].RemainingOptions.IsEmpty())
		{
			if (Tiles[AdjacentIndex].RemainingOptions[0].bIsRoomTile)
			{
				return true;
			}
		}
	}
	return false;
}

TArray<int32> UWaveFunctionCollapseSubsystem02::GetAllAdjacentIndices(int32 TileIndex, FIntVector GridResolution)
{
	TArray<int32> AdjacentIndices;

	FIntVector Position = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, GridResolution);

	// 8방향 오프셋 (상, 하, 좌, 우 + 대각선 4방향)
	TArray<FIntVector> Offsets = {
		FIntVector(-1, 0, 0),  FIntVector(1, 0, 0),   // 좌우
		FIntVector(0, -1, 0),  FIntVector(0, 1, 0),   // 상하
		FIntVector(-1, -1, 0), FIntVector(1, 1, 0),   // 좌하, 우상
		FIntVector(-1, 1, 0),  FIntVector(1, -1, 0)   // 좌상, 우하
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


TArray<int32> UWaveFunctionCollapseSubsystem02::GetTwoStepAdjacentIndices(int32 TileIndex, FIntVector GridResolution)
{
	TArray<int32> AdjacentIndices;

	FIntVector Position = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, GridResolution);

	// 2칸 거리의 오프셋 설정
	TArray<FIntVector> Offsets = {
		FIntVector(0, -2, 0),  FIntVector(0, 2, 0),   // 좌우 2칸
		FIntVector(-2, 0, 0),  FIntVector(2, 0, 0)    // 상하 2칸
	};

	for (const FIntVector& Offset : Offsets)
	{
		FIntVector NeighborPosition = Position + Offset;

		// 유효한 타일인지 확인
		if (NeighborPosition.X >= 0 && NeighborPosition.X < GridResolution.X &&
			NeighborPosition.Y >= 0 && NeighborPosition.Y < GridResolution.Y &&
			NeighborPosition.Z >= 0 && NeighborPosition.Z < GridResolution.Z)
		{
			AdjacentIndices.Add(UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPosition, GridResolution));
		}
	}

	return AdjacentIndices;
}

void UWaveFunctionCollapseSubsystem02::ReplaceGoalTileWithCustomTile(
	int32 GoalTileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	if (!Tiles.IsValidIndex(GoalTileIndex) || Tiles[GoalTileIndex].RemainingOptions.IsEmpty())
	{
		UE_LOG(LogWFC, Warning, TEXT("Invalid Goal Tile Index: %d"), GoalTileIndex);
		return;
	}

	// 목표 타일이 복도 타일인지 확인 (t01 계열인지 체크)
	const FWaveFunctionCollapseOptionCustom& CurrentOption = Tiles[GoalTileIndex].RemainingOptions[0];
	FString CurrentTilePath = CurrentOption.BaseObject.ToString();

	/*if (CurrentTilePath != TEXT("/Game/BP/t01.t01") && CurrentTilePath != TEXT("/Game/BP/t01-01.t01-01"))
	{
		UE_LOG(LogWFC, Warning, TEXT("Goal Tile is not a corridor tile: %s"), *CurrentTilePath);
		return;
	}*/

	// 목표 타일을 'goalt01' 타일로 변경
	FString GoalTilePath = TEXT("/Game/BP/goalt01.goalt01"); // t01과 동일한 경로
	Tiles[GoalTileIndex].RemainingOptions.Empty();
	FWaveFunctionCollapseOptionCustom GoalTileOption(GoalTilePath);
	Tiles[GoalTileIndex].RemainingOptions.Add(GoalTileOption);
	Tiles[GoalTileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
		Tiles[GoalTileIndex].RemainingOptions, WFCModel);

	UE_LOG(LogWFC, Display, TEXT("Replaced Goal Tile at index %d with %s"), GoalTileIndex, *GoalTilePath);
}

void UWaveFunctionCollapseSubsystem02::PlaceGoalTileInFrontOfRoom(
	int32 RoomTileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	if (!Tiles.IsValidIndex(RoomTileIndex) || Tiles[RoomTileIndex].RemainingOptions.Num() != 1)
	{
		return;
	}

	const FWaveFunctionCollapseOptionCustom& RoomTileOption = Tiles[RoomTileIndex].RemainingOptions[0];

	// 방 타일인지 확인
	if (!RoomTileOption.bIsRoomTile)
	{
		return;
	}

	// 방 타일의 위치
	FIntVector RoomTilePosition = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(RoomTileIndex, Resolution);

	// 문이 있는 방향을 확인하고, 해당 방향 앞에 goal 타일 배치
	TArray<FIntVector> DoorOffsets;
	if (RoomTileOption.bHasDoorNorth) DoorOffsets.Add(FIntVector(2, 0, 0));
	if (RoomTileOption.bHasDoorSouth) DoorOffsets.Add(FIntVector(-2, 0, 0));
	if (RoomTileOption.bHasDoorEast) DoorOffsets.Add(FIntVector(0, 2, 0));
	if (RoomTileOption.bHasDoorWest) DoorOffsets.Add(FIntVector(0, -2, 0));

	for (const FIntVector& Offset : DoorOffsets)
	{
		FIntVector GoalTilePosition = RoomTilePosition + Offset;
		int32 GoalTileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(GoalTilePosition, Resolution);

		if (Tiles.IsValidIndex(GoalTileIndex) && !Tiles[GoalTileIndex].RemainingOptions.IsEmpty())
		{
			UE_LOG(LogWFC, Display, TEXT("Placing goalt01 at (%d, %d, %d) in front of room tile at (%d, %d, %d)"),
				GoalTilePosition.X, GoalTilePosition.Y, GoalTilePosition.Z,
				RoomTilePosition.X, RoomTilePosition.Y, RoomTilePosition.Z);

			// 기존 옵션 제거 후 goalt01 배치
			Tiles[GoalTileIndex].RemainingOptions.Empty();
			FWaveFunctionCollapseOptionCustom GoalTileOption(TEXT("/Game/BP/goalt01.goalt01"));
			Tiles[GoalTileIndex].RemainingOptions.Add(GoalTileOption);

			// ShannonEntropy 업데이트
			Tiles[GoalTileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
				Tiles[GoalTileIndex].RemainingOptions, WFCModel);
		}
	}
}

bool UWaveFunctionCollapseSubsystem02::IsIsolatedTile(int32 TileIndex, const TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	// 타일이 유효한지 확인
	if (!Tiles.IsValidIndex(TileIndex) || Tiles[TileIndex].RemainingOptions.Num() == 0)
	{
		return false; // 잘못된 타일이거나 RemainingOptions이 없는 경우 고립이 아님
	}

	// **현재 타일이 goalt01인지 확인**
	auto IsGoalTile = [](const FWaveFunctionCollapseTileCustom& Tile) -> bool
		{
			for (const FWaveFunctionCollapseOptionCustom& Option : Tile.RemainingOptions)
			{
				if (Option.BaseObject.ToString().Contains(TEXT("goalt01"))) // goalt01이면 true
				{
					return true;
				}
			}
			return false;
		};

	// 현재 타일이 goalt01이 아니면 검사할 필요 없음
	if (!IsGoalTile(Tiles[TileIndex]))
	{
		return false;
	}


	// **복도 타일 여부 검사 함수**
	auto IsCorridorTile = [](const FWaveFunctionCollapseTileCustom& Tile) -> bool
		{
			for (const FWaveFunctionCollapseOptionCustom& Option : Tile.RemainingOptions)
			{
				if (Option.bIsCorridorTile) // 옵션 중 하나라도 복도 타일이면 true
				{
					return true;
				}
			}
			return false;
		};

	//현재 타일이 복도인지 확인
	if (!IsCorridorTile(Tiles[TileIndex]))
	{
		return false; // 복도 타일이 아니면 검사할 필요 없음
	}

	//4방향 인접 타일 확인 (상하좌우)
	TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, Resolution);
	for (int32 AdjIndex : AdjacentIndices)
	{
		if (Tiles.IsValidIndex(AdjIndex) && IsCorridorTile(Tiles[AdjIndex]))
		{
			return false; // 인접 타일 중 하나라도 복도라면 고립되지 않음
		}
	}

	// 모든 인접 타일이 복도가 아니라면 고립된 타일!
	return true;
}

float UWaveFunctionCollapseSubsystem02::GetTileSize() const
{
	return (WFCModel != nullptr) ? WFCModel->TileSize : 100.f; // WFCModel이 null일 경우 대비
}

void UWaveFunctionCollapseSubsystem02::PostProcessFixedRoomTileAt(const FIntVector& Coord)
{
	int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

	if (!LastCollapsedTiles.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Invalid Index (%d)yyyyy"), Index);
		return;
	}

	const auto& Tile = LastCollapsedTiles[Index];

	if (Tile.RemainingOptions.Num() != 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Tile at (%d, %d, %d) has %d optionsqwer"), Coord.X, Coord.Y, Coord.Z, Tile.RemainingOptions.Num());
		return;
	}

	if (!Tile.RemainingOptions[0].bIsRoomTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Tile at (%d, %d, %d) is not a room tileasd"), Coord.X, Coord.Y, Coord.Z);
		return;
	}

	// 조건 통과한 경우
	AdjustRoomTileBasedOnCorridors(Index, LastCollapsedTiles);
	ConnectIsolatedRooms(LastCollapsedTiles);
	PlaceGoalTileInFrontOfRoom(Index, LastCollapsedTiles);

	UE_LOG(LogTemp, Warning, TEXT("PostProcessFixedRoomTileAt called for (%d, %d, %d)ttttt"), Coord.X, Coord.Y, Coord.Z);
}