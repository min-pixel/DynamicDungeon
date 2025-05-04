#include "SpellProjectile.h"
#include "Components/BoxComponent.h" // Box로 교체
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "HitInterface.h"

ASpellProjectile::ASpellProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    Box->InitBoxExtent(FVector(50.f, 50.f, 50.f)); // 크기 조정
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 물리 충돌 포함
    Box->SetCollisionObjectType(ECC_WorldDynamic);
    Box->SetCollisionResponseToAllChannels(ECR_Block); // 전부 Block으로 처리
    Box->SetNotifyRigidBodyCollision(true); // Hit 이벤트 활성화
    Box->OnComponentHit.AddDynamic(this, &ASpellProjectile::OnHit); // 델리게이트 등록
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
