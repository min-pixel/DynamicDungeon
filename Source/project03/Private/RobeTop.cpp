// Fill out your copyright notice in the Description page of Project Settings.


#include "RobeTop.h"

ARobeTop::ARobeTop()
{
    ItemName = TEXT("Silken Robe (Top)");
    ArmorType = EArmorType::Chest;
    BonusHealth = 10;

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/casual-t-shirt-.casual-t-shirt-"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/clothes/mesh/SKM_BodyA_Casual_01_Top_a.SKM_BodyA_Casual_01_Top_a"));
    if (MeshAsset.Succeeded() && ArmorVisualMesh)
    {
        ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
    }
}

void ARobeTop::BeginPlay()
{
    Super::BeginPlay();
}