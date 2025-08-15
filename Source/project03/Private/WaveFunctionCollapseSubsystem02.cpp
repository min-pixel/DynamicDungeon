// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaveFunctionCollapseSubsystem02.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "WaveFunctionCollapseBPLibrary02.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "WaveFunctionCollapseModel02.h"
#include <queue>


//#include "Subsystems/EditorActorSubsystem.h"
//#include "Kismet2/ComponentEditorUtils.h"
//#include "Editor.h"


DEFINE_LOG_CATEGORY(LogWFC);


AActor* UWaveFunctionCollapseSubsystem02::CollapseCustom(int32 TryCount /* = 1 */, int32 RandomSeed /* = 0 */, UWorld* World)
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
	TArray<FVector> FixedRoomTilePositions; 
	TArray<int32> RoomTileIndices;

	InitializeWFC(Tiles, RemainingTiles);

	UE_LOG(LogTemp, Warning, TEXT("UserFixedOptions.Num(): %d"), UserFixedOptions.Num());

	//20250419
	//if (StartTile.IsSet())
	//{
	//	int32 StartIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(StartTile.GetValue(), Resolution);

	//	if (Tiles.IsValidIndex(StartIndex))
	//	{
	//		// 주변 인덱스를 구해서 ObservationQueue에 추가
	//		TArray<int32> AdjacentIndices = GetAdjacentIndices(StartIndex, Resolution);
	//		for (int32 AdjIndex : AdjacentIndices)
	//		{
	//			if (Tiles.IsValidIndex(AdjIndex))
	//			{
	//				FWaveFunctionCollapseQueueElementCustom QElem(AdjIndex, EWaveFunctionCollapseAdjacencyCustom::Front);
	//				ObservationQueue.Add(AdjIndex, QElem);
	//			}
	//		}

	//		UE_LOG(LogWFC, Warning, TEXT("Collapse started from custom StartTile's neighbors: %s"), *StartTile.GetValue().ToString());
	//	}
	//}

	//if (StartTile.IsSet())
	//{
	//	int32 StartIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(StartTile.GetValue(), Resolution);
	//	ObservationQueue.Add(StartIndex, FWaveFunctionCollapseQueueElementCustom(StartIndex, EWaveFunctionCollapseAdjacencyCustom::Front)); // 방향은 아무거나 상관없음
	//	UE_LOG(LogWFC, Warning, TEXT("Collapse started from custom StartTile's neighbors: %s"), *StartTile.GetValue().ToString());
	//}

	// StarterOptions 적용  20250417
	for (const auto& Entry : UserFixedOptions)
	{
		const FIntVector& Coord = Entry.Key;
		const FWaveFunctionCollapseOptionCustom& FixedOption = Entry.Value;

		int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

		if (Tiles.IsValidIndex(Index))
		{
			Tiles[Index].RemainingOptions.Empty();
			Tiles[Index].RemainingOptions.Add(FixedOption);

			// ShannonEntropy 재계산
			Tiles[Index].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
				Tiles[Index].RemainingOptions,
				WFCModel
			);

			//UE_LOG(LogTemp, Warning, TEXT("StarterOption applied at (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("Invalid StarterOption index for (%d, %d, %d)"), Coord.X, Coord.Y, Coord.Z);
		}
	}


	bool bSuccessfulSolve = false;

	//PrepareTilePrefabPool(World);
	//실행 시간 측정 시작1
	double PropagationStart = FPlatformTime::Seconds();

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
			//UE_LOG(LogWFC, Warning, TEXT("Failed with Seed Value: %d. Trying again.  Attempt number: %d"), ChosenRandomSeed, CurrentTry);
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
		//UE_LOG(LogWFC, Error, TEXT("Invalid TryCount on Collapse: %d"), TryCount);
		return nullptr;
	}

	  
	//실행 시간 측정 끝1
	double PropagationEnd = FPlatformTime::Seconds();
	UE_LOG(LogWFC, Warning, TEXT("WFC ObservationPropagation took: %.3f seconds"), PropagationEnd - PropagationStart);

	// if Successful, Spawn Actor
	if (bSuccessfulSolve)
	{
		// ====== 시간 측정 1.5 시작 ======
		double TileLoopStart = FPlatformTime::Seconds();
		//실행 시간 측정 시작2
		double PostProcessStart = FPlatformTime::Seconds();


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

					// 방 타일 인덱스 저장
					RoomTileIndices.Add(TileIndex);

					double RoomPostStart = FPlatformTime::Seconds();

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

					if (bIsUserFixed)
					{
						FixedRoomTilePositions.Add(RoomTilePosition); // 고정된 방 위치 저장
					}

					//고정된 방과 겹치는 일반 방 제거
					bool bOverlapsFixedRoom = false;
					for (const FVector& FixedPos : FixedRoomTilePositions)
					{
						if (FVector::DistSquared(RoomTilePosition, FixedPos) < FMath::Square(TileSize * 1.5f))
						{
							bOverlapsFixedRoom = true;
							break;
						}
					}

					if (bOverlapsOtherRoomTile && !bIsUserFixed)
					{
						// 원래 코드 유지: goalt01로 바꾸고 고립 검사까지
						//UE_LOG(LogWFC, Display, TEXT("Room tile at (%s) overlaps with existing room, removing room tile but keeping other options."),
							//*RoomTilePosition.ToString());

						// 대체 타일 설정
						FWaveFunctionCollapseOptionCustom AlternativeTileOption(TEXT("/Game/BP/goalt01.goalt01")); // 대체 타일 경로
						AlternativeTileOption.bIsRoomTile = false;
						AlternativeTileOption.bIsCorridorTile = true;
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
							//UE_LOG(LogWFC, Display, TEXT("Isolated goalt01 tile at (%s), replacing or removing."), *RoomTilePosition.ToString());

							// 삭제할 경우
							 /*Tiles[TileIndex].RemainingOptions.Empty();
							 Tiles[TileIndex].ShannonEntropy = 0.0f; */

							 // 다른 타일로 대체할 경우 (예: /Game/WFCCORE/wfc/SpecialOption/Option_Empty 타일)
							FWaveFunctionCollapseOptionCustom ReplacementTile(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
							ReplacementTile.bIsRoomTile = false;
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

							//UE_LOG(LogWFC, Display, TEXT("Removed overlapping tile inside room boundary at index: %d"), AdjacentIndex);
						}
					}
				
					
					double RoomPostEnd = FPlatformTime::Seconds();
					UE_LOG(LogTemp, Warning, TEXT("[WFC] Room tile end pospos: %.3f초"), RoomPostEnd - RoomPostStart);
			
					
					
					// 방 타일 크기 업데이트
					SelectedOption.BaseScale3D = FVector(3.0f); // 스케일 반영


				
					// ========== AdjustRoomTileBasedOnCorridors #1 ==========
					double AdjustStart1 = FPlatformTime::Seconds();
					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
					double AdjustEnd1 = FPlatformTime::Seconds();
					UE_LOG(LogTemp, Warning, TEXT("[WFC] AdjustRoomTileBasedOnCorridors (1) time: %.6f sec"), AdjustEnd1 - AdjustStart1);


					//TArray<FWaveFunctionCollapseTileCustom> TilesCopy = Tiles;

					

					

					
					
				}
			}

			// ====== 시간 측정 1.5 끝 ======
			double TileLoopEnd = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Warning, TEXT("[WFC] Room Tile timer end (1.5): %.3f SESESE"), TileLoopEnd - TileLoopStart);

		}

		//20250815
		SimpleRoomCountControl(Tiles);

		// ========== ConnectIsolatedRooms ==========   병목의 주 원인, 해결책: ...
		double ConnectStart = FPlatformTime::Seconds();
		//ConnectIsolatedRooms(Tiles);

		ConnectIsolatedRoomsDijkstra(Tiles);
		double ConnectEnd = FPlatformTime::Seconds();
		UE_LOG(LogTemp, Warning, TEXT("[WFC] ConnectIsolatedRoomsDij time: %.6f sec"), ConnectEnd - ConnectStart);

		for (int32 RoomTileIndex : RoomTileIndices)
		{

			// ========== AdjustRoomTileBasedOnCorridors #2 ==========
			double AdjustStart2 = FPlatformTime::Seconds();
			AdjustRoomTileBasedOnCorridors(RoomTileIndex, Tiles);
			double AdjustEnd2 = FPlatformTime::Seconds();
			//UE_LOG(LogTemp, Warning, TEXT("[WFC] AdjustRoomTileBasedOnCorridors (2) time: %.6f sec"), AdjustEnd2 - AdjustStart2);

			// ========== PlaceGoalTileInFrontOfRoom ==========
			double PlaceGoalStart = FPlatformTime::Seconds();
			PlaceGoalTileInFrontOfRoom(RoomTileIndex, Tiles);
			double PlaceGoalEnd = FPlatformTime::Seconds();
			//UE_LOG(LogTemp, Warning, TEXT("[WFC] PlaceGoalTileInFrontOfRoom time: %.6f sec"), PlaceGoalEnd - PlaceGoalStart);
		}

		// 성공한 타일 데이터를 저장
		LastCollapsedTiles = Tiles;

		for (const auto& Elem : UserFixedOptions)
		{
			const FIntVector& Coord = Elem.Key;
			TOptional<FIntVector> CorridorDirection = PostProcessFixedRoomTileAt(Coord, Tiles);

			int32 FixedTileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
			if (Tiles.IsValidIndex(FixedTileIndex) && CorridorDirection.IsSet())
			{
				AdjustRoomTileFacingDirection(FixedTileIndex, Tiles, CorridorDirection.GetValue());
			}
		}

		if (!World)
		{
			UE_LOG(LogWFC, Error, TEXT("World is null"));
			return nullptr;
		}

		AActor* SpawnedActor = SpawnActorFromTiles(Tiles, World);

		if (SpawnedActor)
		{
			SpawnedActor->Tags.Add(FName("WFCGenerated"));
		}

		//UE_LOG(LogWFC, Display, TEXT("Success! Seed Value: %d. Spawned Actor: %s"), ChosenRandomSeed, *SpawnedActor->GetActorLabel());

		// 테두리 블루프린트 소환 함수 호출
		SpawnBorderBlueprints(); 

		RoomTilePositions.Empty();

		//실행 시간 측정 끝2
		double PostProcessEnd = FPlatformTime::Seconds();
		UE_LOG(LogTemp, Warning, TEXT("Post-processing + Spawning took: %.3f seconds"), PostProcessEnd - PostProcessStart);

		
		//PrepareTilePrefabPool(World);

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
	UClass* LeftBorderBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t02-L.t02-L_C"));
	UClass* RightBorderBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t02-r.t02-r_C"));
	UClass* FrontBorderBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t02-m.t02-m_C"));
	UClass* BackBorderBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t02-b.t02-b_C"));

	// 모서리
	UClass* BottomLeftCornerBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t03-back.t03-back_C"));
	UClass* TopLeftCornerBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t03-back1.t03-back1_C"));
	UClass* BottomRightCornerBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t03-back2.t03-back2_C"));
	UClass* TopRightCornerBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/BP/Bedge/t03-back3.t03-back3_C"));

	if (!LeftBorderBlueprint || !RightBorderBlueprint || !FrontBorderBlueprint || !BackBorderBlueprint ||
		!BottomLeftCornerBlueprint || !TopLeftCornerBlueprint || !BottomRightCornerBlueprint || !TopRightCornerBlueprint)
	{
		//UE_LOG(LogWFC, Error, TEXT("Failed to load one or more Blueprint assets"));
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
					//UE_LOG(LogWFC, Display, TEXT("Skipping Border Actor spawn due to overlap with Room Tile at (%d, %d, %d)"), X, Y, Z);
					continue;
				}



				if (IsPositionInnerBorder(Position))
				{
					TSubclassOf<AActor> SelectedBPClass = nullptr;
					FVector PositionOffset = Offset; // 기본 오프셋 복사

					// 각 모서리 위치에 따라 다른 블루프린트 선택
					if (X == 0 && Y == 0) // 좌하단 모서리
					{
						SelectedBPClass = BottomLeftCornerBlueprint;// ->GeneratedClass;
						PositionOffset.X -= WFCModel->TileSize;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (X == 0 && Y == Resolution.Y - 1) // 좌상단 모서리
					{
						SelectedBPClass = BottomRightCornerBlueprint;// ->GeneratedClass;
						PositionOffset.X -= WFCModel->TileSize;
						PositionOffset.Y += WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1 && Y == 0) // 우하단 모서리
					{
						SelectedBPClass = TopLeftCornerBlueprint;
						PositionOffset.X += WFCModel->TileSize;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1 && Y == Resolution.Y - 1) // 우상단 모서리
					{
						SelectedBPClass = TopRightCornerBlueprint;
						PositionOffset.X += WFCModel->TileSize;
						PositionOffset.Y += WFCModel->TileSize;
					}
					else if (X == 0) // 왼쪽 테두리
					{
						SelectedBPClass = BackBorderBlueprint;
						PositionOffset.X -= WFCModel->TileSize;
					}
					else if (X == Resolution.X - 1) // 오른쪽 테두리
					{
						SelectedBPClass = FrontBorderBlueprint;
						PositionOffset.X += WFCModel->TileSize;
					}
					else if (Y == 0) // 앞쪽 테두리
					{
						SelectedBPClass = LeftBorderBlueprint;
						PositionOffset.Y -= WFCModel->TileSize;
					}
					else if (Y == Resolution.Y - 1) // 뒤쪽 테두리
					{
						SelectedBPClass = RightBorderBlueprint;
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

							// Replication 설정 추가
							SpawnedActor->SetReplicates(true);
							SpawnedActor->bAlwaysRelevant = true;
						}
						else
						{
							//UE_LOG(LogWFC, Error, TEXT("Failed to spawn Blueprint Actor at (%d, %d, %d)"), X, Y, Z);
						}
					}
					else
					{
						//UE_LOG(LogWFC, Error, TEXT("Failed to spawn due to invalid Blueprint class or world context at (%d, %d, %d)"), X, Y, Z);
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

						//UE_LOG(LogTemp, Warning, TEXT("ioioio: (%d, %d, %d) → %s"),
						//X, Y, Z, * FixedOption->BaseObject.ToString());

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

UActorComponent* UWaveFunctionCollapseSubsystem02::AddNamedInstanceComponent(
	AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FName ComponentName)
{
	if (!IsValid(Actor)) return nullptr;

	Actor->Modify(); // 에디터에서만 안전

	int32 Counter = 1;
	FName ComponentInstanceName = ComponentName;
	/*#if WITH_EDITOR
	while (!FComponentEditorUtils::IsComponentNameAvailable(ComponentInstanceName.ToString(), Actor))
	{
		ComponentInstanceName = FName(*FString::Printf(TEXT("%s_%d"), *ComponentName.ToString(), Counter++));
	}
	#endif*/
	UActorComponent* InstanceComponent = NewObject<UActorComponent>(Actor, ComponentClass, ComponentInstanceName, RF_Transactional);
	if (InstanceComponent)
	{
		Actor->AddInstanceComponent(InstanceComponent);
		Actor->FinishAddComponent(InstanceComponent, false, FTransform::Identity);
		//Actor->RerunConstructionScripts();
	}

	return InstanceComponent;
}

void UWaveFunctionCollapseSubsystem02::SetTileNetworkPriority(AActor* TileActor, const FVector& TilePosition)
{
	if (!TileActor || !GetWorld()) return;

	float MinDistanceToAnyPlayer = FLT_MAX;

	// 모든 플레이어와의 최소 거리 찾기
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			float Distance = FVector::Dist(TilePosition, PC->GetPawn()->GetActorLocation());
			MinDistanceToAnyPlayer = FMath::Min(MinDistanceToAnyPlayer, Distance);
		}
	}

	// 거리에 따른 간단한 우선순위 (가까우면 높게, 멀면 낮게)
	if (MinDistanceToAnyPlayer < 1000.0f)      // 10m 이내
	{
		TileActor->NetPriority = 10.0f;
	}
	else if (MinDistanceToAnyPlayer < 3000.0f) // 30m 이내  
	{
		TileActor->NetPriority = 5.0f;
	}
	else if (MinDistanceToAnyPlayer < 5000.0f) // 50m 이내
	{
		TileActor->NetPriority = 2.0f;
	}
	else
	{
		TileActor->NetPriority = 1.0f;  // 기본값
	}
}

AActor* UWaveFunctionCollapseSubsystem02::SpawnActorFromTiles(const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* WorldContext)
{
	if (!WorldContext)
	{
		UE_LOG(LogWFC, Error, TEXT("WorldContext is null, cannot spawn actors."));
		return nullptr;
	}

	//시작 시간
	const double StartTime = FPlatformTime::Seconds();

	FActorSpawnParameters SpawnParams;
	AActor* SpawnedActor = WorldContext->SpawnActor<AActor>(AActor::StaticClass(), OriginLocation, Orientation, SpawnParams);
	if (!SpawnedActor)
	{
		UE_LOG(LogWFC, Error, TEXT("Failed to spawn parent actor."));
		return nullptr;
	}

	// 루트 Actor Replicate
	SpawnedActor->SetReplicates(true);
	SpawnedActor->Tags.Add(FName("WFCGenerated"));

	TMap<FSoftObjectPath, UInstancedStaticMeshComponent*> BaseObjectToISM;

	for (int32 Index = 0; Index < Tiles.Num(); Index++)
	{
		if (Tiles[Index].RemainingOptions.IsEmpty())
		{
			//UE_LOG(LogWFC, Display, TEXT("Skipped empty tile at index: %d"), Index);
			continue;
		}

		const FWaveFunctionCollapseOptionCustom& Option = Tiles[Index].RemainingOptions[0];

		if (Option.BaseObject == FWaveFunctionCollapseOptionCustom::EmptyOption.BaseObject
			|| Option.BaseObject == FWaveFunctionCollapseOptionCustom::VoidOption.BaseObject
			|| WFCModel->SpawnExclusion.Contains(Option.BaseObject))
		{
			continue;
		}

		// "_C"가 없는 경우 자동으로 보정
		FSoftObjectPath FixedPath = Option.BaseObject;
		FString PathStr = FixedPath.ToString();
		if (!PathStr.EndsWith(TEXT("_C")) && !PathStr.EndsWith(TEXT("_Empty")))
		{
			PathStr += TEXT("_C");
			FixedPath = FSoftObjectPath(PathStr);
		}



		UObject* LoadedObject = FixedPath.TryLoad();
		FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution)) * WFCModel->TileSize;
		FRotator TileRotation = Option.BaseRotator;
		FVector TileScale = Option.BaseScale3D;

		// Blueprint 클래스 스폰 처리
		if (UClass* ActorClass = Cast<UClass>(LoadedObject))
		{
			if (ActorClass->IsChildOf<AActor>())
			{
				AActor* TileActor = WorldContext->SpawnActor<AActor>(ActorClass, TilePosition, TileRotation);
				if (TileActor)
				{
					 SetTileNetworkPriority(TileActor, TilePosition);

					TileActor->SetActorScale3D(TileScale);
					TileActor->AttachToActor(SpawnedActor, FAttachmentTransformRules::KeepWorldTransform);
					TileActor->Tags.Add(FName("WFCGenerated"));
					//TileActor Replicate
					TileActor->bAlwaysRelevant = true;
					TileActor->SetReplicates(true);

					TArray<UStaticMeshComponent*> MeshComponents;
					TileActor->GetComponents<UStaticMeshComponent>(MeshComponents);

					for (UStaticMeshComponent* MeshComp : MeshComponents)
					{
						MeshComp->SetVisibility(true, true);
						MeshComp->SetHiddenInGame(false, true);
						MeshComp->bOwnerNoSee = false;
						MeshComp->bOnlyOwnerSee = false;
						MeshComp->bRenderInMainPass = true;
						//MeshComp->bRenderCustomDepth = true;
						MeshComp->bAllowCullDistanceVolume = false;

					}

				}
			}
		}
		// StaticMesh 처리 (기존 기능 유지)
		else if (UStaticMesh* LoadedStaticMesh = Cast<UStaticMesh>(LoadedObject))
		{
			UInstancedStaticMeshComponent* ISMComponent = nullptr;

			if (UInstancedStaticMeshComponent** FoundPtr = BaseObjectToISM.Find(Option.BaseObject))
			{
				ISMComponent = *FoundPtr;
			}
			else
			{
				ISMComponent = Cast<UInstancedStaticMeshComponent>(
					AddNamedInstanceComponent(SpawnedActor, UInstancedStaticMeshComponent::StaticClass(), LoadedStaticMesh->GetFName()));
				BaseObjectToISM.Add(Option.BaseObject, ISMComponent);
				ISMComponent->SetStaticMesh(LoadedStaticMesh);
				ISMComponent->SetIsReplicated(false);
			}

			if (ISMComponent)
			{

				ISMComponent->AddInstance(FTransform(TileRotation, TilePosition, TileScale));

				ISMComponent->SetVisibility(true, true);
				ISMComponent->SetHiddenInGame(false, true);
				ISMComponent->bRenderInMainPass = true;
				//ISMComponent->bRenderCustomDepth = true;
				ISMComponent->bAllowCullDistanceVolume = false;

			}
		}
		else
		{
			//UE_LOG(LogWFC, Warning, TEXT("Failed to load tile asset: %s"), *Option.BaseObject.ToString());
		}
	}

	//종료 시간 및 출력
	const double EndTime = FPlatformTime::Seconds();
	const double ElapsedTime = EndTime - StartTime;
	UE_LOG(LogWFC, Log, TEXT("[SpawnActorFromTiles] Tile spawning took %.4f seconds."), ElapsedTime);


	return SpawnedActor;
}


//AActor* UWaveFunctionCollapseSubsystem02::SpawnActorFromTiles(const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* WorldContext)
//{
//
//
//	// UWorld 참조 확인
//	if (!WorldContext)
//	{
//		UE_LOG(LogWFC, Error, TEXT("WorldContext is null, cannot spawn actors."));
//		return nullptr;
//	}
//
//
//	// 최상위 부모 액터 생성
//	FActorSpawnParameters SpawnParams;
//	AActor* SpawnedActor = WorldContext->SpawnActor<AActor>(AActor::StaticClass(), OriginLocation, Orientation, SpawnParams);
//	if (!SpawnedActor)
//	{
//		UE_LOG(LogWFC, Error, TEXT("Failed to spawn actor."));
//		return nullptr;
//	}
//	//#if WITH_EDITOR
//	//// 부모 액터 이름 설정
//	//if (WFCModel)
//	//{
//	//	FActorLabelUtilities::SetActorLabelUnique(SpawnedActor, WFCModel->GetFName().ToString());
//	//}
//	//else
//	//{
//	//	UE_LOG(LogWFC, Error, TEXT("WFCModel is null, cannot set actor label."));
//	//}
//	//#endif
//	// Components 생성
//	TMap<FSoftObjectPath, UInstancedStaticMeshComponent*> BaseObjectToISM;
//	for (int32 Index = 0; Index < Tiles.Num(); Index++)
//	{
//		// 빈 타일 무시
//		if (Tiles[Index].RemainingOptions.IsEmpty())
//		{
//			UE_LOG(LogWFC, Display, TEXT("Skipped empty tile at index: %d"), Index);
//			continue;
//		}
//
//		const FWaveFunctionCollapseOptionCustom& Option = Tiles[Index].RemainingOptions[0];
//
//		// Empty, Void 타일 및 제외된 타일 무시
//		if (Option.BaseObject == FWaveFunctionCollapseOptionCustom::EmptyOption.BaseObject
//			|| Option.BaseObject == FWaveFunctionCollapseOptionCustom::VoidOption.BaseObject
//			|| WFCModel->SpawnExclusion.Contains(Option.BaseObject))
//		{
//			continue;
//		}
//		
//		UObject* LoadedObject = Option.BaseObject.TryLoad();
//		if (LoadedObject)
//		{
//			FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution)) * WFCModel->TileSize;
//			FRotator TileRotation = Option.BaseRotator;
//			FVector TileScale = Option.BaseScale3D;
//
//			// Static Mesh 처리
//			if (UStaticMesh* LoadedStaticMesh = Cast<UStaticMesh>(LoadedObject))
//			{
//				UInstancedStaticMeshComponent* ISMComponent;
//				if (UInstancedStaticMeshComponent** FoundISMComponentPtr = BaseObjectToISM.Find(Option.BaseObject))
//				{
//					ISMComponent = *FoundISMComponentPtr;
//				}
//				else
//				{
//					ISMComponent = Cast<UInstancedStaticMeshComponent>(
//						AddNamedInstanceComponent(SpawnedActor, UInstancedStaticMeshComponent::StaticClass(), LoadedObject->GetFName()));
//					BaseObjectToISM.Add(Option.BaseObject, ISMComponent);
//				}
//				ISMComponent->SetStaticMesh(LoadedStaticMesh);
//				ISMComponent->AddInstance(FTransform(TileRotation, TilePosition, TileScale));
//			}
//			// Blueprint 처리
//			if (UBlueprint* LoadedBlueprint = Cast<UBlueprint>(LoadedObject))
//			{
//				TSubclassOf<AActor> ActorClass = *LoadedBlueprint->GeneratedClass;
//
//				UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(SpawnedActor, UChildActorComponent::StaticClass());
//				ChildActorComponent->SetupAttachment(SpawnedActor->GetRootComponent());
//				ChildActorComponent->RegisterComponent();
//				ChildActorComponent->SetChildActorClass(ActorClass);
//				ChildActorComponent->SetRelativeLocation(TilePosition);
//				ChildActorComponent->SetRelativeRotation(TileRotation);
//				ChildActorComponent->SetRelativeScale3D(TileScale);
//				SpawnedActor->AddInstanceComponent(ChildActorComponent);
//			}
//		}
//	}
//
//	return SpawnedActor;
//}


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

void UWaveFunctionCollapseSubsystem02::ExecuteWFC(int32 TryCount, int32 RandomSeed, UWorld* World)
{
	if (!WFCModel)
	{
		WFCModel = LoadObject<UWaveFunctionCollapseModel02>(nullptr, TEXT("/Game/WFCCORE/zxzx39.zxzx39"));
		if (!WFCModel)
		{
			//UE_LOG(LogTemp, Error, TEXT("WFCModel 로딩 실패! 경로를 확인하세요."));
			return;
		}
	}

	//서버에서만 실행되도록
	/*if (!World || (World->GetNetMode() != NM_DedicatedServer && World->GetNetMode() != NM_ListenServer))
	{
		UE_LOG(LogWFC, Warning, TEXT("ExecuteWFC can only run on server."));
		return;
	}*/
	if (!World || (World->GetAuthGameMode() == nullptr))
	{
		UE_LOG(LogWFC, Warning, TEXT("ExecuteWFC can only run on server (Auth check failed)."));
		return;
	}

	OriginLocation = FVector(0.0f, 0.0f, 0.0f);
	Orientation = FRotator(0.0f, 0.0f, 0.0f);
	bUseEmptyBorder = false;
	//20250419
	//TOptional<FIntVector> StartTile;
	//if (UserFixedOptions.Num() > 0)
	//{
	//	for (const auto& Elem : UserFixedOptions)
	//	{
	//		StartTile = Elem.Key;
	//		break; // 첫 번째 키 하나만 사용
	//	}
	//}
	
	// WFC Collapse 실행
	AActor* ResultActor = CollapseCustom(TryCount, RandomSeed, World);
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
	FSoftObjectPath ModelPath(TEXT("/Game/WFCCORE/zxzx39.zxzx39"));
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	UWaveFunctionCollapseModel02* LoadedModel = Cast<UWaveFunctionCollapseModel02>(Streamable.LoadSynchronous(ModelPath));


	if (LoadedModel)
	{
		WFCModel = LoadedModel;
		//UE_LOG(LogWFC, Log, TEXT("WFCModel이 zxzx34로 설정되었습니다."));
	}
	else
	{
		//UE_LOG(LogWFC, Error, TEXT("WFCModel 로드 실패: %s"), *ModelPath.ToString());
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

	TArray<int32> TempModifiedRoomTiles;

	for (int32 TileIndex : IsolatedRoomIndices)
	{
		if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() > 0)
		{
			FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];

			// 고정된 방이면 corridor처럼 속이기
			FIntVector Coord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
			if (Option.bIsRoomTile && UserFixedOptions.Contains(Coord))
			{
				Option.bIsCorridorTile = true;
				TempModifiedRoomTiles.Add(TileIndex);
			}
		}
	}

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

		//3초
		/*for (int32 OffsetIndex : OffsetIndices)
		{
			if (Tiles.IsValidIndex(OffsetIndex) && !Tiles[OffsetIndex].RemainingOptions.IsEmpty())
			{
				bSurroundedByEmptySpace = false;
				break;
			}
		}*/

		//20250509 속도개선 버전4 - 17초
		//static const FName EmptyTileName(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
		//static const FName GoalTileName(TEXT("/Game/BP/goalt01.goalt01"));

		//for (int32 OffsetIndex : OffsetIndices)
		//{
		//	if (!Tiles.IsValidIndex(OffsetIndex)) continue;

		//	const auto& Options = Tiles[OffsetIndex].RemainingOptions;
		//	if (Options.IsEmpty()) continue;

		//	const FName& AssetName = Options[0].BaseObject.GetAssetPathName();

		//	if (AssetName == EmptyTileName || AssetName == GoalTileName)
		//	{
		//		continue;
		//	}

		//	// 이건 실제로 유효한 공간임
		//	bSurroundedByEmptySpace = false;
		//	break;
		//}

		//20250510 개선 속도 14
		/*for (int32 OffsetIndex : OffsetIndices)
		{
			if (!Tiles.IsValidIndex(OffsetIndex)) continue;

			const auto& Options = Tiles[OffsetIndex].RemainingOptions;
			if (Options.IsEmpty()) continue;

			const FSoftObjectPath& Path = Options[0].BaseObject;
			const FName AssetName = Path.GetAssetPathName();

			if (AssetName == EmptyTileName || AssetName == GoalTileName)
			{
				continue;
			}

			bSurroundedByEmptySpace = false;
			break;
		}*/

		//20250503, 고립된 방 판별 18초
		for (int32 OffsetIndex : OffsetIndices)
		{
			if (!Tiles.IsValidIndex(OffsetIndex)) continue;

			const TArray<FWaveFunctionCollapseOptionCustom>& Options = Tiles[OffsetIndex].RemainingOptions;

			if (Options.IsEmpty())
			{
				continue; // 진짜 빈 타일
			}

			const FString& MeshPath = Options[0].BaseObject.ToString();

			// Option_Empty와 goalt01이면 빈 공간처럼 간주
			if (MeshPath == TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty") ||
				MeshPath == TEXT("/Game/BP/goalt01.goalt01"))
			{
				continue;
			}

			// 이 타일은 실제로 채워져 있는 유효한 공간
			bSurroundedByEmptySpace = false;
			break;
		}

		//20250509 실행속도 개선버전
		/*static const FName EmptyTileName(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
		static const FName GoalTileName(TEXT("/Game/BP/goalt01.goalt01"));

		for (int32 OffsetIndex : OffsetIndices)
		{
			if (!Tiles.IsValidIndex(OffsetIndex)) continue;

			const auto& Options = Tiles[OffsetIndex].RemainingOptions;
			if (Options.IsEmpty()) continue;

			const FName& AssetName = Options[0].BaseObject.GetAssetPathName();

			if (AssetName == EmptyTileName || AssetName == GoalTileName)
			{
				continue;
			}

			bSurroundedByEmptySpace = false;
			break;
		}*/

		//static const FName EmptyTileName(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
		//static const FName GoalTileName(TEXT("/Game/BP/goalt01.goalt01"));

		//const TArray<FIntVector> CardinalOffsets = {
		//	FIntVector(2, 0, 0),   // 동쪽
		//	FIntVector(-2, 0, 0),  // 서쪽
		//	FIntVector(0, 2, 0),   // 북쪽
		//	FIntVector(0, -2, 0)   // 남쪽
		//};

		//	FIntVector RoomCoord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(RoomIndex, Resolution);
		//	//bool bSurroundedByEmptySpace = true;

		//	for (const FIntVector& Offset : CardinalOffsets)
		//	{
		//		FIntVector CheckCoord = RoomCoord + Offset;
		//		int32 CheckIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(CheckCoord, Resolution);
		//		if (!Tiles.IsValidIndex(CheckIndex)) continue;

		//		const auto& Options = Tiles[CheckIndex].RemainingOptions;
		//		if (Options.IsEmpty()) continue;

		//		const FName& AssetName = Options[0].BaseObject.GetAssetPathName();
		//		if (AssetName == EmptyTileName || AssetName == GoalTileName) continue;

		//		bSurroundedByEmptySpace = false;
		//		break;
		//	}
		

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
							//비동기 실험

							/*AsyncTask(ENamedThreads::GameThread, [this, SpawnTransform, PathIndex, TilePosition, RoomIndex]()
							{
									UWorld* World = GetWorld();
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
							});*/
							
						}
					}
				}
			}
		}
	}

	for (int32 TileIndex : TempModifiedRoomTiles)
	{
		if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() > 0)
		{
			FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
			Option.bIsCorridorTile = false;
		}
	}

}

//202050509 속도 개선 버전2
//void UWaveFunctionCollapseSubsystem02::ConnectIsolatedRooms(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
//{
//	TArray<int32> IsolatedRoomIndices;
//	TArray<int32> TempModifiedRoomTiles;
//
//	static const FName EmptyTileName(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
//	static const FName GoalTileName(TEXT("/Game/BP/goalt01.goalt01"));
//
//	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
//	{
//		if (Tiles[TileIndex].RemainingOptions.IsEmpty()) continue;
//		const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
//		if (!Option.bIsRoomTile) continue;
//
//		// 해당 방 타일이 고정 타일이면 일시적으로 복도처럼 간주
//		FIntVector Coord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
//		if (UserFixedOptions.Contains(Coord))
//		{
//			Tiles[TileIndex].RemainingOptions[0].bIsCorridorTile = true;
//			TempModifiedRoomTiles.Add(TileIndex);
//		}
//
//		// 2칸 떨어진 4방향 검사
//		bool bSurroundedByEmpty = true;
//		TArray<int32> OffsetIndices = {
//			TileIndex + 2,
//			TileIndex - 2,
//			TileIndex + Resolution.X * 2,
//			TileIndex - Resolution.X * 2
//		};
//		for (int32 OffsetIndex : OffsetIndices)
//		{
//			if (!Tiles.IsValidIndex(OffsetIndex)) continue;
//			const auto& Options = Tiles[OffsetIndex].RemainingOptions;
//			if (Options.IsEmpty()) continue;
//
//			const FName& AssetName = Options[0].BaseObject.GetAssetPathName();
//			if (AssetName == EmptyTileName || AssetName == GoalTileName) continue;
//
//			bSurroundedByEmpty = false;
//			break;
//		}
//
//		if (!bSurroundedByEmpty) continue;
//
//		// 주변 1칸에서 연결 시작점 선택
//		TArray<int32> Adjacents = GetCardinalAdjacentIndices(TileIndex, Resolution);
//		for (int32 AdjIndex : Adjacents)
//		{
//			if (Tiles.IsValidIndex(AdjIndex) && Tiles[AdjIndex].RemainingOptions.IsEmpty())
//			{
//				IsolatedRoomIndices.Add(AdjIndex);
//				break;
//			}
//		}
//	}
//
//	// A* 경로 연결
//	for (int32 RoomIndex : IsolatedRoomIndices)
//	{
//		int32 ClosestCorridorIndex = FindClosestCorridor(RoomIndex, Tiles, Resolution);
//		if (ClosestCorridorIndex == -1) continue;
//
//		int32 PreviousIndex = RoomIndex;
//		TArray<int32> Adjacents = GetCardinalAdjacentIndices(RoomIndex, Resolution);
//		for (int32 AdjIndex : Adjacents)
//		{
//			if (Tiles.IsValidIndex(AdjIndex) &&
//				!Tiles[AdjIndex].RemainingOptions.IsEmpty() &&
//				Tiles[AdjIndex].RemainingOptions[0].bIsRoomTile)
//			{
//				PreviousIndex = AdjIndex;
//				break;
//			}
//		}
//
//		TArray<int32> Path = FindPathAStar(RoomIndex, ClosestCorridorIndex, PreviousIndex, Tiles, Resolution);
//		if (Path.IsEmpty()) continue;
//		FillEmptyTilesAlongPath(Path, Tiles);
//	}
//
//	// 복원
//	for (int32 TileIndex : TempModifiedRoomTiles)
//	{
//		if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() > 0)
//		{
//			Tiles[TileIndex].RemainingOptions[0].bIsCorridorTile = false;
//		}
//	}
//}

//20250509 속도개선 버전3
//void UWaveFunctionCollapseSubsystem02::ConnectIsolatedRooms(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
//{
//	TSet<int32> ConnectedRoomIndices;
//	TArray<int32> TempModifiedRoomTiles;
//
//	// 1. UserFixedOptions에 있는 방은 corridor처럼 속이기
//	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
//	{
//		if (Tiles[TileIndex].RemainingOptions.IsEmpty()) continue;
//
//		FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
//		if (Option.bIsRoomTile)
//		{
//			FIntVector Coord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
//			if (UserFixedOptions.Contains(Coord))
//			{
//				Option.bIsCorridorTile = true;
//				TempModifiedRoomTiles.Add(TileIndex);
//			}
//		}
//	}
//
//	// 2. 각 방을 기준으로 고립 여부 검사 + A* 연결 시도 (1방당 최대 1회 A*)
//	static const FName EmptyTileName(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
//	static const FName GoalTileName(TEXT("/Game/BP/goalt01.goalt01"));
//
//	for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
//	{
//		if (!Tiles.IsValidIndex(TileIndex)) continue;
//		if (ConnectedRoomIndices.Contains(TileIndex)) continue;
//
//		if (Tiles[TileIndex].RemainingOptions.IsEmpty()) continue;
//
//		const FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
//		if (!Option.bIsRoomTile) continue;
//
//		bool bIsIsolated = true;
//		TArray<int32> AdjacentIndices = GetCardinalAdjacentIndices(TileIndex, Resolution);
//
//		for (int32 AdjIndex : AdjacentIndices)
//		{
//			if (!Tiles.IsValidIndex(AdjIndex)) continue;
//
//			const auto& AdjOptions = Tiles[AdjIndex].RemainingOptions;
//			if (AdjOptions.IsEmpty()) continue;
//
//			const FName AssetName = AdjOptions[0].BaseObject.GetAssetPathName();
//			if (AssetName != EmptyTileName && AssetName != GoalTileName)
//			{
//				bIsIsolated = false;
//				break;
//			}
//		}
//
//		if (!bIsIsolated)
//			continue;
//
//		// 고립되었으니 A*로 복도 연결 시도
//		int32 ClosestCorridorIndex = FindClosestCorridor(TileIndex, Tiles, Resolution);
//		if (ClosestCorridorIndex == -1)
//			continue;
//
//		int32 PreviousIndex = -1;
//		for (int32 AdjIndex : AdjacentIndices)
//		{
//			if (Tiles.IsValidIndex(AdjIndex) &&
//				!Tiles[AdjIndex].RemainingOptions.IsEmpty() &&
//				Tiles[AdjIndex].RemainingOptions[0].bIsRoomTile)
//			{
//				PreviousIndex = AdjIndex;
//				break;
//			}
//		}
//		if (PreviousIndex == -1)
//			PreviousIndex = TileIndex;
//
//		TArray<int32> Path = FindPathAStar(TileIndex, ClosestCorridorIndex, PreviousIndex, Tiles, Resolution);
//		if (Path.IsEmpty())
//		{
//			UE_LOG(LogWFC, Warning, TEXT("A* failed from room %d to corridor %d"), TileIndex, ClosestCorridorIndex);
//			continue;
//		}
//
//		ConnectedRoomIndices.Add(TileIndex);
//		UE_LOG(LogWFC, Display, TEXT("Connected isolated room %d via A* (%d steps)"), TileIndex, Path.Num());
//		FillEmptyTilesAlongPath(Path, Tiles);
//
//		// 3. 복도 메쉬 배치
//		for (int32 PathIndex : Path)
//		{
//			if (!Tiles.IsValidIndex(PathIndex) || !Tiles[PathIndex].RemainingOptions.IsEmpty()) continue;
//
//			FWaveFunctionCollapseOptionCustom CorridorOption(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
//			Tiles[PathIndex].RemainingOptions.Add(CorridorOption);
//			Tiles[PathIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
//				Tiles[PathIndex].RemainingOptions, WFCModel);
//
//			UWorld* World = GetWorld();
//			if (World)
//			{
//				FVector TilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(PathIndex, Resolution)) * WFCModel->TileSize;
//				FTransform SpawnTransform = FTransform(CorridorOption.BaseRotator, TilePosition, CorridorOption.BaseScale3D);
//				AActor* SpawnedCorridorActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnTransform);
//
//				if (!SpawnedCorridorActor)
//				{
//					UE_LOG(LogWFC, Error, TEXT("Failed to spawn corridor actor at index %d"), PathIndex);
//				}
//			}
//		}
//	}
//
//	// 4. corridor로 속였던 방 복구
//	for (int32 TileIndex : TempModifiedRoomTiles)
//	{
//		if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() > 0)
//		{
//			Tiles[TileIndex].RemainingOptions[0].bIsCorridorTile = false;
//		}
//	}
//}


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

			//비동기 실험
			//AActor* SpawnedActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnTransform);

			/*if (SpawnedActor)
			{
				UE_LOG(LogWFC, Display, TEXT("Spawned %s for path at index %d"), *SelectedTilePath, CurrentIndex);
			}*/
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

//void UWaveFunctionCollapseSubsystem02::PostProcessFixedRoomTileAt(const FIntVector& Coord)
//{
//	if (!UserFixedOptions.Contains(Coord))
//	{
//		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Tile at (%d, %d, %d) is not a user-fixed tile"), Coord.X, Coord.Y, Coord.Z);
//		return;
//	}
//
//
//
//	int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
//
//	if (!LastCollapsedTiles.IsValidIndex(Index))
//	{
//		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Invalid Index (%d)yyyyy"), Index);
//		return;
//	}
//
//	const auto& Tile = LastCollapsedTiles[Index];
//
//	if (Tile.RemainingOptions.Num() != 1)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Tile at (%d, %d, %d) has %d optionsqwer"), Coord.X, Coord.Y, Coord.Z, Tile.RemainingOptions.Num());
//		return;
//	}
//
//	if (!Tile.RemainingOptions[0].bIsRoomTile)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("PostProcess skipped: Tile at (%d, %d, %d) is not a room tileasd"), Coord.X, Coord.Y, Coord.Z);
//		return;
//	}
//
//	// 조건 통과한 경우
//	AdjustRoomTileBasedOnCorridors(Index, LastCollapsedTiles);
//	ConnectIsolatedRooms(LastCollapsedTiles);
//	//PlaceGoalTileInFrontOfRoom(Index, LastCollapsedTiles);
//
//	UE_LOG(LogTemp, Warning, TEXT("PostProcessFixedRoomTileAt called for (%d, %d, %d)ttttt"), Coord.X, Coord.Y, Coord.Z);
//}



TOptional<FIntVector> UWaveFunctionCollapseSubsystem02::PostProcessFixedRoomTileAt(const FIntVector& Coord, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	if (!UserFixedOptions.Contains(Coord)) return {};

	const TArray<FIntVector> Directions = {
		FIntVector(2, 0, 0),
		FIntVector(-2, 0, 0),
		FIntVector(0, 2, 0),
		FIntVector(0, -2, 0)
	};

	FWaveFunctionCollapseOptionCustom GoalCorridorOption(TEXT("/Game/BP/goalt01.goalt01"));

	for (const FIntVector& Offset2 : Directions)
	{
		FIntVector Offset3 = Offset2 + Offset2 / 2;

		FIntVector Coord1Step = Offset2 / 2;       // 1칸 거리
		FIntVector Coord2Away = Coord + Offset2;   // 2칸 거리
		FIntVector Coord3Away = Coord + Offset2 + Coord1Step; // = 3칸 거리


		bool bCoord2Valid = Coord2Away.X >= 0 && Coord2Away.X < Resolution.X &&
			Coord2Away.Y >= 0 && Coord2Away.Y < Resolution.Y &&
			Coord2Away.Z >= 0 && Coord2Away.Z < Resolution.Z;

		bool bCoord3Valid = Coord3Away.X >= 0 && Coord3Away.X < Resolution.X &&
			Coord3Away.Y >= 0 && Coord3Away.Y < Resolution.Y &&
			Coord3Away.Z >= 0 && Coord3Away.Z < Resolution.Z;

		if (!bCoord2Valid || !bCoord3Valid)
		{
			continue;
		}

		int32 Index2 = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord2Away, Resolution);
		int32 Index3 = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord3Away, Resolution);

		if (!LastCollapsedTiles.IsValidIndex(Index2) || !LastCollapsedTiles.IsValidIndex(Index3)) continue;

		const auto& Tile2 = LastCollapsedTiles[Index2];
		const auto& Tile3 = LastCollapsedTiles[Index3];

		bool bIs2CompletelyEmpty = Tile2.RemainingOptions.IsEmpty();
		bool bIs2OptionEmpty = (!bIs2CompletelyEmpty && Tile2.RemainingOptions.Num() == 1 &&
			Tile2.RemainingOptions[0].BaseObject.ToString() == TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));

		UE_LOG(LogWFC, Warning, TEXT("Tile3 @ %s -> Num: %d, Option: %s, bIsCorridorTile: %d"),
			*Coord3Away.ToString(),
			Tile3.RemainingOptions.Num(),
			Tile3.RemainingOptions.Num() > 0 ? *Tile3.RemainingOptions[0].BaseObject.ToString() : TEXT("None"),
			Tile3.RemainingOptions.Num() > 0 ? Tile3.RemainingOptions[0].bIsCorridorTile : -1);

	

		bool bIs3Corridor = (Tile3.RemainingOptions.Num() == 1 && Tile3.RemainingOptions[0].bIsCorridorTile);




		// 조건: 3번째는 복도이고, 2번째는 비어 있거나 Option_Empty일 때
		if ((bIs2CompletelyEmpty || bIs2OptionEmpty) && bIs3Corridor)
		{
			// 3번째는 대체
			/*LastCollapsedTiles[Index3].RemainingOptions = { GoalCorridorOption };
			UE_LOG(LogWFC, Warning, TEXT("Replaced Tile3 at %s with goalt01"), *Coord3Away.ToString());*/
			int32 ReplaceIndex3 = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord3Away, Resolution);
			if (Tiles.IsValidIndex(ReplaceIndex3))
			{
				Tiles[ReplaceIndex3].RemainingOptions.Empty();
				Tiles[ReplaceIndex3].RemainingOptions.Add(FWaveFunctionCollapseOptionCustom(TEXT("/Game/BP/goalt01.goalt01")));
				Tiles[ReplaceIndex3].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
					Tiles[ReplaceIndex3].RemainingOptions, WFCModel);

				UE_LOG(LogWFC, Display, TEXT("Forcibly replaced tile at %s with goalt01 (Tiles array)"), *Coord3Away.ToString());
			}


			// 2번째가 Option_Empty면 대체
			if (bIs2OptionEmpty)
			{
				/*LastCollapsedTiles[Index2].RemainingOptions = { GoalCorridorOption };
				UE_LOG(LogWFC, Display, TEXT("Replaced Option_Empty with goalt01 at %s"), *Coord2Away.ToString());*/
				int32 ReplaceIndex2 = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord2Away, Resolution);
				if (Tiles.IsValidIndex(ReplaceIndex2))
				{
					Tiles[ReplaceIndex2].RemainingOptions.Empty();
					Tiles[ReplaceIndex2].RemainingOptions.Add(FWaveFunctionCollapseOptionCustom(TEXT("/Game/BP/goalt01.goalt01")));
					Tiles[ReplaceIndex2].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
						Tiles[ReplaceIndex2].RemainingOptions, WFCModel);
					UE_LOG(LogWFC, Display, TEXT("Forcibly replaced tile at %s with goalt01"), *Coord2Away.ToString());
				}
			}
			// 2번째가 완전 빈 공간이면 스폰
			else if (bIs2CompletelyEmpty)
			{
				UWorld* World = GetWorld();
				if (World)
				{
					UObject* LoadedAsset = GoalCorridorOption.BaseObject.TryLoad();
					if (LoadedAsset)
					{
						UBlueprint* GoalBP = Cast<UBlueprint>(LoadedAsset);
						if (GoalBP)
						{
							FVector Pos = FVector(Coord2Away) * WFCModel->TileSize;
							World->SpawnActor<AActor>(GoalBP->GeneratedClass, Pos, FRotator::ZeroRotator);
							UE_LOG(LogWFC, Display, TEXT("Spawned goalt01 at empty tile %s"), *Coord2Away.ToString());
						}
					}
				}
			}

			// (1) 회전 전 문 방향 기준으로 문 앞 좌표 구함
			const FWaveFunctionCollapseOptionCustom* FixedRoomOption = UserFixedOptions.Find(Coord);
			if (FixedRoomOption)
			{
				FIntVector DoorFrontCoord;

				if (FixedRoomOption->bHasDoorNorth) DoorFrontCoord = Coord + FIntVector(2, 0, 0);
				else if (FixedRoomOption->bHasDoorSouth) DoorFrontCoord = Coord + FIntVector(-2, 0, 0);
				else if (FixedRoomOption->bHasDoorEast)  DoorFrontCoord = Coord + FIntVector(0, 2, 0);
				else if (FixedRoomOption->bHasDoorWest)  DoorFrontCoord = Coord + FIntVector(0, -2, 0);

				int32 DoorFrontIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(DoorFrontCoord, Resolution);
				if (Tiles.IsValidIndex(DoorFrontIndex))
				{
					// (2) 현재 goalt01이면 Option_Empty로 교체
					if (Tiles[DoorFrontIndex].RemainingOptions.Num() == 1 &&
						Tiles[DoorFrontIndex].RemainingOptions[0].BaseObject.ToString() == TEXT("/Game/BP/goalt01.goalt01"))
					{
						Tiles[DoorFrontIndex].RemainingOptions.Empty();
						Tiles[DoorFrontIndex].RemainingOptions.Add(
							FWaveFunctionCollapseOptionCustom(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"))
						);
						Tiles[DoorFrontIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
							Tiles[DoorFrontIndex].RemainingOptions, WFCModel);

						UE_LOG(LogWFC, Display, TEXT("Converted goalt01 to Option_Empty in front of fixed room at %s"), *DoorFrontCoord.ToString());
					}
				}
			}

			return Offset2;
		}
	}
	return {};
}

void UWaveFunctionCollapseSubsystem02::AdjustRoomTileFacingDirection(int32 TileIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles, const FIntVector& CorridorDirection)
{
	if (!Tiles.IsValidIndex(TileIndex) || Tiles[TileIndex].RemainingOptions.Num() != 1)
	{
		return;
	}

	FWaveFunctionCollapseOptionCustom& RoomTileOption = Tiles[TileIndex].RemainingOptions[0];
	if (!RoomTileOption.bIsRoomTile)
	{
		return;
	}

	// 방향 벡터 -> 문자열 변환
	FString TargetDirection;
	if (CorridorDirection == FIntVector(2, 0, 0)) TargetDirection = TEXT("North");
	else if (CorridorDirection == FIntVector(-2, 0, 0)) TargetDirection = TEXT("South");
	else if (CorridorDirection == FIntVector(0, 2, 0)) TargetDirection = TEXT("East");
	else if (CorridorDirection == FIntVector(0, -2, 0)) TargetDirection = TEXT("West");
	else
	{
		UE_LOG(LogWFC, Warning, TEXT("Invalid corridor direction vector: %s"), *CorridorDirection.ToString());
		return;
	}

	RotateRoomTile(RoomTileOption, TargetDirection);

	FIntVector TilePos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
	UE_LOG(LogWFC, Display, TEXT("Forced room tile at %s to face %s"), *TilePos.ToString(), *TargetDirection);
}

// 오버로드 버전 20250419
//AActor* UWaveFunctionCollapseSubsystem02::CollapseCustom002(int32 TryCount, int32 RandomSeed)
//{
//	return CollapseCustom(TryCount, RandomSeed, TOptional<FIntVector>());
//}


//void UWaveFunctionCollapseSubsystem02::PrecomputeMapAsync(int32 TryCount, int32 RandomSeed, TFunction<void()> OnCompleted)
//{
//	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, TryCount, RandomSeed, OnCompleted]()
//	{
//
//			const double StartTime = FPlatformTime::Seconds();
//
//			// 해상도 설정
//			Resolution = FIntVector(60, 60, 1);
//
//			if (!WFCModel)
//			{
//				UE_LOG(LogWFC, Error, TEXT("[Async] WFCModel is null."));
//				return;
//			}
//
//			// 기본 설정
//			int32 SeedToUse = (RandomSeed != 0) ? RandomSeed : FMath::RandRange(1, TNumericLimits<int32>::Max());
//			int32 AttemptCount = 0;
//			bool bSuccess = false;
//
//			TArray<FWaveFunctionCollapseTileCustom> Tiles;
//			TArray<int32> RemainingTiles;
//			TMap<int32, FWaveFunctionCollapseQueueElementCustom> ObservationQueue;
//
//			InitializeWFC(Tiles, RemainingTiles);
//
//			// StarterOptions 적용
//			for (const auto& Entry : UserFixedOptions)
//			{
//				const FIntVector& Coord = Entry.Key;
//				const FWaveFunctionCollapseOptionCustom& FixedOption = Entry.Value;
//
//				int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
//
//				if (Tiles.IsValidIndex(Index))
//				{
//					Tiles[Index].RemainingOptions.Empty();
//					Tiles[Index].RemainingOptions.Add(FixedOption);
//
//					Tiles[Index].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
//						Tiles[Index].RemainingOptions, WFCModel);
//				}
//			}
//
//			// 백업
//			TArray<FWaveFunctionCollapseTileCustom> OriginalTiles = Tiles;
//			TArray<int32> OriginalRemaining = RemainingTiles;
//			FRandomStream RandomStream(SeedToUse);
//
//			// ObservationPropagation 재시도
//			while (!bSuccess && AttemptCount < TryCount)
//			{
//				AttemptCount++;
//				bSuccess = ObservationPropagation(Tiles, RemainingTiles, ObservationQueue, SeedToUse);
//
//				if (!bSuccess)
//				{
//					UE_LOG(LogWFC, Warning, TEXT("[Async] Failed with Seed: %d (Attempt %d/%d)"), SeedToUse, AttemptCount, TryCount);
//					SeedToUse = RandomStream.RandRange(1, TNumericLimits<int32>::Max());
//
//					Tiles = OriginalTiles;
//					RemainingTiles = OriginalRemaining;
//					ObservationQueue.Empty();
//				}
//			}
//
//			if (!bSuccess)
//			{
//				UE_LOG(LogWFC, Error, TEXT("[Async] ObservationPropagation failed after %d attempts."), TryCount);
//				return;
//			}
//
//			// 후처리
//			TArray<FVector> RoomTilePositions;
//			TArray<FVector> FixedRoomTilePositions;
//
//
//
//			RemoveIsolatedCorridorTiles(Tiles);
//			RemoveDisconnectedCorridors(Tiles);
//
//			for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
//			{
//				if (Tiles[TileIndex].RemainingOptions.Num() != 1) continue;
//
//				FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
//
//				if (Option.bIsRoomTile)
//				{
//					float TileSize = WFCModel->TileSize * 3.0f;
//					FVector RoomPos = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * WFCModel->TileSize;
//
//					bool bOverlapsOtherRoom = RoomTilePositions.ContainsByPredicate([&](const FVector& P) {
//						return FVector::DistSquared(RoomPos, P) < FMath::Square(TileSize * 1.5f);
//						});
//
//					FIntVector GridCoord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
//					bool bIsFixed = UserFixedOptions.Contains(GridCoord);
//					if (bIsFixed) FixedRoomTilePositions.Add(RoomPos);
//
//					bool bOverlapsFixed = FixedRoomTilePositions.ContainsByPredicate([&](const FVector& P) {
//						return FVector::DistSquared(RoomPos, P) < FMath::Square(TileSize * 1.5f);
//						});
//
//					if (bOverlapsOtherRoom && !bIsFixed)
//					{
//						Option = FWaveFunctionCollapseOptionCustom(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
//						Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy({ Option }, WFCModel);
//						continue;
//					}
//
//					RoomTilePositions.Add(RoomPos);
//
//					FVector MinBound = RoomPos - FVector(TileSize * 1.5f);
//					FVector MaxBound = RoomPos + FVector(TileSize * 0.4f);
//					for (int32 AdjIdx : GetAdjacentIndices(TileIndex, Resolution))
//					{
//						FVector AdjPos = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(AdjIdx, Resolution)) * WFCModel->TileSize;
//						if (AdjPos.X >= MinBound.X && AdjPos.X <= MaxBound.X &&
//							AdjPos.Y >= MinBound.Y && AdjPos.Y <= MaxBound.Y &&
//							AdjPos.Z >= MinBound.Z && AdjPos.Z <= MaxBound.Z)
//						{
//							Tiles[AdjIdx].RemainingOptions.Empty();
//							Tiles[AdjIdx].ShannonEntropy = 0.0f;
//						}
//					}
//
//					Option.BaseScale3D = FVector(3.0f);
//					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
//					ConnectIsolatedRooms(Tiles);
//					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
//					PlaceGoalTileInFrontOfRoom(TileIndex, Tiles);
//				}
//			}
//
//			// 고정된 방 방향 정렬
//			for (const auto& Elem : UserFixedOptions)
//			{
//				const FIntVector& Coord = Elem.Key;
//				TOptional<FIntVector> CorridorDir = PostProcessFixedRoomTileAt(Coord, Tiles);
//
//				int32 FixedIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
//				if (Tiles.IsValidIndex(FixedIndex) && CorridorDir.IsSet())
//				{
//					AdjustRoomTileFacingDirection(FixedIndex, Tiles, CorridorDir.GetValue());
//				}
//			}
//
//			// ===== WFC 후처리 정식 버전 (CollapseCustom과 동일) =====
//
//			//TSet<FVector> RoomTilePositions;
//			//TSet<FVector> FixedRoomTilePositions;
//
//			
//
//			// 먼저 고정된 방 타일 위치를 기억
//			for (const auto& Pair : UserFixedOptions)
//			{
//				int32 TileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Pair.Key, Resolution);
//				const auto& Tile = Tiles[TileIndex];
//				if (Tile.RemainingOptions.Num() == 1 && Tile.RemainingOptions[0].bIsRoomTile)
//				{
//					FVector RoomPos = FVector(Pair.Key) * WFCModel->TileSize;
//					FixedRoomTilePositions.Add(RoomPos);
//				}
//			}
//
//			// 1. 일반 방 타일 후처리
//			for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
//			{
//				if (Tiles[TileIndex].RemainingOptions.Num() != 1) continue;
//
//				FWaveFunctionCollapseOptionCustom& Option = Tiles[TileIndex].RemainingOptions[0];
//
//				if (Option.bIsRoomTile)
//				{
//					float TileSize = WFCModel->TileSize * 3.0f;
//					FVector RoomPos = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * WFCModel->TileSize;
//
//					bool bOverlapsOtherRoom = RoomTilePositions.ContainsByPredicate([&](const FVector& P) {
//						return FVector::DistSquared(RoomPos, P) < FMath::Square(TileSize * 1.5f);
//						});
//
//					FIntVector GridCoord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
//					bool bIsFixed = UserFixedOptions.Contains(GridCoord);
//					if (bIsFixed) FixedRoomTilePositions.Add(RoomPos);
//
//					bool bOverlapsFixed = FixedRoomTilePositions.ContainsByPredicate([&](const FVector& P) {
//						return FVector::DistSquared(RoomPos, P) < FMath::Square(TileSize * 1.5f);
//						});
//
//					if (bOverlapsOtherRoom && !bIsFixed)
//					{
//						Option = FWaveFunctionCollapseOptionCustom(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
//						Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy({ Option }, WFCModel);
//						continue;
//					}
//
//					RoomTilePositions.Add(RoomPos);
//
//					// 주변 겹침 방지
//					FVector MinBound = RoomPos - FVector(TileSize * 1.5f);
//					FVector MaxBound = RoomPos + FVector(TileSize * 0.4f);
//					for (int32 AdjIdx : GetAdjacentIndices(TileIndex, Resolution))
//					{
//						FVector AdjPos = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(AdjIdx, Resolution)) * WFCModel->TileSize;
//						if (AdjPos.X >= MinBound.X && AdjPos.X <= MaxBound.X &&
//							AdjPos.Y >= MinBound.Y && AdjPos.Y <= MaxBound.Y &&
//							AdjPos.Z >= MinBound.Z && AdjPos.Z <= MaxBound.Z)
//						{
//							Tiles[AdjIdx].RemainingOptions.Empty();
//							Tiles[AdjIdx].ShannonEntropy = 0.0f;
//						}
//					}
//
//					// 방 크기 및 회전 보정
//					Option.BaseScale3D = FVector(3.0f);
//					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
//				}
//			}
//
//			// 2. 고정된 방 타일 전용 후처리 (최우선)
//			for (const auto& Pair : UserFixedOptions)
//			{
//				int32 TileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Pair.Key, Resolution);
//				PostProcessFixedRoomTileAt(Pair.Key, Tiles);
//			}
//
//			// 3. 고립된 방 → 복도 연결
//			ConnectIsolatedRooms(Tiles);
//
//
//			// 결과를 GameThread에서 처리
//			AsyncTask(ENamedThreads::GameThread, [this, FinalTiles = MoveTemp(Tiles), OnCompleted]()
//			{
//					LastCollapsedTiles = FinalTiles;
//					bHasCachedTiles = true;
//
//					//SpawnActorFromTiles(LastCollapsedTiles, GetWorld());
//
//					
//
//					// 새 풀링 기반 함수 호출
//					ReuseTilePrefabsFromPool(LastCollapsedTiles, GetWorld());
//					
//
//					SpawnBorderBlueprints();
//					UE_LOG(LogWFC, Error, TEXT("fine pooling"));
//
//					if (OnCompleted) OnCompleted();
//			});
//
//			
//			const double EndTime = FPlatformTime::Seconds();
//			const double ElapsedTime = EndTime - StartTime;
//
//			UE_LOG(LogTemp, Warning, TEXT("[WFCGEN] maptime: %.3f sec"), ElapsedTime);
//
//	});
//}

//재생성(비동기버전)시, 후처리가 잘안되는 문제 있음. 개선 필요.

//void UWaveFunctionCollapseSubsystem02::PrepareTilePrefabPool(UWorld* World)
//{
//	if (!WFCModel) return;
//
//	for (const auto& Pair : WFCModel->Constraints)
//	{
//		const FWaveFunctionCollapseOptionCustom& Option = Pair.Key;
//
//		if (TilePrefabPool.Contains(Option)) continue;
//
//		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Option.BaseObject.ToString());
//		if (!Mesh) continue;
//
//		AStaticMeshActor* PrefabActor = World->SpawnActor<AStaticMeshActor>();
//
//		UStaticMeshComponent* MeshComp = PrefabActor->GetStaticMeshComponent();
//		if (!MeshComp) continue;
//
//		MeshComp->SetMobility(EComponentMobility::Movable);
//
//		PrefabActor->SetActorHiddenInGame(true);
//		PrefabActor->SetActorEnableCollision(false);
//		PrefabActor->SetActorLocation(FVector::ZeroVector);
//		PrefabActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
//		PrefabActor->SetActorScale3D(Option.BaseScale3D);
//		PrefabActor->SetActorRotation(Option.BaseRotator);
//
//		TilePrefabPool.Add(Option, PrefabActor);
//
//		UE_LOG(LogWFC, Error, TEXT("pooling"));
//	}
//}

void UWaveFunctionCollapseSubsystem02::PrepareTilePrefabPool(UWorld* World)
{
	if (!WFCModel) return;

	TilePrefabPool.Empty();

	for (const auto& Pair : WFCModel->Constraints)
	{
		const FWaveFunctionCollapseOptionCustom& Option = Pair.Key;

		if (TilePrefabPool.Contains(Option)) continue;

		

		// 1. Static Mesh 직접 로드 시도
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Option.BaseObject.ToString());
		if (Mesh)
		{
			AStaticMeshActor* PrefabActor = World->SpawnActor<AStaticMeshActor>();
			UStaticMeshComponent* MeshComp = PrefabActor->GetStaticMeshComponent();
			if (!MeshComp)
			{
				PrefabActor->Destroy();
				continue;
			}

			MeshComp->SetMobility(EComponentMobility::Movable);
			MeshComp->SetStaticMesh(Mesh);
			PrefabActor->SetActorHiddenInGame(true);
			PrefabActor->SetActorEnableCollision(false);
			PrefabActor->SetActorLocation(FVector::ZeroVector);
			PrefabActor->SetActorScale3D(Option.BaseScale3D);
			PrefabActor->SetActorRotation(Option.BaseRotator);

			TilePrefabPool.Add(Option, PrefabActor);
			UE_LOG(LogWFC, Log, TEXT("Pooling (StaticMesh): %s"), *Option.BaseObject.ToString());
			continue;
		}

		// 2. StaticMesh가 아니라면 블루프린트 액터일 가능성 → 클래스 로드

		FString ClassPath = Option.BaseObject.ToString();
		if (!ClassPath.EndsWith(TEXT("_C")))
		{
			ClassPath += TEXT("_C"); //블루프린트 클래스 경로 보정
		}


		UClass* BPClass = LoadClass<AActor>(nullptr, *ClassPath);
		if (!BPClass) continue;

		AActor* PooledActor = World->SpawnActor<AActor>(BPClass);
		if (!PooledActor) continue;

		if (World->GetAuthGameMode())  // 서버에서만
		{
			PooledActor->SetReplicates(true);
			PooledActor->SetReplicateMovement(true);
			PooledActor->bAlwaysRelevant = true;
		}

		PooledActor->SetActorHiddenInGame(true);
		PooledActor->SetActorEnableCollision(false);
		PooledActor->SetActorLocation(FVector::ZeroVector);
		PooledActor->SetActorScale3D(Option.BaseScale3D);
		PooledActor->SetActorRotation(Option.BaseRotator);

		TilePrefabPool.Add(Option, PooledActor);

		UE_LOG(LogWFC, Log, TEXT("Pooling (BP Actor): %s"), *Option.BaseObject.ToString());
	}

	


}


void UWaveFunctionCollapseSubsystem02::ReuseTilePrefabsFromPool(
	const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* World)
{
	if (!World || !WFCModel) return;

	// 서버가 아닐 땐 아무 것도 안 함
	if (!World->GetAuthGameMode()) return;

	UE_LOG(LogTemp, Warning, TEXT("ReuseTilePrefabsFromPool: Start | World: %s | Pool Size: %d"), *World->GetName(), TilePrefabPool.Num());

	// 풀이 비어있거나 액터가 유효하지 않으면 풀을 다시 생성
	bool bNeedRebuild = false;
	for (auto It = TilePrefabPool.CreateIterator(); It; ++It) {
		if (!IsValid(It.Value()) || !IsValid(It.Value()->GetClass())) {
			It.RemoveCurrent();
			bNeedRebuild = true;
		}
	}
	if (bNeedRebuild || TilePrefabPool.Num() == 0) {
		PrepareTilePrefabPool(World);
	}


	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
	{
		const FWaveFunctionCollapseTileCustom& Tile = Tiles[Index];

		// 유효한 하나의 옵션만 있는 경우만 처리
		if (Tile.RemainingOptions.Num() != 1) {
			FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
			UE_LOG(LogTemp, Warning, TEXT("[WFC] Skipped tile at (%d, %d, %d), RemainingOptions.Num() = %d"),
				Pos.X, Pos.Y, Pos.Z, Tile.RemainingOptions.Num());
			continue;
		}

		const FWaveFunctionCollapseOptionCustom& Option = Tile.RemainingOptions[0];
		if (!Option.BaseObject.IsValid())
		{
			FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
			UE_LOG(LogTemp, Warning, TEXT("[WFC] Index %d (%d, %d, %d) - BaseObject is INVALID. BaseObject: %s"),
				Index, Pos.X, Pos.Y, Pos.Z, *Option.BaseObject.ToString());
			continue;
		}

		// 회전/스케일 무시한 키로 풀링 검색
		FWaveFunctionCollapseOptionCustom CleanOption = Option;
		CleanOption.BaseRotator = FRotator::ZeroRotator;
		CleanOption.BaseScale3D = FVector(1.0f);

		if (!TilePrefabPool.Contains(CleanOption))
		{
			UE_LOG(LogWFC, Warning, TEXT("No prefab found for option: %s"), *Option.BaseObject.ToString());
			continue;
		}

		AActor* Prefab = TilePrefabPool[CleanOption];
		if (!IsValid(Prefab)) continue;

		AActor* NewTile = nullptr;

		// Case 1: StaticMeshActor이면 메시 복사 방식
		if (AStaticMeshActor* StaticMeshPrefab = Cast<AStaticMeshActor>(Prefab))
		{
			AStaticMeshActor* NewStaticMeshTile = World->SpawnActor<AStaticMeshActor>();
			if (!NewStaticMeshTile) continue;

			// ── 복제 설정 추가 ──
			/*NewStaticMeshTile->SetReplicates(true);
			NewStaticMeshTile->SetReplicateMovement(true);
			NewStaticMeshTile->bAlwaysRelevant = true;*/

			UStaticMeshComponent* MeshComp = NewStaticMeshTile->GetStaticMeshComponent();
			if (!MeshComp) continue;

			MeshComp->SetMobility(EComponentMobility::Movable);
			MeshComp->SetStaticMesh(StaticMeshPrefab->GetStaticMeshComponent()->GetStaticMesh());
			NewStaticMeshTile->SetActorScale3D(StaticMeshPrefab->GetActorScale3D());
			NewStaticMeshTile->SetActorRotation(StaticMeshPrefab->GetActorRotation());
			NewTile = NewStaticMeshTile;
		}
		else
		{
			// Case 2: 블루프린트 액터라면 동일한 클래스에서 복제
			UClass* ActorClass = Prefab->GetClass();

			// 1) 클래스 유효성 1차 검증
			if (!IsValid(ActorClass) || !ActorClass->IsChildOf(AActor::StaticClass())) {
				// 2) 폴백: 옵션 경로에서 클래스 재로드 (BP면 _C 보정)
				FString ClassPath = Option.BaseObject.ToString();
				if (!ClassPath.EndsWith(TEXT("_C"))) {
					ClassPath += TEXT("_C");
				}
				ActorClass = LoadClass<AActor>(nullptr, *ClassPath);

				if (!IsValid(ActorClass)) {
					// 그래도 실패하면 풀 재빌드 한 번 시도하고 스킵
					UE_LOG(LogWFC, Warning, TEXT("[WFC] Invalid prefab/class. Rebuilding pool and skipping this tile. %s"),
						*Option.BaseObject.ToString());
					PrepareTilePrefabPool(World);
					continue;
				}
			}

			AActor* NewActorTile = World->SpawnActor<AActor>(ActorClass);
			if (!NewActorTile) continue;

			// ── 복제 설정 추가 ──
			NewActorTile->SetReplicates(true);
			NewActorTile->SetReplicateMovement(true);
			NewActorTile->bAlwaysRelevant = true;

			NewActorTile->SetActorScale3D(Prefab->GetActorScale3D());
			NewActorTile->SetActorRotation(Prefab->GetActorRotation());
			NewTile = NewActorTile;
		}

		if (!NewTile) continue;

		// 위치 설정
		FVector Location = FVector(
			UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution)
		) * WFCModel->TileSize;
		NewTile->SetActorLocation(Location);
		NewTile->SetActorScale3D(Option.BaseScale3D);
		NewTile->SetActorRotation(Option.BaseRotator);

		// 보임 및 충돌 활성화
		NewTile->SetActorHiddenInGame(false);
		NewTile->SetActorEnableCollision(true);

		// 태그 추가
		NewTile->Tags.Add("WFCGenerated");
	}
}


//void UWaveFunctionCollapseSubsystem02::ReuseTilePrefabsFromPool(
//	const TArray<FWaveFunctionCollapseTileCustom>& Tiles, UWorld* World)
//{
//	if (!World || !WFCModel) return;
//
//	// 서버가 아닐 땐 아무 것도 안 함
//	if (!World->GetAuthGameMode()) return;
//
//	for (int32 Index = 0; Index < Tiles.Num(); ++Index)
//	{
//		const FWaveFunctionCollapseTileCustom& Tile = Tiles[Index];
//
//
//		
//
//
//		// 유효한 하나의 옵션만 있는 경우만 처리
//		if (Tile.RemainingOptions.Num() != 1) {
//			FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
//			UE_LOG(LogTemp, Warning, TEXT("[WFC] Skipped tile at (%d, %d, %d), RemainingOptions.Num() = %d"),
//				Pos.X, Pos.Y, Pos.Z, Tile.RemainingOptions.Num());
//			continue;
//		}
//
//		const FWaveFunctionCollapseOptionCustom& Option = Tile.RemainingOptions[0];
//
//		if (!Option.BaseObject.IsValid())
//		{
//			FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
//			UE_LOG(LogTemp, Warning, TEXT("[WFC] Index %d (%d, %d, %d) -  BaseObject is INVALID. BaseObject: %s"),
//				Index, Pos.X, Pos.Y, Pos.Z, *Option.BaseObject.ToString());
//			continue;
//		}
//
//
//		if (!Option.BaseObject.IsValid())
//		{
//			//UE_LOG(LogTemp, Warning, TEXT("[WFC] Index %d - BaseObject is INVALID. Option Name: %s"), Index, *Option.ToString());
//			continue;
//		}
//
//		// Key로 사용할 CleanOption 생성 (회전, 스케일 무시)
//		FWaveFunctionCollapseOptionCustom CleanOption = Option;
//		CleanOption.BaseRotator = FRotator::ZeroRotator;
//		CleanOption.BaseScale3D = FVector(1.0f);
//
//		// 풀링에서 회전/스케일 무시한 키로 탐색
//		if (!TilePrefabPool.Contains(CleanOption))
//		{
//			UE_LOG(LogWFC, Warning, TEXT("No prefab found for option: %s"), *Option.BaseObject.ToString());
//			continue;
//		}
//
//		if (!TilePrefabPool.Contains(CleanOption))
//		{
//			FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
//			UE_LOG(LogTemp, Warning, TEXT("[WFC] Index %d (%d, %d, %d) -  Pool doesn't contain CleanOption. BaseObject: %s"),
//				Index, Pos.X, Pos.Y, Pos.Z, *Option.BaseObject.ToString());
//			continue;
//		}
//
//
//		// 풀링된 프리팹 가져오기
//		AActor* Prefab = nullptr;
//		/*if (!TilePrefabPool.Contains(Option))
//		{
//			UE_LOG(LogWFC, Warning, TEXT("No prefab found for option: %s"), *Option.BaseObject.ToString());
//			continue;
//		}*/
//
//		Prefab = TilePrefabPool[CleanOption];
//		if (!IsValid(Prefab)) continue;
//
//		AActor* NewTile = nullptr;
//
//		//Case 1: StaticMeshActor이면 메시 복사 방식
//		if (AStaticMeshActor* StaticMeshPrefab = Cast<AStaticMeshActor>(Prefab))
//		{
//			AStaticMeshActor* NewStaticMeshTile = World->SpawnActor<AStaticMeshActor>();
//			if (!NewStaticMeshTile) continue;
//
//			UStaticMeshComponent* MeshComp = NewStaticMeshTile->GetStaticMeshComponent();
//			if (!MeshComp) continue;
//
//			MeshComp->SetMobility(EComponentMobility::Movable);
//			MeshComp->SetStaticMesh(StaticMeshPrefab->GetStaticMeshComponent()->GetStaticMesh());
//			NewStaticMeshTile->SetActorScale3D(StaticMeshPrefab->GetActorScale3D());
//			NewStaticMeshTile->SetActorRotation(StaticMeshPrefab->GetActorRotation());
//			NewTile = NewStaticMeshTile;
//		}
//		else
//		{
//			//Case 2: 블루프린트 액터라면 동일한 클래스에서 복제
//			UClass* ActorClass = Prefab->GetClass();
//			NewTile = World->SpawnActor<AActor>(ActorClass);
//			if (!NewTile) continue;
//
//			NewTile->SetActorScale3D(Prefab->GetActorScale3D());
//			NewTile->SetActorRotation(Prefab->GetActorRotation());
//
//			if (!NewTile)
//			{
//				FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution);
//				UE_LOG(LogTemp, Warning, TEXT("[WFC] SpawnActor<AActor> failed at (%d, %d, %d) - %s"),
//					Pos.X, Pos.Y, Pos.Z, *Option.BaseObject.ToString());
//				continue;
//			}
//
//		}
//
//		if (!NewTile) continue;
//
//		// 위치 설정
//		FVector Location = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Index, Resolution)) * WFCModel->TileSize;
//		NewTile->SetActorLocation(Location);
//		NewTile->SetActorScale3D(Option.BaseScale3D); 
//		NewTile->SetActorRotation(Option.BaseRotator); 
//		NewTile->SetActorHiddenInGame(false);
//		NewTile->SetActorEnableCollision(true);
//		NewTile->Tags.Add("WFCGenerated");
//
//		//SpawnedTileActors.Add(NewTile);
//	}
//}


void UWaveFunctionCollapseSubsystem02::PrecomputeMapAsync(int32 TryCount, int32 RandomSeed, TFunction<void()> OnCompleted)
{
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, TryCount, RandomSeed, OnCompleted]()
		{
			const double StartTime = FPlatformTime::Seconds();

			// 해상도 설정
			Resolution = FIntVector(60, 60, 1);

			if (!WFCModel)
			{
				UE_LOG(LogWFC, Error, TEXT("[Async] WFCModel is null."));
				return;
			}

			// 기본 설정S
			//int32 SeedToUse = (RandomSeed != 0) ? RandomSeed : FMath::RandRange(1, TNumericLimits<int32>::Max());
			int32 SeedToUse = static_cast<int32>(FDateTime::Now().GetTicks() % INT32_MAX);
			int32 AttemptCount = 0;
			bool bSuccess = false;

			TArray<FWaveFunctionCollapseTileCustom> Tiles;
			TArray<int32> RemainingTiles;
			TMap<int32, FWaveFunctionCollapseQueueElementCustom> ObservationQueue;
			TArray<int32> RoomTileIndices;

			InitializeWFC(Tiles, RemainingTiles);

			// StarterOptions 적용
			for (const auto& Entry : UserFixedOptions)
			{
				const FIntVector& Coord = Entry.Key;
				const FWaveFunctionCollapseOptionCustom& FixedOption = Entry.Value;

				int32 Index = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

				if (Tiles.IsValidIndex(Index))
				{
					Tiles[Index].RemainingOptions.Empty();
					Tiles[Index].RemainingOptions.Add(FixedOption);

					Tiles[Index].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
						Tiles[Index].RemainingOptions, WFCModel);
				}
			}

			// 백업
			TArray<FWaveFunctionCollapseTileCustom> OriginalTiles = Tiles;
			TArray<int32> OriginalRemaining = RemainingTiles;
			FRandomStream RandomStream(SeedToUse);

			// ObservationPropagation 재시도
			while (!bSuccess && AttemptCount < TryCount)
			{
				AttemptCount++;
				bSuccess = ObservationPropagation(Tiles, RemainingTiles, ObservationQueue, SeedToUse);

				if (!bSuccess)
				{
					//UE_LOG(LogWFC, Warning, TEXT("[Async] Failed with Seed: %d (Attempt %d/%d)"), SeedToUse, AttemptCount, TryCount);
					SeedToUse = RandomStream.RandRange(1, TNumericLimits<int32>::Max());

					Tiles = OriginalTiles;
					RemainingTiles = OriginalRemaining;
					ObservationQueue.Empty();
				}
			}

			if (!bSuccess)
			{
				UE_LOG(LogWFC, Error, TEXT("[Async] ObservationPropagation failed after %d attempts."), TryCount);
				return;
			}

			// ===== 후처리 로직 (CollapseCustom과 동일한 순서로 개선) =====
			double PostProcessStart = FPlatformTime::Seconds();

			// 1. 고립된 복도와 연결되지 않은 복도 제거
			RemoveIsolatedCorridorTiles(Tiles);
			RemoveDisconnectedCorridors(Tiles);

			// 2. 방 타일 위치 저장용 배열
			TArray<FVector> RoomTilePositions;
			TArray<FVector> FixedRoomTilePositions;

			// 3. 먼저 고정된 방 타일 위치를 기억 (CollapseCustom과 동일)
			for (const auto& Entry : UserFixedOptions)
			{
				const FIntVector& Coord = Entry.Key;
				int32 TileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

				if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() == 1)
				{
					const auto& Option = Tiles[TileIndex].RemainingOptions[0];
					if (Option.bIsRoomTile)
					{
						FVector RoomPos = FVector(Coord) * WFCModel->TileSize;
						FixedRoomTilePositions.Add(RoomPos);
						RoomTilePositions.Add(RoomPos);
					}
				}
			}

			// 4. 고정된 방 타일 주변 정리 (새로 추가)
			for (const auto& Entry : UserFixedOptions)
			{
				const FIntVector& Coord = Entry.Key;
				int32 TileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);

				if (Tiles.IsValidIndex(TileIndex) && Tiles[TileIndex].RemainingOptions.Num() == 1)
				{
					const auto& Option = Tiles[TileIndex].RemainingOptions[0];
					if (Option.bIsRoomTile)
					{
						// 8방향으로 2칸 떨어진 타일들 처리
						for (int32 X = -2; X <= 2; ++X)
						{
							for (int32 Y = -2; Y <= 2; ++Y)
							{
								if (X == 0 && Y == 0) continue; // 자기 자신은 제외

								FIntVector TargetCoord = Coord + FIntVector(X, Y, 0);
								int32 TargetIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(TargetCoord, Resolution);

								if (Tiles.IsValidIndex(TargetIndex) && Tiles[TargetIndex].RemainingOptions.Num() == 1)
								{
									const auto& TargetOption = Tiles[TargetIndex].RemainingOptions[0];

									// Empty 타일이나 Option_Empty는 건드리지 않음
									FString AssetPath = TargetOption.BaseObject.GetAssetPathString();
									if (AssetPath.Contains(TEXT("Option_Empty")))
									{
										continue;
									}

									// goalt01로 교체
									FWaveFunctionCollapseOptionCustom GoalTileOption(TEXT("/Game/BP/goalt01.goalt01"));
									GoalTileOption.bIsRoomTile = false;
									GoalTileOption.bIsCorridorTile = true;
									Tiles[TargetIndex].RemainingOptions.Empty();
									Tiles[TargetIndex].RemainingOptions.Add(GoalTileOption);

									Tiles[TargetIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
										Tiles[TargetIndex].RemainingOptions, WFCModel);
								}
							}
						}
					}
				}
			}

			// 5. 방 타일 후처리 (CollapseCustom과 동일한 로직)
			for (int32 TileIndex = 0; TileIndex < Tiles.Num(); ++TileIndex)
			{
				if (Tiles[TileIndex].RemainingOptions.Num() != 1) continue;

				FWaveFunctionCollapseOptionCustom& SelectedOption = Tiles[TileIndex].RemainingOptions[0];

				if (SelectedOption.bIsRoomTile)
				{

					RoomTileIndices.Add(TileIndex);

					float TileSize = WFCModel->TileSize * 3.0f;
					FVector RoomTilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution)) * WFCModel->TileSize;

					// 겹침 여부 확인
					bool bOverlapsOtherRoomTile = RoomTilePositions.ContainsByPredicate([&](const FVector& P) {
						return FVector::DistSquared(RoomTilePosition, P) < FMath::Square(TileSize * 1.5f);
						});

					// 고정된 방인지 확인
					FIntVector GridCoord = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
					bool bIsUserFixed = UserFixedOptions.Contains(GridCoord);

					// 고정된 방과 겹치는지 확인
					bool bOverlapsFixedRoom = FixedRoomTilePositions.ContainsByPredicate([&](const FVector& P) {
						return FVector::DistSquared(RoomTilePosition, P) < FMath::Square(TileSize * 1.5f);
						});

					// 겹치는 경우 처리 (고정된 방이 아닌 경우만)
					if (bOverlapsOtherRoomTile && !bIsUserFixed)
					{
						UE_LOG(LogWFC, Display, TEXT("[Async] Room tile overlaps, replacing with alternative tile"));

						// 대체 타일로 변경
						FWaveFunctionCollapseOptionCustom AlternativeTileOption(TEXT("/Game/BP/goalt01.goalt01"));
						Tiles[TileIndex].RemainingOptions.Empty();
						Tiles[TileIndex].RemainingOptions.Add(AlternativeTileOption);

						Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
							Tiles[TileIndex].RemainingOptions, WFCModel);

						// 고립된 타일인지 검사
						if (IsIsolatedTile(TileIndex, Tiles))
						{
							UE_LOG(LogWFC, Display, TEXT("[Async] Isolated tile detected, replacing with empty option"));

							FWaveFunctionCollapseOptionCustom ReplacementTile(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty"));
							Tiles[TileIndex].RemainingOptions.Empty();
							Tiles[TileIndex].RemainingOptions.Add(ReplacementTile);

							Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
								Tiles[TileIndex].RemainingOptions, WFCModel);
						}

						continue;
					}

					// 겹치지 않는 경우 처리
					RoomTilePositions.Add(RoomTilePosition);

					// 방 타일 주변 영역 정리
					//FVector MinBound = RoomTilePosition - FVector(TileSize * 1.5f);
					//FVector MaxBound = RoomTilePosition + FVector(TileSize * 0.4f);
					FVector MinBound = RoomTilePosition - FVector(TileSize * 1.5f, TileSize * 1.5f, TileSize * 1.5f);
					FVector MaxBound = RoomTilePosition + FVector(TileSize * 0.4f, TileSize * 0.4f, TileSize * 0.4f);
					TArray<int32> AdjacentIndices = GetAdjacentIndices(TileIndex, Resolution);
					for (int32 AdjacentIndex : AdjacentIndices)
					{
						FVector AdjacentTilePosition = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(AdjacentIndex, Resolution)) * WFCModel->TileSize;

						if (AdjacentTilePosition.X >= MinBound.X && AdjacentTilePosition.X <= MaxBound.X &&
							AdjacentTilePosition.Y >= MinBound.Y && AdjacentTilePosition.Y <= MaxBound.Y &&
							AdjacentTilePosition.Z >= MinBound.Z && AdjacentTilePosition.Z <= MaxBound.Z)
						{
							Tiles[AdjacentIndex].RemainingOptions.Empty();
							Tiles[AdjacentIndex].ShannonEntropy = 0.0f;
						}
					}

					// 방 타일 크기 설정
					SelectedOption.BaseScale3D = FVector(3.0f);

					// 방 타일 후처리 순서 (CollapseCustom과 동일)
					AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
					//ConnectIsolatedRooms(Tiles);
					//AdjustRoomTileBasedOnCorridors(TileIndex, Tiles);
					//PlaceGoalTileInFrontOfRoom(TileIndex, Tiles);
				}
			}

			RemoveIsolatedCorridorTiles(Tiles);
			RemoveDisconnectedCorridors(Tiles);

			ConnectIsolatedRoomsDijkstra(Tiles);

			//ConnectIsolatedRooms(Tiles);

			// 6. 고정된 방 타일 전용 후처리 (CollapseCustom과 동일)
			/*for (const auto& Entry : UserFixedOptions)
			{
				const FIntVector& Coord = Entry.Key;
				TOptional<FIntVector> CorridorDirection = PostProcessFixedRoomTileAt(Coord, Tiles);

				int32 FixedTileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
				if (Tiles.IsValidIndex(FixedTileIndex) && CorridorDirection.IsSet())
				{
					AdjustRoomTileFacingDirection(FixedTileIndex, Tiles, CorridorDirection.GetValue());
				}
			}*/

			// 6. 고정된 방 타일도 RoomTileIndices에 추가하여 동일한 후처리 적용
			for (const auto& Entry : UserFixedOptions)
			{
				const FIntVector& Coord = Entry.Key;
				const FWaveFunctionCollapseOptionCustom& FixedOption = Entry.Value;

				// 고정된 방 타일인지 확인
				if (FixedOption.bIsRoomTile)
				{
					int32 FixedRoomIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(Coord, Resolution);
					if (Tiles.IsValidIndex(FixedRoomIndex))
					{
						// RoomTileIndices에 추가 (중복 방지)
						RoomTileIndices.AddUnique(FixedRoomIndex);
						UE_LOG(LogWFC, Display, TEXT("Added fixed room tile at %s to post-processing queue"), *Coord.ToString());
					}
				}
			}

			for (int32 RoomTileIndex : RoomTileIndices)
			{
				AdjustRoomTileBasedOnCorridors(RoomTileIndex, Tiles);
				PlaceGoalTileInFrontOfRoom(RoomTileIndex, Tiles);
			}

			

			double PostProcessEnd = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Warning, TEXT("[Async] Post-processing took: %.3f seconds"), PostProcessEnd - PostProcessStart);

			// 결과를 GameThread에서 처리
			AsyncTask(ENamedThreads::GameThread, [this, FinalTiles = MoveTemp(Tiles), RoomTilePositions, RoomTileIndices, OnCompleted]()
				{
					LastCollapsedTiles = FinalTiles;
					bHasCachedTiles = true;
					this->RoomTilePositions = RoomTilePositions;

					

					// 새 풀링 기반 함수 호출
					ReuseTilePrefabsFromPool(LastCollapsedTiles, GetWorld());

					SpawnBorderBlueprints();
					UE_LOG(LogWFC, Log, TEXT("[Async] Pooling completed successfully"));

					this->RoomTilePositions.Empty();



					if (OnCompleted) OnCompleted();
				});

			const double EndTime = FPlatformTime::Seconds();
			const double ElapsedTime = EndTime - StartTime;

			SeedToUse = 0;

			UE_LOG(LogTemp, Warning, TEXT("[WFCGEN] Dij Total async map generation time: %.3f sec"), ElapsedTime);
		});

}

void UWaveFunctionCollapseSubsystem02::FindPathDijkstra(
    const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
    const TSet<int32>& StartIndices,
    TMap<int32, int32>& OutPrevious,
    TMap<int32, int32>& OutDistance)
{
    // 초기화
    TSet<int32> Visited;
    TMap<int32, int32> Distance;
    TMap<int32, int32> Previous;
	//const FIntVector LocalResolution = this->Resolution;

	auto IsValidPosition = [](const FIntVector& Pos, const FIntVector& Res) -> bool
		{
			return Pos.X >= 0 && Pos.X < Res.X &&
				Pos.Y >= 0 && Pos.Y < Res.Y &&
				Pos.Z >= 0 && Pos.Z < Res.Z;
		};

	const TArray<FIntVector> NeighbourDirections = {
	FIntVector(1, 0, 0),   // +X
	FIntVector(-1, 0, 0),  // -X
	FIntVector(0, 1, 0),   // +Y
	FIntVector(0, -1, 0)   // -Y
	};

    struct FDirectionalNode
    {
        int32 Index;
        int32 Cost;
		FIntVector LastDirection; // 이전 이동 방향
		int32 SameDirectionCount; // 같은 방향으로 연속 이동한 횟수

        bool operator<(const FDirectionalNode& Other) const
        {
            return Cost > Other.Cost; // Min Heap용
        }
    };

    TArray<FDirectionalNode> OpenSet;

	// 방향별 방문 체크 (Index + Direction 조합)
	TSet<FString> DirectionalVisited;

	

    for (int32 StartIndex : StartIndices)
    {
        Distance.Add(StartIndex, 0);
        Previous.Add(StartIndex, -1);
		// 각 방향으로 시작할 수 있도록 추가
		for (const FIntVector& Dir : NeighbourDirections)
		{
			OpenSet.Add({
				StartIndex,
				0,
				Dir,  // 첫 번째 이동 방향
				0     // 아직 이동하지 않음
				});
		}
    }

    while (OpenSet.Num() > 0)
    {
        // 최소 비용 노드 선택
        OpenSet.Sort();  // 작은 Cost 먼저
		FDirectionalNode Current = OpenSet[0];
        OpenSet.RemoveAt(0);

		// 방향을 포함한 고유 키 생성
		FString DirectionalKey = FString::Printf(TEXT("%d_%d_%d_%d"),
			Current.Index, Current.LastDirection.X, Current.LastDirection.Y, Current.LastDirection.Z);

		if (DirectionalVisited.Contains(DirectionalKey))
			continue;

		DirectionalVisited.Add(DirectionalKey);


        FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(Current.Index, this->Resolution);

        // 4방향 이웃
        for (FIntVector Dir : NeighbourDirections)
        {
            FIntVector NeighborPos = Pos + Dir;
            if (!IsValidPosition(NeighborPos, this->Resolution)) continue;

            int32 NeighborIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPos, this->Resolution);
            

			// 방향 제약 조건 검사
			bool bCanMove = true;
			int32 NewSameDirectionCount = 0;

			if (Current.SameDirectionCount == 0)
			{
				// 첫 번째 이동: 어떤 방향이든 가능
				NewSameDirectionCount = 1;
			}
			else if (Current.SameDirectionCount == 1)
			{
				// 두 번째 이동: 같은 방향으로만 가능
				if (Dir != Current.LastDirection)
				{
					bCanMove = false; // 다른 방향으로는 이동 불가
				}
				else
				{
					NewSameDirectionCount = 2;
				}
			}
			else
			{
				// 세 번째 이동부터: 자유롭게 이동 가능
				NewSameDirectionCount = (Dir == Current.LastDirection) ? Current.SameDirectionCount + 1 : 1;
			}

			if (!bCanMove)
				continue;

            int32 NewCost = Distance[Current.Index] + 1; // 전부 비용 1로

            if (!Distance.Contains(NeighborIndex) || NewCost < Distance[NeighborIndex])
            {
                Distance.Add(NeighborIndex, NewCost);
                Previous.Add(NeighborIndex, Current.Index);
				OpenSet.Add({
					NeighborIndex,
					NewCost,
					Dir,  // 현재 이동 방향
					NewSameDirectionCount
					});
            }
        }
    }

    OutDistance = Distance;
    OutPrevious = Previous;
}


TArray<int32> UWaveFunctionCollapseSubsystem02::BuildPathFromDijkstra(
	int32 TargetIndex,
	const TMap<int32, int32>& Previous)
{
	TArray<int32> Path;
	int32 Current = TargetIndex;

	while (Previous.Contains(Current) && Current != -1)
	{
		Path.Add(Current);
		Current = Previous[Current];
	}

	Algo::Reverse(Path);
	return Path;
}

bool UWaveFunctionCollapseSubsystem02::IsCorridorTile(const FWaveFunctionCollapseTileCustom& Tile)
{
	if (Tile.RemainingOptions.Num() == 1)
	{
		return Tile.RemainingOptions[0].bIsCorridorTile;
	}
	return false;
}

void UWaveFunctionCollapseSubsystem02::ConnectIsolatedRoomsDijkstra(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	TSet<int32> CorridorIndices;
	TSet<int32> RoomIndices;
	


	// 1. 모든 타일 중 복도와 방을 분류
	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		const FWaveFunctionCollapseTileCustom& Tile = Tiles[i];


		if (Tiles[i].RemainingOptions.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Tile[%d] is EMPTY"), i);
			continue;
		}
		const auto& Options = Tiles[i].RemainingOptions[0];
		UE_LOG(LogTemp, Warning, TEXT("Tile[%d] Option=%s RoomFlag=%d"),
			i, *Options.BaseObject.ToString(), Options.bIsRoomTile);



		if (Tile.RemainingOptions.Num() == 1)
		{

			


			
			
			const FString PathStr = Options.BaseObject.ToString();

			const bool bInvalidTile =
				PathStr.Contains("Option_Empty") ||
				PathStr.Contains("VoidOption") ||  // 있는 경우
				WFCModel->SpawnExclusion.Contains(Options.BaseObject);

			if (bInvalidTile)
			{
				continue; // 완전 필터링
			}

			
			if (Options.bIsCorridorTile)
			{
				CorridorIndices.Add(i);
			}
			
			


			
			const bool bLooksLikeRoom =
				Options.bIsRoomTile &&
				!Options.bIsCorridorTile &&
				!Options.BaseObject.IsNull() &&
				Options.BaseObject.ToString().Contains("romm") &&     // 방 이름 규칙이 있다면
				!Options.BaseObject.ToString().Contains("goalt01") &&
				!Options.BaseObject.ToString().Contains("Option_Empty");

				

			if (bLooksLikeRoom)
			{
				// 현재 타일이 실제로 방 타일인지 다시 한번 확인
				if (Tiles[i].RemainingOptions.Num() == 1 &&
					Tiles[i].RemainingOptions[0].bIsRoomTile)
				{
					RoomIndices.Add(i);
				}
			}
		}
	}

	



	
	UE_LOG(LogTemp, Warning, TEXT("[WFC] Room=%d  Corridor=%d"),
		RoomIndices.Num(), CorridorIndices.Num());

	TSet<int32> DisconnectedRooms = FindDisconnectedRoomIndices(RoomIndices, CorridorIndices, Tiles, this->Resolution);

	UE_LOG(LogTemp, Warning, TEXT("[WFC] DisconnectedRooms=%d"), DisconnectedRooms.Num());







	// 3. 다익스트라 경로 탐색
	TMap<int32, int32> Previous;
	TMap<int32, int32> Distance;
	//FindPathDijkstra(Tiles, CorridorIndices, Previous, Distance);
	const float TileSize = 500.f;
	// 4. 각 고립된 방에서 가장 가까운 복도까지의 경로 생성 및 복도 채우기

	

	for (int32 RoomIndex : DisconnectedRooms)
	{

		

		// 1) 이 방 하나만을 출발점으로 다익스트라
		TSet<int32> Start;
		Start.Add(RoomIndex);

		if (Tiles.IsValidIndex(RoomIndex) && Tiles[RoomIndex].RemainingOptions.Num() > 0)
		{
			const FString ObjectName = Tiles[RoomIndex].RemainingOptions[0].BaseObject.ToString();
			UE_LOG(LogTemp, Warning, TEXT("  DisconnectedRoom  Idx=%d  Obj=%s"), RoomIndex, *ObjectName);
		}

		TMap<int32, int32> Prev;
		TMap<int32, int32> Dist;
		FindPathDijkstra(Tiles, Start, Prev, Dist);     // 시작점 = RoomIndex

		// 2) 가장 가까운 복도 찾기
		int32 BestCorridor = -1;
		int32 BestDist = TNumericLimits<int32>::Max();

		for (int32 CorridorIdx : CorridorIndices)
		{
			if (Dist.Contains(CorridorIdx) && Dist[CorridorIdx] < BestDist)
			{
				BestDist = Dist[CorridorIdx];
				BestCorridor = CorridorIdx;
			}
		}

		if (BestCorridor != -1)
		{
			TArray<int32> Path = BuildPathFromDijkstra(BestCorridor, Prev); // 복도→…→Room
			if (Path.Num() > 0)
			{
				int32 RoomIdx = Path[0];

				if (!Tiles[RoomIdx].RemainingOptions.IsEmpty())
				{
					const FWaveFunctionCollapseOptionCustom& Option = Tiles[RoomIdx].RemainingOptions[0];

					const bool bRealRoom =
						Option.bIsRoomTile &&
						!Option.BaseObject.ToString().Contains("goalt01") &&
						!Option.BaseObject.ToString().Contains("Option_Empty");



					if (bRealRoom)
					{

						UE_LOG(LogWFC, Warning, TEXT("Roomghghgh : Index=%d, Asset=%s, IsRoom=%d, IsCorridor=%d"),
							RoomIdx,
							*Option.BaseObject.ToString(),
							Option.bIsRoomTile,
							Option.bIsCorridorTile);


						//Path.RemoveAt(0); // 진짜 방일 때만 제외
						if (Path.Num() > 0)
							FillEmptyTilesAlongPath(Path, Tiles);

//#if WITH_EDITOR
//						// 디버그 라인 (선택)
//						for (int32 i = 0; i < Path.Num() - 1; ++i)
//						{
//							const int32 IndexA = Path[i];
//							const int32 IndexB = Path[i + 1];
//							const FIntVector PosA = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(IndexA, this->Resolution);
//							const FIntVector PosB = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(IndexB, this->Resolution);
//							const FVector WorldA = FVector(PosA) * TileSize + FVector(0, 0, 50);
//							const FVector WorldB = FVector(PosB) * TileSize + FVector(0, 0, 50);
//							DrawDebugLine(GetWorld(), WorldA, WorldB, FColor::Blue, true, 10.0f, 0, 5.0f);
//						}
//#endif
					}
					else
					{
						// 방이 아닌 경우는 경로 무시
						UE_LOG(LogWFC, Warning, TEXT("[Dijkstra] Rejected path: Start is not a real room. Index: %d, Asset: %s"), RoomIdx, *Option.BaseObject.ToString());
					}
				}
			}
		}
	}
}

TSet<int32> UWaveFunctionCollapseSubsystem02::FindDisconnectedRoomIndices(
	const TSet<int32>& RoomIndices,
	const TSet<int32>& CorridorIndices,
	const TArray<FWaveFunctionCollapseTileCustom>& Tiles,
	const FIntVector& LocalResolution)
{
	TSet<int32> Result;

	auto IsValidPosition = [](const FIntVector& Pos, const FIntVector& Res) -> bool {
		return Pos.X >= 0 && Pos.X < Res.X &&
			Pos.Y >= 0 && Pos.Y < Res.Y &&
			Pos.Z >= 0 && Pos.Z < Res.Z;
		};

	for (int32 RoomIndex : RoomIndices)
	{

		FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(RoomIndex, LocalResolution);
		bool bConnected = false;

		for (const FIntVector& Dir : { FIntVector(2,0,0), FIntVector(-2,0,0), FIntVector(0,2,0), FIntVector(0,-2,0) })
		{
			FIntVector NeighborPos = Pos + Dir;
			if (!IsValidPosition(NeighborPos, LocalResolution)) continue;

			int32 NeighborIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(NeighborPos, LocalResolution);
			if (CorridorIndices.Contains(NeighborIndex))
			{
				bConnected = true;
				break;
			}
		}

		if (!bConnected)
		{
			Result.Add(RoomIndex);
		}
	}

	return Result;
}

void UWaveFunctionCollapseSubsystem02::ClearTilePrefabPool()
{
	for (auto& Pair : TilePrefabPool)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->Destroy();
		}
	}
	TilePrefabPool.Empty();

	// 다른 캐시된 데이터도 정리
	LastCollapsedTiles.Empty();
	RoomTilePositions.Empty();
	UserFixedOptions.Empty();
	bHasCachedTiles = false;
	bWFCCompleted = false;
}

void UWaveFunctionCollapseSubsystem02::SimpleRoomCountControl(TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	// 현재 방 개수 세기
	int32 CurrentRooms = 0;
	TArray<int32> RoomIndices;

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		if (Tiles[i].RemainingOptions.Num() == 1 &&
			Tiles[i].RemainingOptions[0].bIsRoomTile)
		{
			CurrentRooms++;
			RoomIndices.Add(i);
		}
	}

	// 목표 방 개수 정하기
	int32 TargetRooms = FMath::RandRange(MinRoomCount, MaxRoomCount);

	UE_LOG(LogWFC, Warning, TEXT("Room Control: Current=%d, Target=%d"), CurrentRooms, TargetRooms);

	// === 방이 부족하면 추가 ===
	if (CurrentRooms < TargetRooms)
	{
		int32 RoomsToAdd = TargetRooms - CurrentRooms;
		TArray<int32> EmptyTiles;

		// 비어있는 타일 찾기
		for (int32 i = 0; i < Tiles.Num(); ++i)
		{
			if (Tiles[i].RemainingOptions.IsEmpty() ||
				(Tiles[i].RemainingOptions.Num() == 1 &&
					Tiles[i].RemainingOptions[0].BaseObject.ToString().Contains("Option_Empty")))
			{
				// 가장자리는 피하기
				FIntVector Pos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(i, Resolution);
				if (Pos.X > 5 && Pos.X < Resolution.X - 5 && Pos.Y > 5 && Pos.Y < Resolution.Y - 5)
				{
					EmptyTiles.Add(i);
				}
			}
		}

		// 랜덤하게 방 추가
		for (int32 i = 0; i < RoomsToAdd && EmptyTiles.Num() > 0; ++i)
		{
			int32 RandomIndex = FMath::RandRange(0, EmptyTiles.Num() - 1);
			int32 TileIndex = EmptyTiles[RandomIndex];
			EmptyTiles.RemoveAt(RandomIndex);

			// 방 타일로 바꾸기
			FWaveFunctionCollapseOptionCustom RoomOption(TEXT("/Game/BP/BSP/romm.romm"));
			RoomOption.bIsRoomTile = true;
			RoomOption.bIsCorridorTile = false;
			RoomOption.BaseScale3D = FVector(3.0f);

			Tiles[TileIndex].RemainingOptions.Empty();
			Tiles[TileIndex].RemainingOptions.Add(RoomOption);

			UE_LOG(LogWFC, Log, TEXT("Added room at index %d"), TileIndex);
		}
	}

	// === 방이 너무 많으면 제거 ===
	else if (CurrentRooms > TargetRooms)
	{
		int32 RoomsToRemove = CurrentRooms - TargetRooms;

		// 맵 가장자리부터 제거
		FVector MapCenter = FVector(Resolution.X / 2, Resolution.Y / 2, 0);

		RoomIndices.Sort([this, MapCenter](const int32& A, const int32& B) {
			FVector PosA = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(A, Resolution));
			FVector PosB = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(B, Resolution));

			float DistA = FVector::DistSquared(PosA, MapCenter);
			float DistB = FVector::DistSquared(PosB, MapCenter);

			return DistA > DistB; // 멀리 있는 것부터
			});

		for (int32 i = 0; i < RoomsToRemove && i < RoomIndices.Num(); ++i)
		{
			int32 TileIndex = RoomIndices[i];

			// 고정된 방은 건드리지 않기
			FIntVector GridPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
			if (UserFixedOptions.Contains(GridPos)) continue;

			// Empty 타일로 바꾸기
			FWaveFunctionCollapseOptionCustom EmptyOption(TEXT("/Game/WFCCORE/wfc/SpecialOption/Option_Empty.Option_Empty"));
			Tiles[TileIndex].RemainingOptions.Empty();
			Tiles[TileIndex].RemainingOptions.Add(EmptyOption);

			UE_LOG(LogWFC, Log, TEXT("Removed room at index %d"), TileIndex);
		}
	}

	//// === 방이 너무 많으면 goalt01로 대체 ===
	//else if (CurrentRooms > TargetRooms)
	//{
	//	int32 RoomsToRemove = CurrentRooms - TargetRooms;

	//	// 맵 가장자리부터 제거
	//	FVector MapCenter = FVector(Resolution.X / 2, Resolution.Y / 2, 0);

	//	RoomIndices.Sort([this, MapCenter](const int32& A, const int32& B) {
	//		FVector PosA = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(A, Resolution));
	//		FVector PosB = FVector(UWaveFunctionCollapseBPLibrary02::IndexAsPosition(B, Resolution));

	//		float DistA = FVector::DistSquared(PosA, MapCenter);
	//		float DistB = FVector::DistSquared(PosB, MapCenter);

	//		return DistA > DistB; // 멀리 있는 것부터
	//		});

	//	for (int32 i = 0; i < RoomsToRemove && i < RoomIndices.Num(); ++i)
	//	{
	//		int32 TileIndex = RoomIndices[i];

	//		// 고정된 방은 건드리지 않기
	//		FIntVector GridPos = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(TileIndex, Resolution);
	//		if (UserFixedOptions.Contains(GridPos)) continue;

	//		// 방 타일을 3x3 goalt01 타일로 대체
	//		ReplaceRoomWithGoaltTiles(TileIndex, Tiles);
	//	}
	//}

	// 최종 방 개수 확인
	int32 FinalRooms = 0;
	for (const auto& Tile : Tiles)
	{
		if (Tile.RemainingOptions.Num() == 1 && Tile.RemainingOptions[0].bIsRoomTile)
		{
			FinalRooms++;
		}
	}

	UE_LOG(LogWFC, Warning, TEXT("Final room count: %d"), FinalRooms);
}


void UWaveFunctionCollapseSubsystem02::ReplaceRoomWithGoaltTiles(int32 RoomIndex, TArray<FWaveFunctionCollapseTileCustom>& Tiles)
{
	// 방 타일의 중심 위치
	FIntVector RoomCenter = UWaveFunctionCollapseBPLibrary02::IndexAsPosition(RoomIndex, Resolution);

	// 3x3 영역 설정 (방 타일이 차지하는 공간)
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			FIntVector TilePos = RoomCenter + FIntVector(dx, dy, 0);

			// 유효한 위치인지 확인
			if (TilePos.X >= 0 && TilePos.X < Resolution.X &&
				TilePos.Y >= 0 && TilePos.Y < Resolution.Y)
			{
				int32 TileIndex = UWaveFunctionCollapseBPLibrary02::PositionAsIndex(TilePos, Resolution);

				if (Tiles.IsValidIndex(TileIndex))
				{
					// goalt01 타일로 대체
					FWaveFunctionCollapseOptionCustom GoaltOption(TEXT("/Game/BP/goalt01.goalt01"));
					GoaltOption.bIsCorridorTile = true;
					GoaltOption.bIsRoomTile = false;

					Tiles[TileIndex].RemainingOptions.Empty();
					Tiles[TileIndex].RemainingOptions.Add(GoaltOption);
					Tiles[TileIndex].ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(
						Tiles[TileIndex].RemainingOptions, WFCModel);
				}
			}
		}
	}

	UE_LOG(LogWFC, Log, TEXT("Replaced room at index %d with 3x3 goalt01 tiles"), RoomIndex);
}