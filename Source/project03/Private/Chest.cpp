// Fill out your copyright notice in the Description page of Project Settings.


#include "Chest.h"
#include "Components/StaticMeshComponent.h"

AChest::AChest()
{
	ItemName = TEXT("Steel Chestplate");
	ArmorType = EArmorType::Chest;
	BonusHealth = 50;

	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/armor.armor"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}

	

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/armour/Mesh/platemailchest.platemailchest"));
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