// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ItemDataD.h"
#include "PlayerCharacterData.h"
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

    // GameInstance 초기화 시 실행
    virtual void Init() override;

    // SocketManager의 OnDataReceived를 받을 함수
    UFUNCTION()
    void OnSocketDataReceived(const TArray<uint8>& Data);

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
	
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedInventoryItems;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedEquipmentItems;  

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedStorageItems;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 LobbyGold = 1000;

    FPlayerCharacterData CurrentCharacterData;

    void InitializeCharacterData(EPlayerClass SelectedClass);

    UFUNCTION(BlueprintCallable, Category = "Gold")
    int32 GetLobbyGold() const { return LobbyGold; }

    UPROPERTY()
    bool bIsReturningFromGame = false;

    UPROPERTY()
    bool bHasValidCharacterData = false;

    // 게임 종료 시 데이터 저장
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveAllDataToServer();

    // JSON 형태로 모든 게임 데이터 수집
    FString CollectAllGameData();

    UPROPERTY()
    class ULobbyWidget* LobbyWidgetInstance;


    void OnPreLoadMap(const FString& MapName);
    void OnPostLoadMap(UWorld* LoadedWorld);
   
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveDataAndShutdown();

    // 저장 상태 추적
    UPROPERTY()
    bool bIsSaving = false;

    UPROPERTY()
    bool bSaveCompleted = false;

    // 저장 완료 콜백
    UFUNCTION()
    void OnSaveDataCompleted(bool bSuccess);

    // 안전한 종료 처리
    void SafeShutdown();

    void WaitForSaveCompletion();

    bool bShutdownRequested = false;

    // 직업별 기본 스탯을 반환하는 함수
    UFUNCTION(BlueprintCallable)
    void GetBaseStatsForClass(EPlayerClass PlayerClass, float& OutHealth, float& OutStamina, float& OutKnowledge);

    // 현재 직업 기본 스탯 + 보너스 스탯으로 최종 스탯 계산
    UFUNCTION(BlueprintCallable)
    void RecalculateStats();

    bool bPlydie = false;

private:
    // 저장 대기 타이머
    FTimerHandle SaveWaitTimer;

    FTimerHandle CheckSaveTimer;

    // 최대 저장 대기 시간 (초)
    float MaxSaveWaitTime = 5.0f;

};
