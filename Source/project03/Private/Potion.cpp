// Potion.cpp
#include "Potion.h"

#include "MyDCharacter.h"

APotion::APotion()
{
    ItemData.ItemName = "HealthPotion";
    ItemData.ItemType = EItemType::Potion;
    ItemData.Count = 1;
    
    HealAmount = 50; // 기본 회복량

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconAsset(TEXT("/Game/BP/Icon/free-icon-potion-114123.free-icon-potion-114123"));
    if (IconAsset.Succeeded())
    {
        ItemData.ItemIcon = IconAsset.Object;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load potion icon!"));
    }

}

void APotion::BeginPlay()
{
    Super::BeginPlay();
}

