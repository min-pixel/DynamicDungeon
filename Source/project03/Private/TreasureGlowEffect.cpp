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

    // ȸ�� ���� ���
    float AngleDeg = ElapsedTime * OrbitSpeed;
    float AngleRad = FMath::DegreesToRadians(AngleDeg);

    // Ÿ���� �˵� (�缱 ȿ���� ����)
    float XRadius = OrbitRadius;
    float YRadius = OrbitRadius * 0.7f; // �ణ ��׷��� Ÿ��

    // �⺻ Ÿ�� ��ǥ
    float X = FMath::Cos(AngleRad) * XRadius;
    float Y = FMath::Sin(AngleRad) * YRadius;

    // �缱���� ����̱� (45�� ȸ��)
    float TiltRad = FMath::DegreesToRadians(TiltAngle);
    float RotatedX = X * FMath::Cos(TiltRad) - Y * FMath::Sin(TiltRad);
    float RotatedY = X * FMath::Sin(TiltRad) + Y * FMath::Cos(TiltRad);

    // ���Ʒ� ������ �߰� (�� ���̳����ϰ�)
    float ZOffset = FMath::Sin(AngleRad * 2.0f) * VerticalAmplitude;

    // �������� ��ġ �������� �̵�
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

    // ����� ����
    FLinearColor YellowColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // �����
    NiagaraComp->SetVariableLinearColor(FName("Color"), YellowColor);

    // ũ�� ����
    NiagaraComp->SetVariableFloat(FName("Sprites_Size"), 5.0f);

    SetLifeSpan(5.0f);
}
