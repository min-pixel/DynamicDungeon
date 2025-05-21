// Fill out your copyright notice in the Description page of Project Settings.


#include "Legs.h"
#include "Components/StaticMeshComponent.h"


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

void ALegs::BeginPlay()
{
	Super::BeginPlay();

	if (LoadedMesh && MeshComponent)
	{
		MeshComponent->SetStaticMesh(LoadedMesh);
		MeshComponent->SetRelativeScale3D(FVector(0.5f));  // 크기 조정
	}
}