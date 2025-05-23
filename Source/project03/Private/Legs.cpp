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

	

	// SkeletalMesh 리소스 로딩
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/BP/armour/Mesh/platemailpants.platemailpants"));
	if (MeshAsset.Succeeded())
	{
		ArmorVisualMesh = MeshAsset.Object; // 부모 AArmor에 있는 SkeletalMesh* ArmorVisualMesh;
	}

}

void ALegs::BeginPlay()
{
	Super::BeginPlay();

}