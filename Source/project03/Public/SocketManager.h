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

 // ��׶��� ���� ������
class FSocketReceiver : public FRunnable
{
public:
    FSocketReceiver(FSocket* InSocket, class USocketManager* InOwner);
    virtual ~FSocketReceiver();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    /** �����带 ���� ��û�ϰ�, ������ ��� �������� ������ ��� */
    void Shutdown()
    {
        Stop();                                   // bStopThread = true
        if (Thread)
        {
            Thread->WaitForCompletion();          // FRunnableThread �� �޼���
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
    // ����ý��� �ʱ�ȭ/����
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ������ TCP ���� �õ�
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Connect(const FString& Address, int32 Port);

    // ���� ����
    UFUNCTION(BlueprintCallable, Category = "Network")
    void Disconnect();

    // ���� ���� Ȯ��
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool IsConnected() const;

    // ����Ʈ �迭 �۽�
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool Send(const TArray<uint8>& Data);

    // ���� ������ �̺�Ʈ
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnDataReceived OnDataReceived;


    // ���� ���� ���� �� ȣ��
    void OnConnectionLost();

   
    FSocket* Socket = nullptr;


private:
    
    FSocketReceiver* ReceiverRunnable = nullptr;
    bool bIsConnected = false;

protected:
    bool AcceptConnections(float DeltaTime);
    FDelegateHandle AcceptTickHandle;

};
