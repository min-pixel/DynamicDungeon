// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "USpellBase.h"
#include "ScrollItem.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AScrollItem : public AItem
{
	GENERATED_BODY()

public:
    AScrollItem();

protected:
    //virtual void BeginPlay() override;



public:
    virtual void Use_Implementation(class AMyDCharacter* Character) override;
    void UseWithData(AMyDCharacter* Character, const FItemData& Data);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UUSpellBase> AssignedSpell;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SkillIndex;

    TSubclassOf<UUSpellBase> GetSpellFromIndex(int32 Index) const;
 
    void InitFromData(const FItemData& Data);

    static TArray<TSubclassOf<UUSpellBase>> SharedSpellPool;
    static void InitializeSpellPoolIfNeeded();


};
