#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "waterplatform.generated.h"

UCLASS()
class PROJECT03_API Awaterplatform : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	Awaterplatform();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// �÷����� ������ �ö󰡵��� ����
	void StartRising(float TargetHeight, float Duration);

	// �߰��� �κ�: ť�� ����ƽ �޽� �� ��Ƽ����
	UPROPERTY(VisibleAnywhere, Category = "Platform")
	UStaticMeshComponent* PlaneMesh;

	UPROPERTY(EditAnywhere, Category = "Material")
	UMaterialInterface* SimpleWaterMaterial;

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	// AMovingPlatform02���� �浹 �� ó��
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// �÷����� �浹�� ������ �ö� �Ÿ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveAmount = 20.0f;  // ������ ������ �ʱ�ȭ

private:
	FVector InitialLocation;  // �÷����� �ʱ� ��ġ
	bool bIsRising;  // ���� ��ü�� ��� ������ ����
	float TargetZ;    // ��ǥ ����
	float RiseSpeed;  // �ʴ� �̵��� ���� (1�ʴ� �̵��� �Ÿ�)
	FVector StartLocation;  // ���� ��ġ
};
