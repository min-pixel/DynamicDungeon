// Potion.cpp
#include "Potion.h"

#include "MyDCharacter.h"

APotion::APotion()
{
    ItemData.ItemName = "HealthPotion";
    ItemData.ItemType = EItemType::Potion;
    PotionEffect = EPotionEffectType::Health;
    ItemData.PotionEffect = EPotionEffectType::Health;
    ItemData.Count = 1;
    
    HealAmount = 50; // 기본 회복량

    
   



    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/free-icon-potion-114123.free-icon-potion-114123"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;  
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

