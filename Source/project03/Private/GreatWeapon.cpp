// Fill out your copyright notice in the Description page of Project Settings.

#include "GreatWeapon.h"
#include "MyDCharacter.h"

// Sets default values
AGreatWeapon::AGreatWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    WeaponType = EWeaponType::GreatWeapon;
    BaseDamage = 50.0f; //���� ����� �⺻ ���ݷ��� ����
    StaminaCost = 30.0f; //���� �� ���¹̳� �Ҹ� ����

    Damage = BaseDamage;

    // ���� ���� ���� �޽� ����
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_GreatHammer.SM_GreatHammer"));
    if (MeshAsset.Succeeded())
    {
        WeaponMesh->SetStaticMesh(MeshAsset.Object);
    }

}

// Called when the game starts or when spawned
void AGreatWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGreatWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGreatWeapon::ApplyWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->AttackStaminaCost -= 10.0f;
    }
}

void AGreatWeapon::RemoveWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->AttackStaminaCost += 10.0f;
    }
}