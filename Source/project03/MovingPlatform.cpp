#include "MovingPlatform.h"
#include "Components/SphereComponent.h" // 구체 콜리전 사용을 위해 추가
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

    // 발광 머티리얼 할당
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterialAsset(TEXT("/Game/StarterContent/Materials/M_Tech_Hex_Tile_Pulse.M_Tech_Hex_Tile_Pulse"));
    if (GlowMaterialAsset.Succeeded())
    {
        GlowMaterial = GlowMaterialAsset.Object;
    }

    // 동적 머티리얼 생성
    DynamicMaterial = UMaterialInstanceDynamic::Create(GlowMaterial, this);
    ShapesMesh->SetMaterial(0, DynamicMaterial);

    // 콜리전 박스를 구체 콜리전으로 변경
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetCollisionProfileName("BlockAllDynamic");
    CollisionSphere->SetupAttachment(ShapesMesh);

    // 구체 크기를 스태틱 메쉬의 경계에서 구형 반경으로 설정
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
                SetActorHiddenInGame(true);  // 조명 반경 내에 들어오면 액터를 숨김
            }
            else
            {
                SetActorHiddenInGame(false);  // 조명 반경 밖에 있으면 액터를 보이도록 설정
            }
        }
    }
}

void AMovingPlatform::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherComp)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Overlap Detected!"));

        // BP_FirstPersonProjectile과 충돌했을 경우
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

            Destroy();  // 액터 파괴
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

            Destroy();  // 액터 파괴

            UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetPause(true);
            UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
        }
    }
}
