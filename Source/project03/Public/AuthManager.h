// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Misc/SecureHash.h"
#include "AuthManager.generated.h"

/**
 *
 */

UENUM(BlueprintType)
enum class EAuthRequestType : uint8
{
    Login = 0,
    Register = 1,
    SaveData = 2
};

// �������� ���� ĳ���� ������ ����ü
USTRUCT(BlueprintType)
struct FCharacterLoginData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 CharacterId = 0;

    UPROPERTY(BlueprintReadWrite)
    FString CharacterName = TEXT("DefaultName");

    UPROPERTY(BlueprintReadWrite)
    int32 PlayerClass = 0;  // 0=Mage, 1=Warrior, 2=Rogue

    UPROPERTY(BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(BlueprintReadWrite)
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxStamina = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    float MaxKnowledge = 100.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 Gold = 0;


    // �⺻ ������
    FCharacterLoginData()
    {
        CharacterId = 0;
        CharacterName = TEXT("DefaultName");
        PlayerClass = 0;
        Level = 1;
        MaxHealth = 100.0f;
        MaxStamina = 100.0f;
        MaxKnowledge = 100.0f;
        Gold = 0;


    }
};

// ���� �̺�Ʈ�� ĳ���� ������ �߰�
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthResponse, bool, bSuccess, FCharacterLoginData, CharacterData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveDataResponse, bool, bSuccess);

UCLASS()
class PROJECT03_API UAuthManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()


public:
    // ����ý��� �ʱ�ȭ/���� - ���� ����/���� �� �� ������ ȣ��
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // �α���/ȸ������ ��û
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void Login(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Auth")
    void RegisterUser(const FString& Username, const FString& Password);

    //������ ���� ��û
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void SaveGameData(int32 CharacterId, const FString& JsonData);

    //������ ���� �Ϸ� �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnSaveDataResponse OnSaveDataResponse;

    // ����/���� + ĳ���� ������ �˸�
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnAuthResponse OnAuthResponse;

     // ���� ���� ����
    FString ServerIP;
    int32 ServerPort;
    
    // ���� ���Ͽ��� ���� ���� �ε�
    void LoadServerConfig();
    
    // �⺻ ���� ���� ����
    void CreateDefaultConfig();
private:
    // ���� �Ŵ��� ����
    class USocketManager* SocketMgr = nullptr;

    // ���� ���� ����
    bool bIsConnected = false;

    // ���� ���� Ȯ�� �� �ʿ�� �翬��
    bool EnsureConnection();

    // �������κ��� ������ ���� �� - ���� JSON �Ľ� ����
    UFUNCTION()
    void HandleSocketData(const TArray<uint8>& Data);

    // JSON ���ڿ��� FCharacterLoginData�� �Ľ�
    FCharacterLoginData ParseCharacterData(const FString& JsonString);

    // (�ɼ��� ��ŵ���� MD5 �ؽø� ����)
    FString HashPassword(const FString& Password) const;
};