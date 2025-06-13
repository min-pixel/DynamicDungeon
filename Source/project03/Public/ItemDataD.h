// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDataD.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
    Weapon,
    Armor,
    Potion,
    Consumable
};

UENUM(BlueprintType)
enum class EPotionEffectType : uint8
{
    None		UMETA(DisplayName = "None"),
    Health		UMETA(DisplayName = "Health"),
    Mana		UMETA(DisplayName = "Mana"),
    Stamina		UMETA(DisplayName = "Stamina")
};



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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EItemType ItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPotionEffectType PotionEffect = EPotionEffectType::None;

    // 등급 (방어구 또는 무기일 경우)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8 Grade = 0; // 0=C, 1=B, 2=A

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SkillIndex = -1;  // -1이면 유효하지 않음

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Price = 0;


    FItemData()
        : ItemName(TEXT("")), ItemIcon(nullptr), ItemClass(nullptr),
        bIsStackable(false), MaxStack(1), Count(1), PotionEffect(EPotionEffectType::None)
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
