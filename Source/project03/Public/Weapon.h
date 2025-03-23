// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS()
class PROJECT03_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    // 기본 생성자
    AWeapon();

protected:
    // 게임 시작 시 호출
    virtual void BeginPlay() override;

public:
    // 매 프레임 호출
    virtual void Tick(float DeltaTime) override;

    // 무기와 상호작용할 콜리전
    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UStaticMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UBoxComponent* CollisionBox;

    // 무기의 무게(기본값: 100.0)
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float Weight = 100.0f;

    // 플레이어와 오버랩 감지 함수
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    // 오버랩 종료 함수 (선택 사항)
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float Damage = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float TraceRadius = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName AttackStartSocket = "AttackStart";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName AttackEndSocket = "AttackEnd";

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void TraceAttack();

    void StartTrace();

    FVector LastStartLocation;
    FVector LastEndLocation;
};
