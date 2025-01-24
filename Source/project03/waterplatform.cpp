#include "waterplatform.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MovingPlatform02.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

// Sets default values
Awaterplatform::Awaterplatform()
{
    // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    bIsRising = false;

    PrimaryActorTick.bCanEverTick = true;
    bIsRising = false;
    TargetZ = 0.0f;  // 초기 목표 높이를 0으로 설정

    // plane1024 메쉬 컴포넌트 추가
    PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
    RootComponent = PlaneMesh;

    // plane1024 메쉬를 적용
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneVisualAsset(TEXT("/Game/BP/plane1024.plane1024"));
    if (PlaneVisualAsset.Succeeded())
    {
        PlaneMesh->SetStaticMesh(PlaneVisualAsset.Object);
    }

    // simplewater 머티리얼 설정
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterialAsset(TEXT("/Game/BP/simplewater02"));
    if (WaterMaterialAsset.Succeeded())
    {
        SimpleWaterMaterial = WaterMaterialAsset.Object;
        PlaneMesh->SetMaterial(0, SimpleWaterMaterial);
    }

    // 콜리전 박스 컴포넌트 추가
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    CollisionBox->InitBoxExtent(FVector(55.0f, 55.0f, 0.5f));

    // 콜리전 박스의 위치를 올리기 (상대 위치 조정)
    CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));  // Z축 방향으로 25 유닛만큼 올림

    CollisionBox->SetCollisionProfileName("OverlapAllDynamic");
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &Awaterplatform::OnOverlapBegin);
}

// Called when the game starts or when spawned
void Awaterplatform::BeginPlay()
{
    Super::BeginPlay();

    // 플랫폼의 초기 위치 저장
    InitialLocation = GetActorLocation();
    StartLocation = GetActorLocation();
}

// Called every frame
void Awaterplatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsRising)
    {
        FVector CurrentLocation = GetActorLocation();
        CurrentLocation.Z += RiseSpeed * DeltaTime;  // 매 프레임마다 Z축으로 RiseSpeed 만큼 이동

        // 목표 위치를 넘지 않도록 체크
        if (CurrentLocation.Z >= TargetZ)
        {
            CurrentLocation.Z = TargetZ;  // 목표 높이에 도달하면 멈춤
            bIsRising = false;  // 목표 높이에 도달했으니 더 이상 상승하지 않음
        }

        SetActorLocation(CurrentLocation);  // 물체 위치 업데이트
    }
}

// AMovingPlatform02와 플레이어가 충돌할 때 Awaterplatform이 위로 올라가도록 설정
void Awaterplatform::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherActor->IsA(ACharacter::StaticClass()))
    {
        // 충돌 시 게임 종료
        Destroy();
        UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetPause(true);
        UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
    }
}

// 물체가 서서히 올라가도록 하는 함수
void Awaterplatform::StartRising(float HeightIncrement, float Duration)
{
    // 이전 목표 높이에 HeightIncrement를 더해서 새로운 목표 높이를 설정
    TargetZ += HeightIncrement;  // 매번 충돌할 때마다 목표 높이를 추가
    RiseSpeed = HeightIncrement / Duration;  // 목표 높이까지 Duration 초 동안 올라가도록 계산
    bIsRising = true;  // 물체가 상승하도록 플래그 설정
}
