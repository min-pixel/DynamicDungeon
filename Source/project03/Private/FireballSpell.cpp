// Fill out your copyright notice in the Description page of Project Settings.


#include "FireballSpell.h"
#include "MyDCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SpellProjectile.h"  // 파이어볼 클래스

UFireballSpell::UFireballSpell()
{
    // 기본 Projectile 클래스 지정 (BP 사용 시 블루프린트 경로로 초기화 가능)
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

    // 마나/스태미나 소비
    Caster->Knowledge -= ManaCost;
    Caster->Stamina -= StaminaCost;
    Caster->UpdateHUD();

    // 카메라 위치 및 방향
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
