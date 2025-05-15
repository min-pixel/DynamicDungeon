// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT03_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	// 인벤토리 총 칸 수 (기본: 30칸)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Capacity = 32;

	// 아이템 목록 (각 슬롯에 들어있는 아이템 포인터들)
	/*UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<AItem*> InventoryItems;*/

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FItemData> InventoryItemsStruct;

	// 아이템 추가 시도
	/*UFUNCTION(BlueprintCallable)
	bool TryAddItem(AItem* NewItem);*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotkey")
	TArray<FItemData> HotkeyItems;

	bool TryAddItemByClass(TSubclassOf<AItem> ItemClass);

	bool RemoveItemAtStruct(int32 Index);

	// 특정 인덱스에서 아이템 제거
	/*UFUNCTION(BlueprintCallable)
	bool RemoveItemAt(int32 Index);*/
	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
