// Fill out your copyright notice in the Description page of Project Settings.


#include "Helmet.h"
#include "Components/StaticMeshComponent.h"

AHelmet::AHelmet()
{
	ItemName = TEXT("Iron Helmet");
	ArmorType = EArmorType::Helmet;
	BonusHealth = 20;

	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/helmet.helmet"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}

	//�⺻ StaticMesh ������Ʈ ����
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	//�⺻ �ڽ� �޽��� ����
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/BP/StaticMeshhelmet.StaticMeshhelmet"));
	if (MeshAsset.Succeeded())
	{
		HelmetStaticMesh = MeshAsset.Object;
	}
}

void AHelmet::BeginPlay()
{
	Super::BeginPlay();
}