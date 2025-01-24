// Fill out your copyright notice in the Description page of Project Settings.

#include "NewActorComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

// Sets default values for this component's properties
UNewActorComponent::UNewActorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    StartLocation = FVector::ZeroVector;
    HeldLightActor = nullptr;
    bIsHoldingLight = false;
}

// Called when the game starts
void UNewActorComponent::BeginPlay()
{
    Super::BeginPlay();

    StartLocation = GetOwner()->GetActorLocation();

    if (GlowMaterial)
    {
        DynamicMaterial = UMaterialInstanceDynamic::Create(GlowMaterial, this);
        if (DynamicMaterial)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("DynamicMaterial Successfully Created"));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to Create DynamicMaterial"));
        }
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        Owner->OnActorBeginOverlap.AddDynamic(this, &UNewActorComponent::HandleOverlap);
    }
}

// Called every frame
void UNewActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
    {
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();
        FVector CurrentLocation = GetOwner()->GetActorLocation();
        FVector Direction = (PlayerLocation - CurrentLocation).GetSafeNormal();
        FVector NewLocation = CurrentLocation + Direction * PlatformVelocity.Size() * SpeedMultiplier * DeltaTime;

        GetOwner()->SetActorLocation(NewLocation);

        FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
        GetOwner()->SetActorRotation(NewRotation);

        if (bIsHoldingLight)
        {
            if (!IsValid(HeldLightActor))
            {
                bIsHoldingLight = false;
                GetOwner()->SetActorHiddenInGame(false);
            }
            else
            {
                UpdateVisibilityBasedOnLight();
            }
        }
        else
        {
            TArray<AActor*> AttachedActors;
            PlayerCharacter->GetAttachedActors(AttachedActors);
            for (AActor* Actor : AttachedActors)
            {
                if (Actor->GetName().Contains("light"))
                {
                    HeldLightActor = Actor;
                    bIsHoldingLight = true;
                    break;
                }
            }
        }
    }
}

void UNewActorComponent::UpdateVisibilityBasedOnLight()
{
    if (HeldLightActor)
    {
        UPointLightComponent* PointLight = HeldLightActor->FindComponentByClass<UPointLightComponent>();
        if (PointLight)
        {
            FVector PlatformLocation = GetOwner()->GetActorLocation();
            FVector LightLocation = PointLight->GetComponentLocation();

            float Distance = FVector::Dist(PlatformLocation, LightLocation);
            float LightRadius = PointLight->AttenuationRadius;

            if (Distance <= LightRadius)
            {
                GetOwner()->SetActorHiddenInGame(true);
            }
            else
            {
                GetOwner()->SetActorHiddenInGame(false);
            }
        }
    }
}

void UNewActorComponent::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (OtherActor && OtherActor != GetOwner())
    {
        if (ExplosionEffect)
        {
            UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetOwner()->GetActorLocation());
        }

        if (ExplosionSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetOwner()->GetActorLocation());
        }

        GetOwner()->Destroy();
    }
}
