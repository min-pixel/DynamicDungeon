// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuthManager.h"
#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "EquipmentWidget.h"
#include "Components/TextBlock.h"
#include "PlayerCharacterData.h" 
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/EditableTextBox.h"
#include "LobbyWidget.generated.h"

/**
 *
 */
UCLASS()
class PROJECT03_API ULobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void InitializeLobby(class AMyDCharacter* PlayerRef);

    virtual void NativeConstruct() override;

    // UI ���� ��ȯ�� ���� ����ó
    UPROPERTY(meta = (BindWidget))
    class UWidgetSwitcher* MainSwitcher;

    // ========== ���� ȭ�� UI ��ҵ� ==========
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* UsernameTextBox;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordTextBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    UButton* RegisterButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* AuthStatusText;

    // ========== ���� �κ� ȭ�� UI ��ҵ� ==========
    UPROPERTY(meta = (BindWidget))
    UButton* StartGameButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GoToShopButton;

    UPROPERTY(meta = (BindWidget))
    UButton* CloseShopButton;

    UPROPERTY(meta = (BindWidget))
    UButton* LeftArrowButton;

    UPROPERTY(meta = (BindWidget))
    UButton* RightArrowButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ClassText;

    UPROPERTY(meta = (BindWidget))
    class UGoldWidget* GoldWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* InventoryWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* StorageWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UEquipmentWidget* EquipmentWidgetInstance;

    // ========== Ŭ���� ���ǵ� ==========
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> StorageWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UEquipmentWidget> EquipmentWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UGoldWidget> GoldWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UShopWidget> ShopWidgetClass;

    // ========== ������Ʈ �� ������ ==========
    UPROPERTY()
    UInventoryComponent* StorageInventoryComponent;

    UPROPERTY()
    UInventoryComponent* InventoryComponentRef;

    UPROPERTY()
    UInventoryComponent* StorageComponentRef;

    UPROPERTY()
    class UShopWidget* ShopWidgetInstance;

    UPROPERTY()
    AMyDCharacter* PlayerCharacter;

    TArray<EPlayerClass> AvailableClasses = { EPlayerClass::Mage, EPlayerClass::Warrior, EPlayerClass::Rogue };
    int32 CurrentClassIndex = 0;

    // ========== ���� ���� ==========
    bool bWasRegister = false;
    bool bIsAuthenticated = false;
    UAuthManager* AuthMgr = nullptr;

    // ========== �Լ��� ==========
    // ���� ȭ�� �Լ���
    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnRegisterClicked();

    UFUNCTION()
    void OnAuthResponse(bool bSuccess, FCharacterLoginData CharacterData);

    // ���� �κ� �Լ���
    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnGoToShopClicked();

    UFUNCTION()
    void OnCloseShopButtonClicked();

    UFUNCTION()
    void OnLeftArrowClicked();

    UFUNCTION()
    void OnRightArrowClicked();

    void UpdateClassDisplay();

    // UI ���� ��ȯ
    UFUNCTION(BlueprintCallable)
    void ShowAuthScreen();

    UFUNCTION(BlueprintCallable)
    void ShowMainLobby();

    UFUNCTION(BlueprintCallable)
    void SetAuthStatusText(const FString& StatusMessage);

    // ��ƿ��Ƽ �Լ���
    UInventoryWidget* GetInventoryWidget() const { return InventoryWidgetInstance; }
    UEquipmentWidget* GetEquipmentWidget() const { return EquipmentWidgetInstance; }

    // ========== ���� ���׷��̵� ���� ==========
    // ���� ���׷��̵� ���
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 HealthUpgradeCost = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 StaminaUpgradeCost = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 KnowledgeUpgradeCost = 100;

    // ���� ������
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float HealthUpgradeAmount = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float StaminaUpgradeAmount = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float KnowledgeUpgradeAmount = 10.0f;

    // ���� ���׷��̵� �Լ���
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeHealth();

    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeStamina();

    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeKnowledge();

    // ���� ���� ǥ�ÿ� �Լ�
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    FString GetCurrentStats() const;

    // ��� ������� üũ
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    bool CanAffordUpgrade(int32 Cost) const;

    // UI ��ҵ� (�������Ʈ���� ���ε�)
    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeHealthButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeStaminaButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeKnowledgeButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* CurrentStatsText;

    // ��ư Ŭ�� �̺�Ʈ
    UFUNCTION()
    void OnUpgradeHealthClicked();

    UFUNCTION()
    void OnUpgradeStaminaClicked();

    UFUNCTION()
    void OnUpgradeKnowledgeClicked();

    // UI ������Ʈ
    void UpdateStatsDisplay();
    void UpdateGoldDisplay();

};