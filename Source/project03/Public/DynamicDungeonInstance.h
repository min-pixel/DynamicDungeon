// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ItemDataD.h"
#include "PlayerCharacterData.h"
#include "DynamicDungeonInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UDynamicDungeonInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    // ���� ���� (�������Ʈ���� ����� ����)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game State")
    bool bCanInteract;

    // ������
    UDynamicDungeonInstance();

    // �������Ʈ������ ���� �����ϵ��� �Լ� �߰�
    UFUNCTION(BlueprintCallable, Category = "Game State")
    void SetCanInteract(bool NewState);

    //�÷��̾ ��ȣ�ۿ� ������ ��Ÿ���� ����
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool bIsInteracting = false;

    /** �������Ʈ���� ����� ��ȣ�ۿ� ���� */
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool itemEAt; // ������ ��ȣ�ۿ� ����

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool OpenDoor; // �� ���� ����

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Interaction")
    bool WeaponEAt; // ���� ��ȣ�ۿ� ����
	
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedInventoryItems;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedEquipmentItems;  

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "SaveData")
    TArray<FItemData> SavedStorageItems;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 LobbyGold = 10340;

    FPlayerCharacterData CurrentCharacterData;

    void InitializeCharacterData(EPlayerClass SelectedClass);

    UFUNCTION(BlueprintCallable, Category = "Gold")
    int32 GetLobbyGold() const { return LobbyGold; }

    UPROPERTY()
    class ULobbyWidget* LobbyWidgetInstance;

};
