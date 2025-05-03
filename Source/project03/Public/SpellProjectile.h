// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellProjectile.generated.h"

UCLASS()
class PROJECT03_API ASpellProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpellProjectile();
	void Init(class AMyDCharacter* InCaster, float InDamage);

    UFUNCTION(BlueprintCallable)
    void LaunchInDirection(const FVector& LaunchVelocity);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    /*UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);*/

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere)
    class USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    class UProjectileMovementComponent* MovementComponent; 

    UPROPERTY(EditDefaultsOnly)
    class UParticleSystemComponent* VisualEffect; 

    UPROPERTY()
    AMyDCharacter* Caster;

    float Damage;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
