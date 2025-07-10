// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonInstance.h"
#include "SocketManager.h"

UDynamicDungeonInstance ::UDynamicDungeonInstance
()
{
    // 기본값 설정
    bCanInteract = false;
    LobbyGold = 1000;
}

void UDynamicDungeonInstance::Init()
{
    Super::Init();

    //// 서브시스템 얻기
    //USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    //if (SocketMgr)
    //{
    //    // 로컬 테스트용으로 127.0.0.1:3000 에 연결 시도
    //    bool bConnected = SocketMgr->Connect(TEXT("127.0.0.1"), 3000);
    //    UE_LOG(LogTemp, Log, TEXT("[SocketTest] Connect: %s"), bConnected ? TEXT("Success") : TEXT("Fail"));

    //    // Dynamic Multicast Delegate 바인딩 (람다 대신 UFUNCTION)
    //    SocketMgr->OnDataReceived.AddDynamic(this, &UDynamicDungeonInstance::OnSocketDataReceived);
    //}
    //else
    //{
    //    UE_LOG(LogTemp, Error, TEXT("[SocketTest] SocketManager 서브시스템을 찾을 수 없습니다!"));
    //}

    // SocketManager 연결은 AuthManager에서만 담당하도록 변경
    // 여기서는 연결하지 않고, 필요시에만 참조만 얻어옴
    USocketManager* SocketMgr = GetSubsystem<USocketManager>();
    if (SocketMgr)
    {
        UE_LOG(LogTemp, Log, TEXT("[DynamicDungeonInstance] SocketManager found, but not connecting here"));

        // 데이터 수신 이벤트만 바인딩 (AuthManager와 공유)
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
    CurrentCharacterData.PlayerName = TEXT("DefaultName"); // 나중에 입력받게 해도 됨

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

    // 빈 인벤토리 / 장비 초기화
    CurrentCharacterData.InventoryItems.Empty();
    CurrentCharacterData.EquippedItems.Empty();
}


