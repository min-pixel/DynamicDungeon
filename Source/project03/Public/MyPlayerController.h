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

    // �� �÷��̾��� ���� ���� ���� ����
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

    // Seamless Travel ���� �Լ��� - �ùٸ� �Ű����� Ÿ��
    virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;
    virtual void SeamlessTravelTo(APlayerController* NewPC) override;
    virtual void SeamlessTravelFrom(APlayerController* OldPC) override;

    // ���� �÷��̾� �̵��� ���� �Լ� �߰�
    UFUNCTION(Server, Reliable)
    void ServerRequestIndividualTravel(const FString& LevelName);

    FTimerHandle TimerHandle_ShowLobby;

    UFUNCTION(Client, Reliable)
    void ClientReturnToLobby();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ���� ���� ����
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ESC Ű ó��
    UFUNCTION()
    void OnEscapePressed();

protected:
    virtual void SetupInputComponent() override;
	
};
