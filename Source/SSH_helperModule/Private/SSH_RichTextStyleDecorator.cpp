// Fill out your copyright notice in the Description page of Project Settings.

#include "SSH_RichTextStyleDecorator.h"
#include "Components/RichTextBlock.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Text/SlateWidgetRun.h"
#include "Kismet/KismetStringLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"


class SSSH_RichStyleTextWidget : public SCompoundWidget
{
	//"Framework/Application/SlateApplication.h" 을 include해줘야 SLATE_BEGIN_ARGS.. 을 사용가능
	SLATE_BEGIN_ARGS(SSSH_RichStyleTextWidget)
	{

	}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, const FSlateFontInfo& FontInfo, const FSlateColor& FontColor, FText WidgetText)
	{
		ChildSlot
		[
			SNew(STextBlock)
			.Font(FontInfo)
			.ColorAndOpacity(FontColor)
			.Text(WidgetText)
		];
	}
};




//////////////////////////////////////////////////////////////////////
// 실제로 RichTextBlock Deco 처리를 해주는 데코레이터
class FSSH_RichStyleText : public ITextDecorator
{
public:
	FSSH_RichStyleText(URichTextBlock* InOwner)
	{
		OwnerRichTextBlock = InOwner;
	}

	// 스트링을 파싱한 결과가 해당 데코레이터에서 처리가능한지 확인
	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
	{
		//FSSH_RichStyleText 는 <Style [프로퍼티]...>[내용]</> 으로 처리.
		if (RunParseResult.Name == TEXT("style"))
			return true;

		return false;
	}

	// 파싱된 스트링을 확인, 실질적인 위젯을 생성하는 부분
	virtual TSharedRef< ISlateRun > Create(const TSharedRef<class FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef< FString >& ModelText, const ISlateStyle* Style)
	{
		FTextRange ModelRange;
		ModelRange.BeginIndex = ModelText->Len();

		FTextRunInfo RunInfo(RunParseResult.Name, FText::FromString(OriginalText.Mid(RunParseResult.ContentRange.BeginIndex, RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex)));
		for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
		{
			RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
		}

		

		// 해당 데코레이터에서 처리할수 있는 프로퍼티는 font(글꼴), color(글자색), size(글자사이즈) 총 3가지

		// 폰트 처리. 폰트는 RichTextBlock에서 세팅한 font구조체의 것을 의미한다.
		const FTextBlockStyle* TextBlockStylePtr = nullptr;
		FString* FontNamePropertyValue = RunInfo.MetaData.Find(TEXT("font"));
		FSlateFontInfo SlateFontInfo;
		if (nullptr != FontNamePropertyValue)
		{
			FName TextBlockStyleName(*(*FontNamePropertyValue));
			if (Style->HasWidgetStyle<FTextBlockStyle>(TextBlockStyleName))
			{
				TextBlockStylePtr = &Style->GetWidgetStyle<FTextBlockStyle>(TextBlockStyleName);
			}
			else
			{
				TextBlockStylePtr = &OwnerRichTextBlock->GetDefaultTextStyle();
			}
		}
		else
		{
			TextBlockStylePtr = &OwnerRichTextBlock->GetDefaultTextStyle();
		}

		// 위에서 설정된 값으로 사용할 기본 폰트를 세팅
		SlateFontInfo = TextBlockStylePtr->Font;

		// 폰트 사이즈 세팅
		FString* SizePropertyValue = RunInfo.MetaData.Find(TEXT("size"));
		if (nullptr != SizePropertyValue)
		{
			SlateFontInfo.Size = UKismetStringLibrary::Conv_StringToInt(*SizePropertyValue);
		}

		// 폰트 컬러 세팅 컬러는 0~255까지의16진수 RGBA값으로 표현(00000000~FFFFFFFF 까지)
		FString* PropertyValue = RunInfo.MetaData.Find(TEXT("color"));
		FSlateColor FontColor = TextBlockStylePtr->ColorAndOpacity;
		if (nullptr != PropertyValue && PropertyValue->Len() == 8)
		{
			FString Hex_R = FString(TEXT("0x")).Append(PropertyValue->Mid(0, 2));
			FString Hex_G = FString(TEXT("0x")).Append(PropertyValue->Mid(2, 2));
			FString Hex_B = FString(TEXT("0x")).Append(PropertyValue->Mid(4, 2));
			FString Hex_A = FString(TEXT("0x")).Append(PropertyValue->Mid(6, 2));

			float R = FMath::Clamp(FCString::Strtoi(*Hex_R, nullptr, 16), 0, 255) / 255.0f;
			float G = FMath::Clamp(FCString::Strtoi(*Hex_G, nullptr, 16), 0, 255) / 255.0f;
			float B = FMath::Clamp(FCString::Strtoi(*Hex_B, nullptr, 16), 0, 255) / 255.0f;
			float A = FMath::Clamp(FCString::Strtoi(*Hex_A, nullptr, 16), 0, 255) / 255.0f;

			FontColor = FSlateColor(FLinearColor(R, G, B, A));
		}

		// TODO Allow universal mods?
		TSharedPtr<ISlateRun> SlateRun;
		TSharedPtr<SWidget> DecoratorWidget = CreateWidget(SlateFontInfo, FontColor, RunInfo.Content);
		if (DecoratorWidget.IsValid())
		{
			*ModelText += TEXT('\u200B'); // Zero-Width Breaking Space
			ModelRange.EndIndex = ModelText->Len();
			
			// Calculate the baseline of the text within the owning rich text
			const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			int16 WidgetBaseline = FontMeasure->GetBaseline(SlateFontInfo) - FMath::Min(0.0f, TextBlockStylePtr->ShadowOffset.Y);

			FSlateWidgetRun::FWidgetRunInfo WidgetRunInfo(DecoratorWidget.ToSharedRef(), WidgetBaseline);
			SlateRun = FSlateWidgetRun::Create(TextLayout, RunInfo, ModelText, WidgetRunInfo, ModelRange);
		}

		return SlateRun.ToSharedRef();
	}

private:
	TSharedPtr<SWidget> CreateWidget(const FSlateFontInfo& FontInfo, const FSlateColor& FontColor, FText WidgetText)
	{
		return SNew(SSSH_RichStyleTextWidget, FontInfo, FontColor, WidgetText);
	}

private:
	URichTextBlock* OwnerRichTextBlock;
};


//////////////////////////////////////////////////////////////////////
// USSH_RichTextStyleDecorator 구현부 USSH_RichTextStyleDecorator는 ITextDecorator를 상속받는 FSSH_RichStyleText 만 반환.
// 실제 RichText Decorator 처리는 FSSH_RichStyleText 에서 처리된다.

TSharedPtr<ITextDecorator> USSH_RichTextStyleDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FSSH_RichStyleText(InOwner));
}

//////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE