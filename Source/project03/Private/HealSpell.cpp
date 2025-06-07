// Fill out your copyright notice in the Description page of Project Settings.


#include "HealSpell.h"
#include "MyDCharacter.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "OrbitEffectActor.h"
#include "Kismet/GameplayStatics.h"

UHealSpell::UHealSpell()
{
	ManaCost = 10.0f;
	StaminaCost = 5.0f;
	HealAmount = 50.0f;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealEffectAsset(TEXT("/Game/PixieDustTrail/FX/NS_PixieDustTrail.NS_PixieDustTrail"));
	if (HealEffectAsset.Succeeded())
	{
		HealEffect = HealEffectAsset.Object;
	}

}

void UHealSpell::ActivateSpell(AMyDCharacter* Caster)
{
	if (!Caster) return;

	

	Caster->Knowledge -= ManaCost;
	Caster->Stamina -= StaminaCost;
	Caster->UpdateHUD();

	Caster->HealPlayer(HealAmount); // 체력 회복 함수 사용

	// 궤도 이펙트 액터 스폰
	FActorSpawnParameters SpawnParams;
	AOrbitEffectActor* OrbitActor = Caster->GetWorld()->SpawnActor<AOrbitEffectActor>(AOrbitEffectActor::StaticClass(), Caster->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
	if (OrbitActor)
	{
		OrbitActor->InitOrbit(Caster, HealEffect, 100.f, 2.f, 1080.f, FLinearColor(0.4f, 1.f, 0.2f), 5.f); // 반지름 100, 5초간 초당 180도 회전
	}

	// 사운드 추가 필요
}