// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "GameFramework/Actor.h"
#include "Dagger.generated.h"

UCLASS()
class PROJECT03_API ADagger : public AWeapon
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADagger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void ApplyWeaponStats(class AMyDCharacter* Character) override;
	virtual void RemoveWeaponStats(class AMyDCharacter* Character) override;

	virtual void SetDefaultIcon() override;

	UStaticMesh* LoadedDaggerMesh = nullptr;
	UTexture2D* LoadedIconTexture = nullptr;
};
