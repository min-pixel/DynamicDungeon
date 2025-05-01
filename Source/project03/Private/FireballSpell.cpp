// Fill out your copyright notice in the Description page of Project Settings.


#include "FireballSpell.h"
#include "MyDCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SpellProjectile.h"  // ���̾ Ŭ����

UFireballSpell::UFireballSpell()
{
    // �⺻ Projectile Ŭ���� ���� (BP ��� �� �������Ʈ ��η� �ʱ�ȭ ����)
    static ConstructorHelpers::FClassFinder<AActor> FireballBP(TEXT("/Game/BP/Spell/BP_FireballProjectile.BP_FireballProjectile_C"));
    if (FireballBP.Succeeded())
    {
        FireballProjectileClass = FireballBP.Class;
    }

    ManaCost = 20.0f;
    StaminaCost = 10.0f;
    Damage = 50.0f;

}

void UFireballSpell::ActivateSpell(AMyDCharacter* Caster)
{
    if (!Caster || !FireballProjectileClass) return;

    if (!FireballProjectileClass) {
        UE_LOG(LogTemp, Error, TEXT("FireballProjectileClass is null"));
        return;
    }

    // ����/���¹̳� �Һ�
    Caster->Knowledge -= ManaCost;
    Caster->Stamina -= StaminaCost;
    Caster->UpdateHUD();

    // ī�޶� ��ġ �� ����
    FVector SpawnLocation = Caster->GetActorLocation() + Caster->GetActorForwardVector() * 100.0f + FVector(0, 0, 50.0f);
    FRotator SpawnRotation = Caster->GetControlRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Caster;
    SpawnParams.Instigator = Caster;

    ASpellProjectile* Fireball = Caster->GetWorld()->SpawnActor<ASpellProjectile>(
        FireballProjectileClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (Fireball)
    {
        FVector LaunchDirection = SpawnRotation.Vector();
        Fireball->LaunchInDirection(LaunchDirection * FireballSpeed);
    }
}
