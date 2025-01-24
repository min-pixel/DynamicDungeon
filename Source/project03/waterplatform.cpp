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
    TargetZ = 0.0f;  // �ʱ� ��ǥ ���̸� 0���� ����

    // plane1024 �޽� ������Ʈ �߰�
    PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
    RootComponent = PlaneMesh;

    // plane1024 �޽��� ����
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneVisualAsset(TEXT("/Game/BP/plane1024.plane1024"));
    if (PlaneVisualAsset.Succeeded())
    {
        PlaneMesh->SetStaticMesh(PlaneVisualAsset.Object);
    }

    // simplewater ��Ƽ���� ����
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterialAsset(TEXT("/Game/BP/simplewater02"));
    if (WaterMaterialAsset.Succeeded())
    {
        SimpleWaterMaterial = WaterMaterialAsset.Object;
        PlaneMesh->SetMaterial(0, SimpleWaterMaterial);
    }

    // �ݸ��� �ڽ� ������Ʈ �߰�
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootComponent);
    CollisionBox->InitBoxExtent(FVector(55.0f, 55.0f, 0.5f));

    // �ݸ��� �ڽ��� ��ġ�� �ø��� (��� ��ġ ����)
    CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));  // Z�� �������� 25 ���ָ�ŭ �ø�

    CollisionBox->SetCollisionProfileName("OverlapAllDynamic");
    CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &Awaterplatform::OnOverlapBegin);
}

// Called when the game starts or when spawned
void Awaterplatform::BeginPlay()
{
    Super::BeginPlay();

    // �÷����� �ʱ� ��ġ ����
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
        CurrentLocation.Z += RiseSpeed * DeltaTime;  // �� �����Ӹ��� Z������ RiseSpeed ��ŭ �̵�

        // ��ǥ ��ġ�� ���� �ʵ��� üũ
        if (CurrentLocation.Z >= TargetZ)
        {
            CurrentLocation.Z = TargetZ;  // ��ǥ ���̿� �����ϸ� ����
            bIsRising = false;  // ��ǥ ���̿� ���������� �� �̻� ������� ����
        }

        SetActorLocation(CurrentLocation);  // ��ü ��ġ ������Ʈ
    }
}

// AMovingPlatform02�� �÷��̾ �浹�� �� Awaterplatform�� ���� �ö󰡵��� ����
void Awaterplatform::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && (OtherActor != this) && OtherActor->IsA(ACharacter::StaticClass()))
    {
        // �浹 �� ���� ����
        Destroy();
        UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetPause(true);
        UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0), EQuitPreference::Quit, true);
    }
}

// ��ü�� ������ �ö󰡵��� �ϴ� �Լ�
void Awaterplatform::StartRising(float HeightIncrement, float Duration)
{
    // ���� ��ǥ ���̿� HeightIncrement�� ���ؼ� ���ο� ��ǥ ���̸� ����
    TargetZ += HeightIncrement;  // �Ź� �浹�� ������ ��ǥ ���̸� �߰�
    RiseSpeed = HeightIncrement / Duration;  // ��ǥ ���̱��� Duration �� ���� �ö󰡵��� ���
    bIsRising = true;  // ��ü�� ����ϵ��� �÷��� ����
}
