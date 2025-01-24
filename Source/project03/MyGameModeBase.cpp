// MyGameMode.cpp
#include "MyGameModeBase.h"
#include "MyCharacter.h"

AMyGameModeBase::AMyGameModeBase()
{
    // Set default pawn class to our custom character class
    DefaultPawnClass = AMyCharacter::StaticClass();
}

