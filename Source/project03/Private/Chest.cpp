// Fill out your copyright notice in the Description page of Project Settings.


#include "Chest.h"
#include "Components/StaticMeshComponent.h"

AChest::AChest()
{
	ItemName = TEXT("Chestplate");
	ArmorType = EArmorType::Chest;
	BonusHealth = 50;
	Price = 250;


	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/armor.armor"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}

	
	///Game/BP/armour/Mesh/platemailchest.platemailchest
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Clothing/Meshes/Weighted/Male/SKM_Male_Armor_Chest.SKM_Male_Armor_Chest"));
	if (MeshAsset.Succeeded())
	{
		if (ArmorVisualMesh)
		{
			ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
		}
	}

}

void AChest::BeginPlay()
{
	Super::BeginPlay();

	
}