// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "OrbitEffectActor.generated.h"

UCLASS()
class PROJECT03_API AOrbitEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOrbitEffectActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    void InitOrbit(AActor* CenterActor, UNiagaraSystem* Effect, float Radius, float Duration, float Speed, const FLinearColor& Color, float SpriteSize);

    UPROPERTY()
    UNiagaraComponent* NiagaraComp;

    TWeakObjectPtr<AActor> Center;

    float OrbitRadius = 100.f;
    float OrbitSpeed = 180.f;
    float LifeTime = 5.0f;
    float Elapsed = 0.0f;

    
};
