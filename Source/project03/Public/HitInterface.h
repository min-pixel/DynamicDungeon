// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HitInterface.generated.h"




UENUM(BlueprintType)
enum class EDebuffType : uint8
{
	Slow UMETA(DisplayName = "Slow")
};


// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UHitInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECT03_API IHitInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void GetHit(const FHitResult& HitResult, AActor* InstigatorActor, float Damage);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ApplyDebuff(EDebuffType DebuffType, float Magnitude, float Duration);

};
