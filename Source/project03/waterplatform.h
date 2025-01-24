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

	// 플랫폼이 서서히 올라가도록 설정
	void StartRising(float TargetHeight, float Duration);

	// 추가된 부분: 큐브 스태틱 메쉬 및 머티리얼
	UPROPERTY(VisibleAnywhere, Category = "Platform")
	UStaticMeshComponent* PlaneMesh;

	UPROPERTY(EditAnywhere, Category = "Material")
	UMaterialInterface* SimpleWaterMaterial;

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* CollisionBox;

	// AMovingPlatform02와의 충돌 시 처리
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 플랫폼이 충돌할 때마다 올라갈 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveAmount = 20.0f;  // 적당한 값으로 초기화

private:
	FVector InitialLocation;  // 플랫폼의 초기 위치
	bool bIsRising;  // 현재 물체가 상승 중인지 여부
	float TargetZ;    // 목표 높이
	float RiseSpeed;  // 초당 이동할 높이 (1초당 이동할 거리)
	FVector StartLocation;  // 시작 위치
};
