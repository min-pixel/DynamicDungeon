#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingPlatform.generated.h"

UCLASS()
class PROJECT03_API AMovingPlatform : public AActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AMovingPlatform();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:
    // Called every frame
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Moving Platform")
    FVector platformV = FVector(0, 0, 100);
    UPROPERTY(EditAnywhere, Category = "Moving Platform")
    float MovedDistance = 100;
    UPROPERTY(EditAnywhere, Category = "Rotation")
    FRotator RotationSpeed = FRotator(0, 100, 0);

    UPROPERTY(EditAnywhere, Category = "Effects")
    UParticleSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, Category = "Sound")
    USoundBase* ExplosionSound;

    // UBoxComponent 대신 USphereComponent 사용
    UPROPERTY(VisibleAnywhere)
    class USphereComponent* CollisionSphere;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShapesMesh; // 큐브 스태틱 메쉬 컴포넌트 추가

    UPROPERTY(EditAnywhere, Category = "Movement")
    float SpeedMultiplier; // 속도 배수 변수 추가

    UPROPERTY(EditAnywhere, Category = "Lighting")
    UMaterialInterface* GlowMaterial = nullptr;  // 발광 머티리얼

    UPROPERTY(EditAnywhere, Category = "Lighting")
    UMaterialInterface* InvisibleMaterial = nullptr;  // 비가시 머티리얼

    FVector StartLocation;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    void UpdateVisibilityBasedOnLight();  // 포인트 라이트에 기반한 가시성 업데이트 함수

    UMaterialInstanceDynamic* DynamicMaterial = nullptr;  // 동적 머티리얼 인스턴스

    // 추가된 부분
    AActor* HeldLightActor; // 플레이어가 들고 있는 라이트 액터를 참조
    bool bIsHoldingLight;   // 플레이어가 라이트를 들고 있는지 여부를 나타내는 플래그
};
