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
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/armour/Mesh/platemailchest.platemailchest"));  ///Script/Engine.SkeletalMesh''
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