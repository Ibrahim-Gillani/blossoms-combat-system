// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UAnimMontage;
class USoundBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLOSSOMS_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	//Montage end handler
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	//Trace
	//Blade positions
	FVector LastBasePos;
	FVector LastTipPos;
	//Current Attack's Damage
	float CurrentAttackDamage = 0.0f;
	//Tick
	void PerformTraceTick();
	//Samples along weapon
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	int32 BladeTraceSamples = 6;
	//Blade Thickness
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	float TraceRadius = 20.f;

	//IFrames and dodging
	bool bIsInvincible = false;
	bool bIsDodging = false;

	//State
	float LastStaminaUseTime = -1000.f;

public:	
	// Sets default values for this component's properties
	UCombatComponent();

	//Attack function(s)
	void LightAttack();
	void HeavyAttack();

	//Animation montage relfection macros
	UPROPERTY(EditAnywhere, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimMontage>> LightComboMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|Montages")
	TArray<TObjectPtr<UAnimMontage>> HeavyComboMontages;

	UPROPERTY(EditAnywhere, Category = "Combat|Montages")
	TObjectPtr<UAnimMontage> DodgeMontage;

	//Fetch Mesh
	USkeletalMeshComponent* GetOwnerMesh() const;

	//State flag
	bool bIsAttacking = false;

	//Attack Combos
	int32 ComboIndex = 0;
	int32 HeavyComboIndex = 0;
	bool bComboQueued = false;
	bool bCanQueueCombo = false;
	bool bIsHeavyAttack = false;

	//Damage Window 
	void StartDamageWindow(float Damage);
	void EndDamageWindow();
	bool bDamageWindowActive = false;
	bool bHasDamageWindowFired = false;

	//Combo
	void TryAdvanceCombo();
	void OpenComboQueue();
	void CloseComboQueue();

	//Combat movement lock
	bool CanMove() const;
	bool CanDodge() const;
	
	//IFrames and Dodging
	void StartIFrames();
	void EndIFrames();
	void Dodge();

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DodgeImpulseStrength = 800.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MinImpulseScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float DodgeUpwardBias = 0.1f;

	//Stamina System
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stamina")
	float MaxStamina = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stamina")
	float CurrentStamina = 200.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Stamina")
	float StaminaRegenRate = 62.5f;

	UPROPERTY(EditAnywhere, Category = "Combat|Stamina")
	float StaminaRegenDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Stamina")
	float DodgeStaminaCost = 60.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Stamina")
	float LightAttackStaminaCost = 25.f;

	UPROPERTY(EditAnywhere, Category = "Combat|Stamina")
	float HeavyAttackStaminaCost = 35.f;

	//Stamina Specific Functions
	void ConsumeStamina(float Amount);
	bool CanAttack(float Cost) const;

	//Sounds
	UPROPERTY(EditAnywhere, Category = "Combat|Sound")
	TObjectPtr<USoundBase> HitImpactSound;

	//Weapon Trace
	//Socket Names
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	FName BladeBaseSocket;
	UPROPERTY(EditAnywhere, Category = "Combat|Trace")
	FName BladeTipSocket;
	//Actors already hit
	UPROPERTY() 
	TArray<TObjectPtr<AActor>> HitActors;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//

	//Getters
	bool GetInvincible() const { return bIsInvincible; }

	//Stamina Getter
	UFUNCTION(BlueprintCallable, Category = "Combat|Stamina")
	float GetStaminaRatio() const {
		return MaxStamina > 0 ? CurrentStamina / MaxStamina : 0.f;
	}
		
};
