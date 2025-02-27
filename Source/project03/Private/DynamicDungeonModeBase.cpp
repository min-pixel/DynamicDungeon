// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"

ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ Ŭ������ StaticClass() ��� (�������Ʈ�� �ƴϹǷ� ��� ã�� ���ʿ�)
	DefaultPawnClass = AMyDCharacter::StaticClass();

	//�÷��̾� ��Ʈ�ѷ��� ������ ������ �� �����Ƿ� ��������� ����
	PlayerControllerClass = APlayerController::StaticClass();
}

