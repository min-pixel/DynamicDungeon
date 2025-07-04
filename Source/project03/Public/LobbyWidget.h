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

    UPROPERTY(meta = (BindWidget))
    UButton* StartGameButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GoToShopButton;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UInventoryWidget> StorageWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
    TSubclassOf<UEquipmentWidget> EquipmentWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UGoldWidget> GoldWidgetClass;

    UPROPERTY(meta = (BindWidget))
    UGoldWidget* GoldWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* InventoryWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UInventoryWidget* StorageWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    UEquipmentWidget* EquipmentWidgetInstance;

    UPROPERTY()
    UInventoryComponent* StorageInventoryComponent;

    UPROPERTY()
    UInventoryComponent* InventoryComponentRef;

    UPROPERTY()
    UInventoryComponent* StorageComponentRef;

    TArray<EPlayerClass> AvailableClasses = { EPlayerClass::Warrior, EPlayerClass::Rogue, EPlayerClass::Mage };
    int32 CurrentClassIndex = 0;

    UPROPERTY(meta = (BindWidget))
    UButton* LeftArrowButton;

    UPROPERTY(meta = (BindWidget))
    UButton* RightArrowButton;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UShopWidget> ShopWidgetClass;

    UPROPERTY()
    class UShopWidget* ShopWidgetInstance;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ClassText;

    // 로그인·회원가입용 입력창 & 버튼
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* UsernameTextBox;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordTextBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    UButton* RegisterButton;

    UFUNCTION()
    void OnLeftArrowClicked();

    UFUNCTION()
    void OnRightArrowClicked();

    void UpdateClassDisplay();

    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnGoToShopClicked();

    /*UPROPERTY(meta = (BindWidget))
    UButton* StartGameButton;

    UPROPERTY(meta = (BindWidget))
    UButton* GoToShopButton;*/

    UInventoryWidget* GetInventoryWidget() const { return InventoryWidgetInstance; }
    UEquipmentWidget* GetEquipmentWidget() const { return EquipmentWidgetInstance; }

    UFUNCTION(BlueprintCallable)
    void OnCloseShopButtonClicked();

    UPROPERTY(meta = (BindWidget))
    UButton* CloseShopButton;

    UPROPERTY()
    AMyDCharacter* PlayerCharacter;


    // 마지막에 호출한 요청 타입을 기억 (true=Register, false=Login)
    bool bWasRegister = false;

        // AuthManager 참조
        UAuthManager* AuthMgr = nullptr;

        // 버튼 클릭 핸들러
        UFUNCTION()
        void OnLoginClicked();

        UFUNCTION()
        void OnRegisterClicked();

        // 인증 결과 콜백
        UFUNCTION()
        void OnAuthResponse(bool bSuccess);
};
