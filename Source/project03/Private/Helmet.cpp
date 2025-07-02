// Fill out your copyright notice in the Description page of Project Settings.


#include "Helmet.h"
#include "Components/StaticMeshComponent.h"

AHelmet::AHelmet()
{
	ItemName = TEXT("Helmet");
	ArmorType = EArmorType::Helmet;
	BonusHealth = 20;
	Price = 220;
	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/helmet.helmet"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}

	//기본 StaticMesh 컴포넌트 생성
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/BP/StaticMeshhelmet.StaticMeshhelmet"));
	if (MeshAsset.Succeeded())
	{
		HelmetStaticMesh = MeshAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SilverMat(TEXT("/Game/StarterContent/Materials/M_Metal_Steel.M_Metal_Steel"));
	if (SilverMat.Succeeded())
	{
		SilverMaterial = SilverMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMat(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
	if (GoldMat.Succeeded())
	{
		GoldMaterial = GoldMat.Object;
	}

}

void AHelmet::BeginPlay()
{
	Super::BeginPlay();
}