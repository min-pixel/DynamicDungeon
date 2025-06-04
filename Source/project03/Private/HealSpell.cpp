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

	// �� ����Ʈ �ε� - ���� ���� ã�� ����.
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

	Caster->HealPlayer(HealAmount); // ü�� ȸ�� �Լ� ���

	//// ���̾ư��� ����Ʈ�� �ִٸ� �� ��ġ�� �߰�
	//if (HealEffect)
	//{
	//	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	//		Caster->GetWorld(),
	//		HealEffect,
	//		Caster->GetActorLocation(),
	//		FRotator::ZeroRotator
	//	);
	//}

	// ���� �߰� �ʿ�
}