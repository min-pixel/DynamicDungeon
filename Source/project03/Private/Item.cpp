// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"

// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItem::SetDefaultIcon()
{
    // 기본 텍스처 로딩 (필요시 자식에서 오버라이드)
    static ConstructorHelpers::FObjectFinder<UTexture2D> DefaultIcon(TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
    if (DefaultIcon.Succeeded())
    {
        ItemIcon = DefaultIcon.Object;
    }
}
