// Fill out your copyright notice in the Description page of Project Settings.


#include "Dagger.h"
#include "MyDCharacter.h"

// Sets default values
ADagger::ADagger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    WeaponType = EWeaponType::Dagger;
    BaseDamage = 10.0f; //���� ����� �⺻ ���ݷ��� ����
    StaminaCost = 5.0f; //���� �� ���¹̳� �Ҹ� ����

    Damage = BaseDamage;

    // �ܰ� ���� ���� �޽� ����
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Dagger_2.SM_Dagger_2"));
    if (MeshAsset.Succeeded())
    {
        WeaponMesh->SetStaticMesh(MeshAsset.Object);
    }

}

// Called when the game starts or when spawned
void ADagger::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADagger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADagger::ApplyWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->AttackStaminaCost -= StaminaCost;
    }
}

void ADagger::RemoveWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->AttackStaminaCost += StaminaCost;
    }
}