// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item.h"
#include "InventoryWidget.h"
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

	UPROPERTY(ReplicatedUsing = OnRep_InventoryItemsStruct, BlueprintReadOnly, Category = "Inventory")
	TArray<FItemData> InventoryItemsStruct;

	UPROPERTY()
	UInventoryWidget* OwningWidgetInstance = nullptr;

	// ������ �߰� �õ�
	/*UFUNCTION(BlueprintCallable)
	bool TryAddItem(AItem* NewItem);*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotkey")
	TArray<FItemData> HotkeyItems;

	bool TryAddItemByClass(TSubclassOf<AItem> ItemClass);

	bool RemoveItemAtStruct(int32 Index);

	bool TryAddItemByClassWithGrade(TSubclassOf<AItem> ItemClass, uint8 GradeOverride);

	// Ư�� �ε������� ������ ����
	/*UFUNCTION(BlueprintCallable)
	bool RemoveItemAt(int32 Index);*/
	
	// ���� RPC: ������ �̵� ó��
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMoveItem(UInventoryComponent* SourceInventory, int32 FromIndex, int32 ToIndex);

	// ���� RPC: ������ ���� ó��  
	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 Index);

	// ���� RPC: ������ �߰� ó��
	UFUNCTION(Server, Reliable)
	void ServerAddItem(const FItemData& NewItem, int32 ToIndex);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_InventoryItemsStruct();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
