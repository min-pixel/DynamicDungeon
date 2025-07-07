// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyWidget.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	

	UFUNCTION(BlueprintCallable)
	void CheckAllPlayersReady();

	void BroadcastReadyStatus();

	void StartTravel();

	int32 GetReadyCount() const;
	int32 GetNotReadyCount() const;

	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	class ULobbyWidget* LobbyWidgetInstance;

	FTimerHandle DelayHandle; 

};
