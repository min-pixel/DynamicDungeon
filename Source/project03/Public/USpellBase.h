// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "USpellBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UUSpellBase : public UObject
{
	GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FName SpellName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float Damage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float ManaCost;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float StaminaCost;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float CooldownTime;

    virtual void ActivateSpell(class AMyDCharacter* Caster);
    virtual bool CanActivate(class AMyDCharacter* Caster) const;
};
