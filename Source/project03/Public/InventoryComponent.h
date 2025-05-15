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

	// �κ��丮 �� ĭ �� (�⺻: 30ĭ)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Capacity = 32;

	// ������ ��� (�� ���Կ� ����ִ� ������ �����͵�)
	/*UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<AItem*> InventoryItems;*/

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FItemData> InventoryItemsStruct;

	// ������ �߰� �õ�
	/*UFUNCTION(BlueprintCallable)
	bool TryAddItem(AItem* NewItem);*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotkey")
	TArray<FItemData> HotkeyItems;

	bool TryAddItemByClass(TSubclassOf<AItem> ItemClass);

	bool RemoveItemAtStruct(int32 Index);

	// Ư�� �ε������� ������ ����
	/*UFUNCTION(BlueprintCallable)
	bool RemoveItemAt(int32 Index);*/
	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
