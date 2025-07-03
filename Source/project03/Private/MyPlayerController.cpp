// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "LobbyWidget.h"
#include "LobbyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"




AMyPlayerController::AMyPlayerController()
{
    bAutoManageActiveCameraTarget = false;

    static ConstructorHelpers::FClassFinder<ULobbyWidget> WidgetBPClass(TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C"));
    if (WidgetBPClass.Succeeded())
    {
        LobbyWidgetClass = WidgetBPClass.Class;
    }

}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();




    
        LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C")); 
    

    if (LobbyWidgetClass)
    {
        LobbyWidgetInstance = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
        if (LobbyWidgetInstance)
        {
            LobbyWidgetInstance->AddToViewport(1);

            AMyDCharacter* PlayerCharacter = Cast<AMyDCharacter>(GetPawn());
            LobbyWidgetInstance->InitializeLobby(PlayerCharacter);

            bShowMouseCursor = true;

            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);

            UE_LOG(LogTemp, Warning, TEXT("Lobby Widget successfully created on LOCAL CLIENT!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create Lobby Widget instance."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyWidgetClass is null."));
    }
}


void AMyPlayerController::ServerRequestStart_Implementation()
{
    bIsReady = true;

    AGameModeBase* GM = UGameplayStatics::GetGameMode(GetWorld());
    if (GM)
    {
        // GameMode¿¡ ¾Ë¸²
        Cast<ALobbyGameMode>(GM)->CheckAllPlayersReady();
    }
}

void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyPlayerController, bIsReady);
}