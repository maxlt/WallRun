// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPPlayableCharacter.generated.h"

USTRUCT(BlueprintType)
struct FRunOnWallInfo
{
	GENERATED_BODY()

	TWeakObjectPtr<class AActor> Wall;
	FVector WallNormal; // Normal on the side of the wall the player is running on.

	FORCEINLINE void Clear()
	{
		Wall.Reset();
		WallNormal = FVector::ZeroVector;
	}
};

UCLASS()
class WALLRUN_API ATPPlayableCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components (C++)")
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components (C++)")
	class USpringArmComponent* SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="0.0", ClampMax="45.0"))
	float MinFacingAngle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running (C++)", meta = (ClampMin="45.0", ClampMax="90.0"))
	float MaxFacingAngle;

public:
	// Sets default values for this character's properties
	ATPPlayableCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCollided(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Axis);
	void MoveRight(float Axis);

	void BeginJump();
	void EndJump();

	bool IsMovingForward() const;

	void BeginWallRun();
	void EndWallRun();

	// Checks whether the wall is still present while wall running, or stamina runs out, etc.
	void TickWallRunning();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wall Running (C++)", meta = (AllowPrivateAccess="true"))
	bool bIsRunningOnWall;

	float ForwardInputAxis;

	FRunOnWallInfo CurrentWallInfo;
	FVector CurrentRunDirection;
	FTimerHandle RunningTimer;
};
