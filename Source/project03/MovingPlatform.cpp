#include "MovingPlatform.h"
#include "Components/SphereComponent.h" // ��ü �ݸ��� ����� ���� �߰�
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Engine/PointLight.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Components/PointLightComponent.h"

// Sets default values
AMovingPlatform::AMovingPlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    ShapesMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapesMesh"));
    RootComponent = ShapesMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShapesVisualAsset(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
    if (ShapesVisualAsset.Succeeded())
    {
        ShapesMesh->SetStaticMesh(ShapesVisualAsset.Object);
    }

    // �߱� ��Ƽ���� �Ҵ�
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterialAsset(TEXT("/Game/StarterContent/Materials/M_Tech_Hex_Tile_Pulse.M_Tech_Hex_Tile_Pulse"));
    if (GlowMaterialAsset.Succeeded())
    {
        GlowMaterial = GlowMaterialAsset.Object;
    }

    // ���� ��Ƽ���� ����
    DynamicMaterial = UMaterialInstanceDynamic::Create(GlowMaterial, this);
    ShapesMesh->SetMaterial(0, DynamicMaterial);

    // �ݸ��� �ڽ��� ��ü �ݸ������� ����
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetCollisionProfileName("BlockAllDynamic");
    CollisionSphere->SetupAttachment(ShapesMesh);

    // ��ü ũ�⸦ ����ƽ �޽��� ��迡�� ���� �ݰ����� ����
    float SphereRadius = ShapesMesh->GetStaticMesh()->GetBounds().SphereRadius;
    CollisionSphere->SetSphereRadius(SphereRadius * 1.2f);

    CollisionSphere->SetRelativeLocation(FVector(0.0f, 0.0f, SphereRadius));

    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    ShapesMesh->SetSimulatePhysics(true);
    ShapesMesh->SetEnableGravity(true);
    ShapesMesh->BodyInstance.bLockZRotation = false;

    static ConstructorHelpers::FObjectFinder<USoundBase> ExplosionSoundAsset(TEXT("/Game/StarterContent/Audio/Explosion02.Explosion02"));
    if (ExplosionSoundAsset.Succeeded())
    {
        ExplosionSound = ExplosionSoundAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UParticleSystem> ExplosionEffectAsset(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
    if (ExplosionEffectAsset.Succeeded())
    {
        ExplosionEffect = ExplosionEffectAsset.Object;
    }

    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMovingPlatform::OnOverlapBegin);

    SpeedMultiplier = 1.5f;

    HeldLightActor = nullptr;
    bIsHoldingLight = false;
}

void AMovingPlatform::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();
    // ��Ƽ������ ����� �����ǰ� ����Ǿ����� Ȯ��
    if (DynamicMaterial)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("DynamicMaterial Successfully Created"));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to Create DynamicMaterial"));
    }
}

void AMovingPlatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    if (PlayerCharacter)
    {
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();
        FVector CurrentLocation = GetActorLocation();
        FVector Direction = (PlayerLocation - CurrentLocation).GetSafeNormal();
        FVector NewLocation = CurrentLocation + Direction * platformV.Size() * SpeedMultiplier * DeltaTime;
        SetActorLocation(NewLocation);

        FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
        SetActorRotation(NewRotation);

        if (bIsHoldingLight)
        {
            // ���� ���Ͱ� �ı��Ǿ����� Ȯ��
            if (!IsValid(HeldLightActor) || HeldLightActor == nullptr)
            {
                bIsHoldingLight = false;
                SetActorHiddenInGame(false);  // ����Ʈ�� �ı��Ǹ� �÷����� �ٽ� ���̵��� ����
            }
            else
            {
                UpdateVisibilityBasedOnLight();
            }
        }
        else
        {
            // ����Ʈ�� ��� �ִ��� ���θ� Ȯ��
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

void AMovingPlatform::UpdateVisibilityBasedOnLight()
{
    if (HeldLightActor)
    {
        UPointLightComponent* PointLight = HeldLightActor->FindComponentByClass<UPointLightComponent>();
        if (PointLight)
        {
            FVector PlatformLocation = GetActorLocation();
            FVector LightLocation = PointLight->GetComponentLocation();

            float Distance = FVector::Dist(PlatformLocation, LightLocation);
            float LightRadius = PointLight->AttenuationRadius;

            if (Distance <= LightRadius)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, FString::Printf(TEXT("Within light radius. Hiding actor.")));
                SetActorHiddenInGame(true);  // ���� �ݰ� ���� ������ ���͸� ����
            }
            else
            {
                SetActorHiddenInGame(false);  // ���� �ݰ� �ۿ� ������ ���͸� ���̵��� ����
            }
        }
    }
}

void AMovingPlatform::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Overlap Detected!"));

        // BP_FirstPersonProjectile�� �浹���� ���
        if (OtherActor->GetClass()->GetName().Contains(TEXT("arrow03")))
        {
            if (ExplosionEffect)
            {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
            }

            if (ExplosionSound)
            {
                UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
            }

            Destroy();  // ���� �ı�
        }
        // ĳ���Ϳ� �浹���� ���
        else if (OtherActor->IsA(ACharacter::StaticClass()))
        {
            if (ExplosionEffect)
            {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
            }

            if (ExplosionSound)
            {
                UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
            }

            Destroy();  // ���� �ı�

            UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetPause(true);
            UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
        }
    }
}
