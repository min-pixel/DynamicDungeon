// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "InventoryWidget.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    //InventoryItems.SetNum(Capacity); // Capacity��ŭ ���� Ȯ��
    //InventoryItemsStruct.SetNum(Capacity);
	// ...

   

    

    // ������Ʈ ��ü ����
    SetIsReplicated(true);

    InventoryItemsStruct.SetNum(Capacity);
    HotkeyItems.SetNum(5);
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
    //InventoryItems.Init(nullptr, Capacity);
	// ...
    // (���� ������ �ִ� �ʿ��� �� �� �� Ȯ���� �ʱ�ȭ)
    if (GetOwner()->HasAuthority() && InventoryItemsStruct.Num() == 0)
    {
        InventoryItemsStruct.SetNum(Capacity);
    }
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


//bool UInventoryComponent::TryAddItem(AItem* NewItem)
//{
//    if (!NewItem) return false;
//
//    for (int32 i = 0; i < InventoryItems.Num(); ++i)
//    {
//        if (!InventoryItems[i])
//        {
//            InventoryItems[i] = NewItem;
//            //NewItem->Destroy();
//            return true;
//        }
//    }
//
//    return false; // ���� ����
//}
//
//bool UInventoryComponent::RemoveItemAt(int32 Index)
//{
//    if (InventoryItems.IsValidIndex(Index) && InventoryItems[Index])
//    {
//        InventoryItems[Index] = nullptr;
//        return true;
//    }
//    return false;
//}

bool UInventoryComponent::TryAddItemByClass(TSubclassOf<AItem> ItemClass)
{
    if (!ItemClass) return false;

    AItem* DefaultItem = ItemClass->GetDefaultObject<AItem>();
    if (!DefaultItem) return false;

    FItemData NewData = DefaultItem->ToItemData();

    for (int32 i = 0; i < InventoryItemsStruct.Num(); ++i)
    {
        if (InventoryItemsStruct[i].ItemClass == nullptr)
        {
            InventoryItemsStruct[i] = NewData;
            return true;
        }
    }

    return false;
}

bool UInventoryComponent::RemoveItemAtStruct(int32 Index)
{
    if (InventoryItemsStruct.IsValidIndex(Index))
    {
        InventoryItemsStruct[Index] = FItemData(); // �� ����ü�� �ʱ�ȭ
        return true;
    }
    return false;
}

bool UInventoryComponent::TryAddItemByClassWithGrade(TSubclassOf<AItem> ItemClass, uint8 GradeOverride)
{
    if (!ItemClass) return false;

    AItem* DefaultItem = ItemClass->GetDefaultObject<AItem>();
    if (!DefaultItem) return false;

    FItemData NewData = DefaultItem->ToItemData();

    //��� �����
    NewData.Grade = GradeOverride;

    for (int32 i = 0; i < InventoryItemsStruct.Num(); ++i)
    {
        if (InventoryItemsStruct[i].ItemClass == nullptr)
        {
            InventoryItemsStruct[i] = NewData;
            return true;
        }
    }

    return false;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // �迭 ���� ���
    DOREPLIFETIME(UInventoryComponent, InventoryItemsStruct);
}

void UInventoryComponent::OnRep_InventoryItemsStruct()
{
    UE_LOG(LogTemp, Log, TEXT("OnRep_InventoryItemsStruct called - Items count: %d"), InventoryItemsStruct.Num());

    // UI�� �� ������, �������� ������ �ֽ� �迭�� �ٽ� ä�� �ִ´�.
    if (OwningWidgetInstance)
    {
        OwningWidgetInstance->RefreshInventoryStruct();
    }
}

bool UInventoryComponent::ServerMoveItem_Validate(UInventoryComponent* SourceInv, int32 FromIndex, int32 ToIndex)
{
    return true; 
}

void UInventoryComponent::ServerMoveItem_Implementation(UInventoryComponent* SourceInv, int32 FromIndex, int32 ToIndex)
{
    // 1) ���������� ���� �迭 ����
    if (SourceInv && SourceInv->InventoryItemsStruct.IsValidIndex(FromIndex) &&
        InventoryItemsStruct.IsValidIndex(ToIndex))
    {
        FItemData Moved = SourceInv->InventoryItemsStruct[FromIndex];
        SourceInv->InventoryItemsStruct.RemoveAt(FromIndex);
        InventoryItemsStruct[ToIndex] = Moved;
    }

    //// 2) ������ ���� �ڽ�(UI)�� ��� ����
    //if (OwningWidgetInstance)
    //{
    //    OwningWidgetInstance->RefreshInventoryStruct();
    //}
    //if (SourceInv->OwningWidgetInstance)
    //{
    //    SourceInv->OwningWidgetInstance->RefreshInventoryStruct();
    //}
}

void UInventoryComponent::ServerRemoveItem_Implementation(int32 Index)
{
    if (InventoryItemsStruct.IsValidIndex(Index))
    {
        InventoryItemsStruct[Index] = FItemData();
        UE_LOG(LogTemp, Log, TEXT("Server: Removed item at index %d"), Index);
    }
}

void UInventoryComponent::ServerAddItem_Implementation(const FItemData& NewItem, int32 ToIndex)
{
    if (InventoryItemsStruct.IsValidIndex(ToIndex))
    {
        InventoryItemsStruct[ToIndex] = NewItem;
        UE_LOG(LogTemp, Log, TEXT("Server: Added item to index %d"), ToIndex);
    }
}