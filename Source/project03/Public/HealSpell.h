// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "USpellBase.h"
#include "NiagaraSystem.h"
#include "HealSpell.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UHealSpell : public UUSpellBase
{
	GENERATED_BODY()

public:
	UHealSpell();

	virtual void ActivateSpell(AMyDCharacter* Caster) override;

	UPROPERTY(EditDefaultsOnly, Category = "Spell")
	float HealAmount = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spell|Heal")
	UNiagaraSystem* HealEffect;
};
