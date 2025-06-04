// Fill out your copyright notice in the Description page of Project Settings.


#include "CurseSpell.h"
#include "MyDCharacter.h"
#include "HitInterface.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


UCurseSpell::UCurseSpell()
{
    SpellName = TEXT("Curse");
    ManaCost = 20.f;
    StaminaCost = 10.f;
}

void UCurseSpell::ActivateSpell(AMyDCharacter* Caster)
{
    if (!Caster) return;

   

    // ���� / ���¹̳� ����
    Caster->Knowledge -= ManaCost;
    Caster->Stamina -= StaminaCost;
    Caster->UpdateHUD();

    // ī�޶� ���� ����Ʈ���̽�
    FVector Start = Caster->FirstPersonCameraComponent->GetComponentLocation() +
                    Caster->FirstPersonCameraComponent->GetForwardVector() * 30.0f;;
    FVector End = Start + Caster->FirstPersonCameraComponent->GetForwardVector() * MaxDistance;

    FHitResult Hit;
    FCollisionQueryParams Params;
    //Params.AddIgnoredActor(Caster);

    DrawDebugLine(Caster->GetWorld(), Start, End, FColor::Blue, false, 2.0f, 0, 2.0f);

    bool bHit = Caster->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

    if (bHit)
    {

        AActor* HitActor = Hit.GetActor();

        UE_LOG(LogTemp, Warning, TEXT("[CurseSpell] linehitwho?: %s"), *HitActor->GetName());

        if (HitActor && HitActor->Implements<UHitInterface>())
        {
            UE_LOG(LogTemp, Warning, TEXT("[CurseSpell] %s is have IHitInterface. ApplyDebuff go."), *HitActor->GetName());

            // ����� ����
            IHitInterface::Execute_ApplyDebuff(HitActor, EDebuffType::Slow, SlowAmount, Duration);
        }
    }

    // ���⼭ ���̾ư��� ����Ʈ�� ���� �߰�
}
