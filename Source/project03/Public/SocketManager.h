// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/Runnable.h"
//#include "IPv4Address.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketManager.generated.h"

/**
 * 
 */

 // 백그라운드 수신 쓰레드
class FSocketReceiver : public FRunnable
{
public:
    FSocketReceiver(FSocket* InSocket, class USocketManager* InOwner);
    virtual ~FSocketReceiver();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    /** 쓰레드를 종료 요청하고, 안전히 모두 빠져나올 때까지 대기 */
    void Shutdown()
    {
        Stop();                                   // bStopThread = true
        if (Thread)
        {
            Thread->WaitForCompletion();          // FRunnableThread 의 메서드
            delete Thread;
            Thread = nullptr;
        }
    }

private:
    FSocket* Socket;
    USocketManager* Owner;
    FThreadSafeBool bStopThread;
    FRunnableThread* Thread;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDataReceived, const TArray<uint8>&, Data);

UCLASS()
class PROJECT03_API USocketManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    // 서브시스템 초기화/해제
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 서버에 TCP 연결 시도
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Connect(const FString& Address, int32 Port);

    // 연결 해제
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Disconnect();

    // 연결 상태 확인
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool IsConnected() const;

    // 바이트 배열 송신
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Send(const TArray<uint8>& Data);

    // 수신 데이터 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnDataReceived OnDataReceived;


    // 연결 끊김 감지 시 호출
    void OnConnectionLost();

   
    FSocket* Socket = nullptr;


private:
    
    FSocketReceiver* ReceiverRunnable = nullptr;
    bool bIsConnected = false;

protected:
    bool AcceptConnections(float DeltaTime);
    FDelegateHandle AcceptTickHandle;

};
