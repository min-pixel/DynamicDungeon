// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"


ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ Ŭ������ StaticClass() ��� (�������Ʈ�� �ƴϹǷ� ��� ã�� ���ʿ�)
	//DefaultPawnClass = AMyDCharacter::StaticClass();

	DefaultPawnClass = nullptr;

	//�÷��̾� ��Ʈ�ѷ��� ������ ������ �� �����Ƿ� ��������� ����
	PlayerControllerClass = APlayerController::StaticClass();
}

void ADynamicDungeonModeBase::BeginPlay()
{
	Super::BeginPlay(); // �ݵ�� ȣ��

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = false; // ���콺 Ŀ�� �����
		FInputModeGameOnly InputMode; // ���� ���� �Է� ���
		PC->SetInputMode(InputMode);
	}
}
