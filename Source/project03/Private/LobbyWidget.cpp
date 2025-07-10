// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"
#include "ShopWidget.h"
#include "GoldWidget.h"
#include "MyPlayerController.h"
#include "GameFramework/HUD.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);
    bIsAuthenticated = false;

    // AuthManager 가져오기
    AuthMgr = GetGameInstance()->GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnAuthResponse.AddDynamic(this, &ULobbyWidget::OnAuthResponse);
    }

    // ========== 인증 화면 버튼 바인딩 ==========
    if (LoginButton)
        LoginButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLoginClicked);

    if (RegisterButton)
        RegisterButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnRegisterClicked);

    // ========== 메인 로비 버튼 바인딩 ==========
    if (StartGameButton)
        StartGameButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);

    if (GoToShopButton)
        GoToShopButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnGoToShopClicked);

    if (LeftArrowButton)
        LeftArrowButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLeftArrowClicked);

    if (RightArrowButton)
        RightArrowButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnRightArrowClicked);

    if (CloseShopButton)
        CloseShopButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnCloseShopButtonClicked);

    // GameInstance에 위젯 참조 저장
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LobbyWidgetInstance = this;
    }

    // 초기 상태: 인증 화면 표시
    ShowAuthScreen();
}

void ULobbyWidget::ShowAuthScreen()
{
    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(0); // 인덱스 0: 인증 화면
        UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Showing authentication screen"));
    }

    // 마우스 커서 표시 및 UI 모드 설정
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(UsernameTextBox ? UsernameTextBox->TakeWidget() : TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }

    //SetAuthStatusText(TEXT("로그인 또는 회원가입을 해주세요"));
}

void ULobbyWidget::ShowMainLobby()
{
    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(1); // 인덱스 1: 메인 로비
        UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Showing main lobby screen"));
    }

    // 메인 로비 초기화
    InitializeLobby(PlayerCharacter);
}

void ULobbyWidget::SetAuthStatusText(const FString& StatusMessage)
{
    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(StatusMessage));
    }
}

// ========== 인증 관련 함수들 ==========
void ULobbyWidget::OnLoginClicked()
{
    if (!AuthMgr || !UsernameTextBox || !PasswordTextBox)
    {
        //SetAuthStatusText(TEXT("로그인 시스템 오류"));
        return;
    }

    const FString User = UsernameTextBox->GetText().ToString();
    const FString Pass = PasswordTextBox->GetText().ToString();

    if (User.IsEmpty() || Pass.IsEmpty())
    {
        //SetAuthStatusText(TEXT("아이디와 비밀번호를 입력해주세요"));
        return;
    }

    bWasRegister = false;
    //SetAuthStatusText(TEXT("로그인 중..."));

    // 버튼 비활성화 (중복 요청 방지)
    if (LoginButton) LoginButton->SetIsEnabled(false);
    if (RegisterButton) RegisterButton->SetIsEnabled(false);

    AuthMgr->Login(User, Pass);
}

void ULobbyWidget::OnRegisterClicked()
{
    if (!AuthMgr || !UsernameTextBox || !PasswordTextBox)
    {
        //SetAuthStatusText(TEXT("회원가입 시스템 오류"));
        return;
    }

    const FString User = UsernameTextBox->GetText().ToString();
    const FString Pass = PasswordTextBox->GetText().ToString();

    if (User.IsEmpty() || Pass.IsEmpty())
    {
        //SetAuthStatusText(TEXT("아이디와 비밀번호를 입력해주세요"));
        return;
    }

    bWasRegister = true;
    //SetAuthStatusText(TEXT("회원가입 중..."));

    // 버튼 비활성화 (중복 요청 방지)
    if (LoginButton) LoginButton->SetIsEnabled(false);
    if (RegisterButton) RegisterButton->SetIsEnabled(false);

    AuthMgr->RegisterUser(User, Pass);
}

void ULobbyWidget::OnAuthResponse(bool bSuccess, FCharacterLoginData CharacterData)
{
    // 버튼 다시 활성화
    if (LoginButton) LoginButton->SetIsEnabled(true);
    if (RegisterButton) RegisterButton->SetIsEnabled(true);

    if (bSuccess)
    {
        if (bWasRegister)
        {
            //SetAuthStatusText(TEXT("회원가입 성공! 이제 로그인해주세요"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Registration SUCCESS"));

            // 회원가입 성공 시 입력창 초기화
            if (PasswordTextBox) PasswordTextBox->SetText(FText::GetEmpty());
        }
        else
        {
            // 로그인 성공 - 캐릭터 데이터 활용
            //SetAuthStatusText(FString::Printf(TEXT("환영합니다, %s! 로비로 이동 중..."), *CharacterData.CharacterName));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Login SUCCESS - Character: %s, Class: %d, Gold: %d"),
                *CharacterData.CharacterName, CharacterData.PlayerClass, CharacterData.Gold);

            bIsAuthenticated = true;

            // 받은 캐릭터 데이터를 GameInstance에 저장
            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
            if (GameInstance)
            {
                // 캐릭터 기본 정보 설정
                GameInstance->CurrentCharacterData.PlayerName = CharacterData.CharacterName;
                GameInstance->CurrentCharacterData.PlayerClass = static_cast<EPlayerClass>(CharacterData.PlayerClass);
                GameInstance->CurrentCharacterData.Level = CharacterData.Level;
                GameInstance->CurrentCharacterData.MaxHealth = CharacterData.MaxHealth;
                GameInstance->CurrentCharacterData.MaxStamina = CharacterData.MaxStamina;
                GameInstance->CurrentCharacterData.MaxKnowledge = CharacterData.MaxKnowledge;
                GameInstance->LobbyGold = CharacterData.Gold;

                UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Saved character data to GameInstance"));
            }

            // 잠시 후 메인 로비로 전환
            FTimerHandle TransitionTimer;
            GetWorld()->GetTimerManager().SetTimer(TransitionTimer, [this]()
                {
                    ShowMainLobby();
                }, 1.5f, false);
        }
    }
    else
    {
        if (bWasRegister)
        {
            //SetAuthStatusText(TEXT("회원가입 실패: 이미 존재하는 아이디이거나 서버 오류"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Registration FAILED"));
        }
        else
        {
            //SetAuthStatusText(TEXT("로그인 실패: 아이디 또는 비밀번호가 틀렸습니다"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Login FAILED"));
        }
    }
}

// ========== 메인 로비 함수들 ==========
void ULobbyWidget::InitializeLobby(AMyDCharacter* Player)
{
    // 인증되지 않은 경우 초기화하지 않음
    if (!bIsAuthenticated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Cannot initialize lobby - not authenticated"));
        return;
    }

    PlayerCharacter = Player;

    // (1) 플레이어 인벤토리 컴포넌트 연결 (없으면 더미 생성)
    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
    {
        InventoryComponentRef = PlayerCharacter->InventoryComponent;
    }
    else
    {
        // 더미 생성
        InventoryComponentRef = NewObject<UInventoryComponent>(this);
        InventoryComponentRef->RegisterComponent();
        InventoryComponentRef->Capacity = 32;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // GameInstance에서 저장된 인벤토리를 복원한다
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && InventoryComponentRef)
    {
        const int32 SavedCount = GameInstance->SavedInventoryItems.Num();
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        for (int32 i = 0; i < SavedCount; ++i)
        {
            if (i < InventoryComponentRef->InventoryItemsStruct.Num())
            {
                InventoryComponentRef->InventoryItemsStruct[i] = GameInstance->SavedInventoryItems[i];
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Restored Inventory from GameInstance (%d items)"), SavedCount);
    }

    if (GameInstance && EquipmentWidgetInstance)
    {
        EquipmentWidgetInstance->RestoreEquipmentFromData(GameInstance->SavedEquipmentItems);
    }

    // (2) 창고 인벤토리 컴포넌트 생성 (항상 새로 만든다)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 50; // 창고는 더 크게
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

        if (GameInstance && GameInstance->SavedStorageItems.Num() > 0)
        {
            // 저장된 데이터가 있을 때만 복원
            StorageComponentRef->InventoryItemsStruct = GameInstance->SavedStorageItems;
            UE_LOG(LogTemp, Warning, TEXT("Restored Storage Inventory from SavedStorageItems"));
        }
        else
        {
            // 저장된 게 없으면 (처음 시작이면)
            UE_LOG(LogTemp, Warning, TEXT("No SavedStorageItems found, starting empty Storage"));
        }

        if (StorageWidgetInstance)
        {
            StorageWidgetInstance->InventoryRef = StorageComponentRef;
            StorageWidgetInstance->bIsChestInventory = true;
            StorageWidgetInstance->RefreshInventoryStruct();
        }

        UE_LOG(LogTemp, Warning, TEXT("Created Storage InventoryComponent"));
    }

    // (3) 인벤토리 위젯 설정
    if (InventoryWidgetInstance)
    {
        InventoryWidgetInstance->InventoryRef = InventoryComponentRef;
        InventoryWidgetInstance->bIsChestInventory = false;
        InventoryWidgetInstance->AddToViewport(1);
        InventoryWidgetInstance->RefreshInventoryStruct();
    }

    // (4) 창고 위젯 설정
    if (StorageWidgetInstance)
    {
        StorageWidgetInstance->InventoryRef = StorageComponentRef;
        StorageWidgetInstance->bIsChestInventory = true;
        StorageWidgetInstance->AddToViewport(1);
        StorageWidgetInstance->RefreshInventoryStruct();
    }

    // (5) 장비창 위젯 설정
    if (EquipmentWidgetInstance)
    {
        EquipmentWidgetInstance->AddToViewport(1);
        EquipmentWidgetInstance->RefreshEquipmentSlots();
    }

    // (6) 골드 위젯 업데이트
    if (GoldWidgetInstance && GameInstance)
    {
        GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
    }

    UE_LOG(LogTemp, Warning, TEXT("Main Lobby Initialized Successfully"));
}

void ULobbyWidget::OnStartGameClicked()
{
    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

    if (PC && PC->GetPawn())
    {
        PC->GetPawn()->Destroy();
        PC->UnPossess();
    }

    if (PC)
    {
        PC->ServerRequestStart();
        UE_LOG(LogTemp, Warning, TEXT("Request Start sent to server (bIsReady = true)."));

        if (PC->LobbyWidgetInstance)
        {
            PC->LobbyWidgetInstance->RemoveFromParent();
            PC->LobbyWidgetInstance = nullptr;
        }

        PC->bShowMouseCursor = false;
        FInputModeGameOnly GameMode;
        PC->SetInputMode(GameMode);
    }
}

void ULobbyWidget::OnGoToShopClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Go to Shop button clicked"));

    // 기존 창 숨기기
    //if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
    if (EquipmentWidgetInstance) EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

    if (!ShopWidgetInstance && ShopWidgetClass)
    {
        ShopWidgetInstance = CreateWidget<UShopWidget>(GetWorld(), ShopWidgetClass);

        if (ShopWidgetInstance)
        {
            // 상점 아이템 세팅
            TArray<FItemData> Items;

            for (TObjectIterator<UClass> It; It; ++It)
            {
                if (It->IsChildOf(AItem::StaticClass()) &&
                    !It->HasAnyClassFlags(CLASS_Abstract) &&
                    *It != AItem::StaticClass())
                {
                    AItem* DefaultItem = (*It)->GetDefaultObject<AItem>();
                    if (DefaultItem)
                    {
                        FItemData Data = DefaultItem->ToItemData();
                        Items.Add(Data);
                    }
                }
            }

            ShopWidgetInstance->ShopItemList = Items;
            ShopWidgetInstance->TargetInventory = InventoryComponentRef;
            ShopWidgetInstance->SlotWidgetClass = InventoryWidgetInstance->SlotWidgetClass;

            ShopWidgetInstance->PopulateShopItems();
            ShopWidgetInstance->AddToViewport(2);
        }
    }
}

void ULobbyWidget::OnLeftArrowClicked()
{
    CurrentClassIndex = (CurrentClassIndex - 1 + AvailableClasses.Num()) % AvailableClasses.Num();
    UpdateClassDisplay();
}

void ULobbyWidget::OnRightArrowClicked()
{
    CurrentClassIndex = (CurrentClassIndex + 1) % AvailableClasses.Num();
    UpdateClassDisplay();
}

void ULobbyWidget::UpdateClassDisplay()
{
    FString ClassName;
    switch (AvailableClasses[CurrentClassIndex])
    {
    case EPlayerClass::Warrior: ClassName = TEXT("W"); break;
    case EPlayerClass::Rogue:   ClassName = TEXT("R"); break;
    case EPlayerClass::Mage:    ClassName = TEXT("M"); break;
    }

    if (ClassText)
    {
        ClassText->SetText(FText::FromString(ClassName));
    }
}

void ULobbyWidget::OnCloseShopButtonClicked()
{
    if (ShopWidgetInstance)
    {
        ShopWidgetInstance->RemoveFromParent();
        ShopWidgetInstance = nullptr;
    }

    if (InventoryWidgetInstance)
        InventoryWidgetInstance->SetVisibility(ESlateVisibility::Visible);

    if (EquipmentWidgetInstance)
        EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Warning, TEXT("Shop closed via LobbyWidget"));
}