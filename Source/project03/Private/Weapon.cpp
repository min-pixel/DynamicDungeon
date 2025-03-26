// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "MyDCharacter.h"
#include "Kismet/GameplayStatics.h"

// 기본 생성자
AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;

    // 무기 메쉬 생성
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;

    BaseDamage = 20.0f;
    Damage = BaseDamage; // 무기 생성 시 기본 데미지 적용

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder"));
    if (MeshAsset.Succeeded())
    {
        WeaponMesh->SetStaticMesh(MeshAsset.Object);
    }

    // 콜리전 박스 추가
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    CollisionBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f)); // 크기 조정
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnOverlapBegin);

    // 물리 적용 (기본 상태)
    WeaponMesh->SetSimulatePhysics(true);
    WeaponMesh->SetEnableGravity(true);
    WeaponMesh->SetMassOverrideInKg(NAME_None, Weight); // 무게 적용



}

// 게임 시작 시 호출
void AWeapon::BeginPlay()
{
    Super::BeginPlay();
}

// 매 프레임 호출
void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// 오버랩 감지 함수
void AWeapon::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    AMyDCharacter* Player = Cast<AMyDCharacter>(OtherActor);
    if (Player)
    {
        UE_LOG(LogTemp, Log, TEXT("Player overlapped with weapon: %s"), *GetName());
        Player->SetOverlappingWeapon(this);  // 플레이어에게 현재 무기를 전달
    }
}

// 오버랩 종료 감지 함수
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

void AWeapon::TraceAttack()
{
    if (!WeaponMesh) return;

    // 현재 프레임의 위치
    FVector CurrentStart = WeaponMesh->GetSocketLocation(FName("AttackStart"));
    FVector CurrentEnd = WeaponMesh->GetSocketLocation(FName("AttackEnd"));

    AMyDCharacter* OwnerCharacter = Cast<AMyDCharacter>(Owner);
    if (!OwnerCharacter) return;

    AActor* OwnerActor = GetOwner(); // 캐릭터 가져오기

    // 트레이스
    TArray<FHitResult> HitResults;
    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(this);
    if (OwnerActor)
    {
        IgnoredActors.Add(OwnerActor);
    }


    // Start 소켓 기준 선형 궤적 판정
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

    // End 소켓 기준 선형 궤적 판정
    UKismetSystemLibrary::SphereTraceMulti(
        GetWorld(),
        LastEndLocation,
        CurrentEnd,
        15.0f,
        UEngineTypes::ConvertToTraceType(ECC_Pawn),
        false,
        IgnoredActors,
        EDrawDebugTrace::ForDuration,
        HitResults,
        true
    );

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->Implements<UHitInterface>())
        {
            if (!OwnerCharacter->HitActors.Contains(HitActor)) // 중복 피격 방지
            {
                OwnerCharacter->HitActors.Add(HitActor);
                IHitInterface::Execute_GetHit(HitActor, Hit, OwnerCharacter, Damage);
            }
        }
    }

    // 위치 갱신
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