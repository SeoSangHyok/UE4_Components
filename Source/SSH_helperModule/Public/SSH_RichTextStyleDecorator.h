// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "SSH_RichTextStyleDecorator.generated.h"

/**
 * 
 */
class URichTextBlock;

UCLASS()
class SSH_HELPERMODULE_API USSH_RichTextStyleDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner);
};
