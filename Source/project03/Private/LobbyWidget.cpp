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

    // AuthManager ��������
    AuthMgr = GetGameInstance()->GetSubsystem<UAuthManager>();
    if (AuthMgr)
    {
        AuthMgr->OnAuthResponse.AddDynamic(this, &ULobbyWidget::OnAuthResponse);
    }

    // ========== ���� ȭ�� ��ư ���ε� ==========
    if (LoginButton)
        LoginButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLoginClicked);

    if (RegisterButton)
        RegisterButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnRegisterClicked);

    // ========== ���� �κ� ��ư ���ε� ==========
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

    // GameInstance�� ���� ���� ����
    UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
    if (GameInstance)
    {
        GameInstance->LobbyWidgetInstance = this;
    }

    // �ʱ� ����: ���� ȭ�� ǥ��
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
    if (MainSwitcher)
    {
        MainSwitcher->SetActiveWidgetIndex(1); // �ε��� 1: ���� �κ�
        UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Showing main lobby screen"));
    }

    // ���� �κ� �ʱ�ȭ
    InitializeLobby(PlayerCharacter);
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

            bIsAuthenticated = true;

            // ���� ĳ���� �����͸� GameInstance�� ����
            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
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
        EquipmentWidgetInstance->AddToViewport(1);
        EquipmentWidgetInstance->RefreshEquipmentSlots();
    }

    // (6) ��� ���� ������Ʈ
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

    // ���� â �����
    //if (InventoryWidgetInstance) InventoryWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
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