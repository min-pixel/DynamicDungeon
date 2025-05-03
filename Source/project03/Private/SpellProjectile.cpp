#include "SpellProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "HitInterface.h" // HitInterface 관련 include 필요

ASpellProjectile::ASpellProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    USphereComponent* Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    Sphere->InitSphereRadius(50.0f);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); // 캐릭터에 반응
    Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Sphere->OnComponentBeginOverlap.AddDynamic(this, &ASpellProjectile::OnOverlapBegin);
    RootComponent = Sphere;

    VisualEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Effect"));
    VisualEffect->SetupAttachment(RootComponent);

    MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    MovementComponent->InitialSpeed = 1200.f;
    MovementComponent->MaxSpeed = 1200.f;
    MovementComponent->ProjectileGravityScale = 0.0f;
    MovementComponent->bRotationFollowsVelocity = true;
    MovementComponent->bAutoActivate = true;
    MovementComponent->SetUpdatedComponent(RootComponent);
}

void ASpellProjectile::Init(AMyDCharacter* InCaster, float InDamage)
{
    Caster = InCaster;
    Damage = InDamage;
}

void ASpellProjectile::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this || OtherActor == Caster) return;

    UE_LOG(LogTemp, Warning, TEXT("Fireball overlapped with: %s"), *OtherActor->GetName());

    if (OtherActor->Implements<UHitInterface>())
    {
        IHitInterface::Execute_GetHit(OtherActor, SweepResult, Caster, Damage);
    }

    Destroy();
}

void ASpellProjectile::BeginPlay()
{
    Super::BeginPlay();
}

void ASpellProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASpellProjectile::LaunchInDirection(const FVector& LaunchVelocity)
{
    UE_LOG(LogTemp, Warning, TEXT("LaunchInDirection called with velocity: %s"), *LaunchVelocity.ToString());

    if (MovementComponent)
    {
        MovementComponent->Velocity = LaunchVelocity;
        MovementComponent->Activate(true);
    }
}
