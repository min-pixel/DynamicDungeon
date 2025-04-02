// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDataD.generated.h"



USTRUCT(BlueprintType)
struct FItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* ItemIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<class AItem> ItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsStackable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxStack = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 1;

    FItemData()
        : ItemName(TEXT("")), ItemIcon(nullptr), ItemClass(nullptr),
        bIsStackable(false), MaxStack(1), Count(1)
    {}
};




UCLASS()
class PROJECT03_API AItemDataD : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemDataD();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
