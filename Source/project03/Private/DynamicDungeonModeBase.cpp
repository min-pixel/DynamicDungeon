// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"


ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ Ŭ������ StaticClass() ��� (�������Ʈ�� �ƴϹǷ� ��� ã�� ���ʿ�)
	//DefaultPawnClass = AMyDCharacter::StaticClass();

	DefaultPawnClass = nullptr;

	//�÷��̾� ��Ʈ�ѷ��� ������ ������ �� �����Ƿ� ��������� ����
	PlayerControllerClass = APlayerController::StaticClass();
}

