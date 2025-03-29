// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API USlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:

    UPROPERTY(meta = (BindWidget))
    class UImage* ItemIcon;

    UFUNCTION(BlueprintCallable)
    void SetItem(class AItem* InItem);

protected:
    AItem* StoredItem;

};
