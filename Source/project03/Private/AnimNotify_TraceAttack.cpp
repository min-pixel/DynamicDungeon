// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_TraceAttack.h"
#include "MyDCharacter.h"
#include "Weapon.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HitInterface.h"

void UAnimNotify_TraceAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;


    AMyDCharacter* Character = Cast<AMyDCharacter>(Owner);
    if (Character && Character->GetEquippedWeapon()) // 캐릭터가 무기 들고 있으면
    {
        Character->GetEquippedWeapon()->TraceAttack(); // 무기의 TraceAttack() 호출
    }
    else
    {
        // 비무장일 경우: 기존 손 공격 로직
        FVector Start = MeshComp->GetSocketLocation(TraceSocketName);
        FVector End = Start + Owner->GetActorForwardVector() * 33.0f;

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
}
