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

    // UI 상태 전환용 위젯 스위처
    UPROPERTY(meta = (BindWidget))
    class UWidgetSwitcher* MainSwitcher;

    // ========== 인증 화면 UI 요소들 ==========
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

    // ========== 메인 로비 화면 UI 요소들 ==========
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

    // ========== 클래스 정의들 ==========
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

    // ========== 컴포넌트 및 데이터 ==========
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

    // ========== 인증 관련 ==========
    bool bWasRegister = false;
    bool bIsAuthenticated = false;
    UAuthManager* AuthMgr = nullptr;

    // ========== 함수들 ==========
    // 인증 화면 함수들
    UFUNCTION()
    void OnLoginClicked();

    UFUNCTION()
    void OnRegisterClicked();

    UFUNCTION()
    void OnAuthResponse(bool bSuccess, FCharacterLoginData CharacterData);

    // 메인 로비 함수들
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

    // UI 상태 전환
    UFUNCTION(BlueprintCallable)
    void ShowAuthScreen();

    UFUNCTION(BlueprintCallable)
    void ShowMainLobby();

    UFUNCTION(BlueprintCallable)
    void SetAuthStatusText(const FString& StatusMessage);

    // 유틸리티 함수들
    UInventoryWidget* GetInventoryWidget() const { return InventoryWidgetInstance; }
    UEquipmentWidget* GetEquipmentWidget() const { return EquipmentWidgetInstance; }

    // ========== 스탯 업그레이드 관련 ==========
    // 스탯 업그레이드 비용
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 HealthUpgradeCost = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 StaminaUpgradeCost = 100;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    int32 KnowledgeUpgradeCost = 100;

    // 스탯 증가량
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float HealthUpgradeAmount = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float StaminaUpgradeAmount = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stat Upgrade")
    float KnowledgeUpgradeAmount = 10.0f;

    // 스탯 업그레이드 함수들
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeHealth();

    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeStamina();

    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    void UpgradeKnowledge();

    // 현재 스탯 표시용 함수
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    FString GetCurrentStats() const;

    // 골드 충분한지 체크
    UFUNCTION(BlueprintCallable, Category = "Stat Upgrade")
    bool CanAffordUpgrade(int32 Cost) const;

    // UI 요소들 (블루프린트에서 바인딩)
    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeHealthButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeStaminaButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* UpgradeKnowledgeButton;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* CurrentStatsText;

    // 버튼 클릭 이벤트
    UFUNCTION()
    void OnUpgradeHealthClicked();

    UFUNCTION()
    void OnUpgradeStaminaClicked();

    UFUNCTION()
    void OnUpgradeKnowledgeClicked();

    // UI 업데이트
    void UpdateStatsDisplay();
    void UpdateGoldDisplay();

};