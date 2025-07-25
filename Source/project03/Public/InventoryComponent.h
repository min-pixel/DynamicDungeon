// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item.h"
#include "InventoryWidget.h"
#include "InventoryComponent.generated.h"


class AMyDCharacter;

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

	UPROPERTY(ReplicatedUsing = OnRep_InventoryItemsStruct, BlueprintReadOnly, Category = "Inventory")
	TArray<FItemData> InventoryItemsStruct;

	UPROPERTY()
	UInventoryWidget* OwningWidgetInstance = nullptr;

	// 아이템 추가 시도
	/*UFUNCTION(BlueprintCallable)
	bool TryAddItem(AItem* NewItem);*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hotkey")
	TArray<FItemData> HotkeyItems;

	bool TryAddItemByClass(TSubclassOf<AItem> ItemClass);

	bool RemoveItemAtStruct(int32 Index);

	bool TryAddItemByClassWithGrade(TSubclassOf<AItem> ItemClass, uint8 GradeOverride);

	// 특정 인덱스에서 아이템 제거
	/*UFUNCTION(BlueprintCallable)
	bool RemoveItemAt(int32 Index);*/
	
	// 서버 RPC: 아이템 이동 처리
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMoveItem(UInventoryComponent* SourceInventory, int32 FromIndex, int32 ToIndex);

	// 서버 RPC: 아이템 제거 처리  
	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 Index);

	// 서버 RPC: 아이템 추가 처리
	UFUNCTION(Server, Reliable)
	void ServerAddItem(const FItemData& NewItem, int32 ToIndex);

	// 서버 RPC: 아이템 구매 처리
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPurchaseItem(const FItemData& ItemData, int32 ToIndex, int32 Price);

	// 서버 RPC: 아이템 판매 처리
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSellItem(int32 FromIndex, int32 SellPrice);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerMoveItemBetweenInventories(UInventoryComponent* SourceInv, UInventoryComponent* DestInv, int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSyncInventoryFromLobby(const TArray<FItemData>& LobbyItems);

	UFUNCTION()
	void OnRep_InventoryItemsStruct();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
