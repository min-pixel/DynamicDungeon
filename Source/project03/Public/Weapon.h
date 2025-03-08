// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

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

    // ���� ���� �Լ�
    //void AttachToPlayer(class AMyDCharacter* Player);
};
