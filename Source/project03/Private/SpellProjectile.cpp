#include "SpellProjectile.h"
#include "Components/BoxComponent.h" // Box로 교체
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "MyDCharacter.h"
#include "Components/AudioComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HitInterface.h"

ASpellProjectile::ASpellProjectile()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

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

    // 시각용 작은 구체 메시 추가 (충돌 없음)
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(CollisionComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetRelativeScale3D(FVector(0.1f)); // 아주 작게
    VisualMesh->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereAsset.Object);
    }

    // 나이아가라 - 비행 중 이펙트 로딩
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> FireballEffectAsset(TEXT("/Game/Vefects/Free_Fire/Shared/Particles/NS_Fire_Big.NS_Fire_Big")); 
    if (FireballEffectAsset.Succeeded())
    {
        FireballEffect = FireballEffectAsset.Object;
    }

    // 나이아가라 - 충돌 시 폭발 이펙트
    static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionEffectAsset(TEXT("/Game/MsvFx_Niagara_Explosion_Pack_01/Prefabs/Niagara_Explosion_03.Niagara_Explosion_03")); 
    if (ExplosionEffectAsset.Succeeded())
    {
        ExplosionEffect = ExplosionEffectAsset.Object;
    }

    // 사운드 - 발사음
    static ConstructorHelpers::FObjectFinder<USoundBase> LaunchSoundAsset(TEXT("/Game/Vefects/Free_Fire/Shared/Audio/SFX_FireBig_L.SFX_FireBig_L"));
    if (LaunchSoundAsset.Succeeded())
    {
        LaunchSound = LaunchSoundAsset.Object;
    }

    // 사운드 - 폭발음
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

void ASpellProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    // 서버에서만 실행
    if (!HasAuthority()) return;

    if (!OtherActor || OtherActor == this || OtherActor == Caster) return;

    UE_LOG(LogTemp, Warning, TEXT("Projectile hit: %s"), *OtherActor->GetName());

    // 데미지 처리 (서버에서만)
    if (OtherActor->Implements<UHitInterface>())
    {
        IHitInterface::Execute_GetHit(OtherActor, Hit, Caster, Damage);
    }

    // 모든 클라이언트에서 폭발 이펙트 재생
    MulticastPlayExplosionEffect(GetActorLocation());

    // 투사체 제거
    Destroy();
}

bool ASpellProjectile::ServerOnHit_Validate(AActor* HitActor, const FHitResult& HitResult)
{
    return HitActor != nullptr;
}

void ASpellProjectile::ServerOnHit_Implementation(AActor* HitActor, const FHitResult& HitResult)
{
    // 서버에서 충돌 처리
    OnHit(nullptr, HitActor, nullptr, FVector::ZeroVector, HitResult);
}

void ASpellProjectile::MulticastPlayExplosionEffect_Implementation(FVector Location)
{
    // 폭발 이펙트
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, Location);
    }

    // 폭발 사운드
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
    }

    // 발사 사운드 정지
    if (FireLoopSound && FireLoopSound->IsPlaying())
    {
        FireLoopSound->Stop();
    }
}

void ASpellProjectile::LaunchInDirection(const FVector& LaunchVelocity)
{
    if (MovementComponent)
    {
        MovementComponent->Velocity = LaunchVelocity;
        MovementComponent->Activate(true);

        // 속도 복제
        if (HasAuthority())
        {
            ReplicatedVelocity = LaunchVelocity;
        }
    }
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


    // 서버에서만 충돌 이벤트 바인딩
    if (HasAuthority())
    {
        CollisionComponent->OnComponentHit.AddDynamic(this, &ASpellProjectile::OnHit);
    }

    if (FireballEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            FireballEffect,           // 나이아가라 시스템
            RootComponent,            // 붙일 컴포넌트
            NAME_None,                // 소켓 없음
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



void ASpellProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASpellProjectile, Caster);
    DOREPLIFETIME(ASpellProjectile, Damage);
    DOREPLIFETIME(ASpellProjectile, ReplicatedVelocity);
}

void ASpellProjectile::OnRep_ProjectileData()
{
    // 클라이언트에서 속도 동기화
    if (MovementComponent && !HasAuthority())
    {
        MovementComponent->Velocity = ReplicatedVelocity;
    }
}

