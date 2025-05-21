// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "Helmet.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AHelmet : public AArmor
{
	GENERATED_BODY()
public:
	AHelmet();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UStaticMesh* LoadedMesh;
};
