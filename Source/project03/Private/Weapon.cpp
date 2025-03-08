// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"

// �⺻ ������
AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;

    // ���� �޽� ����
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"));
    if (MeshAsset.Succeeded())
    {
        WeaponMesh->SetStaticMesh(MeshAsset.Object);
    }

    // �ݸ��� �ڽ� �߰�
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    CollisionBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f)); // ũ�� ����
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnOverlapBegin);

    // ���� ���� (�⺻ ����)
    WeaponMesh->SetSimulatePhysics(true);
    WeaponMesh->SetEnableGravity(true);
    WeaponMesh->SetMassOverrideInKg(NAME_None, Weight); // ���� ����
}

// ���� ���� �� ȣ��
void AWeapon::BeginPlay()
{
    Super::BeginPlay();
}

// �� ������ ȣ��
void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// ������ ���� �Լ�
void AWeapon::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player)
    {
        UE_LOG(LogTemp, Log, TEXT("Player overlapped with weapon: %s"), *GetName());
        Player->SetOverlappingWeapon(this);  // �÷��̾�� ���� ���⸦ ����
    }
}

// ������ ���� ���� �Լ�
void AWeapon::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player && Player->GetOverlappingWeapon() == this)
    {
        Player->SetOverlappingWeapon(nullptr);
        UE_LOG(LogTemp, Log, TEXT("Weapon overlap ended: %s"), *GetName());
    }
}

// �÷��̾ ���⸦ �����ϴ� �Լ�
//void AWeapon::AttachToPlayer(AMyDCharacter* Player)
//{
//    if (Player)
//    {
//        UE_LOG(LogTemp, Log, TEXT("Weapon attached to player: %s"), *GetName());
//
//        // �� ���� ��ġ ��������
//        if (Player->GetMesh()->DoesSocketExist(FName("hand_r")))
//        {
//            UE_LOG(LogTemp, Log, TEXT("Socket hand_r exists!"));
//        }
//        else
//        {
//            UE_LOG(LogTemp, Error, TEXT("Socket hand_r NOT found! Check Skeleton settings."));
//            //return; // ������ ������ �������� ����
//        }
//
//        // �� ������ ��ġ�� �����ͼ� ��ġ ����
//        FTransform HandSocketTransform = Player->GetMesh()->GetSocketTransform(FName("hand_r"));
//
//        UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());
//
//        SetActorTransform(HandSocketTransform, false, nullptr, ETeleportType::TeleportPhysics);
//
//        // �� ���Ͽ� ����
//        FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
//        AttachToComponent(Player->GetMesh(), AttachRules, FName("hand_r"));
//
//        // ���� ũ�� ���� (��: 2��� Ű���)
//        SetActorScale3D(FVector(0.25f, 0.25f, 1.0f));
//
//        // ���⸦ ��Ȱ��ȭ (�� �̻� �ٴڿ� �������� �ʵ���)
//        SetActorEnableCollision(false);
//        //SetActorHiddenInGame(true);
//    }
//}
