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

	//�⺻ StaticMesh ������Ʈ ����
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	//�⺻ �ڽ� �޽��� ����
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
		MeshComponent->SetRelativeScale3D(FVector(0.5f));  // ũ�� ����
	}
}