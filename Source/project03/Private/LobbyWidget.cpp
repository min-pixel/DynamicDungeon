// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "Components/Button.h"
#include "MyDCharacter.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartGameButton)
    {
        StartGameButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);
    }

    if (GoToShopButton)
    {
        GoToShopButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnGoToShopClicked);
    }
}

void ULobbyWidget::InitializeLobby(AMyDCharacter* Player)
{
    PlayerCharacter = Player;
    // �ʱ� ���� �ʿ� �� ���⿡ �ۼ�
}

void ULobbyWidget::OnStartGameClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Start Game button clicked"));

    // ���� �غ� ó��
    // ��: �÷��̾� ���� ����, ������ �˸� ��
}

void ULobbyWidget::OnGoToShopClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Go to Shop button clicked"));

    // ���� UI ���� ���� ó��
}