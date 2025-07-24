// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveFunctionCollapseModel02.h" 
#include "WFCRegenerator.generated.h"



UCLASS()
class PROJECT03_API AWFCRegenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWFCRegenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "WFC")
	int32 TryCount = 90;

	UPROPERTY(EditAnywhere, Category = "WFC")
	int32 RandomSeed = 0;

	UPROPERTY(EditAnywhere, Category = "WFC")
	bool bGenerateOnOverlap = true;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* TriggerVolume;

	// 妮府傈 贸府
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	// WFC 角青
	UFUNCTION(BlueprintCallable, Category = "WFC")
	void GenerateWFCAtLocation();

	void ClearPreviousWFCActors();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRestoreGravity();

	void UpdateTileNetworkPriorities();

	FIntVector FixedTileCoord;
	FWaveFunctionCollapseOptionCustom FixedTileOption;
	bool bHasFixedTile = false;

};
