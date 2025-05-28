// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "USpellBase.h"
#include "SpellProjectile.h"
#include "FireballSpell.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UFireballSpell : public UUSpellBase
{
	GENERATED_BODY()
	
public:

	UFireballSpell();

	virtual void ActivateSpell(AMyDCharacter* Caster) override;

	UPROPERTY(EditDefaultsOnly, Category = "Spell")
	TSubclassOf<ASpellProjectile> FireballProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Spell")
	float FireballSpeed = 1000.0f;

};
