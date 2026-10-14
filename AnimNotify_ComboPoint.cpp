// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_ComboPoint.h"
#include "Components/SkeletalMeshComponent.h"
#include "CombatComponent.h"

void UAnimNotify_ComboPoint::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference) {
	//Guard
	if (!MeshComp) { return; }
	//Get owner
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) { return; }
	//Find the component on owner
	UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
	if (!CombatComponent) { return; }
	//Call TryAdvanceCombo
	CombatComponent->TryAdvanceCombo();
}