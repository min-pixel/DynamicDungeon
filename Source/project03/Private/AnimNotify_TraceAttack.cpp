// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_TraceAttack.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HitInterface.h"

void UAnimNotify_TraceAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	FVector Start = MeshComp->GetSocketLocation("hand_r"); // 소켓 이름은 무기나 손 위치
	FVector End = Start + Owner->GetActorForwardVector() * 100.0f;

	TArray<FHitResult> HitResults;
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Owner);

	UKismetSystemLibrary::SphereTraceMulti(
		Owner->GetWorld(),
		Start,
		End,
		Radius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		IgnoredActors,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<UHitInterface>())
		{
			IHitInterface::Execute_GetHit(HitActor, Hit, Owner, Damage);
		}
	}
}

