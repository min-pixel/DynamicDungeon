// Fill out your copyright notice in the Description page of Project Settings.


#include "RobeBottom.h"


ARobeBottom::ARobeBottom()
{
    ItemName = TEXT("Silken Robe (Bottom)");
    ArmorType = EArmorType::Legs;
    BonusHealth = 15;
    BonusStamina = 15;
    Price = 80;
    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/trousers.trousers"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/clothes/mesh/SKM_BodyA_Casual_01_Pants_a.SKM_BodyA_Casual_01_Pants_a"));
    if (MeshAsset.Succeeded() && ArmorVisualMesh)
    {
        ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
    }
}

void ARobeBottom::BeginPlay()
{
    Super::BeginPlay();
}