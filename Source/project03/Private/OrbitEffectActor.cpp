// Fill out your copyright notice in the Description page of Project Settings.


#include "OrbitEffectActor.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

// Sets default values
AOrbitEffectActor::AOrbitEffectActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	RootComponent = NiagaraComp;

    bReplicates = true;
    SetReplicateMovement(false); 

}

// Called when the game starts or when spawned
void AOrbitEffectActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOrbitEffectActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!Center.IsValid()) return;

    Elapsed += DeltaTime;
    if (Elapsed >= LifeTime)
    {
        Destroy();
        return;
    }

    float AngleDeg = Elapsed * OrbitSpeed;
    float AngleRad = FMath::DegreesToRadians(AngleDeg);

    FVector Offset = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f) * OrbitRadius;
    SetActorLocation(Center->GetActorLocation() + Offset);
}

void AOrbitEffectActor::InitOrbit(AActor* CenterActor, UNiagaraSystem* Effect, float Radius, float Duration, float Speed, const FLinearColor& Color, float SpriteSize)
{
    if (!CenterActor || !Effect) return;

    Center = CenterActor;
    OrbitRadius = Radius;
    LifeTime = Duration;
    OrbitSpeed = Speed;


    NiagaraComp->SetAsset(Effect);

    NiagaraComp->Activate();

    NiagaraComp->SetVariableLinearColor(FName("Color"), Color);

    NiagaraComp->SetVariableFloat(FName("Sprites_Size"), SpriteSize);
}

