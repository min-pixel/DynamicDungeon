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

    // GameInstance �ʱ�ȭ �� ����
    virtual void Init() override;

    // SocketManager�� OnDataReceived�� ���� �Լ�
    UFUNCTION()
    void OnSocketDataReceived(const TArray<uint8>& Data);

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
    int32 LobbyGold = 1000;

    FPlayerCharacterData CurrentCharacterData;

    void InitializeCharacterData(EPlayerClass SelectedClass);

    UFUNCTION(BlueprintCallable, Category = "Gold")
    int32 GetLobbyGold() const { return LobbyGold; }

    UPROPERTY()
    bool bIsReturningFromGame = false;

    UPROPERTY()
    bool bHasValidCharacterData = false;

    // ���� ���� �� ������ ����
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveAllDataToServer();

    // JSON ���·� ��� ���� ������ ����
    FString CollectAllGameData();

    UPROPERTY()
    class ULobbyWidget* LobbyWidgetInstance;


    void OnPreLoadMap(const FString& MapName);
    void OnPostLoadMap(UWorld* LoadedWorld);
   
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveDataAndShutdown();

    // ���� ���� ����
    UPROPERTY()
    bool bIsSaving = false;

    UPROPERTY()
    bool bSaveCompleted = false;

    // ���� �Ϸ� �ݹ�
    UFUNCTION()
    void OnSaveDataCompleted(bool bSuccess);

    // ������ ���� ó��
    void SafeShutdown();

    void WaitForSaveCompletion();

    bool bShutdownRequested = false;

    // ������ �⺻ ������ ��ȯ�ϴ� �Լ�
    UFUNCTION(BlueprintCallable)
    void GetBaseStatsForClass(EPlayerClass PlayerClass, float& OutHealth, float& OutStamina, float& OutKnowledge);

    // ���� ���� �⺻ ���� + ���ʽ� �������� ���� ���� ���
    UFUNCTION(BlueprintCallable)
    void RecalculateStats();

    bool bPlydie = false;

private:
    // ���� ��� Ÿ�̸�
    FTimerHandle SaveWaitTimer;

    FTimerHandle CheckSaveTimer;

    // �ִ� ���� ��� �ð� (��)
    float MaxSaveWaitTime = 5.0f;

};
