// Fill out your copyright notice in the Description page of Project Settings.


#include "SpellProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASpellProjectile::ASpellProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
    CollisionComponent->InitSphereRadius(10.0f);
    CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComponent->OnComponentHit.AddDynamic(this, &ASpellProjectile::OnHit);
    RootComponent = CollisionComponent;

    VisualEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Effect"));
    VisualEffect->SetupAttachment(RootComponent);

    MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    MovementComponent->InitialSpeed = 1200.f;
    MovementComponent->MaxSpeed = 1200.f;
    MovementComponent->bRotationFollowsVelocity = true;

}

void ASpellProjectile::Init(AMyDCharacter* InCaster, float InDamage)
{
    Caster = InCaster;
    Damage = InDamage;
}

void ASpellProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this && OtherActor != Caster)
    {
        UGameplayStatics::ApplyDamage(OtherActor, Damage, Caster->GetController(), this, nullptr);
    }

    Destroy();
}

// Called when the game starts or when spawned
void ASpellProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpellProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpellProjectile::LaunchInDirection(const FVector& LaunchVelocity)
{
    if (MovementComponent)
    {
        MovementComponent->Velocity = LaunchVelocity;
    }
}

