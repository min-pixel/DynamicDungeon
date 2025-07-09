// Fill out your copyright notice in the Description page of Project Settings.


#include "TreasureGlowEffect.h"
#include "NiagaraFunctionLibrary.h"

// Sets default values
ATreasureGlowEffect::ATreasureGlowEffect()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	RootComponent = NiagaraComp;
}

// Called when the game starts or when spawned
void ATreasureGlowEffect::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATreasureGlowEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!TreasureChest.IsValid())
    {
        Destroy();
        return;
    }

    ElapsedTime += DeltaTime;

    // 회전 각도 계산
    float AngleDeg = ElapsedTime * OrbitSpeed;
    float AngleRad = FMath::DegreesToRadians(AngleDeg);

    // 타원형 궤도 (사선 효과를 위해)
    float XRadius = OrbitRadius;
    float YRadius = OrbitRadius * 0.7f; // 약간 찌그러진 타원

    // 기본 타원 좌표
    float X = FMath::Cos(AngleRad) * XRadius;
    float Y = FMath::Sin(AngleRad) * YRadius;

    // 사선으로 기울이기 (45도 회전)
    float TiltRad = FMath::DegreesToRadians(TiltAngle);
    float RotatedX = X * FMath::Cos(TiltRad) - Y * FMath::Sin(TiltRad);
    float RotatedY = X * FMath::Sin(TiltRad) + Y * FMath::Cos(TiltRad);

    // 위아래 움직임 추가 (더 다이나믹하게)
    float ZOffset = FMath::Sin(AngleRad * 2.0f) * VerticalAmplitude;

    // 보물상자 위치 기준으로 이동
    FVector ChestLocation = TreasureChest->GetActorLocation();
    FVector NewLocation = ChestLocation + FVector(RotatedX, RotatedY, ZOffset + BaseZOffset); 

    SetActorLocation(NewLocation);

}

void ATreasureGlowEffect::InitEffect(AActor* Chest, UNiagaraSystem* Effect, float InBaseZOffset)
{
    if (!Chest || !Effect) return;

    TreasureChest = Chest;

    NiagaraComp->SetAsset(Effect);
    NiagaraComp->Activate();

    BaseZOffset = InBaseZOffset;

    // 노란색 설정
    FLinearColor YellowColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
    NiagaraComp->SetVariableLinearColor(FName("Color"), YellowColor);

    // 크기 설정
    NiagaraComp->SetVariableFloat(FName("Sprites_Size"), 5.0f);

    SetLifeSpan(5.0f);
}
