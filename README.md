# Blossoms – Combat System (C++ Code Sample)

This repository contains selected C++ source files from **Blossoms**, a third-person Souls-like action game prototype developed in Unreal Engine 5.

The files are provided as a focused example of my gameplay programming work, demonstrating how I implemented combat mechanics, managed gameplay state, and integrated C++ systems with Unreal Engine's animation framework.

## Included Files

**`CombatComponent.h` / `CombatComponent.cpp`**

The core combat component, responsible for:

- Light and heavy attack combos, including buffered inputs and combo progression.
- Stamina consumption and regeneration.
- Dodging, movement impulses, and temporary invulnerability.
- Animation montage handling and combat state transitions.
- Melee hit detection using sampled sword positions and sphere sweeps.

**`AnimNotify_ComboPoint.cpp`**

An animation notify used to trigger combo progression at specific points during attack animations, connecting animation timing with the combat component's gameplay logic.

## Implementation Highlights

### Combo System

Attack sequences are controlled through a combination of combat state, queued player inputs, and animation events. This allows the player to buffer follow-up attacks while keeping combo transitions aligned with the animation.

### Melee Collision Detection

Rather than relying solely on a static collision volume, the system samples positions along the weapon and performs sphere sweeps between successive positions. This helps detect targets contacted by the sword during an attack.

### Stamina and Dodging

Attacking and dodging interact with a shared stamina system. Dodging also incorporates directional movement and temporary invulnerability, with behaviour coordinated through combat state and animation playback.

## Scope and Dependencies

These files are excerpts from a larger Unreal Engine 5 project and are **not intended to compile independently**.

They depend on project-specific character classes, animation assets, and other Unreal Engine systems that are not included here.

The purpose of this repository is to demonstrate the structure and implementation of selected gameplay systems rather than distribute the complete game project.
