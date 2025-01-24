// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaveFunctionCollapseBPLibrary02.h"
#include "Engine/StaticMesh.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "Engine/StaticMeshActor.h"
#include "Components/InstancedStaticMeshComponent.h"

UWaveFunctionCollapseBPLibrary02::UWaveFunctionCollapseBPLibrary02(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(const TArray<FWaveFunctionCollapseOptionCustom>& Options, UWaveFunctionCollapseModel02* WFCModel)
{
	if (Options.IsEmpty())
	{
		UE_LOG(LogWFC, Display, TEXT("Cannot calculate shannon entropy because the options are empty."));
		return -1;
	}

	float SumWeights = 0;
	float SumWeightXLogWeight = 0;
	for (const FWaveFunctionCollapseOptionCustom& Option : Options)
	{
		if (WFCModel->Constraints.Contains(Option))
		{
			const float& Weight = WFCModel->Constraints.FindRef(Option).Weight;
			SumWeights += Weight;
			SumWeightXLogWeight += (Weight * log(Weight));
		}
	}

	if (SumWeights == 0)
	{
		UE_LOG(LogWFC, Display, TEXT("Cannot calculate shannon entropy because the sum of weights equals zero."));
		return -1;
	}

	return log(SumWeights) - (SumWeightXLogWeight / SumWeights);
}

int32 UWaveFunctionCollapseBPLibrary02::PositionAsIndex(FIntVector Position, FIntVector Resolution)
{
	return Position.X + (Position.Y * Resolution.X) + (Position.Z * Resolution.X * Resolution.Y);
}

FIntVector UWaveFunctionCollapseBPLibrary02::IndexAsPosition(int32 Index, FIntVector Resolution)
{
	FIntVector Position;
	Position.Z = floor(Index / (Resolution.X * Resolution.Y));
	Position.Y = floor((Index - (Position.Z * Resolution.X * Resolution.Y)) / Resolution.X);
	Position.X = Index - (Position.Y * Resolution.X) - (Position.Z * Resolution.X * Resolution.Y);
	return Position;
}

FWaveFunctionCollapseTileCustom UWaveFunctionCollapseBPLibrary02::BuildInitialTile(UWaveFunctionCollapseModel02* WFCModel)
{
	FWaveFunctionCollapseTileCustom InitialTile;
	for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : WFCModel->Constraints)
	{
		InitialTile.RemainingOptions.AddUnique(Constraint.Key);
	}
	InitialTile.ShannonEntropy = UWaveFunctionCollapseBPLibrary02::CalculateShannonEntropy(InitialTile.RemainingOptions, WFCModel);
	return InitialTile;
}

TMap<int32, EWaveFunctionCollapseAdjacencyCustom> UWaveFunctionCollapseBPLibrary02::GetAdjacentIndices(int32 Index, FIntVector Resolution)
{
	TMap<int32, EWaveFunctionCollapseAdjacencyCustom> AdjacentIndices;
	FIntVector Position = IndexAsPosition(Index, Resolution);
	if (Position.X + 1 < Resolution.X)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(1, 0, 0), Resolution), EWaveFunctionCollapseAdjacencyCustom::Front);
	}
	if (Position.X - 1 >= 0)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(-1, 0, 0), Resolution), EWaveFunctionCollapseAdjacencyCustom::Back);
	}
	if (Position.Y + 1 < Resolution.Y)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(0, 1, 0), Resolution), EWaveFunctionCollapseAdjacencyCustom::Right);
	}
	if (Position.Y - 1 >= 0)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(0, -1, 0), Resolution), EWaveFunctionCollapseAdjacencyCustom::Left);
	}
	if (Position.Z + 1 < Resolution.Z)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(0, 0, 1), Resolution), EWaveFunctionCollapseAdjacencyCustom::Up);
	}
	if (Position.Z - 1 >= 0)
	{
		AdjacentIndices.Add(PositionAsIndex(Position + FIntVector(0, 0, -1), Resolution), EWaveFunctionCollapseAdjacencyCustom::Down);
	}

	return AdjacentIndices;
}

TMap<FIntVector, EWaveFunctionCollapseAdjacencyCustom> UWaveFunctionCollapseBPLibrary02::GetAdjacentPositions(FIntVector Position, FIntVector Resolution)
{
	TMap<FIntVector, EWaveFunctionCollapseAdjacencyCustom> AdjacentPositions;
	if (Position.X + 1 < Resolution.X)
	{
		AdjacentPositions.Add(Position + FIntVector(1, 0, 0), EWaveFunctionCollapseAdjacencyCustom::Front);
	}
	if (Position.X - 1 >= 0)
	{
		AdjacentPositions.Add(Position + FIntVector(-1, 0, 0), EWaveFunctionCollapseAdjacencyCustom::Back);
	}
	if (Position.Y + 1 < Resolution.Y)
	{
		AdjacentPositions.Add(Position + FIntVector(0, 1, 0), EWaveFunctionCollapseAdjacencyCustom::Right);
	}
	if (Position.Y - 1 >= 0)
	{
		AdjacentPositions.Add(Position + FIntVector(0, -1, 0), EWaveFunctionCollapseAdjacencyCustom::Left);
	}
	if (Position.Z + 1 < Resolution.Z)
	{
		AdjacentPositions.Add(Position + FIntVector(0, 0, 1), EWaveFunctionCollapseAdjacencyCustom::Up);
	}
	if (Position.Z - 1 >= 0)
	{
		AdjacentPositions.Add(Position + FIntVector(0, 0, -1), EWaveFunctionCollapseAdjacencyCustom::Down);
	}

	return AdjacentPositions;
}

bool UWaveFunctionCollapseBPLibrary02::IsOptionContained(const FWaveFunctionCollapseOptionCustom& Option, const TArray<FWaveFunctionCollapseOptionCustom>& Options)
{
	for (const FWaveFunctionCollapseOptionCustom& CheckAgainst : Options)
	{
		if (Option == CheckAgainst)
		{
			return true;
		}
	}
	return false;
}

EWaveFunctionCollapseAdjacencyCustom UWaveFunctionCollapseBPLibrary02::GetOppositeAdjacency(EWaveFunctionCollapseAdjacencyCustom Adjacency)
{
	switch (Adjacency)
	{
	case EWaveFunctionCollapseAdjacencyCustom::Front:
		return EWaveFunctionCollapseAdjacencyCustom::Back;
	case EWaveFunctionCollapseAdjacencyCustom::Back:
		return EWaveFunctionCollapseAdjacencyCustom::Front;
	case EWaveFunctionCollapseAdjacencyCustom::Right:
		return EWaveFunctionCollapseAdjacencyCustom::Left;
	case EWaveFunctionCollapseAdjacencyCustom::Left:
		return EWaveFunctionCollapseAdjacencyCustom::Right;
	case EWaveFunctionCollapseAdjacencyCustom::Up:
		return EWaveFunctionCollapseAdjacencyCustom::Down;
	case EWaveFunctionCollapseAdjacencyCustom::Down:
		return EWaveFunctionCollapseAdjacencyCustom::Up;
	default:
		return Adjacency;
	}
}

EWaveFunctionCollapseAdjacencyCustom UWaveFunctionCollapseBPLibrary02::GetNextZAxisClockwiseAdjacency(EWaveFunctionCollapseAdjacencyCustom Adjacency)
{
	switch (Adjacency)
	{
	case EWaveFunctionCollapseAdjacencyCustom::Front:
		return EWaveFunctionCollapseAdjacencyCustom::Right;
	case EWaveFunctionCollapseAdjacencyCustom::Right:
		return EWaveFunctionCollapseAdjacencyCustom::Back;
	case EWaveFunctionCollapseAdjacencyCustom::Back:
		return EWaveFunctionCollapseAdjacencyCustom::Left;
	case EWaveFunctionCollapseAdjacencyCustom::Left:
		return EWaveFunctionCollapseAdjacencyCustom::Front;
	case EWaveFunctionCollapseAdjacencyCustom::Up:
		return EWaveFunctionCollapseAdjacencyCustom::Up;
	case EWaveFunctionCollapseAdjacencyCustom::Down:
		return EWaveFunctionCollapseAdjacencyCustom::Down;
	default:
		return Adjacency;
	}
}

void UWaveFunctionCollapseBPLibrary02::AddToAdjacencyToOptionsMap(UPARAM(ref) FWaveFunctionCollapseAdjacencyToOptionsMapCustom& AdjacencyToOptionsMap, EWaveFunctionCollapseAdjacencyCustom OptionAdjacency, FWaveFunctionCollapseOptionCustom OptionObject)
{
	if (FWaveFunctionCollapseOptionsCustom* Options = AdjacencyToOptionsMap.AdjacencyToOptionsMap.Find(OptionAdjacency))
	{
		// If the Options array does not contain the Option, create it
		if (!Options->Options.Contains(OptionObject))
		{
			Options->Options.Add(OptionObject);
		}
	}
	else // Create the Option and add it to the AdjacencyToOptionsMap
	{
		FWaveFunctionCollapseOptionsCustom NewOptions;
		NewOptions.Options.Add(OptionObject);
		AdjacencyToOptionsMap.AdjacencyToOptionsMap.Add(OptionAdjacency, NewOptions);
	}
}

bool UWaveFunctionCollapseBPLibrary02::IsSoftObjPathEqual(const FSoftObjectPath& SoftObjectPathA, const FSoftObjectPath& SoftObjectPathB)
{
	return SoftObjectPathA == SoftObjectPathB;
}

FRotator UWaveFunctionCollapseBPLibrary02::SanitizeRotator(FRotator Rotator)
{
	FRotator OutputRotator;
	OutputRotator = Rotator;

	// Round to orthogonal values
	OutputRotator.Roll = FMath::RoundHalfToEven(OutputRotator.Roll / 90) * 90;
	OutputRotator.Pitch = FMath::RoundHalfToEven(OutputRotator.Pitch / 90) * 90;
	OutputRotator.Yaw = FMath::RoundHalfToEven(OutputRotator.Yaw / 90) * 90;
	OutputRotator.Normalize();

	// Convert from Rotator to Quaternion to Matrix and back To Rotator to ensure single representation of rotation value
	FTransform MyTransform;
	MyTransform.SetRotation(FQuat(OutputRotator));
	FMatrix MyMatrix = MyTransform.ToMatrixNoScale();
	OutputRotator = MyMatrix.Rotator();

	// Ensure that -180 values are adjusted to 180
	OutputRotator.Normalize();
	OutputRotator.Roll = FMath::RoundHalfToEven(OutputRotator.Roll);
	if (OutputRotator.Roll == -180)
	{
		OutputRotator.Roll = 180;
	}
	OutputRotator.Pitch = FMath::RoundHalfToEven(OutputRotator.Pitch);
	if (OutputRotator.Pitch == -180)
	{
		OutputRotator.Pitch = 180;
	}
	OutputRotator.Yaw = FMath::RoundHalfToEven(OutputRotator.Yaw);
	if (OutputRotator.Yaw == -180)
	{
		OutputRotator.Yaw = 180;
	}

	return OutputRotator;
}

void UWaveFunctionCollapseBPLibrary02::DeriveModelFromActors(UPARAM(ref) const TArray<AActor*>& InputActors,
	UWaveFunctionCollapseModel02* WFCModel,
	float TileSize,
	bool bIsBorderEmptyOption,
	bool bIsMinZFloorOption,
	bool bUseUniformWeightDistribution,
	bool bAutoDeriveZAxisRotationConstraints,
	const TArray<FSoftObjectPath>& SpawnExclusionAssets,
	const TArray<FSoftObjectPath>& IgnoreRotationAssets)
{
	// Check if there are valid actors
	if (InputActors.IsEmpty())
	{
		return;
	}

	// Check if Model is valid
	if (!WFCModel)
	{
		return;
	}

	// Initialize Variables
	TMap<FIntVector, FWaveFunctionCollapseOptionCustom> PositionToOptionWithBorderMap;
	FIntVector ResolutionWithBorder;

	// Set TileSize
	WFCModel->Modify();
	WFCModel->TileSize = TileSize;

	// Create all Empty Option adjacency constraints
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Front, FWaveFunctionCollapseOptionCustom::EmptyOption);
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Back, FWaveFunctionCollapseOptionCustom::EmptyOption);
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Left, FWaveFunctionCollapseOptionCustom::EmptyOption);
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Right, FWaveFunctionCollapseOptionCustom::EmptyOption);
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Up, FWaveFunctionCollapseOptionCustom::EmptyOption);
	WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, EWaveFunctionCollapseAdjacencyCustom::Down, FWaveFunctionCollapseOptionCustom::EmptyOption);

	// Derive Grid from Actors
	FIntVector MinPosition = FIntVector::ZeroValue;
	FIntVector MaxPosition = FIntVector::ZeroValue;
	FVector TmpOrigin = InputActors[0]->GetActorLocation(); // Set TmpOrigin based on first actor in the array
	TMap<FIntVector, FWaveFunctionCollapseOptionCustom> TmpPositionToOptionMap;
	for (int32 InputActorIndex = 0; InputActorIndex < InputActors.Num(); InputActorIndex++)
	{
		AStaticMeshActor* InputStaticMeshActor = Cast<AStaticMeshActor>(InputActors[InputActorIndex]);
		if (InputStaticMeshActor)
		{
			UStaticMesh* InputStaticMesh = InputStaticMeshActor->GetStaticMeshComponent()->GetStaticMesh();
			if (!InputStaticMesh)
			{
				UE_LOG(LogWFC, Display, TEXT("%s contains invalid StaticMesh, skipping."), *InputStaticMeshActor->GetFName().ToString());
				continue;
			}

			// Find CurrentGridPosition
			FVector CurrentGridPositionFloat = (InputStaticMeshActor->GetActorLocation() - TmpOrigin) / TileSize;
			FIntVector CurrentGridPosition = FIntVector(FMath::RoundHalfFromZero(CurrentGridPositionFloat.X), FMath::RoundHalfFromZero(CurrentGridPositionFloat.Y), FMath::RoundHalfFromZero(CurrentGridPositionFloat.Z));
			if (TmpPositionToOptionMap.Contains(CurrentGridPosition))
			{
				UE_LOG(LogWFC, Display, TEXT("%s is in an overlapping position, skipping."), *InputStaticMeshActor->GetFName().ToString());
				continue;
			}

			// Find min/max positions
			MinPosition.X = FMath::Min(MinPosition.X, CurrentGridPosition.X);
			MinPosition.Y = FMath::Min(MinPosition.Y, CurrentGridPosition.Y);
			MinPosition.Z = FMath::Min(MinPosition.Z, CurrentGridPosition.Z);
			MaxPosition.X = FMath::Max(MaxPosition.X, CurrentGridPosition.X);
			MaxPosition.Y = FMath::Max(MaxPosition.Y, CurrentGridPosition.Y);
			MaxPosition.Z = FMath::Max(MaxPosition.Z, CurrentGridPosition.Z);

			// Add to Temporary Position to Option Map
			FRotator CurrentRotator = FRotator::ZeroRotator;
			if (!IgnoreRotationAssets.Contains(FSoftObjectPath(InputStaticMesh->GetPathName())))
			{
				CurrentRotator = SanitizeRotator(InputStaticMeshActor->GetActorRotation());
			}
			FWaveFunctionCollapseOptionCustom CurrentOption(InputStaticMesh->GetPathName(), CurrentRotator, InputStaticMeshActor->GetActorScale3D());
			TmpPositionToOptionMap.Add(CurrentGridPosition, CurrentOption);
		}
	}

	// Re-order grid from 0,0,0 and add border
	for (TPair<FIntVector, FWaveFunctionCollapseOptionCustom>& TmpPositionToOption : TmpPositionToOptionMap)
	{
		PositionToOptionWithBorderMap.Add(TmpPositionToOption.Key - MinPosition + FIntVector(1), TmpPositionToOption.Value);
	}
	TmpPositionToOptionMap.Empty();

	// Set Resolution with Border
	ResolutionWithBorder = MaxPosition - MinPosition + FIntVector(3);

	// Derive Constraints
	for (TPair<FIntVector, FWaveFunctionCollapseOptionCustom>& PositionToOptionWithBorder : PositionToOptionWithBorderMap)
	{
		TMap<FIntVector, EWaveFunctionCollapseAdjacencyCustom> PositionToAdjacenciesMap = GetAdjacentPositions(PositionToOptionWithBorder.Key, ResolutionWithBorder);
		for (TPair<FIntVector, EWaveFunctionCollapseAdjacencyCustom>& PositionToAdjacencies : PositionToAdjacenciesMap)
		{
			FIntVector PositionToCheck = PositionToAdjacencies.Key;

			// if Adjacent Option exists, add constraint to model
			if (PositionToOptionWithBorderMap.Contains(PositionToCheck))
			{
				WFCModel->AddConstraint(PositionToOptionWithBorder.Value, PositionToAdjacencies.Value, PositionToOptionWithBorderMap.FindRef(PositionToCheck));
			}
			// if MinZ and bIsMinZFloorOption, add Border Option constraint
			else if (bIsMinZFloorOption && PositionToCheck.Z == 0)
			{
				WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::BorderOption, GetOppositeAdjacency(PositionToAdjacencies.Value), PositionToOptionWithBorder.Value);
			}
			// if ExteriorBorder and bIsBorderEmptyOption, add Empty Option and Inverse constraints
			else if (PositionToCheck.X == 0
				|| PositionToCheck.Y == 0
				|| PositionToCheck.Z == 0
				|| PositionToCheck.X == ResolutionWithBorder.X - 1
				|| PositionToCheck.Y == ResolutionWithBorder.Y - 1
				|| PositionToCheck.Z == ResolutionWithBorder.Z - 1)
			{
				if (bIsBorderEmptyOption)
				{
					WFCModel->AddConstraint(PositionToOptionWithBorder.Value, PositionToAdjacencies.Value, FWaveFunctionCollapseOptionCustom::EmptyOption);
					WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, GetOppositeAdjacency(PositionToAdjacencies.Value), PositionToOptionWithBorder.Value);
				}
			}
			// otherwise it is an empty space, add Empty Option and Inverse constraints
			else
			{
				WFCModel->AddConstraint(PositionToOptionWithBorder.Value, PositionToAdjacencies.Value, FWaveFunctionCollapseOptionCustom::EmptyOption);
				WFCModel->AddConstraint(FWaveFunctionCollapseOptionCustom::EmptyOption, GetOppositeAdjacency(PositionToAdjacencies.Value), PositionToOptionWithBorder.Value);
			}
		}
	}
	PositionToOptionWithBorderMap.Empty();

	// Auto Derive ZAxis Rotation Constraints
	if (bAutoDeriveZAxisRotationConstraints)
	{
		TArray<FWaveFunctionCollapseOptionCustom> NewKeyOptions;
		TArray<EWaveFunctionCollapseAdjacencyCustom> NewAdjacencies;
		TArray<FWaveFunctionCollapseOptionCustom> NewAdjacentOptions;

		for (const TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : WFCModel->Constraints)
		{
			for (const TPair<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom>& AdjacencyToOptionsPair : Constraint.Value.AdjacencyToOptionsMap)
			{
				for (const FWaveFunctionCollapseOptionCustom& AdjacentOption : AdjacencyToOptionsPair.Value.Options)
				{
					FWaveFunctionCollapseOptionCustom NewKeyOption = Constraint.Key;
					FWaveFunctionCollapseOptionCustom NewAdjacentOption = AdjacentOption;
					EWaveFunctionCollapseAdjacencyCustom NewAdjacency = AdjacencyToOptionsPair.Key;

					int32 KeyOptionRotationMultiplier = 0;
					if (!(IgnoreRotationAssets.Contains(NewKeyOption.BaseObject)
						|| NewKeyOption == FWaveFunctionCollapseOptionCustom::EmptyOption
						|| NewKeyOption == FWaveFunctionCollapseOptionCustom::BorderOption
						|| NewKeyOption == FWaveFunctionCollapseOptionCustom::VoidOption))
					{
						KeyOptionRotationMultiplier = 1;
					}

					int32 AdjacentOptionRotationMultiplier = 0;
					if (!(IgnoreRotationAssets.Contains(NewAdjacentOption.BaseObject)
						|| NewAdjacentOption == FWaveFunctionCollapseOptionCustom::EmptyOption
						|| NewAdjacentOption == FWaveFunctionCollapseOptionCustom::BorderOption
						|| NewAdjacentOption == FWaveFunctionCollapseOptionCustom::VoidOption))
					{
						AdjacentOptionRotationMultiplier = 1;
					}

					for (int32 RotationIncrement = 1; RotationIncrement <= 3; RotationIncrement++)
					{
						NewKeyOption.BaseRotator.Yaw += (90.0f * KeyOptionRotationMultiplier);
						NewKeyOption.BaseRotator = SanitizeRotator(NewKeyOption.BaseRotator);
						NewAdjacentOption.BaseRotator.Yaw += (90.0f * AdjacentOptionRotationMultiplier);
						NewAdjacentOption.BaseRotator = SanitizeRotator(NewAdjacentOption.BaseRotator);
						NewAdjacency = GetNextZAxisClockwiseAdjacency(NewAdjacency);

						// Store new constraints
						NewKeyOptions.Add(NewKeyOption);
						NewAdjacencies.Add(NewAdjacency);
						NewAdjacentOptions.Add(NewAdjacentOption);
					}
				}
			}
		}

		// Add stored new constraints to the model
		for (int32 Index = 0; Index < NewKeyOptions.Num(); Index++)
		{
			WFCModel->AddConstraint(NewKeyOptions[Index], NewAdjacencies[Index], NewAdjacentOptions[Index]);
		}
	}

	// If Empty->OptionA constraint and Empty<-OptionB exist, store OptionA->OptionB constraint
	TArray<FWaveFunctionCollapseOptionCustom> EmptyVoidAdjacentKeyOptions;
	TArray<EWaveFunctionCollapseAdjacencyCustom> EmptyVoidAdjacentAdjacencies;
	TArray<FWaveFunctionCollapseOptionCustom> EmptyVoidAdjacentAdjacentOptions;
	const FWaveFunctionCollapseAdjacencyToOptionsMapCustom& EmptyOptionConstraints = WFCModel->Constraints.FindRef(FWaveFunctionCollapseOptionCustom::EmptyOption);
	for (const TPair<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom>& AdjacencyToOptionsPair : EmptyOptionConstraints.AdjacencyToOptionsMap)
	{
		for (const FWaveFunctionCollapseOptionCustom& EmptyAdjacentOption : AdjacencyToOptionsPair.Value.Options)
		{
			if (EmptyAdjacentOption == FWaveFunctionCollapseOptionCustom::EmptyOption)
			{
				continue;
			}

			if (const FWaveFunctionCollapseOptionsCustom* OppositeEmptyAdjacenctOptions = EmptyOptionConstraints.AdjacencyToOptionsMap.Find(GetOppositeAdjacency(AdjacencyToOptionsPair.Key)))
			{
				for (const FWaveFunctionCollapseOptionCustom& OppositeEmptyAdjacentOption : OppositeEmptyAdjacenctOptions->Options)
				{
					if (OppositeEmptyAdjacentOption == FWaveFunctionCollapseOptionCustom::EmptyOption)
					{
						continue;
					}

					EmptyVoidAdjacentKeyOptions.Add(EmptyAdjacentOption);
					EmptyVoidAdjacentAdjacencies.Add(GetOppositeAdjacency(AdjacencyToOptionsPair.Key));
					EmptyVoidAdjacentAdjacentOptions.Add(OppositeEmptyAdjacentOption);
				}
			}
		}
	}

	// If Void->OptionA constraint and Void<-OptionB exist, store OptionA->OptionB constraint
	const FWaveFunctionCollapseAdjacencyToOptionsMapCustom& VoidOptionConstraints = WFCModel->Constraints.FindRef(FWaveFunctionCollapseOptionCustom::VoidOption);
	for (const TPair<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom>& AdjacencyToOptionsPair : VoidOptionConstraints.AdjacencyToOptionsMap)
	{
		for (const FWaveFunctionCollapseOptionCustom& VoidAdjacentOption : AdjacencyToOptionsPair.Value.Options)
		{
			if (VoidAdjacentOption == FWaveFunctionCollapseOptionCustom::VoidOption)
			{
				continue;
			}

			if (const FWaveFunctionCollapseOptionsCustom* OppositeVoidAdjacenctOptions = VoidOptionConstraints.AdjacencyToOptionsMap.Find(GetOppositeAdjacency(AdjacencyToOptionsPair.Key)))
			{
				for (const FWaveFunctionCollapseOptionCustom& OppositeVoidAdjacentOption : OppositeVoidAdjacenctOptions->Options)
				{
					if (VoidAdjacentOption == FWaveFunctionCollapseOptionCustom::VoidOption)
					{
						continue;
					}

					EmptyVoidAdjacentKeyOptions.Add(VoidAdjacentOption);
					EmptyVoidAdjacentAdjacencies.Add(GetOppositeAdjacency(AdjacencyToOptionsPair.Key));
					EmptyVoidAdjacentAdjacentOptions.Add(OppositeVoidAdjacentOption);
				}
			}
		}
	}

	// Add stored new constraints to the model
	for (int32 Index = 0; Index < EmptyVoidAdjacentKeyOptions.Num(); Index++)
	{
		WFCModel->AddConstraint(EmptyVoidAdjacentKeyOptions[Index], EmptyVoidAdjacentAdjacencies[Index], EmptyVoidAdjacentAdjacentOptions[Index]);
	}

	// Set Contributions
	if (bUseUniformWeightDistribution)
	{
		WFCModel->SetAllContributions(1);
	}
	WFCModel->SetOptionContribution(FWaveFunctionCollapseOptionCustom::BorderOption, 0);
	WFCModel->SetWeightsFromContributions();

	// Append to SpawnExlusion
	for (FSoftObjectPath SpawnExclusionAsset : SpawnExclusionAssets)
	{
		if (SpawnExclusionAsset.IsValid())
		{
			WFCModel->SpawnExclusion.AddUnique(SpawnExclusionAsset);
		}
	}
}

bool UWaveFunctionCollapseBPLibrary02::GetPositionToOptionMapFromActor(AActor* Actor, float TileSize, UPARAM(ref) TMap<FIntVector, FWaveFunctionCollapseOptionCustom>& PositionToOptionMap)
{
	// Check if Model is valid
	if (!Actor)
	{
		UE_LOG(LogWFC, Display, TEXT("GetPositionToOptionMapFromActor called with a null Actor"));
		return false;
	}

	// Derive Grid from Actors
	FIntVector MinPosition = FIntVector::ZeroValue;
	FIntVector MaxPosition = FIntVector::ZeroValue;
	TMap<FIntVector, FWaveFunctionCollapseOptionCustom> TmpPositionToOptionMap;

	// Gather ISMComponents
	TInlineComponentArray<UInstancedStaticMeshComponent*> ISMComponents;
	Actor->GetComponents(ISMComponents);
	for (int32 ISMComponentIndex = 0; ISMComponentIndex < ISMComponents.Num(); ISMComponentIndex++)
	{
		UInstancedStaticMeshComponent* ISMComponent = ISMComponents[ISMComponentIndex];
		UStaticMesh* InputStaticMesh = ISMComponent->GetStaticMesh();
		if (!InputStaticMesh)
		{
			UE_LOG(LogWFC, Display, TEXT("%s.%s contains invalid StaticMesh, skipping."), *Actor->GetFName().ToString(), *ISMComponent->GetFName().ToString());
			continue;
		}
		else
		{
			for (int32 InstanceIndex = 0; InstanceIndex < ISMComponent->GetInstanceCount(); InstanceIndex++)
			{
				FTransform InstanceTransform;
				ISMComponent->GetInstanceTransform(InstanceIndex, InstanceTransform);

				// Find CurrentGridPosition
				FVector CurrentGridPositionFloat = InstanceTransform.GetLocation() / TileSize;
				FIntVector CurrentGridPosition = FIntVector(FMath::RoundHalfFromZero(CurrentGridPositionFloat.X), FMath::RoundHalfFromZero(CurrentGridPositionFloat.Y), FMath::RoundHalfFromZero(CurrentGridPositionFloat.Z));
				if (TmpPositionToOptionMap.Contains(CurrentGridPosition))
				{
					UE_LOG(LogWFC, Display, TEXT("%s.%s Index %d is in an overlapping position, skipping."), *Actor->GetFName().ToString(), *ISMComponent->GetFName().ToString(), InstanceIndex);
					continue;
				}

				// Find min/max positions
				MinPosition.X = FMath::Min(MinPosition.X, CurrentGridPosition.X);
				MinPosition.Y = FMath::Min(MinPosition.Y, CurrentGridPosition.Y);
				MinPosition.Z = FMath::Min(MinPosition.Z, CurrentGridPosition.Z);
				MaxPosition.X = FMath::Max(MaxPosition.X, CurrentGridPosition.X);
				MaxPosition.Y = FMath::Max(MaxPosition.Y, CurrentGridPosition.Y);
				MaxPosition.Z = FMath::Max(MaxPosition.Z, CurrentGridPosition.Z);

				// Add to Temporary Position to Option Map
				FWaveFunctionCollapseOptionCustom CurrentOption(InputStaticMesh->GetPathName(), InstanceTransform.Rotator(), InstanceTransform.GetScale3D());
				TmpPositionToOptionMap.Add(CurrentGridPosition, CurrentOption);
			}
		}
	}

	// Re-order grid 
	for (TPair<FIntVector, FWaveFunctionCollapseOptionCustom>& TmpPositionToOption : TmpPositionToOptionMap)
	{
		PositionToOptionMap.Add(TmpPositionToOption.Key - MinPosition, TmpPositionToOption.Value);
	}

	return !PositionToOptionMap.IsEmpty();
}

FWaveFunctionCollapseOptionCustom UWaveFunctionCollapseBPLibrary02::MakeEmptyOption()
{
	return FWaveFunctionCollapseOptionCustom::EmptyOption;
}

FWaveFunctionCollapseOptionCustom UWaveFunctionCollapseBPLibrary02::MakeBorderOption()
{
	return FWaveFunctionCollapseOptionCustom::BorderOption;
}

FWaveFunctionCollapseOptionCustom UWaveFunctionCollapseBPLibrary02::MakeVoidOption()
{
	return FWaveFunctionCollapseOptionCustom::VoidOption;
}
