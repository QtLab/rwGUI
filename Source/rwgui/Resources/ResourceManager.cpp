#include "ResourceManager.h"

ID2D1Brush * ResourceManager::MakeBrush(Color color)
{
	ID2D1SolidColorBrush* brush = nullptr;
	if (ResourceManager::Get()->renderTarget != nullptr)
	{
		std::vector<ResourceBase*> Resources = ResourceManager::Get()->Resources;
		for (auto resource : Resources)
		{
			if (resource == nullptr) continue;
			Resource_SolidBrush* sBrush = dynamic_cast<Resource_SolidBrush*>(resource);
			if (sBrush!=nullptr && sBrush->clr == color)
			{
				return sBrush->GetValue();
			}
		}
		ResourceManager::Get()->renderTarget->CreateSolidColorBrush(color.ToD2D1ColorF(), &brush);
		new Resource_SolidBrush(color, brush);
	}
	return brush;
}

IDWriteTextFormat * ResourceManager::MakeTextFormat(char* aFontFamily, float fontSize,DWRITE_FONT_WEIGHT weight,DWRITE_FONT_STYLE style, DWRITE_FONT_STRETCH stretch)
{
	IDWriteTextFormat* format = nullptr;
	if (ResourceManager::Get()->writeFactory != nullptr)
	{
		std::wstring fontFamily(strlen(aFontFamily), L'#');
		mbstowcs(&fontFamily[0], aFontFamily, strlen(aFontFamily));
		std::vector<ResourceBase*> Resources = ResourceManager::Get()->Resources;
		for (auto resource : Resources)
		{
			if (resource == nullptr) continue;
			Resource_TextFormat* sTextFormat = dynamic_cast<Resource_TextFormat*>(resource);
			if (sTextFormat != nullptr)
			{
				WCHAR* familyName = new WCHAR[sTextFormat->value->GetFontFamilyNameLength()];
				int familyNameLength = 0;
				HRESULT result = sTextFormat->value->GetFontFamilyName(familyName, familyNameLength);
				if (result == S_OK)
				{
					if (lstrcmpW(familyName, fontFamily.c_str()) == 0
						&& sTextFormat->value->GetFontSize() == fontSize
						&& sTextFormat->value->GetFontStretch() == stretch
						&& sTextFormat->value->GetFontStyle() == style
						&& sTextFormat->value->GetFontWeight() == weight
						)
					{
						return sTextFormat->GetValue();
					}
				}
			}
		}
	ResourceManager::Get()->writeFactory->CreateTextFormat(fontFamily.c_str(), nullptr, weight, style, stretch, fontSize, L"", &format);
	}
	return format;
}

ID2D1Bitmap * ResourceManager::MakeBitmap(char * bitmapPath)
{
	ID2D1Bitmap* bitmap = nullptr;
	if (ResourceManager::Get()->renderTarget != nullptr)
	{
		std::vector<ResourceBase*> Resources = ResourceManager::Get()->Resources;
		for (auto resource : Resources)
		{
			if (resource == nullptr) continue;
			// TODO : ��������� ����� ��� ������ ������� 
			Resource_Bitmap* sBitmap = dynamic_cast<Resource_Bitmap*>(resource);
			if (sBitmap != nullptr && strcmp(sBitmap->bitmapPath,bitmapPath)==0)
			{
				return sBitmap->GetValue();
			}
		}
		if (ResourceManager::Get()->imageFactory!=nullptr)
		{
			IWICBitmapDecoder* pDecoder = nullptr;
			IWICBitmapFrameDecode* pSource = nullptr;
			IWICFormatConverter* pConverter = nullptr;


			std::wstring wBitmapPath(strlen(bitmapPath), L'#');
			mbstowcs(&wBitmapPath[0], bitmapPath, strlen(bitmapPath));

			HRESULT result = ResourceManager::Get()->imageFactory->CreateDecoderFromFilename(wBitmapPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
			if (result == S_OK)
			{
				result = pDecoder->GetFrame(0, &pSource);
				if (result == S_OK)
				{
					result = ResourceManager::Get()->imageFactory->CreateFormatConverter(&pConverter);
					if (result == S_OK)
					{
						result = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeMedianCut);
						if (result == S_OK)
						{
							result = ResourceManager::Get()->renderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, &bitmap);
							new Resource_Bitmap(bitmapPath, bitmap);
						}
					}
				}
			}
			if (pDecoder != nullptr) pDecoder->Release();
			if (pSource != nullptr) pSource->Release();
			if (pConverter != nullptr) pConverter->Release();
		}
	}
	return bitmap;
}
