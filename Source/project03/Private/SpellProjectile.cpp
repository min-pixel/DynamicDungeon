#include "SpellProjectile.h"
#include "Components/BoxComponent.h" // Box�� ��ü
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "HitInterface.h"

ASpellProjectile::ASpellProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    Box->InitBoxExtent(FVector(50.f, 50.f, 50.f)); // ũ�� ����
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // ���� �浹 ����
    Box->SetCollisionObjectType(ECC_WorldDynamic);
    Box->SetCollisionResponseToAllChannels(ECR_Block); // ���� Block���� ó��
    Box->SetNotifyRigidBodyCollision(true); // Hit �̺�Ʈ Ȱ��ȭ
    Box->OnComponentHit.AddDynamic(this, &ASpellProjectile::OnHit); // ��������Ʈ ���
    RootComponent = Box;

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

void ASpellProjectile::OnHit(
    UPrimitiveComponent* HitComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (!OtherActor || OtherActor == this || OtherActor == Caster) return;

    UE_LOG(LogTemp, Warning, TEXT("Hit! Projectile hit: %s"), *OtherActor->GetName());

    if (OtherActor->Implements<UHitInterface>())
    {
        IHitInterface::Execute_GetHit(OtherActor, Hit, Caster, Damage);
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
    if (MovementComponent)
    {
        MovementComponent->Velocity = LaunchVelocity;
        MovementComponent->Activate(true);
    }
}
