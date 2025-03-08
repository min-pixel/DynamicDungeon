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

// 플레이어가 무기를 장착하는 함수
//void AWeapon::AttachToPlayer(AMyDCharacter* Player)
//{
//    if (Player)
//    {
//        UE_LOG(LogTemp, Log, TEXT("Weapon attached to player: %s"), *GetName());
//
//        // 손 소켓 위치 가져오기
//        if (Player->GetMesh()->DoesSocketExist(FName("hand_r")))
//        {
//            UE_LOG(LogTemp, Log, TEXT("Socket hand_r exists!"));
//        }
//        else
//        {
//            UE_LOG(LogTemp, Error, TEXT("Socket hand_r NOT found! Check Skeleton settings."));
//            //return; // 소켓이 없으면 부착하지 않음
//        }
//
//        // 손 소켓의 위치를 가져와서 위치 조정
//        FTransform HandSocketTransform = Player->GetMesh()->GetSocketTransform(FName("hand_r"));
//
//        UE_LOG(LogTemp, Log, TEXT("Socket Location: %s"), *HandSocketTransform.GetLocation().ToString());
//
//        SetActorTransform(HandSocketTransform, false, nullptr, ETeleportType::TeleportPhysics);
//
//        // 손 소켓에 부착
//        FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
//        AttachToComponent(Player->GetMesh(), AttachRules, FName("hand_r"));
//
//        // 무기 크기 조정 (예: 2배로 키우기)
//        SetActorScale3D(FVector(0.25f, 0.25f, 1.0f));
//
//        // 무기를 비활성화 (더 이상 바닥에 존재하지 않도록)
//        SetActorEnableCollision(false);
//        //SetActorHiddenInGame(true);
//    }
//}
