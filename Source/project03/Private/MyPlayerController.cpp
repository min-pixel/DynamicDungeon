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

    static ConstructorHelpers::FClassFinder<UWaitForPlayersWidget> WaitBPClass(TEXT("/Game/BP/UI/WaitForPlayersWidget_BP.WaitForPlayersWidget_BP_C"));
    if (WaitBPClass.Succeeded())
    {
        WaitWidgetClass = WaitBPClass.Class;
    }

}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();



    if (!IsLocalController())
    {
        return; // 로컬 컨트롤러가 아니면 UI 생성하지 않음
    }
    
        LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C")); 
    

        FString MapName = GetWorld()->GetMapName();
        MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

        if (!MapName.Contains("Lobby"))
        {
            // 로비 맵이 아니면 UI 생성 안 함
            return;
        }

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
        // GameMode에 알림
        Cast<ALobbyGameMode>(GM)->CheckAllPlayersReady();
    }
}

void AMyPlayerController::ClientUpdateWaitWidget_Implementation(int32 ReadyCount, int32 TotalCount)
{
    if (!WaitWidgetInstance && WaitWidgetClass)
    {
        WaitWidgetInstance = CreateWidget<UWaitForPlayersWidget>(this, WaitWidgetClass);
        if (WaitWidgetInstance)
        {
            WaitWidgetInstance->AddToViewport(1);
        }
    }

    if (WaitWidgetInstance)
    {
        WaitWidgetInstance->UpdateStatus(ReadyCount, TotalCount);
    }
}


void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMyPlayerController, bIsReady);
}