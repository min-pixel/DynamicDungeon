// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "GameFramework/Actor.h"
#include "GreatWeapon.generated.h"

UCLASS()
class PROJECT03_API AGreatWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGreatWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void ApplyWeaponStats(class AMyDCharacter* Character) override;
	virtual void RemoveWeaponStats(class AMyDCharacter* Character) override;

};
