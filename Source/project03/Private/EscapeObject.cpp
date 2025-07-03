// Fill out your copyright notice in the Description page of Project Settings.


#include "EscapeObject.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyDCharacter.h"


// Sets default values
AEscapeObject::AEscapeObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    RootComponent = TriggerVolume;
    TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
    TriggerVolume->SetBoxExtent(FVector(100.f, 100.f, 100.f));

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    MeshComp->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
    if (MeshAsset.Succeeded())
    {
        MeshComp->SetStaticMesh(MeshAsset.Object);
        MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
        MeshComp->SetWorldScale3D(FVector(1.0f));
    }

    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AEscapeObject::OnOverlapBegin);
    TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AEscapeObject::OnOverlapEnd);

    bReplicates = true;
    bAlwaysRelevant = true;

    Tags.Add(FName("Escape"));

}

// Called when the game starts or when spawned
void AEscapeObject::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEscapeObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEscapeObject::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player)
    {
        MeshComp->SetRenderCustomDepth(true);
        Player->OverlappedActor = this;
    }
}

void AEscapeObject::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    MeshComp->SetRenderCustomDepth(false);
}