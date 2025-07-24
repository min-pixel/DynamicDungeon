// Fill out your copyright notice in the Description page of Project Settings.

#include "GreatWeapon.h"
#include "Engine/Texture2D.h"
#include "MyDCharacter.h"

// Sets default values
AGreatWeapon::AGreatWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    WeaponType = EWeaponType::GreatWeapon;
    ItemType = EItemType::Weapon;
    BaseDamage = 40.0f; //대형 무기라서 기본 공격력이 높음
    StaminaCost = -10.0f; //공격 시 스태미나 소모량 증가
    Price = 350;
    Damage = BaseDamage;

    // 대형 무기 전용 메쉬 적용
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_GreatHammer.SM_GreatHammer"));
    if (MeshAsset.Succeeded())
    {
        LoadedGreatWeaponMesh = MeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/free-icon-weapon-1583527.free-icon-weapon-1583527"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    ItemName = TEXT("WarHammer");

}

// Called when the game starts or when spawned
void AGreatWeapon::BeginPlay()
{
	Super::BeginPlay();
    //SetDefaultIcon();

    if (LoadedGreatWeaponMesh && WeaponMesh)
    {
        WeaponMesh->SetStaticMesh(LoadedGreatWeaponMesh); // 안전하게 여기서 호출
    }

    
	
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
        Character->AttackStaminaCost -= StaminaCost;
    }
}

void AGreatWeapon::RemoveWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->AttackStaminaCost += StaminaCost;
    }
}

void AGreatWeapon::SetDefaultIcon()
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("/Game/BP/스크린샷_2024-08-12_122024.스크린샷_2024-08-12_122024"));
    if (Icon.Succeeded())
    {
        ItemIcon = Icon.Object;
    }
}