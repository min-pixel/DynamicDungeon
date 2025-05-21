// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "Chest.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AChest : public AArmor
{
	GENERATED_BODY()
public:
	AChest();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UStaticMesh* LoadedMesh;
};
