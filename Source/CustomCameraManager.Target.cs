// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CustomCameraManagerTarget : TargetRules
{
	public CustomCameraManagerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        ExtraModuleNames.AddRange(new string[]
        {
            "CustomCameraManager",
            "SSH_HelperModule"
        });
    }
}
