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
#include "EngineUtils.h"  // TActorIterator를 사용하기 위해 추가
#include "waterplatform.h" // Awaterplatform 클래스를 참조하기 위해 추가


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

    // 발광 머티리얼 할당
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterialAsset(TEXT("/Game/StarterContent/Materials/Bat.Bat"));
    if (GlowMaterialAsset.Succeeded())
    {
        GlowMaterial = GlowMaterialAsset.Object;
    }

    // 동적 머티리얼 생성
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
    // 머티리얼이 제대로 생성되고 적용되었는지 확인
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
            // 조명 액터가 파괴되었는지 확인
            if (!IsValid(HeldLightActor) || HeldLightActor == nullptr)
            {
                bIsHoldingLight = false;
                SetActorHiddenInGame(false);  // 라이트가 파괴되면 플랫폼을 다시 보이도록 설정
            }
            else
            {
                UpdateVisibilityBasedOnLight();
            }
        }
        else
        {
            // 라이트를 들고 있는지 여부를 확인
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
          
                SetActorHiddenInGame(true);  // 조명 반경 내에 들어오면 액터를 숨김
            }
            else
            {
                SetActorHiddenInGame(false);  // 조명 반경 밖에 있으면 액터를 보이도록 설정
            }
        }
    }
}

void AMovingPlatform02::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Overlap Detected!"));

        // BP_FirstPersonProjectile 또는 arrow03과 충돌했을 경우
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

            Destroy();  // arrow03과 충돌 시 AMovingPlatform02를 파괴
        }
        // 캐릭터와 충돌했을 경우
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

            // 플레이어와 충돌할 때마다 Awaterplatform이 서서히 상승
            for (TActorIterator<Awaterplatform> WaterPlatformItr(GetWorld()); WaterPlatformItr; ++WaterPlatformItr)
            {
                Awaterplatform* WaterPlatform = *WaterPlatformItr;
                if (WaterPlatform)
                {
                    WaterPlatform->StartRising(50.0f, 5.0f);  // 충돌할 때마다 50 유닛을 5초 동안 상승
                }
            }

            Destroy();  // 플레이어와 충돌 후 AMovingPlatform02를 제거 또는 다른 처리를 할 수 있음
        }
    }
}



