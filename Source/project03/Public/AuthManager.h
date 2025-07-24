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

// 서버에서 받은 캐릭터 데이터 구조체
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


    // 기본 생성자
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

// 기존 이벤트에 캐릭터 데이터 추가
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAuthResponse, bool, bSuccess, FCharacterLoginData, CharacterData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveDataResponse, bool, bSuccess);

UCLASS()
class PROJECT03_API UAuthManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()


public:
    // 서브시스템 초기화/해제 - 게임 시작/종료 시 한 번씩만 호출
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 로그인/회원가입 요청
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void Login(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Auth")
    void RegisterUser(const FString& Username, const FString& Password);

    //데이터 저장 요청
    UFUNCTION(BlueprintCallable, Category = "Auth")
    void SaveGameData(int32 CharacterId, const FString& JsonData);

    //데이터 저장 완료 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnSaveDataResponse OnSaveDataResponse;

    // 성공/실패 + 캐릭터 데이터 알림
    UPROPERTY(BlueprintAssignable, Category = "Auth")
    FOnAuthResponse OnAuthResponse;

     // 서버 설정 정보
    FString ServerIP;
    int32 ServerPort;
    
    // 설정 파일에서 서버 정보 로드
    void LoadServerConfig();
    
    // 기본 설정 파일 생성
    void CreateDefaultConfig();
private:
    // 소켓 매니저 참조
    class USocketManager* SocketMgr = nullptr;

    // 연결 상태 추적
    bool bIsConnected = false;

    // 연결 상태 확인 및 필요시 재연결
    bool EnsureConnection();

    // 소켓으로부터 데이터 받을 때 - 이제 JSON 파싱 포함
    UFUNCTION()
    void HandleSocketData(const TArray<uint8>& Data);

    // JSON 문자열을 FCharacterLoginData로 파싱
    FCharacterLoginData ParseCharacterData(const FString& JsonString);

    // (옵션은 스킵으로 MD5 해시만 수행)
    FString HashPassword(const FString& Password) const;
};