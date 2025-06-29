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
	Register = 1
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAuthResponse, bool, bSuccess);

UCLASS()
class PROJECT03_API UAuthManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	

public:
    //virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // �α���/ȸ������ ��û
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void Login(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Auth")
    void RegisterUser(const FString& Username, const FString& Password);

    // ����(true)/����(false) �˸�
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnAuthResponse OnAuthResponse;

private:
    // ���� �Ŵ��� ����
    class USocketManager* SocketMgr = nullptr;

    // �������κ��� ������ �޾��� ��
    UFUNCTION()
    void HandleSocketData(const TArray<uint8>& Data);

    // (������ �������� MD5 �ؽø� ����)
    FString HashPassword(const FString& Password) const;
};
