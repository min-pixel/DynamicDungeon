// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    GreatWeapon UMETA(DisplayName = "Great Weapon"),  // 대형 무기 (대검, 해머 등)
    Longsword UMETA(DisplayName = "Longsword"),      // 일반 무기 (롱소드, 카타나 등)
    Dagger UMETA(DisplayName = "Dagger"),            // 단검 (쌍단검, 독 단검 등)
    Staff UMETA(DisplayName = "Staff")               // 지팡이 (마법 지팡이)
};

UENUM(BlueprintType)
enum class EWeaponGrade : uint8
{
    C UMETA(DisplayName = "C"),
    B UMETA(DisplayName = "B"),
    A UMETA(DisplayName = "A")
};


UCLASS()
class PROJECT03_API AWeapon : public AActor
{
    GENERATED_BODY()

public:
    // 기본 생성자
    AWeapon();

protected:
    // 게임 시작 시 호출
    virtual void BeginPlay() override;

public:
    // 매 프레임 호출
    virtual void Tick(float DeltaTime) override;

    // 무기와 상호작용할 콜리전
    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UStaticMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UBoxComponent* CollisionBox;

    // 무기의 무게(기본값: 100.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float Weight = 100.0f;

    // 플레이어와 오버랩 감지 함수
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    // 오버랩 종료 함수 (선택 사항)
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

   

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float Damage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float TraceRadius = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName AttackStartSocket = "AttackStart";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FName AttackEndSocket = "AttackEnd";

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void TraceAttack();

    void StartTrace();

   

    FVector LastStartLocation;
    FVector LastEndLocation;

    /** 무기의 유형 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponType WeaponType = EWeaponType::Longsword;

    /** 무기의 등급 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponGrade WeaponGrade = EWeaponGrade::C;

    /** 기본 데미지 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float BaseDamage = 20.0f;

    /** 스태미나 소모량 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float StaminaCost = 10.0f;

    /** 무기를 장착했을 때 캐릭터 스탯에 영향을 주는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void ApplyWeaponStats(class AMyDCharacter* Character);

    /** 무기를 해제했을 때 캐릭터 스탯을 원래대로 돌리는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void RemoveWeaponStats(class AMyDCharacter* Character);

};
