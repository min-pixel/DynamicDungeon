// Fill out your copyright notice in the Description page of Project Settings.


#include "Mask.h"
#include "Components/StaticMeshComponent.h"

AMask::AMask()
{
    ItemName = TEXT("Rogue Mask");
    ArmorType = EArmorType::Helmet;
    BonusHealth = 0;
    BonusStamina = 30;


    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/plague-doctor.plague-doctor"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/BP/clothes/mesh/Mask.Mask"));
    if (MeshAsset.Succeeded())
    {
        HelmetStaticMesh = MeshAsset.Object;
    }

}

void AMask::BeginPlay()
{
    Super::BeginPlay();
}