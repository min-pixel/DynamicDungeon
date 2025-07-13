// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "MyDCharacter.h" 
#include "InventoryWidget.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    //InventoryItems.SetNum(Capacity); // Capacity만큼 슬롯 확보
    //InventoryItemsStruct.SetNum(Capacity);
	// ...

   

    

    // 컴포넌트 자체 복제
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
    // (서버 권한이 있는 쪽에서 한 번 더 확실히 초기화)
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
//    return false; // 공간 없음
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
        InventoryItemsStruct[Index] = FItemData(); // 빈 구조체로 초기화
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

    //등급 덮어쓰기
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
    // 배열 복제 등록
    DOREPLIFETIME(UInventoryComponent, InventoryItemsStruct);
}

void UInventoryComponent::OnRep_InventoryItemsStruct()
{
    UE_LOG(LogTemp, Log, TEXT("OnRep_InventoryItemsStruct called - Items count: %d"), InventoryItemsStruct.Num());

    // UI가 떠 있으면, 서버에서 내려온 최신 배열로 다시 채워 넣는다.
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
    // 1) 서버에서만 실제 배열 수정
    if (SourceInv && SourceInv->InventoryItemsStruct.IsValidIndex(FromIndex) &&
        InventoryItemsStruct.IsValidIndex(ToIndex))
    {
        FItemData Moved = SourceInv->InventoryItemsStruct[FromIndex];
        SourceInv->InventoryItemsStruct[FromIndex] = FItemData();
        InventoryItemsStruct[ToIndex] = Moved;
    }

    //// 2) 리스너 서버 자신(UI)을 즉시 갱신
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

bool UInventoryComponent::ServerPurchaseItem_Validate(const FItemData& ItemData, int32 ToIndex, int32 Price)
{
    // 기본 유효성 검사
    return ToIndex >= 0 && ToIndex < InventoryItemsStruct.Num() && Price >= 0;
}

void UInventoryComponent::ServerPurchaseItem_Implementation(const FItemData& ItemData, int32 ToIndex, int32 Price)
{
    // 서버에서만 실행
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    // 플레이어 캐릭터 가져오기
    AMyDCharacter* Character = Cast<AMyDCharacter>(GetOwner());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("ServerPurchaseItem: Owner is not AMyDCharacter"));
        return;
    }

    // 골드 확인
    if (Character->Gold < Price)
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerPurchaseItem: Not enough gold. Need %d, have %d"), Price, Character->Gold);
        return;
    }

    // 인벤토리 슬롯이 비어있는지 확인
    if (InventoryItemsStruct.IsValidIndex(ToIndex) && InventoryItemsStruct[ToIndex].ItemClass == nullptr)
    {
        // 골드 차감
        Character->Gold -= Price;

        // 아이템 추가
        InventoryItemsStruct[ToIndex] = ItemData;

        // 클라이언트에 골드 업데이트 알림 (Gold가 Replicated 변수라면 자동으로 동기화됨)
        UE_LOG(LogTemp, Log, TEXT("ServerPurchaseItem: Successfully purchased %s for %d gold"),
            *ItemData.ItemName, Price);

        // 골드 UI 업데이트를 위한 멀티캐스트 RPC 호출 (필요한 경우)
        Character->UpdateGoldUI();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerPurchaseItem: Invalid slot index or slot not empty"));
    }
}

bool UInventoryComponent::ServerSellItem_Validate(int32 FromIndex, int32 SellPrice)
{
    return FromIndex >= 0 && FromIndex < InventoryItemsStruct.Num() && SellPrice >= 0;
}

void UInventoryComponent::ServerSellItem_Implementation(int32 FromIndex, int32 SellPrice)
{
    // 서버에서만 실행
    if (!GetOwner()->HasAuthority())
    {
        return;
    }

    AMyDCharacter* Character = Cast<AMyDCharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    // 아이템이 있는지 확인
    if (InventoryItemsStruct.IsValidIndex(FromIndex) && InventoryItemsStruct[FromIndex].ItemClass != nullptr)
    {
        FString ItemName = InventoryItemsStruct[FromIndex].ItemName;

        // 아이템 제거
        InventoryItemsStruct[FromIndex] = FItemData();

        // 골드 추가
        Character->Gold += SellPrice;

        UE_LOG(LogTemp, Log, TEXT("ServerSellItem: Sold %s for %d gold"), *ItemName, SellPrice);

        // 골드 UI 업데이트
        Character->UpdateGoldUI();
    }
}