// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "USpellBase.h"
#include "CurseSpell.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UCurseSpell : public UUSpellBase
{
	GENERATED_BODY()

public:
	UCurseSpell();

	virtual void ActivateSpell(AMyDCharacter* Caster) override;

    UPROPERTY(EditDefaultsOnly, Category = "Curse")
    float SlowAmount = 0.5f; // 이동속도 감소 계수 (0.5 = 50%)

    UPROPERTY(EditDefaultsOnly, Category = "Curse")
    float Duration = 50.0f;    // 지속 시간 (초) 원래는 10초

    UPROPERTY(EditDefaultsOnly, Category = "Curse")
    float MaxDistance = 1000.f; // 라인트레이스 최대 거리
};
