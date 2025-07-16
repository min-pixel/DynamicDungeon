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

            // 수정: InitializeLobby() 호출하지 않음!
            // 인증 후에 LobbyWidget에서 알아서 초기화할 것임

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

void AMyPlayerController::ClientReturnToLobby_Implementation()
{
    // 1) 클라이언트 자신의 GameInstance 플래그 세팅
    if (UDynamicDungeonInstance* GI = Cast<UDynamicDungeonInstance>(GetGameInstance()))
    {
        GI->bIsReturningFromGame = true;
        
            GI->bHasValidCharacterData = true;
    }

    // 2) 실제 로비 맵으로 이동
    //ClientTravel(TEXT("LobbyMap"), ETravelType::TRAVEL_Absolute);

}

// Seamless Travel 구현
void AMyPlayerController::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
    Super::GetSeamlessTravelActorList(bToTransition, ActorList);

    // 플레이어와 함께 이동해야 할 액터가 있다면 여기에 추가
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

    // 로비로 돌아왔을 때 처리
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && GameInstance->bIsReturningFromGame)
    {
        if (IsLocalController())
        {
            // 로비 위젯 생성 타이밍 조정
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

                            // 게임에서 돌아온 경우 바로 메인 로비 표시
                            LobbyWidgetInstance->bIsAuthenticated = true;
                            LobbyWidgetInstance->ShowMainLobby();

                            // 마우스 커서 표시
                            bShowMouseCursor = true;
                            SetInputMode(FInputModeUIOnly());
                        }
                    }

                    // 플래그 리셋
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

    // 현재 레벨을 떠나기 전 정리 작업
    if (IsLocalController())
    {
        // UI 정리
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

    // 이 플레이어만 개별적으로 이동
    ClientTravel(LevelName, ETravelType::TRAVEL_Relative);

    UE_LOG(LogTemp, Log, TEXT("Server: Sending player to %s"), *LevelName);
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // ESC 키 바인딩 추가
    if (InputComponent)
    {
        InputComponent->BindAction("Escape", IE_Pressed, this, &AMyPlayerController::OnEscapePressed);
    }
}

void AMyPlayerController::OnEscapePressed()
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ESC pressed - requesting safe game exit"));

    // 데이터 저장 후 게임 종료
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        // 저장 후 게임 종료
        GameInstance->SaveDataAndShutdown();
    }

    // 게임 종료
    UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("[PlayerController] EndPlay called - Reason: %d"), (int32)EndPlayReason);

    // 게임 종료 시 데이터 저장
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