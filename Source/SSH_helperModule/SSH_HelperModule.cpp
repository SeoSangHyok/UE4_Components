// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SSH_HelperModule.h"

#define LOCTEXT_NAMESPACE "FSSH_HelperModule"

void FSSH_HelperModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FSSH_HelperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
//IMPLEMENT_MODULE(FDefaultGameModuleImpl, SSH_HelperModule)

// IMPLEMENT_PRIMARY_GAME_MODULE 으로 모듈을 세팅해야 핫리로딩이 지원된다.
IMPLEMENT_PRIMARY_GAME_MODULE(FSSH_HelperModule, SSH_HelperModule, "SSH_HelperModule");