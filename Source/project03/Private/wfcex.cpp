// Fill out your copyright notice in the Description page of Project Settings.


#include "wfcex.h"
#include "Engine/World.h"
#include "Editor.h" 

// Sets default values
Awfcex::Awfcex()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void Awfcex::BeginPlay()
{
	Super::BeginPlay();
	ExecuteWFCInSubsystem(60, 0);
}

// Called every frame
void Awfcex::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void Awfcex::ExecuteWFCInSubsystem(int32 TryCount, int32 RandomSeed)
{
    // WFC 서브시스템 가져오기
    UWaveFunctionCollapseSubsystem02* WFCSubsystem = GetWFCSubsystem();

    if (WFCSubsystem)
    {
        WFCSubsystem->ExecuteWFC(TryCount, RandomSeed);
        
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("WFCSubsystemNO"));
    }
}

UWaveFunctionCollapseSubsystem02* Awfcex::GetWFCSubsystem()
{
    // 현재 월드의 GameInstance를 통해 서브시스템 가져오기
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UWaveFunctionCollapseSubsystem02>();
        }
    }

    UE_LOG(LogTemp, Error, TEXT("WFCSubsystemNOS!"));
    return nullptr;
}