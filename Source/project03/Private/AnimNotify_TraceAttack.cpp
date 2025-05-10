// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_TraceAttack.h"
#include "MyDCharacter.h"
#include "Weapon.h"
#include "EnemyCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HitInterface.h"

void UAnimNotify_TraceAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    AActor* Owner = MeshComp->GetOwner();
    if (!Owner) return;

    

    AMyDCharacter* Character = Cast<AMyDCharacter>(Owner);
    if (Character && Character->GetEquippedWeapon()) // ĳ���Ͱ� ���� ��� ������
    {
        Character->GetEquippedWeapon()->TraceAttack(); // ������ TraceAttack() ȣ��
    }
    else if (AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Owner))
    {
        Enemy->StartTrace();
        Enemy->TraceAttack(); // �� ĳ���� ���� TraceAttack
        //return;
    }
    else
    {
        // ������ ���: ���� �� ���� ����
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
            EDrawDebugTrace::None,
            HitResults,
            true
        );

        for (const FHitResult& Hit : HitResults)
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && HitActor->Implements<UHitInterface>())
            {
                if (!Character->HitActors.Contains(HitActor)) // �ߺ� �ǰ� ����
                {
                    Character->HitActors.Add(HitActor);
                    IHitInterface::Execute_GetHit(HitActor, Hit, Owner, Damage);
                }
            }
        }
    }
}
