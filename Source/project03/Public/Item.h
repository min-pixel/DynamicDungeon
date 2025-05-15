// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "MyDCharacter.h"
#include "ItemDataD.h"
#include "Item.generated.h"


UCLASS()
class PROJECT03_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();

	virtual void SetDefaultIcon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* ItemIcon;

	virtual FItemData ToItemData() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType;

	/*UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void Use(AMyDCharacter* Character);

	virtual void Use_Implementation(AMyDCharacter* Character);*/

	//virtual FItemData ToItemData() const;

};
