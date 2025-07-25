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

    // AuthManager ��������
    AuthMgr = GetGameInstance()->GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnAuthResponse.AddDynamic(this, &ULobbyWidget::OnAuthResponse);
    }

    // ��ư ���ε�
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

    // ���� ���׷��̵� ��ư ���ε�
    if (UpgradeHealthButton)
        UpgradeHealthButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeHealthClicked);
    if (UpgradeStaminaButton)
        UpgradeStaminaButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeStaminaClicked);
    if (UpgradeKnowledgeButton)
        UpgradeKnowledgeButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnUpgradeKnowledgeClicked);

    // �α׾ƿ� ��ư ���ε� �߰�
    if (LogoutButton)
        LogoutButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLogoutButtonClicked);

    if (ToggleBSPButton)
        ToggleBSPButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnToggleBSPPreviewClicked);

    // GameInstance Ȯ��
    UDynamicDungeonInstance* GameInstance =Cast<UDynamicDungeonInstance>(GetWorld()->GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LobbyWidgetInstance = this;

        // ���ӿ��� ���ƿ� ��� ó��
        if (GameInstance->bIsReturningFromGame && GameInstance->bHasValidCharacterData)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Returning from game with valid data - skip login"));

            // �̹� ������ ���·� ����
            bIsAuthenticated = true;

            UE_LOG(LogTemp, Warning,
                TEXT("[DEBUG LobbyWidget] Construct: Returning=%d, ValidData=%d"),
                GameInstance->bIsReturningFromGame, GameInstance->bHasValidCharacterData);

            // �÷��� ����
            GameInstance->bIsReturningFromGame = false;

            // �ٷ� ���� �κ�� �̵�
            ShowMainLobby();
            return;
        }
        else {
            UE_LOG(LogTemp, Error, TEXT("[DEBUG LobbyWidget] GameInstance cast failed!"));
        }
    }

    // ó�� �����ϴ� ��� - ���� ȭ�� ǥ��
    ShowAuthScreen();
}

void ULobbyWidget::ShowAuthScreen()
{
    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(0); // �ε��� 0: ���� ȭ��
        UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Showing authentication screen"));
    }

    // ���콺 Ŀ�� ǥ�� �� UI ��� ����
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(UsernameTextBox ? UsernameTextBox->TakeWidget() : TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }

    //SetAuthStatusText(TEXT("�α��� �Ǵ� ȸ�������� ���ּ���"));
}

void ULobbyWidget::ShowMainLobby()
{

    // �������� �ʾ����� �α��� ȭ������
    if (!bIsAuthenticated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Not authenticated - showing auth screen"));
        ShowAuthScreen();
        return;
    }


    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(1); // �ε��� 1: ���� �κ�
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

    // ���� �κ� �ʱ�ȭ
    InitializeLobby(PlayerCharacter);
    // ���� ǥ�� ������Ʈ
    UpdateStatsDisplay();

    
}

void ULobbyWidget::SetAuthStatusText(const FString& StatusMessage)
{
    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(StatusMessage));
    }
}

// ========== ���� ���� �Լ��� ==========
void ULobbyWidget::OnLoginClicked()
{
    if (!AuthMgr || !UsernameTextBox || !PasswordTextBox)
    {
        //SetAuthStatusText(TEXT("�α��� �ý��� ����"));
        return;
    }

    const FString User = UsernameTextBox->GetText().ToString();
    const FString Pass = PasswordTextBox->GetText().ToString();

    if (User.IsEmpty() || Pass.IsEmpty())
    {
        //SetAuthStatusText(TEXT("���̵�� ��й�ȣ�� �Է����ּ���"));
        return;
    }

    bWasRegister = false;
    //SetAuthStatusText(TEXT("�α��� ��..."));

    // ��ư ��Ȱ��ȭ (�ߺ� ��û ����)
    if (LoginButton) LoginButton->SetIsEnabled(false);
    if (RegisterButton) RegisterButton->SetIsEnabled(false);

    AuthMgr->Login(User, Pass);
}

void ULobbyWidget::OnRegisterClicked()
{
    if (!AuthMgr || !UsernameTextBox || !PasswordTextBox)
    {
        //SetAuthStatusText(TEXT("ȸ������ �ý��� ����"));
        return;
    }

    const FString User = UsernameTextBox->GetText().ToString();
    const FString Pass = PasswordTextBox->GetText().ToString();

    if (User.IsEmpty() || Pass.IsEmpty())
    {
        //SetAuthStatusText(TEXT("���̵�� ��й�ȣ�� �Է����ּ���"));
        return;
    }

    bWasRegister = true;
    //SetAuthStatusText(TEXT("ȸ������ ��..."));

    // ��ư ��Ȱ��ȭ (�ߺ� ��û ����)
    if (LoginButton) LoginButton->SetIsEnabled(false);
    if (RegisterButton) RegisterButton->SetIsEnabled(false);

    AuthMgr->RegisterUser(User, Pass);
}

void ULobbyWidget::OnAuthResponse(bool bSuccess, FCharacterLoginData CharacterData)
{
    // ��ư �ٽ� Ȱ��ȭ
    if (LoginButton) LoginButton->SetIsEnabled(true);
    if (RegisterButton) RegisterButton->SetIsEnabled(true);

    if (bSuccess)
    {
        if (bWasRegister)
        {
            //SetAuthStatusText(TEXT("ȸ������ ����! ���� �α������ּ���"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Registration SUCCESS"));

            // ȸ������ ���� �� �Է�â �ʱ�ȭ
            if (PasswordTextBox) PasswordTextBox->SetText(FText::GetEmpty());


        }
        else
        {
            // �α��� ���� - ĳ���� ������ Ȱ��
            //SetAuthStatusText(FString::Printf(TEXT("ȯ���մϴ�, %s! �κ�� �̵� ��..."), *CharacterData.CharacterName));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Login SUCCESS - Character: %s, Class: %d, Gold: %d"),
                *CharacterData.CharacterName, CharacterData.PlayerClass, CharacterData.Gold);

            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());

            // 1. Ŭ������ �⺻ ���� ���
            float BaseHealth, BaseStamina, BaseKnowledge;
            GameInstance->GetBaseStatsForClass(
                static_cast<EPlayerClass>(CharacterData.PlayerClass),
                BaseHealth, BaseStamina, BaseKnowledge
            );

            // 2. ���ʽ� ����� (���� ����)
            GameInstance->CurrentCharacterData.BonusHealth =
                FMath::Max(0.0f, CharacterData.MaxHealth - BaseHealth);
            GameInstance->CurrentCharacterData.BonusStamina =
                FMath::Max(0.0f, CharacterData.MaxStamina - BaseStamina);
            GameInstance->CurrentCharacterData.BonusKnowledge =
                FMath::Max(0.0f, CharacterData.MaxKnowledge - BaseKnowledge);



            // 3. ����� �α� (Ȯ�ο�)
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



            // ���� ĳ���� �����͸� GameInstance�� ����
           
            if (GameInstance)
            {

               

                // ĳ���� �⺻ ���� ����
                GameInstance->CurrentCharacterData.PlayerName = CharacterData.CharacterName;
                GameInstance->CurrentCharacterData.PlayerClass = static_cast<EPlayerClass>(CharacterData.PlayerClass);
                GameInstance->CurrentCharacterData.Level = CharacterData.Level;
                GameInstance->CurrentCharacterData.MaxHealth = CharacterData.MaxHealth;
                GameInstance->CurrentCharacterData.MaxStamina = CharacterData.MaxStamina;
                GameInstance->CurrentCharacterData.MaxKnowledge = CharacterData.MaxKnowledge;
                GameInstance->LobbyGold = CharacterData.Gold;
                GameInstance->bHasValidCharacterData = true;

                // ���� �⺻ ����(100/100/100)�̶�� ������ �������� �ʱ�ȭ
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

            // ��� �� ���� �κ�� ��ȯ
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
            //SetAuthStatusText(TEXT("ȸ������ ����: �̹� �����ϴ� ���̵��̰ų� ���� ����"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Registration FAILED"));
        }
        else
        {
            //SetAuthStatusText(TEXT("�α��� ����: ���̵� �Ǵ� ��й�ȣ�� Ʋ�Ƚ��ϴ�"));
            UE_LOG(LogTemp, Warning, TEXT("[Auth] Login FAILED"));
        }
    }
}

// ========== ���� �κ� �Լ��� ==========
void ULobbyWidget::InitializeLobby(AMyDCharacter* Player)
{
    // �������� ���� ��� �ʱ�ȭ���� ����
    if (!bIsAuthenticated)
    {
        UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Cannot initialize lobby - not authenticated"));
        return;
    }

    

    PlayerCharacter = Player;

    // (1) �÷��̾� �κ��丮 ������Ʈ ���� (������ ���� ����)
    if (PlayerCharacter && PlayerCharacter->InventoryComponent)
    {
        InventoryComponentRef = PlayerCharacter->InventoryComponent;
    }
    else
    {
        // ���� ����
        InventoryComponentRef = NewObject<UInventoryComponent>(this);
        InventoryComponentRef->RegisterComponent();
        InventoryComponentRef->Capacity = 32;
        InventoryComponentRef->InventoryItemsStruct.SetNum(InventoryComponentRef->Capacity);

        UE_LOG(LogTemp, Warning, TEXT("Created Dummy InventoryComponent"));
    }

    // GameInstance���� ����� �κ��丮�� �����Ѵ�
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

    // (2) â�� �κ��丮 ������Ʈ ���� (�׻� ���� �����)
    StorageComponentRef = NewObject<UInventoryComponent>(this);
    if (StorageComponentRef)
    {
        StorageComponentRef->RegisterComponent();
        StorageComponentRef->Capacity = 50; // â��� �� ũ��
        StorageComponentRef->InventoryItemsStruct.SetNum(StorageComponentRef->Capacity);

        if (GameInstance && GameInstance->SavedStorageItems.Num() > 0)
        {
            // ����� �����Ͱ� ���� ���� ����
            StorageComponentRef->InventoryItemsStruct = GameInstance->SavedStorageItems;
            UE_LOG(LogTemp, Warning, TEXT("Restored Storage Inventory from SavedStorageItems"));
        }
        else
        {
            // ����� �� ������ (ó�� �����̸�)
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

    // (3) �κ��丮 ���� ����
    if (InventoryWidgetInstance)
    {
        InventoryWidgetInstance->InventoryRef = InventoryComponentRef;
        InventoryWidgetInstance->bIsChestInventory = false;
        InventoryWidgetInstance->AddToViewport(1);
        InventoryWidgetInstance->RefreshInventoryStruct();
    }

    // (4) â�� ���� ����
    if (StorageWidgetInstance)
    {
        StorageWidgetInstance->InventoryRef = StorageComponentRef;
        StorageWidgetInstance->bIsChestInventory = true;
        StorageWidgetInstance->AddToViewport(1);
        StorageWidgetInstance->RefreshInventoryStruct();
    }

    // (5) ���â ���� ����
    if (EquipmentWidgetInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("=== EQUIPMENT WIDGET CONNECTION DEBUG ==="));

        EquipmentWidgetInstance->InventoryOwner = InventoryWidgetInstance;

        

        EquipmentWidgetInstance->AddToViewport(1);
        EquipmentWidgetInstance->RefreshEquipmentSlots();
    }

    // (6) ��� ���� ������Ʈ
    if (GoldWidgetInstance && GameInstance)
    {
        GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
    }

    // �÷��̾� �κ��丮 ������Ʈ ����
    if (InventoryComponentRef)
    {
        UE_LOG(LogTemp, Error, TEXT("=== PLAYER INVENTORY COMPONENT CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("InventoryComponentRef: VALID"));
        UE_LOG(LogTemp, Error, TEXT("IsValidLowLevel: %s"), InventoryComponentRef->IsValidLowLevel() ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Error, TEXT("Array Size: %d"), InventoryComponentRef->InventoryItemsStruct.Num());
        UE_LOG(LogTemp, Error, TEXT("Capacity: %d"), InventoryComponentRef->Capacity);
        UE_LOG(LogTemp, Error, TEXT("Owner: %s"), InventoryComponentRef->GetOwner() ? TEXT("VALID") : TEXT("NULL"));
    }

    // â�� ������Ʈ ����
    if (StorageComponentRef)
    {
        UE_LOG(LogTemp, Error, TEXT("=== STORAGE COMPONENT CHECK ==="));
        UE_LOG(LogTemp, Error, TEXT("StorageComponentRef: VALID"));
        UE_LOG(LogTemp, Error, TEXT("IsValidLowLevel: %s"), StorageComponentRef->IsValidLowLevel() ? TEXT("TRUE") : TEXT("FALSE"));
        UE_LOG(LogTemp, Error, TEXT("Array Size: %d"), StorageComponentRef->InventoryItemsStruct.Num());
        UE_LOG(LogTemp, Error, TEXT("Capacity: %d"), StorageComponentRef->Capacity);
        UE_LOG(LogTemp, Error, TEXT("Owner: %s"), StorageComponentRef->GetOwner() ? TEXT("VALID") : TEXT("NULL"));
    }

    // ���� ���� ���� ����
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

    // GameInstance�� ���� �κ� ���� ����
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {

        if (CurrentClassIndex >= 0 && CurrentClassIndex < AvailableClasses.Num())
        {
            GameInstance->CurrentCharacterData.PlayerClass = AvailableClasses[CurrentClassIndex];
            GameInstance->RecalculateStats();
            UE_LOG(LogTemp, Warning, TEXT("Selected class in lobby: %d"), (int32)AvailableClasses[CurrentClassIndex]);
        }

        // �κ��丮 ������ ����
        if (InventoryComponentRef && InventoryComponentRef->InventoryItemsStruct.Num() > 0)
        {
            GameInstance->SavedInventoryItems = InventoryComponentRef->InventoryItemsStruct;
            UE_LOG(LogTemp, Warning, TEXT("Saved %d inventory items"), GameInstance->SavedInventoryItems.Num());

            // ����� �����۵� �α� ���
            for (int32 i = 0; i < GameInstance->SavedInventoryItems.Num(); ++i)
            {
                if (GameInstance->SavedInventoryItems[i].ItemClass)
                {
                    UE_LOG(LogTemp, Warning, TEXT("  Slot %d: %s"), i, *GameInstance->SavedInventoryItems[i].ItemName);
                }
            }
        }

        // â�� ������ ����
        if (StorageComponentRef && StorageComponentRef->InventoryItemsStruct.Num() > 0)
        {
            GameInstance->SavedStorageItems = StorageComponentRef->InventoryItemsStruct;
            UE_LOG(LogTemp, Warning, TEXT("Saved %d storage items"), GameInstance->SavedStorageItems.Num());
        }

        // ��� ������ ����
        if (EquipmentWidgetInstance)
        {
            GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
            UE_LOG(LogTemp, Warning, TEXT("Saved equipment data"));
        }

        // �κ� ��带 ĳ���� �����Ϳ� ����ȭ
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

    // ���� â �����
    if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
    if (EquipmentWidgetInstance) EquipmentWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

    if (!ShopWidgetInstance && ShopWidgetClass)
    {
        ShopWidgetInstance = CreateWidget<UShopWidget>(GetWorld(), ShopWidgetClass);

        if (ShopWidgetInstance)
        {
            // ���� ������ ����
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
        GameInstance->RecalculateStats(); //  �ٽ�: ���� ����

        // UI ������Ʈ
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
        GameInstance->RecalculateStats(); //  �ٽ�: ���� ����

        // UI ������Ʈ
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
        EPlayerClass CurrentDisplayedClass = AvailableClasses[CurrentClassIndex]; // ȭ�� ǥ�� Ŭ����
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

// ���� ���׷��̵� ����
void ULobbyWidget::UpgradeHealth()
{
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (!GameInstance) return;

    if (!CanAffordUpgrade(HealthUpgradeCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("Not enough gold for health upgrade!"));
        return;
    }

    // ��� ����
    GameInstance->LobbyGold -= HealthUpgradeCost;

    // ���� ����
    GameInstance->CurrentCharacterData.BonusHealth += HealthUpgradeAmount;

    GameInstance->RecalculateStats();

    // UI ������Ʈ
    UpdateGoldDisplay();
    UpdateStatsDisplay();

    // ������ ����
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
    GameInstance->CurrentCharacterData.BonusStamina += StaminaUpgradeAmount; // ���ʽ��� �߰�
    GameInstance->RecalculateStats(); // ����

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
    GameInstance->CurrentCharacterData.BonusKnowledge += KnowledgeUpgradeAmount; // ���ʽ��� �߰�
    GameInstance->RecalculateStats(); // ����


    UpdateGoldDisplay();
    UpdateStatsDisplay();
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Log, TEXT("Knowledge upgraded! New MaxKnowledge: %.0f"),
        GameInstance->CurrentCharacterData.MaxKnowledge);
}

// ���� �Լ���
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

// ��ư Ŭ�� �̺�Ʈ
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

    // �α׾ƿ� �� UI �ǵ��
    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(TEXT("Logout...")));
    }

    // ��� ��ư ��Ȱ��ȭ (�ߺ� Ŭ�� ����)
    SetButtonsEnabled(false);

    // ���� �κ� �����͸� �����ͺ��̽��� ����
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

    // ��ȿ�� ĳ���� �����Ͱ� �ִ��� Ȯ��
    if (GameInstance->CurrentCharacterData.PlayerName.IsEmpty() ||
        GameInstance->CurrentCharacterData.PlayerName == TEXT("DefaultName"))
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid character data to save for logout"));
        ReturnToLoginScreen();
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Saving lobby data for logout - Character: %s"),
        *GameInstance->CurrentCharacterData.PlayerName);

    // ���� �Ϸ� �̺�Ʈ ���ε� (���� ��� ���� AuthMgr ���)
    if (AuthMgr)
    {
        // ���� ���ε� ���� �� ���� �߰�
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
        AuthMgr->OnSaveDataResponse.AddDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
    }

    // 1. â�� �κ��丮 ������ ����ȭ
    if (StorageComponentRef && StorageComponentRef->InventoryItemsStruct.Num() > 0)
    {
        GameInstance->SavedStorageItems = StorageComponentRef->InventoryItemsStruct;
        UE_LOG(LogTemp, Warning, TEXT("Synced %d storage items for logout"),
            GameInstance->SavedStorageItems.Num());
    }

    // 2. ��� ������ ����ȭ
    if (EquipmentWidgetInstance)
    {
        GameInstance->SavedEquipmentItems = EquipmentWidgetInstance->GetAllEquipmentData();
        UE_LOG(LogTemp, Warning, TEXT("Synced equipment data for logout"));
    }

    // 3. �κ��丮 ������ ����ȭ (�÷��̾� �κ��丮)
    if (InventoryComponentRef && InventoryComponentRef->InventoryItemsStruct.Num() > 0)
    {
        GameInstance->SavedInventoryItems = InventoryComponentRef->InventoryItemsStruct;
        UE_LOG(LogTemp, Warning, TEXT("Synced %d inventory items for logout"),
            GameInstance->SavedInventoryItems.Num());
    }

    // 4. ��� ����ȭ
    GameInstance->CurrentCharacterData.Gold = GameInstance->LobbyGold;
    UE_LOG(LogTemp, Warning, TEXT("Synced gold: %d"), GameInstance->LobbyGold);

    // 5. �����ͺ��̽� ���� ��û (���� SaveAllDataToServer �Լ� ����)
    GameInstance->SaveAllDataToServer();

    UE_LOG(LogTemp, Warning, TEXT("Logout save request sent to server"));
}

void ULobbyWidget::OnLogoutSaveCompleted(bool bSuccess)
{
    UE_LOG(LogTemp, Warning, TEXT("Logout save completed: %s"),
        bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    // ���� ���ο� ������� �α׾ƿ� ����
    ReturnToLoginScreen();

    // �̺�Ʈ ���ε� ���� (���� ��� ���� AuthMgr ���)
    if (AuthMgr)
    {
        AuthMgr->OnSaveDataResponse.RemoveDynamic(this, &ULobbyWidget::OnLogoutSaveCompleted);
    }
}

void ULobbyWidget::ReturnToLoginScreen()
{
    UE_LOG(LogTemp, Warning, TEXT("Returning to login screen..."));

    // 1. GameInstance ������ �ʱ�ȭ (���� �α����� ����)
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        // ���� ���� �ʱ�ȭ
        GameInstance->bHasValidCharacterData = false;
        GameInstance->bIsReturningFromGame = false;

        // ĳ���� ������ �ʱ�ȭ (������ ����)
        GameInstance->CurrentCharacterData.PlayerName = TEXT("DefaultName");
        GameInstance->CurrentCharacterData.CharacterId = 0;
        GameInstance->LobbyGold = 1000; // �⺻������ ����

        // �ӽ� ������ Ŭ����
        GameInstance->SavedInventoryItems.Empty();
        GameInstance->SavedEquipmentItems.Empty();
        GameInstance->SavedStorageItems.Empty();

        UE_LOG(LogTemp, Warning, TEXT("GameInstance data cleared for logout"));
    }

    // 2. �κ� ���� ���� �ʱ�ȭ
    bIsAuthenticated = false;

    // 3. UI ���� ����
    SetButtonsEnabled(true);

    if (AuthStatusText)
    {
        AuthStatusText->SetText(FText::FromString(TEXT("Plesse Login")));
    }

    // 4. �Է� �ʵ� �ʱ�ȭ
    if (UsernameTextBox) UsernameTextBox->SetText(FText::GetEmpty());
    if (PasswordTextBox) PasswordTextBox->SetText(FText::GetEmpty());

    // 5. ���� ȭ������ ��ȯ
    ShowAuthScreen();

    UE_LOG(LogTemp, Warning, TEXT("Successfully returned to login screen"));
}

void ULobbyWidget::OnToggleBSPPreviewClicked()
{
    if (!bBSPPreviewActive)
    {
        // �α��� �г� ���� (��ư�� Switcher �ۿ� �־� ������� ����)
        if (MainSwitcher)
        {
            MainSwitcher->SetVisibility(ESlateVisibility::Collapsed);
        }

        // BSPMapGenerator ����
        if (!BSPPreviewActor && BSPGeneratorClass)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            FVector SpawnLoc = FVector::ZeroVector;      // �ʿ��ϸ� ��ġ ����
            FRotator SpawnRot = FRotator::ZeroRotator;

            BSPPreviewActor = GetWorld()->SpawnActor<ABSPMapGenerator>(BSPGeneratorClass, SpawnLoc, SpawnRot, Params);
        }

        bBSPPreviewActive = true;
    }
    else
    {
        // BSP ���� ����
        if (BSPPreviewActor)
        {
            BSPPreviewActor->Destroy();
            BSPPreviewActor = nullptr;
        }

        // �α��� ȭ�� �ٽ� ǥ��
        if (MainSwitcher)
        {
            MainSwitcher->SetVisibility(ESlateVisibility::Visible);
        }
        ShowAuthScreen(); // �Է¸��/Ŀ�� ���� ����.

        bBSPPreviewActive = false;
    }
}