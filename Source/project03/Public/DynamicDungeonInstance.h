// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "DynamicDungeonInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UDynamicDungeonInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    // 예제 변수 (블루프린트에서 사용할 변수)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game State")
    bool bCanInteract;

    // 생성자
    UDynamicDungeonInstance();

    // 블루프린트에서도 접근 가능하도록 함수 추가
    UFUNCTION(BlueprintCallable, Category = "Game State")
    void SetCanInteract(bool NewState);

    //플레이어가 상호작용 중인지 나타내는 변수
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool bIsInteracting = false;

    /** 블루프린트에서 사용할 상호작용 변수 */
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool itemEAt; // 아이템 상호작용 여부

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool OpenDoor; // 문 열기 여부

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool WeaponEAt; // 무기 상호작용 여부
	
};
