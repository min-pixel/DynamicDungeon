// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"   
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundCue.h"
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

   /* UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);*/

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    UNiagaraSystem* FireballEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    UNiagaraSystem* ExplosionEffect;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* LaunchSound;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    USoundBase* ExplosionSound;

    UPROPERTY()
    UAudioComponent* FireLoopSound;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere)
    UBoxComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere)
    class UProjectileMovementComponent* MovementComponent;

    UPROPERTY(EditDefaultsOnly)
    class UParticleSystemComponent* VisualEffect;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* VisualMesh;

   

    

    // ��Ʈ��ũ ���� ����
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ������ ������
    UPROPERTY(Replicated)
    AMyDCharacter* Caster;

    UPROPERTY(Replicated)
    float Damage;

    UPROPERTY(ReplicatedUsing = OnRep_ProjectileData)
    FVector ReplicatedVelocity;

    UFUNCTION()
    void OnRep_ProjectileData();

    // ���������� ����Ǵ� �浹 ó��
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerOnHit(AActor* HitActor, const FHitResult& HitResult);

    // ��� Ŭ���̾�Ʈ���� ���� ����Ʈ
    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayExplosionEffect(FVector Location);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    

  


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
