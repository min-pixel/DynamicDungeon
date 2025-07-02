// Fill out your copyright notice in the Description page of Project Settings.


#include "Hat.h"
#include "Components/StaticMeshComponent.h"

AHat::AHat()
{
    ItemName = TEXT("magic Hat");
    ArmorType = EArmorType::Helmet;
    BonusHealth = 0;
    BonusMana = 30;
    Price = 150;

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/wizard-hat.wizard-hat"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/BP/clothes/mesh/witch_hatID.witch_hatID"));
    if (MeshAsset.Succeeded())
    {
        HelmetStaticMesh = MeshAsset.Object;
    }

}

void AHat::BeginPlay()
{
    Super::BeginPlay();
}
