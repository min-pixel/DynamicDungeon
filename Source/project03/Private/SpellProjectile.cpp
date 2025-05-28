#include "SpellProjectile.h"
#include "Components/BoxComponent.h" // Box�� ��ü
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "HitInterface.h"

ASpellProjectile::ASpellProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionComponent->InitBoxExtent(FVector(50.f));
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComponent->SetNotifyRigidBodyCollision(true);
    CollisionComponent->SetGenerateOverlapEvents(true);
    CollisionComponent->OnComponentHit.AddDynamic(this, &ASpellProjectile::OnHit);

    CollisionComponent->bHiddenInGame = false;
    CollisionComponent->SetVisibility(true);
    CollisionComponent->ShapeColor = FColor::Green;

    SetRootComponent(CollisionComponent);

    // �ð��� ���� ��ü �޽� �߰� (�浹 ����)
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(CollisionComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(0.1f)); // ���� �۰�
    VisualMesh->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereAsset.Object);
    }

    // ���̾ư��� - ���� �� ����Ʈ �ε�
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireballEffectAsset(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big.NS_Fire_Big")); 
    if (FireballEffectAsset.Succeeded())
    {
        FireballEffect = FireballEffectAsset.Object;
    }

    // ���̾ư��� - �浹 �� ���� ����Ʈ
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionEffectAsset(TEXT("/Game/MsvFx_Niagara_Explosion_Pack_01/Prefabs/Niagara_Explosion_03.Niagara_Explosion_03")); 
    if (ExplosionEffectAsset.Succeeded())
    {
        ExplosionEffect = ExplosionEffectAsset.Object;
    }

    // ���� - �߻���
    static ConstructorHelpers::FObjectFinder<USoundBase> LaunchSoundAsset(TEXT("/Game/Vefects/Free_Fire/Shared/Audio/SFX_FireBig_L.SFX_FireBig_L"));
    if (LaunchSoundAsset.Succeeded())
    {
        LaunchSound = LaunchSoundAsset.Object;
    }

    // ���� - ������
    static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundAsset(TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01")); 
    if (ExplosionSoundAsset.Succeeded())
    {
        ExplosionSound = ExplosionSoundAsset.Object;
    }

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

    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            ExplosionEffect,
            GetActorLocation()
        );
    }

    if (FireLoopSound && FireLoopSound->IsPlaying())
    {
        FireLoopSound->Stop(); // ������ �ҼҸ� ���� ����
    }

    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }


    Destroy();
}

void ASpellProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this || OtherActor == Caster) return;

    UE_LOG(LogTemp, Warning, TEXT("Overlap Hit %s"), *OtherActor->GetName());

    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
    }

    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    Destroy();
}


void ASpellProjectile::BeginPlay()
{
    Super::BeginPlay();

   


    if (RootComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RootComponent is null in SpellProjectile"));
    }
    if (!CollisionComponent->IsRegistered())
    {
        UE_LOG(LogTemp, Warning, TEXT("CollisionComponent is not registered"));
    }




    if (FireballEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            FireballEffect,           // ���̾ư��� �ý���
            RootComponent,            // ���� ������Ʈ
            NAME_None,                // ���� ����
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            true
        );
    }

    if (LaunchSound)
    {
        FireLoopSound = UGameplayStatics::SpawnSoundAtLocation(this, LaunchSound, GetActorLocation());
    }

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
