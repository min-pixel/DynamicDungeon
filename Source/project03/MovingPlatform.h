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

    // UBoxComponent ��� USphereComponent ���
    UPROPERTY(VisibleAnywhere)
    class USphereComponent* CollisionSphere;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShapesMesh; // ť�� ����ƽ �޽� ������Ʈ �߰�

    UPROPERTY(EditAnywhere, Category = "Movement")
    float SpeedMultiplier; // �ӵ� ��� ���� �߰�

    UPROPERTY(EditAnywhere, Category = "Lighting")
    UMaterialInterface* GlowMaterial = nullptr;  // �߱� ��Ƽ����

    UPROPERTY(EditAnywhere, Category = "Lighting")
    UMaterialInterface* InvisibleMaterial = nullptr;  // �񰡽� ��Ƽ����

    FVector StartLocation;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    void UpdateVisibilityBasedOnLight();  // ����Ʈ ����Ʈ�� ����� ���ü� ������Ʈ �Լ�

    UMaterialInstanceDynamic* DynamicMaterial = nullptr;  // ���� ��Ƽ���� �ν��Ͻ�

    // �߰��� �κ�
    AActor* HeldLightActor; // �÷��̾ ��� �ִ� ����Ʈ ���͸� ����
    bool bIsHoldingLight;   // �÷��̾ ����Ʈ�� ��� �ִ��� ���θ� ��Ÿ���� �÷���
};
