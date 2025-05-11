// Copyright Epic Games, Inc. All Rights Reserved.

#include "WaveFunctionCollapseModel02.h"
#include "Engine/StaticMesh.h"

const FWaveFunctionCollapseOptionCustom FWaveFunctionCollapseOptionCustom::EmptyOption(FString(TEXT("/WaveFunctionCollapse/Core/SpecialOptions/Option_Empty.Option_Empty")));
const FWaveFunctionCollapseOptionCustom FWaveFunctionCollapseOptionCustom::BorderOption(FString(TEXT("/WaveFunctionCollapse/Core/SpecialOptions/Option_Border.Option_Border")));
const FWaveFunctionCollapseOptionCustom FWaveFunctionCollapseOptionCustom::VoidOption(FString(TEXT("/WaveFunctionCollapse/Core/SpecialOptions/Option_Void.Option_Void")));

UWaveFunctionCollapseModel02::UWaveFunctionCollapseModel02()
{
	// �� �ʱ�ȭ
	InitializeModel();
}

void UWaveFunctionCollapseModel02::AddConstraint(const FWaveFunctionCollapseOptionCustom& KeyOption, EWaveFunctionCollapseAdjacencyCustom Adjacency, const FWaveFunctionCollapseOptionCustom& AdjacentOption)
{
	Modify(true);
	// If the KeyOption key exists in Constraints
	if (FWaveFunctionCollapseAdjacencyToOptionsMapCustom* AdjacencyToOptionsMap = Constraints.Find(KeyOption))
	{
		// If the Adjacency key exists in the AdjacencyToOptionsMap
		if (FWaveFunctionCollapseOptionsCustom* Options = AdjacencyToOptionsMap->AdjacencyToOptionsMap.Find(Adjacency))
		{
			// If the Options array does not contain the Option, create it
			if (!Options->Options.Contains(AdjacentOption))
			{
				Options->Options.Add(AdjacentOption);
				AdjacencyToOptionsMap->Contribution += 1;
			}
		}
		else // Create the Option and add it to the AdjacencyToOptionsMap
		{
			FWaveFunctionCollapseOptionsCustom NewOptions;
			NewOptions.Options.Add(AdjacentOption);
			AdjacencyToOptionsMap->AdjacencyToOptionsMap.Add(Adjacency, NewOptions);
			AdjacencyToOptionsMap->Contribution += 1;
		}
	}
	else // Create the Option, add it to the AdjacencyToOptionsMap, and add it the Constraints map
	{
		FWaveFunctionCollapseOptionsCustom NewOptions;
		NewOptions.Options.Add(AdjacentOption);
		FWaveFunctionCollapseAdjacencyToOptionsMapCustom NewAdjacencyToOptionsMap;
		NewAdjacencyToOptionsMap.AdjacencyToOptionsMap.Add(Adjacency, NewOptions);
		Constraints.Add(KeyOption, NewAdjacencyToOptionsMap);
	}

}

FWaveFunctionCollapseOptionsCustom UWaveFunctionCollapseModel02::GetOptions(const FWaveFunctionCollapseOptionCustom& KeyOption, EWaveFunctionCollapseAdjacencyCustom Adjacency) const
{
	// If KeyOption key exists
	if (const FWaveFunctionCollapseAdjacencyToOptionsMapCustom* AdjacencyToOptionsMap = Constraints.Find(KeyOption))
	{
		// If Adjacency key exists, return FoundOptions
		if (const FWaveFunctionCollapseOptionsCustom* FoundOptions = AdjacencyToOptionsMap->AdjacencyToOptionsMap.Find(Adjacency))
		{
			return *FoundOptions;
		}
	}
	// If nothing is found above, return empty
	return FWaveFunctionCollapseOptionsCustom();
}

void UWaveFunctionCollapseModel02::SetWeightsFromContributions()
{
	if (Constraints.IsEmpty())
	{
		return;
	}

	Modify(true);
	int32 SumOfContributions = 0;
	for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
	{
		SumOfContributions += Constraint.Value.Contribution;
	}

	for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
	{
		Constraint.Value.Weight = float(Constraint.Value.Contribution) / SumOfContributions;
	}
}

void UWaveFunctionCollapseModel02::SetAllWeights(float Weight)
{
	if (Constraints.IsEmpty())
	{
		return;
	}

	Modify(true);
	for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
	{
		Constraint.Value.Weight = Weight;
	}
}

void UWaveFunctionCollapseModel02::SetAllContributions(int32 Contribution)
{
	if (Constraints.IsEmpty())
	{
		return;
	}

	Modify(true);
	for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
	{
		Constraint.Value.Contribution = Contribution;
	}
}

void UWaveFunctionCollapseModel02::SetOptionContribution(const FWaveFunctionCollapseOptionCustom& Option, int32 Contribution)
{
	if (Constraints.Contains(Option))
	{
		Modify(true);
		FWaveFunctionCollapseAdjacencyToOptionsMapCustom& AdjacencyToOptionsMap = *Constraints.Find(Option);
		AdjacencyToOptionsMap.Contribution = Contribution;
	}
}

float UWaveFunctionCollapseModel02::GetOptionWeight(const FWaveFunctionCollapseOptionCustom& Option) const
{
	if (const FWaveFunctionCollapseAdjacencyToOptionsMapCustom* Constraint = Constraints.Find(Option))
	{
		return Constraint->Weight;
	}
	else
	{
		return 0;
	}
}

int32 UWaveFunctionCollapseModel02::GetOptionContribution(const FWaveFunctionCollapseOptionCustom& Option) const
{
	if (const FWaveFunctionCollapseAdjacencyToOptionsMapCustom* Constraint = Constraints.Find(Option))
	{
		return Constraint->Contribution;
	}
	else
	{
		return 0;
	}
}

int32 UWaveFunctionCollapseModel02::GetConstraintCount() const
{
	int32 ConstraintCount = 0;
	for (const TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
	{
		for (const TPair<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom>& AdjacencyToOptions : Constraint.Value.AdjacencyToOptionsMap)
		{
			ConstraintCount += AdjacencyToOptions.Value.Options.Num();
		}
	}
	return ConstraintCount;
}

void UWaveFunctionCollapseModel02::SwapMeshes(TMap<UStaticMesh*, UStaticMesh*> SourceToTargetMeshMap)
{
	Modify(true);
	for (TPair<UStaticMesh*, UStaticMesh*>& SourceToTargetMesh : SourceToTargetMeshMap)
	{
		UStaticMesh* SourceMesh = SourceToTargetMesh.Key;
		UStaticMesh* TargetMesh = SourceToTargetMesh.Value;
		for (TPair<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom>& Constraint : Constraints)
		{
			if (Constraint.Key.BaseObject == SourceMesh)
			{
				Constraint.Key.BaseObject = TargetMesh;
			}

			for (TPair<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom>& AdjacencyToOptions : Constraint.Value.AdjacencyToOptionsMap)
			{
				for (FWaveFunctionCollapseOptionCustom& Option : AdjacencyToOptions.Value.Options)
				{
					if (Option.BaseObject == SourceMesh)
					{
						Option.BaseObject = TargetMesh;
					}
				}
			}
		}
	}
}

void UWaveFunctionCollapseModel02::InitializeModel()
{
	// �� Ÿ�� ����
	FWaveFunctionCollapseOptionCustom RoomTile(TEXT("/Game/BP")); // �� Ÿ�� �޽� ���
	RoomTile.bIsRoomTile = true; // �� Ÿ�� �÷��� ����

	// �Ϲ� Ÿ�� ����
	FWaveFunctionCollapseOptionCustom OtherTile(TEXT("/Game/BP"));

	// �� Ÿ�ϰ� �Ϲ� Ÿ�� �� ���� ����
	AddConstraint(RoomTile, EWaveFunctionCollapseAdjacencyCustom::Front, OtherTile);
	AddConstraint(OtherTile, EWaveFunctionCollapseAdjacencyCustom::Back, RoomTile);
}

