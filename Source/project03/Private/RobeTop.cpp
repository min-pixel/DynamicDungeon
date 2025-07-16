// Fill out your copyright notice in the Description page of Project Settings.


#include "RobeTop.h"

ARobeTop::ARobeTop()
{
    ItemName = TEXT("Silken Robe (Top)");
    ArmorType = EArmorType::Chest;
    BonusHealth = 25;
    BonusStamina = 25;
    Price = 100;
    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/casual-t-shirt-.casual-t-shirt-"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Clothing/Meshes/Weighted/Male/SKM_Male_Clothing_Shirt.SKM_Male_Clothing_Shirt"));
    if (MeshAsset.Succeeded() && ArmorVisualMesh)
    {
        ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
    }
}

void ARobeTop::BeginPlay()
{
    Super::BeginPlay();
}