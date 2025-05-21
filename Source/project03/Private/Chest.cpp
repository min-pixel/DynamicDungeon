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

	//기본 StaticMesh 컴포넌트 생성
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	//기본 박스 메쉬로 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (MeshAsset.Succeeded())
	{
		LoadedMesh = MeshAsset.Object;
	}
}

void AChest::BeginPlay()
{
	Super::BeginPlay();

	if (LoadedMesh && MeshComponent)
	{
		MeshComponent->SetStaticMesh(LoadedMesh);
		MeshComponent->SetRelativeScale3D(FVector(0.5f));  // 크기 조정
	}
}