// Fill out your copyright notice in the Description page of Project Settings.


#include "Dagger.h"
#include "MyDCharacter.h"

// Sets default values
ADagger::ADagger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    WeaponType = EWeaponType::Dagger;
    ItemType = EItemType::Weapon;
    BaseDamage = 10.0f; //대형 무기라서 기본 공격력이 높음
    StaminaCost = 5.0f; //공격 시 스태미나 소모량 증가
    Price = 200;
    Damage = BaseDamage;

    // 단검 무기 전용 메쉬 적용
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Dagger_2.SM_Dagger_2"));
    if (MeshAsset.Succeeded())
    {
        LoadedDaggerMesh = MeshAsset.Object; // 저장만
    }

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/free-icon-combat-dagger-8210712.free-icon-combat-dagger-8210712"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    ItemName = TEXT("Dagger");

}

// Called when the game starts or when spawned
void ADagger::BeginPlay()
{
	Super::BeginPlay();
    if (LoadedDaggerMesh && WeaponMesh)
    {
        WeaponMesh->SetStaticMesh(LoadedDaggerMesh); // 여기서는 안전
    }

    

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

void ADagger::SetDefaultIcon()
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("/Game/BP/스크린샷_2024-08-12_122024.스크린샷_2024-08-12_122024"));
    if (Icon.Succeeded())
    {
        ItemIcon = Icon.Object;
    }
}