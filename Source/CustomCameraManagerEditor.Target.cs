// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CustomCameraManagerEditorTarget : TargetRules
{
	public CustomCameraManagerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
        ExtraModuleNames.AddRange(new string[]
        {
            "CustomCameraManager",
            "SSH_HelperModule"
        });
	}
}
