// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyWidget.h"
#include "MyDCharacter.h" 
#include "WaitForPlayersWidget.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AMyPlayerController();

    virtual void BeginPlay() override;

    UFUNCTION(Server, Reliable)
    void ServerRequestStart();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<ULobbyWidget> LobbyWidgetClass;

    UPROPERTY()
    ULobbyWidget* LobbyWidgetInstance;

    UPROPERTY(Replicated)
    bool bIsReady;

    // 각 플레이어의 개별 스탯 정보 저장
    UPROPERTY(Replicated)
    float PlayerMaxHealth = 100.0f;

    UPROPERTY(Replicated)
    float PlayerMaxStamina = 100.0f;

    UPROPERTY(Replicated)
    float PlayerMaxKnowledge = 100.0f;

    UPROPERTY(Replicated)
    int32 PlayerGold = 1000;

   

    UPROPERTY(BlueprintReadWrite)
    UWaitForPlayersWidget* WaitWidgetInstance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UI)
    TSubclassOf<UWaitForPlayersWidget> WaitWidgetClass;


    UFUNCTION(Client, Reliable)
    void ClientUpdateWaitWidget(int32 ReadyCount, int32 TotalCount);

    // Seamless Travel 관련 함수들 - 올바른 매개변수 타입
    virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;
    virtual void SeamlessTravelTo(APlayerController* NewPC) override;
    virtual void SeamlessTravelFrom(APlayerController* OldPC) override;

    // 개별 플레이어 이동을 위한 함수 추가
    UFUNCTION(Server, Reliable)
    void ServerRequestIndividualTravel(const FString& LevelName);

    FTimerHandle TimerHandle_ShowLobby;

    UFUNCTION(Client, Reliable)
    void ClientReturnToLobby();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 게임 종료 관련
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ESC 키 처리
    UFUNCTION()
    void OnEscapePressed();

protected:
    virtual void SetupInputComponent() override;
	
};
