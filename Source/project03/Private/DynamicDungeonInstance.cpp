// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonInstance.h"
#include "SocketManager.h"

UDynamicDungeonInstance ::UDynamicDungeonInstance
()
{
    // �⺻�� ����
    bCanInteract = false;
    LobbyGold = 1000;
}

void UDynamicDungeonInstance::Init()
{
    Super::Init();

    //// ����ý��� ���
    //USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    //if (SocketMgr)
    //{
    //    // ���� �׽�Ʈ������ 127.0.0.1:3000 �� ���� �õ�
    //    bool bConnected = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
    //    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Connect: %s"), bConnected ? TEXT("Success") : TEXT("Fail"));

    //    // Dynamic Multicast Delegate ���ε� (���� ��� UFUNCTION)
    //    SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("[SocketTest] SocketManager ����ý����� ã�� �� �����ϴ�!"));
    //}

    // SocketManager ������ AuthManager������ ����ϵ��� ����
    // ���⼭�� �������� �ʰ�, �ʿ�ÿ��� ������ ����
    USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    if (SocketMgr)
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicDungeonInstance] SocketManager found, but not connecting here"));

        // ������ ���� �̺�Ʈ�� ���ε� (AuthManager�� ����)
        if (!SocketMgr->OnDataReceived.IsAlreadyBound(this, &UDynamicDungeonInstance::OnSocketDataReceived))
        {
            SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[DynamicDungeonInstance] SocketManager subsystem not found!"));
    }

}

void UDynamicDungeonInstance::OnSocketDataReceived(const TArray<uint8>& Data)
{
    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Received %d bytes"), Data.Num());
}

void UDynamicDungeonInstance::SetCanInteract(bool NewState)
{
    bCanInteract = NewState;
}

void UDynamicDungeonInstance::InitializeCharacterData(EPlayerClass SelectedClass)
{
    CurrentCharacterData.PlayerClass = SelectedClass;
    CurrentCharacterData.Level = 1;
    CurrentCharacterData.PlayerName = TEXT("DefaultName"); // ���߿� �Է¹ް� �ص� ��

    switch (SelectedClass)
    {
    case EPlayerClass::Warrior:
        CurrentCharacterData.MaxHealth = 150.f;
        CurrentCharacterData.MaxStamina = 100.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Rogue:
        CurrentCharacterData.MaxHealth = 100.f;
        CurrentCharacterData.MaxStamina = 150.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Mage:
        CurrentCharacterData.MaxHealth = 100.f;
        CurrentCharacterData.MaxStamina = 10.f;
        CurrentCharacterData.MaxKnowledge = 200.f;
        break;
    }

    // �� �κ��丮 / ��� �ʱ�ȭ
    CurrentCharacterData.InventoryItems.Empty();
    CurrentCharacterData.EquippedItems.Empty();
}


