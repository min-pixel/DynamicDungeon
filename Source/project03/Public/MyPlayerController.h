// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyWidget.h"
#include "MyDCharacter.h" 
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

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
};
