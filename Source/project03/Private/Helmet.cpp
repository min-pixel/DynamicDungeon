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

	//기본 StaticMesh 컴포넌트 생성
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	//기본 박스 메쉬로 설정
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