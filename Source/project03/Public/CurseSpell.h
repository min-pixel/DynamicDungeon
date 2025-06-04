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
    float SlowAmount = 0.5f; // �̵��ӵ� ���� ��� (0.5 = 50%)

    UPROPERTY(EditDefaultsOnly, Category = "Curse")
    float Duration = 50.0f;    // ���� �ð� (��) ������ 10��

    UPROPERTY(EditDefaultsOnly, Category = "Curse")
    float MaxDistance = 1000.f; // ����Ʈ���̽� �ִ� �Ÿ�
};
