// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveFunctionCollapseSubsystem02.h"
#include "wfcex.generated.h"

UCLASS()
class PROJECT03_API Awfcex : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	Awfcex();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ���� �޼���
	UFUNCTION(BlueprintCallable, Category = "WFC")
	void ExecuteWFCInSubsystem(int32 TryCount, int32 RandomSeed);

	// ���� Ÿ�� ���� �÷��̾� ����
	void SpawnPlayerOnCorridor();

	// ������ ��ġ�� �÷��̾� ����
	void SpawnPlayerAtLocation(const FVector& Location);

	// WFC ����ý��ۿ� ����
	UWaveFunctionCollapseSubsystem02* GetWFCSubsystem();

	// �÷��̾� Ŭ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> PlayerClass;

	void UpdatePlayerStartLocation(const FVector& NewLocation);

};
