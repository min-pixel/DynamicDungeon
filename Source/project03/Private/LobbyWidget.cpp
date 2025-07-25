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

    // AuthManager 가져오기
    AuthMgr = GetGameInstance()->GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnAuthResponse.AddDynamic(this, &ULobbyWidget::OnAuthResponse);
    }

    // 버튼 바인딩
    if (LoginButton)
        LoginButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLoginClicked);
    if (RegisterButton)
        RegisterButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnRegisterClicked);
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

    // 스탯 업그레이드 버튼 바인딩
    if (UpgradeHealthButton)
        UpgradeHealthButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeHealthClicked);
    if (UpgradeStaminaButton)
        UpgradeStaminaButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeStaminaClicked);
    if (UpgradeKnowledgeButton)
        UpgradeKnowledgeButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeKnowledgeClicked);

    // 로그아웃 버튼 바인딩 추가
    if (LogoutButton)
        LogoutButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLogoutButtonClicked);

    if (ToggleBSPButton)
        ToggleBSPButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnToggleBSPPreviewClicked);

    // GameInstance 확인
    UDynamicDungeonInstance* GameInstance =Cast<UDynamicDungeonInstance>(GetWorld()->GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LobbyWidgetInstance = this;

        // 게임에서 돌아온 경우 처리
        if (GameInstance->bIsReturningFromGame && GameInstance->bHasValidCharacterData)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Returning from game with valid data - skip login"));

            // 이미 인증된 상태로 설정
            bIsAuthenticated = true;

            UE_LOG(LogTemp, Warning,
                TEXT("[DEBUG LobbyWidget] Construct: Returning=%d, ValidData=%d"),
                GameInstance->bIsReturningFromGame, GameInstance->bHasValidCharacterData);

            // 플래그 리셋
            GameInstance->bIsReturningFromGame = false;

            // 바로 메인 로비로 이동
            ShowMainLobby();
            return;
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("[DEBUG LobbyWidget] GameInstance cast failed!"));
        }
    }

    // 처음 시작하는 경우 - 인증 화면 표시
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

    // 인증되지 않았으면 로그인 화면으로
    if (!bIsAuthenticated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Not authenticated - showing auth screen"));
        ShowAuthScreen();
        return;
    }


    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(1); // 인덱스 1: 메인 로비
        UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Showing main lobby screen"));
    }

    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        EPlayerClass currentClass = GameInstance->CurrentCharacterData.PlayerClass;
        for (int32 i = 0; i < AvailableClasses.Num(); ++i)
        {
            if (AvailableClasses[i] == currentClass)
            {
                CurrentClassIndex = i;
                break;
            }
        }
        UpdateClassDisplay();
        UE_LOG(LogTemp, Warning, TEXT("Lobby initialized with class: %d"), (int32)currentClass);
    }

    // 메인 로비 초기화
    InitializeLobby(PlayerCharacter);
    // 스탯 표시 업데이트
    UpdateStatsDisplay();

    
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

            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

            // 1. 클래스별 기본 스탯 계산
            float BaseHealth, BaseStamina, BaseKnowledge;
            GameInstance->GetBaseStatsForClass(
                static_cast<EPlayerClass>(CharacterData.PlayerClass),
                BaseHealth, BaseStamina, BaseKnowledge
            );

            // 2. 보너스 역계산 (음수 방지)
            GameInstance->CurrentCharacterData.BonusHealth =
                FMath::Max(0.0f, CharacterData.MaxHealth - BaseHealth);
            GameInstance->CurrentCharacterData.BonusStamina =
                FMath::Max(0.0f, CharacterData.MaxStamina - BaseStamina);
            GameInstance->CurrentCharacterData.BonusKnowledge =
                FMath::Max(0.0f, CharacterData.MaxKnowledge - BaseKnowledge);



            // 3. 디버그 로그 (확인용)
            UE_LOG(LogTemp, Warning, TEXT("=== BONUS STATS RESTORED ==="));
            UE_LOG(LogTemp, Warning, TEXT("Class: %d"), (int32)CharacterData.PlayerClass);
            UE_LOG(LogTemp, Warning, TEXT("Total Stats: %.1f/%.1f/%.1f"),
                CharacterData.MaxHealth, CharacterData.MaxStamina, CharacterData.MaxKnowledge);
            UE_LOG(LogTemp, Warning, TEXT("Base Stats: %.1f/%.1f/%.1f"),
                BaseHealth, BaseStamina, BaseKnowledge);
            UE_LOG(LogTemp, Warning, TEXT("Calculated Bonus: %.1f/%.1f/%.1f"),
                GameInstance->CurrentCharacterData.BonusHealth,
                GameInstance->CurrentCharacterData.BonusStamina,
                GameInstance->CurrentCharacterData.BonusKnowledge);


            bIsAuthenticated = true;



            // 받은 캐릭터 데이터를 GameInstance에 저장
           
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
                GameInstance->bHasValidCharacterData = true;

                // 만약 기본 스탯(100/100/100)이라면 직업별 스탯으로 초기화
                if (CharacterData.MaxHealth == 100 && CharacterData.MaxStamina == 100 && CharacterData.MaxKnowledge == 100 &&
                    CharacterData.Gold == 1000) {
                    EPlayerClass playerClass = static_cast<EPlayerClass>(CharacterData.PlayerClass);

                    switch (playerClass)
                    {
                    case EPlayerClass::Warrior:
                        GameInstance->CurrentCharacterData.MaxHealth = 150.f;
                        GameInstance->CurrentCharacterData.MaxStamina = 100.f;
                        GameInstance->CurrentCharacterData.MaxKnowledge = 0.f;
                        break;
                    case EPlayerClass::Rogue:
                        GameInstance->CurrentCharacterData.MaxHealth = 100.f;
                        GameInstance->CurrentCharacterData.MaxStamina = 150.f;
                        GameInstance->CurrentCharacterData.MaxKnowledge = 0.f;
                        break;
                    case EPlayerClass::Mage:
                        GameInstance->CurrentCharacterData.MaxHealth = 50.f;
                        GameInstance->CurrentCharacterData.MaxStamina = 100.f;
                        GameInstance->CurrentCharacterData.MaxKnowledge = 150.f;
                        break;
                    }
                    GameInstance->CurrentCharacterData.BonusHealth = 0.0f;
                    GameInstance->CurrentCharacterData.BonusStamina = 0.0f;
                    GameInstance->CurrentCharacterData.BonusKnowledge = 0.0f;
                    UE_LOG(LogTemp, Warning, TEXT("Applied class-specific stats for first-time user"));
                }

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

        /*if (GameInstance->bPlydie)
        {
            GameInstance->SavedInventoryItems.Empty();
            GameInstance->SavedStorageItems.Empty();
            InventoryComponentRef->InventoryItemsStruct.Empty();
            InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);
            StorageComponentRef->InventoryItemsStruct.Empty();
            StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);
        }*/

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
        UE_LOG(LogTemp, Error, TEXT("=== EQUIPMENT WIDGET CONNECTION DEBUG ==="));

        EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;

        

        EquipmentWidgetInstance->AddToViewport(1);
        EquipmentWidgetInstance->RefreshEquipmentSlots();
    }

    // (6) 골드 위젯 업데이트
    if (GoldWidgetInstance && GameInstance)
    {
        GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
    }

    // 플레이어 인벤토리 컴포넌트 검증
    if (InventoryComponentRef)
    {
        UE_LOG(LogTemp, Error, TEXT("=== PLAYER INVENTORY COMPONENT CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("InventoryComponentRef: VALID"));
        UE_LOG(LogTemp, Error, TEXT("IsValidLowLevel: %s"), InventoryComponentRef->IsValidLowLevel() ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Error, TEXT("Array Size: %d"), InventoryComponentRef->InventoryItemsStruct.Num());
        UE_LOG(LogTemp, Error, TEXT("Capacity: %d"), InventoryComponentRef->Capacity);
        UE_LOG(LogTemp, Error, TEXT("Owner: %s"), InventoryComponentRef->GetOwner() ? TEXT("VALID") : TEXT("NULL"));
    }

    // 창고 컴포넌트 검증
    if (StorageComponentRef)
    {
        UE_LOG(LogTemp, Error, TEXT("=== STORAGE COMPONENT CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("StorageComponentRef: VALID"));
        UE_LOG(LogTemp, Error, TEXT("IsValidLowLevel: %s"), StorageComponentRef->IsValidLowLevel() ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Error, TEXT("Array Size: %d"), StorageComponentRef->InventoryItemsStruct.Num());
        UE_LOG(LogTemp, Error, TEXT("Capacity: %d"), StorageComponentRef->Capacity);
        UE_LOG(LogTemp, Error, TEXT("Owner: %s"), StorageComponentRef->GetOwner() ? TEXT("VALID") : TEXT("NULL"));
    }

    // 위젯 연결 상태 검증
    if (InventoryWidgetInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("=== INVENTORY WIDGET CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("InventoryWidgetInstance: VALID"));
        UE_LOG(LogTemp, Error, TEXT("InventoryRef: %s"), InventoryWidgetInstance->InventoryRef ? TEXT("VALID") : TEXT("NULL"));
        if (InventoryWidgetInstance->InventoryRef)
        {
            UE_LOG(LogTemp, Error, TEXT("Widget's InventoryRef Size: %d"), InventoryWidgetInstance->InventoryRef->InventoryItemsStruct.Num());
        }
    }

    if (StorageWidgetInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("=== STORAGE WIDGET CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("StorageWidgetInstance: VALID"));
        UE_LOG(LogTemp, Error, TEXT("InventoryRef: %s"), StorageWidgetInstance->InventoryRef ? TEXT("VALID") : TEXT("NULL"));
        if (StorageWidgetInstance->InventoryRef)
        {
            UE_LOG(LogTemp, Error, TEXT("Widget's InventoryRef Size: %d"), StorageWidgetInstance->InventoryRef->InventoryItemsStruct.Num());
        }
    }


    UE_LOG(LogTemp, Warning, TEXT("Main Lobby Initialized Successfully"));
}

void ULobbyWidget::OnStartGameClicked()
{

    if (ShopWidgetInstance) ShopWidgetInstance->RemoveFromParent();

    // GameInstance에 현재 로비 상태 저장
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {

        if (CurrentClassIndex >= 0 && CurrentClassIndex < AvailableClasses.Num())
        {
            GameInstance->CurrentCharacterData.PlayerClass = AvailableClasses[CurrentClassIndex];
            GameInstance->RecalculateStats();
            UE_LOG(LogTemp, Warning, TEXT("Selected class in lobby: %d"), (int32)AvailableClasses[CurrentClassIndex]);
        }

        // 인벤토리 데이터 저장
        if (InventoryComponentRef && InventoryComponentRef->InventoryItemsStruct.Num() > 0)
        {
            GameInstance->SavedInventoryItems = InventoryComponentRef->InventoryItemsStruct;
            UE_LOG(LogTemp, Warning, TEXT("Saved %d inventory items"), GameInstance->SavedInventoryItems.Num());

            // 저장된 아이템들 로그 출력
            for (int32 i = 0; i < GameInstance->SavedInventoryItems.Num(); ++i)
            {
                if (GameInstance->SavedInventoryItems[i].ItemClass)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s"), i, *GameInstance->SavedInventoryItems[i].ItemName);
                }
            }
        }

        // 창고 데이터 저장
        if (StorageComponentRef && StorageComponentRef->InventoryItemsStruct.Num() > 0)
        {
            GameInstance->SavedStorageItems = StorageComponentRef->InventoryItemsStruct;
            UE_LOG(LogTemp, Warning, TEXT("Saved %d storage items"), GameInstance->SavedStorageItems.Num());
        }

        // 장비 데이터 저장
        if (EquipmentWidgetInstance)
        {
            GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
            UE_LOG(LogTemp, Warning, TEXT("Saved equipment data"));
        }

        // 로비 골드를 캐릭터 데이터에 동기화
        GameInstance->CurrentCharacterData.Gold = GameInstance->LobbyGold;
        UE_LOG(LogTemp, Warning, TEXT("Synchronized gold: %d"), GameInstance->LobbyGold);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found - data will be lost!"));
    }

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
    if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
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
            ShopWidgetInstance->StorageInventory = StorageComponentRef;
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

    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && CurrentClassIndex >= 0 && CurrentClassIndex < AvailableClasses.Num())
    {
        GameInstance->CurrentCharacterData.PlayerClass = AvailableClasses[CurrentClassIndex];
        GameInstance->RecalculateStats(); //  핵심: 스탯 재계산

        // UI 업데이트
        UpdateStatsDisplay();

        UE_LOG(LogTemp, Warning, TEXT("Class changed to: %d, stats recalculated"), (int32)AvailableClasses[CurrentClassIndex]);
    }
}

void ULobbyWidget::OnRightArrowClicked()
{
    CurrentClassIndex = (CurrentClassIndex + 1) % AvailableClasses.Num();
    UpdateClassDisplay();

    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && CurrentClassIndex >= 0 && CurrentClassIndex < AvailableClasses.Num())
    {
        GameInstance->CurrentCharacterData.PlayerClass = AvailableClasses[CurrentClassIndex];
        GameInstance->RecalculateStats(); //  핵심: 스탯 재계산

        // UI 업데이트
        UpdateStatsDisplay();

        UE_LOG(LogTemp, Warning, TEXT("Class changed to: %d, stats recalculated"), (int32)AvailableClasses[CurrentClassIndex]);
    }
}

void ULobbyWidget::UpdateClassDisplay()
{
    FString ClassName;
    switch (AvailableClasses[CurrentClassIndex])
    {
    case EPlayerClass::Mage: ClassName = TEXT("M"); break;     // 0
    case EPlayerClass::Warrior: ClassName = TEXT("W"); break;  // 1
    case EPlayerClass::Rogue: ClassName = TEXT("R"); break;    // 2
    }

    if (ClassText)
    {
        ClassText->SetText(FText::FromString(ClassName));
    }

    if (UpgradeKnowledgeButton)
    {
        EPlayerClass CurrentDisplayedClass = AvailableClasses[CurrentClassIndex]; // 화면 표시 클래스
        bool bCanUpgradeKnowledge = (CurrentDisplayedClass == EPlayerClass::Mage);
        UpgradeKnowledgeButton->SetIsEnabled(bCanUpgradeKnowledge);
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

// 스탯 업그레이드 구현
void ULobbyWidget::UpgradeHealth()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance) return;

    if (!CanAffordUpgrade(HealthUpgradeCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough gold for health upgrade!"));
        return;
    }

    // 골드 차감
    GameInstance->LobbyGold -= HealthUpgradeCost;

    // 스탯 증가
    GameInstance->CurrentCharacterData.BonusHealth += HealthUpgradeAmount;

    GameInstance->RecalculateStats();

    // UI 업데이트
    UpdateGoldDisplay();
    UpdateStatsDisplay();

    // 서버에 저장
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Log, TEXT("Health upgraded! New MaxHealth: %.0f"),
        GameInstance->CurrentCharacterData.MaxHealth);
}

void ULobbyWidget::UpgradeStamina()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance) return;

    if (!CanAffordUpgrade(StaminaUpgradeCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough gold for stamina upgrade!"));
        return;
    }

    GameInstance->LobbyGold -= StaminaUpgradeCost;
    GameInstance->CurrentCharacterData.BonusStamina += StaminaUpgradeAmount; // 보너스에 추가
    GameInstance->RecalculateStats(); // 재계산

    UpdateGoldDisplay();
    UpdateStatsDisplay();
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Log, TEXT("Stamina upgraded! New MaxStamina: %.0f"),
        GameInstance->CurrentCharacterData.MaxStamina);
}

void ULobbyWidget::UpgradeKnowledge()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance) return;

    if (!CanAffordUpgrade(KnowledgeUpgradeCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough gold for knowledge upgrade!"));
        return;
    }

    if (GameInstance->CurrentCharacterData.PlayerClass != EPlayerClass::Mage)
    {
        UE_LOG(LogTemp, Warning, TEXT("Only Mages can upgrade Knowledge!"));
        return;
    }

    GameInstance->LobbyGold -= KnowledgeUpgradeCost;
    GameInstance->CurrentCharacterData.BonusKnowledge += KnowledgeUpgradeAmount; // 보너스에 추가
    GameInstance->RecalculateStats(); // 재계산


    UpdateGoldDisplay();
    UpdateStatsDisplay();
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Log, TEXT("Knowledge upgraded! New MaxKnowledge: %.0f"),
        GameInstance->CurrentCharacterData.MaxKnowledge);
}

// 헬퍼 함수들
bool ULobbyWidget::CanAffordUpgrade(int32 Cost) const
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    return GameInstance && GameInstance->LobbyGold >= Cost;
}

FString ULobbyWidget::GetCurrentStats() const
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance) return TEXT("No Data");

    return FString::Printf(TEXT("Health: %.0f\nStamina: %.0f\nKnowledge: %.0f"),
        GameInstance->CurrentCharacterData.MaxHealth,
        GameInstance->CurrentCharacterData.MaxStamina,
        GameInstance->CurrentCharacterData.MaxKnowledge);
}

void ULobbyWidget::UpdateStatsDisplay()
{
    if (CurrentStatsText)
    {
        CurrentStatsText->SetText(FText::FromString(GetCurrentStats()));
    }
}

void ULobbyWidget::UpdateGoldDisplay()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance && GoldWidgetInstance)
    {
        GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
    }
}

// 버튼 클릭 이벤트
void ULobbyWidget::OnUpgradeHealthClicked()
{
    UpgradeHealth();
}

void ULobbyWidget::OnUpgradeStaminaClicked()
{
    UpgradeStamina();
}

void ULobbyWidget::OnUpgradeKnowledgeClicked()
{
    UpgradeKnowledge();
}

void ULobbyWidget::OnLogoutButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Logout button clicked - starting logout process"));

    // 로그아웃 중 UI 피드백
    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(TEXT("Logout...")));
    }

    // 모든 버튼 비활성화 (중복 클릭 방지)
    SetButtonsEnabled(false);

    // 현재 로비 데이터를 데이터베이스에 저장
    SaveLobbyDataToDatabase();
}

void ULobbyWidget::SetButtonsEnabled(bool bEnabled)
{
    if (StartGameButton) StartGameButton->SetIsEnabled(bEnabled);
    if (GoToShopButton) GoToShopButton->SetIsEnabled(bEnabled);
    if (LogoutButton) LogoutButton->SetIsEnabled(bEnabled);
    if (UpgradeHealthButton) UpgradeHealthButton->SetIsEnabled(bEnabled);
    if (UpgradeStaminaButton) UpgradeStaminaButton->SetIsEnabled(bEnabled);
    if (UpgradeKnowledgeButton) UpgradeKnowledgeButton->SetIsEnabled(bEnabled);
    if (LeftArrowButton) LeftArrowButton->SetIsEnabled(bEnabled);
    if (RightArrowButton) RightArrowButton->SetIsEnabled(bEnabled);
}

void ULobbyWidget::SaveLobbyDataToDatabase()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance not found for logout save"));
        ReturnToLoginScreen();
        return;
    }

    // 유효한 캐릭터 데이터가 있는지 확인
    if (GameInstance->CurrentCharacterData.PlayerName.IsEmpty() ||
        GameInstance->CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid character data to save for logout"));
        ReturnToLoginScreen();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Saving lobby data for logout - Character: %s"),
        *GameInstance->CurrentCharacterData.PlayerName);

    // 저장 완료 이벤트 바인딩 (기존 멤버 변수 AuthMgr 사용)
    if (AuthMgr)
    {
        // 기존 바인딩 제거 후 새로 추가
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
        AuthMgr->OnSaveDataResponse.AddDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
    }

    // 1. 창고 인벤토리 데이터 동기화
    if (StorageComponentRef && StorageComponentRef->InventoryItemsStruct.Num() > 0)
    {
        GameInstance->SavedStorageItems = StorageComponentRef->InventoryItemsStruct;
        UE_LOG(LogTemp, Warning, TEXT("Synced %d storage items for logout"),
            GameInstance->SavedStorageItems.Num());
    }

    // 2. 장비 데이터 동기화
    if (EquipmentWidgetInstance)
    {
        GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
        UE_LOG(LogTemp, Warning, TEXT("Synced equipment data for logout"));
    }

    // 3. 인벤토리 데이터 동기화 (플레이어 인벤토리)
    if (InventoryComponentRef && InventoryComponentRef->InventoryItemsStruct.Num() > 0)
    {
        GameInstance->SavedInventoryItems = InventoryComponentRef->InventoryItemsStruct;
        UE_LOG(LogTemp, Warning, TEXT("Synced %d inventory items for logout"),
            GameInstance->SavedInventoryItems.Num());
    }

    // 4. 골드 동기화
    GameInstance->CurrentCharacterData.Gold = GameInstance->LobbyGold;
    UE_LOG(LogTemp, Warning, TEXT("Synced gold: %d"), GameInstance->LobbyGold);

    // 5. 데이터베이스 저장 요청 (기존 SaveAllDataToServer 함수 재사용)
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Warning, TEXT("Logout save request sent to server"));
}

void ULobbyWidget::OnLogoutSaveCompleted(bool bSuccess)
{
    UE_LOG(LogTemp, Warning, TEXT("Logout save completed: %s"),
        bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    // 성공 여부와 관계없이 로그아웃 진행
    ReturnToLoginScreen();

    // 이벤트 바인딩 해제 (기존 멤버 변수 AuthMgr 사용)
    if (AuthMgr)
    {
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
    }
}

void ULobbyWidget::ReturnToLoginScreen()
{
    UE_LOG(LogTemp, Warning, TEXT("Returning to login screen..."));

    // 1. GameInstance 데이터 초기화 (다음 로그인을 위해)
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        // 인증 상태 초기화
        GameInstance->bHasValidCharacterData = false;
        GameInstance->bIsReturningFromGame = false;

        // 캐릭터 데이터 초기화 (보안을 위해)
        GameInstance->CurrentCharacterData.PlayerName = TEXT("DefaultName");
        GameInstance->CurrentCharacterData.CharacterId = 0;
        GameInstance->LobbyGold = 1000; // 기본값으로 리셋

        // 임시 데이터 클리어
        GameInstance->SavedInventoryItems.Empty();
        GameInstance->SavedEquipmentItems.Empty();
        GameInstance->SavedStorageItems.Empty();

        UE_LOG(LogTemp, Warning, TEXT("GameInstance data cleared for logout"));
    }

    // 2. 로비 위젯 상태 초기화
    bIsAuthenticated = false;

    // 3. UI 상태 복원
    SetButtonsEnabled(true);

    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(TEXT("Plesse Login")));
    }

    // 4. 입력 필드 초기화
    if (UsernameTextBox) UsernameTextBox->SetText(FText::GetEmpty());
    if (PasswordTextBox) PasswordTextBox->SetText(FText::GetEmpty());

    // 5. 인증 화면으로 전환
    ShowAuthScreen();

    UE_LOG(LogTemp, Warning, TEXT("Successfully returned to login screen"));
}

void ULobbyWidget::OnToggleBSPPreviewClicked()
{
    if (!bBSPPreviewActive)
    {
        // 로그인 패널 숨김 (버튼은 Switcher 밖에 둬야 사라지지 않음)
        if (MainSwitcher)
        {
            MainSwitcher->SetVisibility(ESlateVisibility::Collapsed);
        }

        // BSPMapGenerator 스폰
        if (!BSPPreviewActor && BSPGeneratorClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            FVector SpawnLoc = FVector::ZeroVector;      // 필요하면 위치 조정
            FRotator SpawnRot = FRotator::ZeroRotator;

            BSPPreviewActor = GetWorld()->SpawnActor<ABSPMapGenerator>(BSPGeneratorClass, SpawnLoc, SpawnRot, Params);
        }

        bBSPPreviewActive = true;
    }
    else
    {
        // BSP 액터 제거
        if (BSPPreviewActor)
        {
            BSPPreviewActor->Destroy();
            BSPPreviewActor = nullptr;
        }

        // 로그인 화면 다시 표시
        if (MainSwitcher)
        {
            MainSwitcher->SetVisibility(ESlateVisibility::Visible);
        }
        ShowAuthScreen(); // 입력모드/커서 세팅 포함.

        bBSPPreviewActive = false;
    }
}