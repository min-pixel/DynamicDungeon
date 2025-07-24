// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDataD.h"
#include "Item.h"
#include "InventoryComponent.h"
#include "InventoryWidget.h"
#include "TreasureChest.generated.h"

class AMyDCharacter;

UCLASS()
class PROJECT03_API ATreasureChest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATreasureChest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ChestMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    UInventoryComponent* ChestInventory;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TSubclassOf<AItem>> PossibleItems;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxItemsInChest = 5;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Treasure")
    TArray<FItemData> GeneratedItems;

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* InteractionBox;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void OpenChestUI(AMyDCharacter* InteractingPlayer);

    class TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY()
    UInventoryWidget* ChestInventoryWidgetInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<USlotWidget> SlotWidgetClass;

    UPROPERTY(Replicated)
    AMyDCharacter* CurrentUser = nullptr;

    

    void GenerateRandomItems();

    void CloseChestUI(AMyDCharacter* InteractingPlayer);

    // 플레이어 사망으로 생성된 상자인지 확인하는 플래그
    UPROPERTY(BlueprintReadWrite, Category = "Chest")
    bool bIsPlayerDeathChest = false;

};
