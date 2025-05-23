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

	

	// SkeletalMesh ���ҽ� �ε�
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/armour/Mesh/platemailpants.platemailpants"));
	if (MeshAsset.Succeeded())
	{
		ArmorVisualMesh = MeshAsset.Object; // �θ� AArmor�� �ִ� SkeletalMesh* ArmorVisualMesh;
	}

}

void ALegs::BeginPlay()
{
	Super::BeginPlay();

}