// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInterface.h"
#include "NewActorComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT03_API UNewActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UNewActorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Properties
	UPROPERTY(EditAnywhere, Category = "Moving Platform")
	FVector PlatformVelocity = FVector(0, 0, 100);

	UPROPERTY(EditAnywhere, Category = "Moving Platform")
	float MovedDistance = 100;

	UPROPERTY(EditAnywhere, Category = "Rotation")
	FRotator RotationSpeed = FRotator(0, 100, 0);

	UPROPERTY(EditAnywhere, Category = "Effects")
	UParticleSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float SpeedMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	UMaterialInterface* GlowMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Lighting")
	UMaterialInterface* InvisibleMaterial = nullptr;

	// Methods
	void UpdateVisibilityBasedOnLight();

private:
	FVector StartLocation;
	AActor* HeldLightActor;
	bool bIsHoldingLight;

	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);
};