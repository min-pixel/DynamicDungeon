// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonInstance.h"

UDynamicDungeonInstance ::UDynamicDungeonInstance
()
{
    // �⺻�� ����
    bCanInteract = false;
}

void UDynamicDungeonInstance::SetCanInteract(bool NewState)
{
    bCanInteract = NewState;
}