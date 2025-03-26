// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    GreatWeapon UMETA(DisplayName = "Great Weapon"),  // ���� ���� (���, �ظ� ��)
    Longsword UMETA(DisplayName = "Longsword"),      // �Ϲ� ���� (�ռҵ�, īŸ�� ��)
    Dagger UMETA(DisplayName = "Dagger"),            // �ܰ� (�ִܰ�, �� �ܰ� ��)
    Staff UMETA(DisplayName = "Staff")               // ������ (���� ������)
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
    // �⺻ ������
    AWeapon();

protected:
    // ���� ���� �� ȣ��
    virtual void BeginPlay() override;

public:
    // �� ������ ȣ��
    virtual void Tick(float DeltaTime) override;

    // ����� ��ȣ�ۿ��� �ݸ���
    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UStaticMeshComponent* WeaponMesh;

    UPROPERTY(VisibleAnywhere, Category = "Weapon")
    class UBoxComponent* CollisionBox;

    // ������ ����(�⺻��: 100.0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    float Weight = 100.0f;

    // �÷��̾�� ������ ���� �Լ�
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    // ������ ���� �Լ� (���� ����)
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

    /** ������ ���� */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponType WeaponType = EWeaponType::Longsword;

    /** ������ ��� */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponGrade WeaponGrade = EWeaponGrade::C;

    /** �⺻ ������ */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float BaseDamage = 20.0f;

    /** ���¹̳� �Ҹ� */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float StaminaCost = 10.0f;

    /** ���⸦ �������� �� ĳ���� ���ȿ� ������ �ִ� �Լ� */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void ApplyWeaponStats(class AMyDCharacter* Character);

    /** ���⸦ �������� �� ĳ���� ������ ������� ������ �Լ� */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    virtual void RemoveWeaponStats(class AMyDCharacter* Character);

};
