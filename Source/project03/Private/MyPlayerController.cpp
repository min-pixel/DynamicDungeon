// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "LobbyWidget.h"
#include "LobbyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DynamicDungeonInstance.h"
#include "UCharacterHUDWidget.h"

AMyPlayerController::AMyPlayerController()
{
    bAutoManageActiveCameraTarget = true;

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
        return; // ���� ��Ʈ�ѷ��� �ƴϸ� UI �������� ����
    }

    LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C"));

    FString MapName = GetWorld()->GetMapName();
    MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    if (!MapName.Contains("Lobby"))
    {
        // �κ� ���� �ƴϸ� UI ���� �� ��
        return;
    }

    if (LobbyWidgetClass)
    {
        LobbyWidgetInstance = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
        if (LobbyWidgetInstance)
        {
            LobbyWidgetInstance->AddToViewport(1);

            // ����: InitializeLobby() ȣ������ ����!
            // ���� �Ŀ� LobbyWidget���� �˾Ƽ� �ʱ�ȭ�� ����

            bShowMouseCursor = true;

            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);

            UE_LOG(LogTemp, Warning, TEXT("Lobby Widget successfully created on LOCAL CLIENT! (No early initialization)"));
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
        // GameMode�� �˸�
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

void AMyPlayerController::ClientReturnToLobby_Implementation()
{
    // 1) Ŭ���̾�Ʈ �ڽ��� GameInstance �÷��� ����
    if (UDynamicDungeonInstance* GI = Cast<UDynamicDungeonInstance>(GetGameInstance()))
    {
        GI->bIsReturningFromGame = true;
        
            GI->bHasValidCharacterData = true;
    }

    // 2) ���� �κ� ������ �̵�
    //ClientTravel(TEXT("LobbyMap"), ETravelType::TRAVEL_Absolute);

}

// Seamless Travel ����
void AMyPlayerController::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToTransition, ActorList);

    // �÷��̾�� �Բ� �̵��ؾ� �� ���Ͱ� �ִٸ� ���⿡ �߰�
    if (GetPawn())
    {
        ActorList.Add(GetPawn());
    }

    UE_LOG(LogTemp, Log, TEXT("GetSeamlessTravelActorList - Actor count: %d"), ActorList.Num());
}

void AMyPlayerController::SeamlessTravelTo(APlayerController* NewPC)
{
    Super::SeamlessTravelTo(NewPC);

    UE_LOG(LogTemp, Log, TEXT("SeamlessTravelTo - Arrived at new level"));

    // �κ�� ���ƿ��� �� ó��
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && GameInstance->bIsReturningFromGame)
    {
        if (IsLocalController())
        {
            // �κ� ���� ���� Ÿ�̹� ����
            GetWorld()->GetTimerManager().SetTimer(
                TimerHandle_ShowLobby,
                [this, GameInstance]()
                {
                    if (LobbyWidgetClass && !LobbyWidgetInstance)
                    {
                        LobbyWidgetInstance = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
                        if (LobbyWidgetInstance)
                        {
                            LobbyWidgetInstance->AddToViewport();

                            // ���ӿ��� ���ƿ� ��� �ٷ� ���� �κ� ǥ��
                            LobbyWidgetInstance->bIsAuthenticated = true;
                            LobbyWidgetInstance->ShowMainLobby();

                            // ���콺 Ŀ�� ǥ��
                            bShowMouseCursor = true;
                            SetInputMode(FInputModeUIOnly());
                        }
                    }

                    // �÷��� ����
                    GameInstance->bIsReturningFromGame = false;
                },
                0.5f,
                false
            );
        }
    }
}

void AMyPlayerController::SeamlessTravelFrom(APlayerController* OldPC)
{
    Super::SeamlessTravelFrom(OldPC);

    UE_LOG(LogTemp, Log, TEXT("SeamlessTravelFrom - Leaving current level"));

    // ���� ������ ������ �� ���� �۾�
    if (IsLocalController())
    {
        // UI ����
        if (AMyDCharacter* MyChar = Cast<AMyDCharacter>(GetPawn()))
        {
            if (MyChar->HUDWidget)
            {
                MyChar->HUDWidget->RemoveFromParent();
            }
            if (MyChar->InventoryWidgetInstance)
            {
                MyChar->InventoryWidgetInstance->RemoveFromParent();
            }
            if (MyChar->EquipmentWidgetInstance)
            {
                MyChar->EquipmentWidgetInstance->RemoveFromParent();
            }
        }
    }
}

void AMyPlayerController::ServerRequestIndividualTravel_Implementation(const FString& LevelName)
{
    if (!HasAuthority()) return;

    // �� �÷��̾ ���������� �̵�
    ClientTravel(LevelName, ETravelType::TRAVEL_Relative);

    UE_LOG(LogTemp, Log, TEXT("Server: Sending player to %s"), *LevelName);
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // ESC Ű ���ε� �߰�
    if (InputComponent)
    {
        InputComponent->BindAction("Escape", IE_Pressed, this, &AMyPlayerController::OnEscapePressed);
    }
}

void AMyPlayerController::OnEscapePressed()
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ESC pressed - requesting safe game exit"));

    // ������ ���� �� ���� ����
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        // ���� �� ���� ����
        GameInstance->SaveDataAndShutdown();
    }

    // ���� ����
    UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EndPlay called - Reason: %d"), (int32)EndPlayReason);

    // ���� ���� �� ������ ����
    if (EndPlayReason == EEndPlayReason::Quit ||
        EndPlayReason == EEndPlayReason::EndPlayInEditor)
    {
        UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
        if (GameInstance && !GameInstance->bIsSaving)
        {
            UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Triggering data save on EndPlay"));

            if (AMyDCharacter* MyChar = Cast<AMyDCharacter>(GetPawn()))
            {
                if (MyChar->InventoryComponent)
                    GameInstance->SavedInventoryItems = MyChar->InventoryComponent->InventoryItemsStruct;

                
            }

            GameInstance->SaveDataAndShutdown();
        }
    }

    Super::EndPlay(EndPlayReason);
}