// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "EquipmentWidget.h"
#include "Components/Button.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeLobby(class AMyDCharacter* PlayerRef);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* StartGameButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GoToShopButton;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> StorageWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UEquipmentWidget> EquipmentWidgetClass;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* InventoryWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* StorageWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UEquipmentWidget* EquipmentWidgetInstance;

    UPROPERTY()
    UInventoryComponent* StorageInventoryComponent;

    UPROPERTY()
    UInventoryComponent* InventoryComponentRef;

    UPROPERTY()
    UInventoryComponent* StorageComponentRef;

    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnGoToShopClicked();

    /*UPROPERTY(meta = (BindWidget))
    UButton* StartGameButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GoToShopButton;*/

    UPROPERTY()
    AMyDCharacter* PlayerCharacter;
};
