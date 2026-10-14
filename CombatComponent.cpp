// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Damageable.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//Set Stamina
	CurrentStamina = MaxStamina;
	
}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Ticks When Damage Window
	if (bDamageWindowActive) { PerformTraceTick(); }

	//Stamina regen
	if (GetWorld()->GetTimeSeconds() - LastStaminaUseTime > StaminaRegenDelay) {
		CurrentStamina += StaminaRegenRate * GetWorld()->GetDeltaSeconds();
		if (CurrentStamina > MaxStamina) {
			CurrentStamina = MaxStamina;
		}
	}
}

//Get Mesh
USkeletalMeshComponent* UCombatComponent::GetOwnerMesh() const {
	//Get owner
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) { return nullptr; }
	//Get the mesh
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh) { return nullptr; }

	return Mesh;
}

//Light Attack
void UCombatComponent::LightAttack() {
	//Guard if already attacking
	if (bIsAttacking) {
		if (bCanQueueCombo && !bIsHeavyAttack && CanAttack(LightAttackStaminaCost)) {
			bComboQueued = true;
		}
		return;
	}
	if (!CanAttack(LightAttackStaminaCost)) { return; }
	ComboIndex = 0;
	//Get Mesh
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh) { return; }
	//Get animation instance
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance) { return; }
	//Play montage
	if (!LightComboMontages.IsValidIndex(ComboIndex) || !LightComboMontages[ComboIndex]) { return; }
	float Duration = AnimInstance->Montage_Play(LightComboMontages[ComboIndex]);
	if (Duration > 0) {
		bIsAttacking = true;
		//Consume Stamina
		ConsumeStamina(LightAttackStaminaCost);
		//block movement and prevent rotation/spinning on the spot
		if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
			Character->GetCharacterMovement()->SetMovementMode(MOVE_None);
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;

			UE_LOG(LogTemp, Error, TEXT("=== COMBAT LOG === LightAttack locked movement to MOVE_None"));

		}
		//End montage delegate
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UCombatComponent::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, LightComboMontages[ComboIndex]);
	}
}

//Heavy Attack
void UCombatComponent::HeavyAttack() {
	//Guard if already attacking
	if (bIsAttacking) {
		if (bCanQueueCombo && bIsHeavyAttack && CanAttack(HeavyAttackStaminaCost)) {
			bComboQueued = true;
		}
		return;
	}
	//Stmaina check
	if (!CanAttack(HeavyAttackStaminaCost)) { return; }
	HeavyComboIndex = 0;
	bIsHeavyAttack = true;
	//Get Mesh
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh) { return; }
	//Get animation instance
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance) { return; }
	//Play montage
	if (!HeavyComboMontages.IsValidIndex(HeavyComboIndex) || !HeavyComboMontages[HeavyComboIndex]) { return; }
	float Duration = AnimInstance->Montage_Play(HeavyComboMontages[HeavyComboIndex]);
	if (Duration > 0) {
		bIsAttacking = true;
		//Consume Stamina
		ConsumeStamina(HeavyAttackStaminaCost);
		if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
			Character->GetCharacterMovement()->SetMovementMode(MOVE_None);
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		}
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UCombatComponent::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, HeavyComboMontages[HeavyComboIndex]);
	}
}

//Montage End
void UCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) {
	// If a combo didn;t cleanly chain into a new attack unlock movement
	// FIX: We are a heavy finisher if we are in a heavy attack state and the montage ending is the last one in our array
	bool bIsHeavyFinisher = bIsHeavyAttack && HeavyComboMontages.Num() > 0 && (Montage == HeavyComboMontages.Last());

	if (!bComboQueued && (!bIsAttacking || bIsHeavyFinisher)) {
		bIsAttacking = false;
		if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}

	if (bInterrupted && bIsAttacking) {
		return;
	}
	//Resets
	bDamageWindowActive = false;
	bHasDamageWindowFired = false;
	HitActors.Empty();
	HeavyComboIndex = 0;
	ComboIndex = 0;
	bComboQueued = false;
	bIsHeavyAttack = false;
}

//Mopntage End for Dodge
void UCombatComponent::OnDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted) {
	bIsDodging = false;
	bIsAttacking = false;
	bHasDamageWindowFired = false;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character) {
		Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

//Combo Advance
void UCombatComponent::TryAdvanceCombo() {

	UE_LOG(LogTemp, Warning, TEXT("=== COMBAT LOG === TryAdvanceCombo() was fired by an Anim Notify! Queued flag is: %s"), bComboQueued ? TEXT("TRUE") : TEXT("FALSE"));

	bDamageWindowActive = false;
	HitActors.Empty();
	//If no Combo queued, do nothing
	if (!bComboQueued) {
		bIsAttacking = false;
		bIsHeavyAttack = false;
		ComboIndex = 0;
		HeavyComboIndex = 0;

		if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		}
		return;
	}
	//Consume flag and advance
	bComboQueued = false;
	if (bIsHeavyAttack) { //HEAVY branch
		//Stamina Check
		if (!CanAttack(HeavyAttackStaminaCost)) {
			//Clear flags
			bIsAttacking = false;
			bComboQueued = false;

			//Return movement
			if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Character->GetCharacterMovement()->bOrientRotationToMovement = true;

				UE_LOG(LogTemp, Display, TEXT("=== COMBAT LOG === TryAdvanceCombo rejected due to low stamina. Resetting movement to MOVE_Walking."));

			}
			return;
		}
		HeavyComboIndex++;
		//Out of bounds check
		if (!HeavyComboMontages.IsValidIndex(HeavyComboIndex)) {
			HeavyComboIndex = 0;

			//Clear flags
			bIsAttacking = false;
			bComboQueued = false;

			//Return movement
			if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Character->GetCharacterMovement()->bOrientRotationToMovement = true;
			}
			return;
		}
		//Get Mesh
		USkeletalMeshComponent* Mesh = GetOwnerMesh();
		if (!Mesh) { return; }
		//Get animation instance
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (!AnimInstance) { return; }
		//Play montage
		if (!HeavyComboMontages[HeavyComboIndex]) { return; }
		float Duration = AnimInstance->Montage_Play(HeavyComboMontages[HeavyComboIndex]);
		if (Duration > 0) {
			bIsAttacking = true;
			//Consume Stamina
			ConsumeStamina(HeavyAttackStaminaCost);
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UCombatComponent::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, HeavyComboMontages[HeavyComboIndex]);
		}
	}
	else { //LIGHT branch
		//Stamina Check
		if (!CanAttack(LightAttackStaminaCost)) {
			//Clear flags
			bIsAttacking = false;
			bComboQueued = false;

			//Return movement
			if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Character->GetCharacterMovement()->bOrientRotationToMovement = true;
			}
			return;
		}
		ComboIndex++;
		//Out of bounds check
		if (!LightComboMontages.IsValidIndex(ComboIndex)) {
			ComboIndex = 0;

			//Clear flags
			bIsAttacking = false;
			bComboQueued = false;

			//Return movement
			if (ACharacter* Character = Cast<ACharacter>(GetOwner())) {
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				Character->GetCharacterMovement()->bOrientRotationToMovement = true;
			}
			return;
		}
		//Get Mesh
		USkeletalMeshComponent* Mesh = GetOwnerMesh();
		if (!Mesh) { return; }
		//Get animation instance
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (!AnimInstance) { return; }
		//Play montage
		if (!LightComboMontages[ComboIndex]) { return; }
		float Duration = AnimInstance->Montage_Play(LightComboMontages[ComboIndex]);
		if (Duration > 0) {
			bIsAttacking = true;
			//Consume Stamina
			ConsumeStamina(LightAttackStaminaCost);
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UCombatComponent::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, LightComboMontages[ComboIndex]);
		}
	}
}

//Combo Window
void UCombatComponent::OpenComboQueue() {
	bCanQueueCombo = true;
	bComboQueued = false; //fresh window with no leftover queue (3 attacks, 2 clicks)
}

void UCombatComponent::CloseComboQueue() {
	bCanQueueCombo = false;
}

//Combat movement lock
bool UCombatComponent::CanMove() const {
	return !bIsAttacking;
}

//Damage Window
void UCombatComponent::StartDamageWindow(float Damage) {
	bDamageWindowActive = true;
	bHasDamageWindowFired = true;
	CurrentAttackDamage = Damage;
	//Empty Hit Actors
	HitActors.Empty();
	//Read current socket position
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh) { return; }
	LastBasePos = Mesh->GetSocketLocation(BladeBaseSocket);
	LastTipPos = Mesh->GetSocketLocation(BladeTipSocket);
}

void UCombatComponent::EndDamageWindow() {
	bDamageWindowActive = false;
}

//Trace Tick
void UCombatComponent::PerformTraceTick() {
	//Guard
	if (BladeTraceSamples == 1) { return; }
	//Get Mesh
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh) { return; }
	//Read current socket positions
	FVector CurrentBase = Mesh->GetSocketLocation(BladeBaseSocket);
	FVector CurrentTip = Mesh->GetSocketLocation(BladeTipSocket);
	//Set Params to ignore Character
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	//Collision sphere
	FCollisionShape Sphere = FCollisionShape::MakeSphere(TraceRadius);
	//Loop per sample point along blade
	for (int32 i = 0; i < BladeTraceSamples; i++) {
		float Alpha = (float)i / (BladeTraceSamples - 1);
		FVector LastSamplePos = FMath::Lerp(LastBasePos, LastTipPos, Alpha);
		FVector CurrentSamplePos = FMath::Lerp(CurrentBase, CurrentTip, Alpha);
		//Sweep
		FHitResult Hit;
		bool bHit = GetWorld()->SweepSingleByChannel(Hit, LastSamplePos, CurrentSamplePos,
			FQuat::Identity, ECC_GameTraceChannel1, Sphere, Params);
		//Act on hit
		if (bHit) {
			//Get actor
			AActor* HitActor = Hit.GetActor();
			if (!HitActor) { return; }
			//Skip if already hit
			if (HitActors.Contains(HitActor)) { continue; }
			//Check for Damageable interface
			if (HitActor->Implements<UDamageable>()) {
				IDamageable::Execute_TakeHit(HitActor, CurrentAttackDamage, GetOwner());
				if (HitImpactSound) {
					UGameplayStatics::PlaySoundAtLocation(this, HitImpactSound, HitActor->GetActorLocation());
				}
				HitActors.Add(HitActor);
			}
		}
	}
	//Store endpoints
	LastBasePos = CurrentBase;
	LastTipPos = CurrentTip;
}

//IFrames
void UCombatComponent::StartIFrames() {
	bIsInvincible = true;
}

void UCombatComponent::EndIFrames() {
	bIsInvincible = false;
}

bool UCombatComponent:: CanDodge() const {
	//Can't dodge while already dodging
	if (bIsDodging) { return false; }  
	//Stamina check
	if (CurrentStamina < DodgeStaminaCost) { return false; }
	//Can't dodge in mid air
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetCharacterMovement()->IsFalling()) {
		return false;
	}
	return true;
}

void UCombatComponent::Dodge() {
	//Guard
	if (!CanDodge()) { return; }

	//ConsumeStamina
	ConsumeStamina(DodgeStaminaCost);

	//Use isAttacking as movement lock
	bIsDodging = true;
	bIsAttacking = true;

	//Let Montage_Play blend into dodge - Note may need to cancel manually

	//Direction change to ensure character faces the right way
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	FVector InputDir = Character->GetLastMovementInputVector();
	if (InputDir.IsNearlyZero() == false) {
		double Yaw = InputDir.Rotation().Yaw;
		Character->SetActorRotation(FRotator(0.f, Yaw, 0.f));
	}

	//Disable orient-to-movement so rotation snaps
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	//Get dodge direction and allow turning into direction
	FVector LaunchDir = InputDir.IsNearlyZero() ? Character->GetActorForwardVector() : InputDir;
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	//Trace to ground for slope awareness
	FVector Start = Character->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 150.f);
	FHitResult GroundHit;
	FCollisionQueryParams GroundParams;
	GroundParams.AddIgnoredActor(GetOwner());
	bool bHitGround = GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_Visibility, GroundParams);
	//Get normalised slope direction
	if (bHitGround) {
		LaunchDir = FVector::VectorPlaneProject(LaunchDir, GroundHit.ImpactNormal);
		//Add small vertical offset to avoid friction
		LaunchDir.Z += DodgeUpwardBias;
		LaunchDir.Normalize();
	}
	//Sacle Impulse to match slope steepness
	float SlopeFactor = FVector::DotProduct(GroundHit.ImpactNormal, FVector::UpVector);
	float ImpulseScale = FMath::Clamp(SlopeFactor, MinImpulseScale, 1.0f);
	//Launch the character in the dodge direction
	Character->LaunchCharacter(LaunchDir * DodgeImpulseStrength * ImpulseScale, true, true);

	//Get Mesh
	USkeletalMeshComponent* Mesh = GetOwnerMesh();
	if (!Mesh) { return; }
	//Get animation instance
	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance) { return; }
	//Play montage
	if (!DodgeMontage) { return; }
	float Duration = AnimInstance->Montage_Play(DodgeMontage);
	if (Duration > 0) {
		bIsDodging = true;
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UCombatComponent::OnDodgeMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, DodgeMontage);
	}
}

//Stamina Specifics
void UCombatComponent::ConsumeStamina(float Amount) {
	CurrentStamina -= Amount;
	LastStaminaUseTime = GetWorld()->GetTimeSeconds();
}

bool UCombatComponent::CanAttack(float Cost) const {
	if (CurrentStamina < Cost) { return false; }

	//Prevent Atack during Jump
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && Character->GetCharacterMovement()->IsFalling()) {
		return false;
	}

	return true;
}



