// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"


UMaterialInterface* ChromeMaterial = nullptr;
UMaterialInterface* GoldMaterial = nullptr;

// �⺻ ������
AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;


    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ChromeMatFinder(TEXT("/Game/StarterContent/Materials/M_Metal_Chrome.M_Metal_Chrome"));
    if (ChromeMatFinder.Succeeded())
    {
        ChromeMaterial = ChromeMatFinder.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMatFinder(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
    if (GoldMatFinder.Succeeded())
    {
        GoldMaterial = GoldMatFinder.Object;
    }

    // ���� �޽� ����
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;
    ItemType = EItemType::Weapon;
    Price = 300;
    BaseDamage = 20.0f;
    Damage = BaseDamage; // ���� ���� �� �⺻ ������ ����

    /*static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Sword.SM_Sword"));
    if (MeshAsset.Succeeded())
    {
        WeaponMesh->SetStaticMesh(MeshAsset.Object);
    }*/

   

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/free-icon-sword-9078345.free-icon-sword-9078345"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/Weapon_Pack/Mesh/Weapons/Weapons_Kit/SM_Sword.SM_Sword"));
    if (MeshAsset.Succeeded())
    {
        LoadedWeaponMesh = MeshAsset.Object; // ���常
    }

    /*static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/free-icon-sword-9078345.free-icon-sword-9078345"));
    if (IconTexture.Succeeded())
    {
        LoadedIcon = IconTexture.Object;
    }*/

    ItemName = TEXT("Long Sword");

    
}

// ���� ���� �� ȣ��
void AWeapon::BeginPlay()
{
    Super::BeginPlay();
    AMyDCharacter* Character = Cast<AMyDCharacter>(GetOwner());
    ApplyGradeEffects(Character);

    if (LoadedWeaponMesh && WeaponMesh)
    {
        WeaponMesh->SetStaticMesh(LoadedWeaponMesh);
    }

    /*if (LoadedIcon)
    {
        ItemIcon = LoadedIcon;
    }*/


    //// �ݸ��� �ڽ� �߰�
    //CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    //CollisionBox->SetupAttachment(RootComponent);
    //CollisionBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f)); // ũ�� ����
    //CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    //CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    //CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    //CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnOverlapBegin);

    //// ���� ���� (�⺻ ����)
    //WeaponMesh->SetSimulatePhysics(true);
    //WeaponMesh->SetEnableGravity(true);
    //WeaponMesh->SetMassOverrideInKg(NAME_None, Weight); // ���� ����

    

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
        //UE_LOG(LogTemp, Log, TEXT("Player overlapped with weapon: %s"), *GetName());
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
        //UE_LOG(LogTemp, Log, TEXT("Weapon overlap ended: %s"), *GetName());
    }
}

void AWeapon::TraceAttack()
{
    if (!WeaponMesh) return;

    // ���� �������� ��ġ
    FVector CurrentStart = WeaponMesh->GetSocketLocation(FName("AttackStart"));
    FVector CurrentEnd = WeaponMesh->GetSocketLocation(FName("AttackEnd"));

    AMyDCharacter* OwnerCharacter = Cast<AMyDCharacter>(Owner);
    if (!OwnerCharacter) return;

    AActor* OwnerActor = GetOwner(); // ĳ���� ��������

    // Ʈ���̽�
    TArray<FHitResult> HitResults;
    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(this);
    if (OwnerActor)
    {
        IgnoredActors.Add(OwnerActor);
    }


    // Start ���� ���� ���� ���� ����
    /*UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        LastStartLocation,
        CurrentStart,
        15.0f,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoredActors,
        EDrawDebugTrace::ForDuration,
        HitResults,
        true
    );*/

    // End ���� ���� ���� ���� ����
    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        LastEndLocation,
        CurrentEnd,
        15.0f,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoredActors,
        EDrawDebugTrace::None,
        HitResults,
        true
    );

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {
            if (!OwnerCharacter->HitActors.Contains(HitActor)) // �ߺ� �ǰ� ����
            {
                OwnerCharacter->HitActors.Add(HitActor);
                IHitInterface::Execute_GetHit(HitActor, Hit, OwnerCharacter, Damage);
            }
        }
    }

    // ��ġ ����
    LastStartLocation = CurrentStart;
    LastEndLocation = CurrentEnd;
}

void AWeapon::StartTrace()
{
    if (WeaponMesh)
    {
        LastStartLocation = WeaponMesh->GetSocketLocation(FName("AttackStart"));
        LastEndLocation = WeaponMesh->GetSocketLocation(FName("AttackEnd"));
    }
}

void AWeapon::ApplyWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->Stamina += 0.0f;
    }
}

void AWeapon::RemoveWeaponStats(AMyDCharacter* Character)
{
    if (Character)
    {
        Character->Stamina += 0.0f;
    }
}

void AWeapon::ApplyGradeEffects(AMyDCharacter* Character)
{
    switch (WeaponGrade)
    {
    case EWeaponGrade::C:
        break;

    case EWeaponGrade::B:
        BaseDamage += 10.0f;

        Character->AttackStaminaCost -= 5.0f;
        if (WeaponMesh && ChromeMaterial)
        {
            WeaponMesh->SetMaterial(0, ChromeMaterial);
        }
        break;

    case EWeaponGrade::A:
        BaseDamage += 20.0f;
        Character->AttackStaminaCost -= 10.0f;
        if (WeaponMesh && GoldMaterial)
        {
            WeaponMesh->SetMaterial(0, GoldMaterial);
        }
        break;
    }

    Damage = BaseDamage;
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    AMyDCharacter* Character = Cast<AMyDCharacter>(GetOwner());
    // ��� ��ȭ �� ����
    ApplyGradeEffects(Character);
}