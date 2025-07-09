// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "TreasureGlowEffect.generated.h"

UCLASS()
class PROJECT03_API ATreasureGlowEffect : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATreasureGlowEffect();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere)
    UNiagaraComponent* NiagaraComp;

    UPROPERTY()
    TWeakObjectPtr<AActor> TreasureChest;

    // 궤도 설정
    float OrbitRadius = 100.0f;
    float OrbitSpeed = 180.0f; // 초당 회전 각도
    float ElapsedTime = 0.0f;

    // 사선 궤도 설정
    float TiltAngle = 45.0f; // 기울기 각도
    float VerticalAmplitude = 45.0f; // 위아래 움직임 폭

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float BaseZOffset = 50.0f;

public:	
	// Called every frame
	
    void InitEffect(AActor* Chest, UNiagaraSystem* Effect, float InBaseZOffset);

};
