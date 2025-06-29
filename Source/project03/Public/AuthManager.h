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

    // 로그인/회원가입 요청
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void Login(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Auth")
    void RegisterUser(const FString& Username, const FString& Password);

    // 성공(true)/실패(false) 알림
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnAuthResponse OnAuthResponse;

private:
    // 소켓 매니저 참조
    class USocketManager* SocketMgr = nullptr;

    // 소켓으로부터 데이터 받았을 때
    UFUNCTION()
    void HandleSocketData(const TArray<uint8>& Data);

    // (오늘은 스텁으로 MD5 해시만 수행)
    FString HashPassword(const FString& Password) const;
};
