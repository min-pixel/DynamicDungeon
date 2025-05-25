// Fill out your copyright notice in the Description page of Project Settings.


#include "Legs.h"
#include "Components/SkeletalMeshComponent.h"


ALegs::ALegs()
{
	ItemName = TEXT("Leather Greaves");
	ArmorType = EArmorType::Legs;
	BonusHealth = 30;

	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/boots.boots"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}

	

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/armour/Mesh/platemailpants.platemailpants"));
	if (MeshAsset.Succeeded())
	{
		//ArmorVisualMesh는 컴포넌트니까, 여기서는 SkeletalMesh를 넣어야 함
		if (ArmorVisualMesh)
		{
			ArmorVisualMesh->SetSkeletalMesh(MeshAsset.Object);
		}
	}

}

void ALegs::BeginPlay()
{
	Super::BeginPlay();

}