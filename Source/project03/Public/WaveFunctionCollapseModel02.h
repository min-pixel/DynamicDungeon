#pragma once// Copyright Epic Games, Inc. All Rights Reserved.



#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "WaveFunctionCollapseModel02.generated.h"

UENUM(BlueprintType)
enum class EWaveFunctionCollapseAdjacencyCustom : uint8
{
	Front	UMETA(DisplayName = "X+ Front"),
	Back	UMETA(DisplayName = "X- Back"),
	Right	UMETA(DisplayName = "Y+ Right"),
	Left	UMETA(DisplayName = "Y- Left"),
	Up		UMETA(DisplayName = "Z+ Up"),
	Down	UMETA(DisplayName = "Z- Down")
};


/**
* Base Option Struct which holds an object, its orientation and scale
*/
USTRUCT(BlueprintType)
struct PROJECT03_API FWaveFunctionCollapseOptionCustom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse", meta = (AllowedClasses = "StaticMesh, Blueprint"))
	FSoftObjectPath BaseObject;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	FRotator BaseRotator = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	FVector BaseScale3D = FVector::OneVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	bool bIsRoomTile = false; // 방 타일 여부

	FWaveFunctionCollapseOptionCustom() = default;

	FWaveFunctionCollapseOptionCustom(FString Object, FRotator Rotator, FVector Scale3d)
	{
		BaseObject = FSoftObjectPath(Object);
		BaseRotator = Rotator;
		BaseScale3D = Scale3d;
	}

	FWaveFunctionCollapseOptionCustom(FString Object)
	{
		BaseObject = FSoftObjectPath(Object);
	}

	static const FWaveFunctionCollapseOptionCustom EmptyOption;
	static const FWaveFunctionCollapseOptionCustom BorderOption;
	static const FWaveFunctionCollapseOptionCustom VoidOption;

	friend uint32 GetTypeHash(FWaveFunctionCollapseOptionCustom Output)
	{
		uint32 OutputHash;
		OutputHash = HashCombine(GetTypeHash(Output.BaseRotator.Vector()), GetTypeHash(Output.BaseScale3D));
		OutputHash = HashCombine(OutputHash, GetTypeHash(Output.BaseObject));
		return OutputHash;
	}

	bool operator==(const FWaveFunctionCollapseOptionCustom& Rhs) const
	{
		return BaseObject == Rhs.BaseObject && BaseRotator.Equals(Rhs.BaseRotator) && BaseScale3D.Equals(Rhs.BaseScale3D);
	}

	bool operator!=(const FWaveFunctionCollapseOptionCustom& Rhs) const
	{
		return BaseObject != Rhs.BaseObject || !BaseRotator.Equals(Rhs.BaseRotator) || !BaseScale3D.Equals(Rhs.BaseScale3D);
	}

	bool IsBaseObject(FString ObjectPath)
	{
		return (BaseObject == FSoftObjectPath(ObjectPath));
	}
};

/**
* Container struct for array of Options
*/
USTRUCT(BlueprintType)
struct PROJECT03_API FWaveFunctionCollapseOptionsCustom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	TArray<FWaveFunctionCollapseOptionCustom> Options;
};

/**
* Container struct for AdjacencyToOptionsMap
* Stores the weight and contribution of an option
*/
USTRUCT(BlueprintType)
struct PROJECT03_API FWaveFunctionCollapseAdjacencyToOptionsMapCustom
{
	GENERATED_BODY()

	/**
	* The amount of times an option is present when deriving a model.
	* This value is used to calculate its weight.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse", meta = (ClampMin = "0"))
	int32 Contribution = 1;

	/**
	* The weight of an option calculated by dividing this Contribution by the sum of all contributions of all options.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse", meta = (ClampMin = "0", ClampMax = "1"))
	float Weight;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	TMap<EWaveFunctionCollapseAdjacencyCustom, FWaveFunctionCollapseOptionsCustom> AdjacencyToOptionsMap;
};

/**
* A Model of WFC constraints.
* This data asset should contain all necessary data to allow for a WFC solve of an arbitrary grid size.
*/
UCLASS(Blueprintable, BlueprintType)
class PROJECT03_API UWaveFunctionCollapseModel02 : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	* Grid Tile Size in cm^3
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	float TileSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	TMap<FWaveFunctionCollapseOptionCustom, FWaveFunctionCollapseAdjacencyToOptionsMapCustom> Constraints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse", meta = (AllowedClasses = "StaticMesh, Blueprint"))
	TArray<FSoftObjectPath> SpawnExclusion;

	UWaveFunctionCollapseModel02();
	// 모델 초기화를 위한 함수
	void InitializeModel();
	/**
	* Create a constraint
	* @param KeyOption Key option
	* @param Adjacency Adjacency from KeyOption to AdjacentOption
	* @param AdjacentOption Adjacent option
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void AddConstraint(const FWaveFunctionCollapseOptionCustom& KeyOption, EWaveFunctionCollapseAdjacencyCustom Adjacency, const FWaveFunctionCollapseOptionCustom& AdjacentOption);

	/**
	* Get all options for a given key option in a given adjacency
	* @param KeyOption Key option
	* @param Adjacency Adjacency from KeyOption to AdjacentOption
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	FWaveFunctionCollapseOptionsCustom GetOptions(const FWaveFunctionCollapseOptionCustom& KeyOption, EWaveFunctionCollapseAdjacencyCustom Adjacency) const;

	/**
	* Set the weights of key objects based on their contribution values
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void SetWeightsFromContributions();

	/**
	* Set the weights of key objects to a given value
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void SetAllWeights(float Weight);

	/**
	* Set the contribution values of key objects to a given value
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void SetAllContributions(int32 Contribution);

	/**
	* Set the contribution value of a key object to a given value
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void SetOptionContribution(const FWaveFunctionCollapseOptionCustom& Option, int32 Contribution);

	/**
	* Get the weight value of an option
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	float GetOptionWeight(const FWaveFunctionCollapseOptionCustom& Option) const;

	/**
	* Get the contribution value of an option
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	int32 GetOptionContribution(const FWaveFunctionCollapseOptionCustom& Option) const;

	/**
	* Get the total count of constraints in this model
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	int32 GetConstraintCount() const;

	/**
	* Swap meshes in the model with other meshes based on a map.
	* This is useful when working with template meshes that need to be swapped.
	*/
	UFUNCTION(BlueprintCallable, Category = "WaveFunctionCollapse")
	void SwapMeshes(TMap<UStaticMesh*, UStaticMesh*> SourceToTargetMeshMap);
};

/**
* Base Tile Struct which holds an array of remaining Options and its Shannon Entropy value
*/
USTRUCT(BlueprintType)
struct PROJECT03_API FWaveFunctionCollapseTileCustom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	float ShannonEntropy;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	TArray<FWaveFunctionCollapseOptionCustom> RemainingOptions;

	FWaveFunctionCollapseTileCustom() = default;

	FWaveFunctionCollapseTileCustom(const TArray<FWaveFunctionCollapseOptionCustom>& Options, float Entropy)
	{
		RemainingOptions = Options;
		ShannonEntropy = Entropy;
	}

	// constructor with only one option
	FWaveFunctionCollapseTileCustom(const FWaveFunctionCollapseOptionCustom& Option, float Entropy)
	{
		RemainingOptions.Add(Option);
		ShannonEntropy = Entropy;
	}
};

/**
* A helper struct used for queuing during Observation and Propagation phases
*/
USTRUCT(BlueprintType)
struct PROJECT03_API FWaveFunctionCollapseQueueElementCustom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	int32 CenterObjectIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WaveFunctionCollapse")
	EWaveFunctionCollapseAdjacencyCustom Adjacency = EWaveFunctionCollapseAdjacencyCustom::Front;

	FWaveFunctionCollapseQueueElementCustom() = default;

	FWaveFunctionCollapseQueueElementCustom(int32 CenterObjectIndexInput, EWaveFunctionCollapseAdjacencyCustom AdjacencyInput)
	{
		CenterObjectIndex = CenterObjectIndexInput;
		Adjacency = AdjacencyInput;
	}
};

