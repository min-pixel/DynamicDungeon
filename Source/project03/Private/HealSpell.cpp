// Fill out your copyright notice in the Description page of Project Settings.


#include "HealSpell.h"
#include "MyDCharacter.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

UHealSpell::UHealSpell()
{
	ManaCost = 10.0f;
	StaminaCost = 5.0f;
	HealAmount = 50.0f;

	// 힐 이펙트 로드 - 현재 아직 찾지 못함.
	/*static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HealEffectAsset(TEXT("/Game/Effects/NS_HealEffect.NS_HealEffect"));
	if (HealEffectAsset.Succeeded())
	{
		HealEffect = HealEffectAsset.Object;
	}*/

}

void UHealSpell::ActivateSpell(AMyDCharacter* Caster)
{
	if (!Caster) return;

	

	Caster->Knowledge -= ManaCost;
	Caster->Stamina -= StaminaCost;
	Caster->UpdateHUD();

	Caster->HealPlayer(HealAmount); // 체력 회복 함수 사용

	//// 나이아가라 이펙트가 있다면 이 위치에 추가
	//if (HealEffect)
	//{
	//	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	//		Caster->GetWorld(),
	//		HealEffect,
	//		Caster->GetActorLocation(),
	//		FRotator::ZeroRotator
	//	);
	//}

	// 사운드 추가 필요
}