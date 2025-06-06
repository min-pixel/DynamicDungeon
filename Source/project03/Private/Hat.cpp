// Fill out your copyright notice in the Description page of Project Settings.


#include "Hat.h"
#include "Components/StaticMeshComponent.h"

AHat::AHat()
{
    ItemName = TEXT("magic cloak");
    ArmorType = EArmorType::Helmet;
    BonusHealth = 0;

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/cloak.cloak"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    /*static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/clothes/mesh/SKM_Medieval_Hooded_Cloak_Mesh.SKM_Medieval_Hooded_Cloak_Mesh"));
    if (MeshAsset.Succeeded() && ArmorVisualMesh)
    {
        ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
    }*/
}

void AHat::BeginPlay()
{
    Super::BeginPlay();
}
