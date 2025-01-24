// Fill out your copyright notice in the Description page of Project Settings.


#include "MovingPlatform02.h"
#include "Components/BoxComponent.h"
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
#include "EngineUtils.h"  // TActorIterator�� ����ϱ� ���� �߰�
#include "waterplatform.h" // Awaterplatform Ŭ������ �����ϱ� ���� �߰�


// Sets default values
AMovingPlatform02::AMovingPlatform02()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    ShapesMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShapesMesh"));
    RootComponent = ShapesMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShapesVisualAsset(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
    if (ShapesVisualAsset.Succeeded())
    {
        ShapesMesh->SetStaticMesh(ShapesVisualAsset.Object);
    }

    // �߱� ��Ƽ���� �Ҵ�
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterialAsset(TEXT("/Game/StarterContent/Materials/Bat.Bat"));
    if (GlowMaterialAsset.Succeeded())
    {
        GlowMaterial = GlowMaterialAsset.Object;
    }

    // ���� ��Ƽ���� ����
    DynamicMaterial = UMaterialInstanceDynamic::Create(GlowMaterial, this);
    ShapesMesh->SetMaterial(0, DynamicMaterial);

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetCollisionProfileName("BlockAllDynamic");
    CollisionBox->SetupAttachment(ShapesMesh);

    FVector BoxExtent = ShapesMesh->GetStaticMesh()->GetBoundingBox().GetExtent();
    CollisionBox->SetBoxExtent(BoxExtent * 1.0f);

    CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, BoxExtent.Z));

    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    ShapesMesh->SetSimulatePhysics(true);
    ShapesMesh->SetEnableGravity(false);
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

    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AMovingPlatform02::OnOverlapBegin);

    SpeedMultiplier = 3.0f;

    HeldLightActor = nullptr;
    bIsHoldingLight = false;

}

// Called when the game starts or when spawned
void AMovingPlatform02::BeginPlay()
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

// Called every frame
void AMovingPlatform02::Tick(float DeltaTime)
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

void AMovingPlatform02::UpdateVisibilityBasedOnLight()
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
          
                SetActorHiddenInGame(true);  // ���� �ݰ� ���� ������ ���͸� ����
            }
            else
            {
                SetActorHiddenInGame(false);  // ���� �ݰ� �ۿ� ������ ���͸� ���̵��� ����
            }
        }
    }
}

void AMovingPlatform02::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Overlap Detected!"));

        // BP_FirstPersonProjectile �Ǵ� arrow03�� �浹���� ���
        if (OtherActor->GetClass()->GetName().Contains(TEXT("BP_FirstPersonProjectile")))
        {
            if (ExplosionEffect)
            {
                UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
            }

            if (ExplosionSound)
            {
                UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
            }

            Destroy();  // arrow03�� �浹 �� AMovingPlatform02�� �ı�
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

            // �÷��̾�� �浹�� ������ Awaterplatform�� ������ ���
            for (TActorIterator<Awaterplatform> WaterPlatformItr(GetWorld()); WaterPlatformItr; ++WaterPlatformItr)
            {
                Awaterplatform* WaterPlatform = *WaterPlatformItr;
                if (WaterPlatform)
                {
                    WaterPlatform->StartRising(50.0f, 5.0f);  // �浹�� ������ 50 ������ 5�� ���� ���
                }
            }

            Destroy();  // �÷��̾�� �浹 �� AMovingPlatform02�� ���� �Ǵ� �ٸ� ó���� �� �� ����
        }
    }
}



