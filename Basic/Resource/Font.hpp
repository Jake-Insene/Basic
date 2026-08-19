#pragma once
#include "Collections/Array.hpp"
#include "Collections/Error.hpp"
#include "math/rect_2d.h"

#include "Basic/Core/GPUMemoryAllocator.hpp"
#include "Basic/Core/GPUResourceManager.hpp"
#include "Basic/Resource/Resource.hpp"


namespace Basic
{

struct Font : Resource
{
	RESOURCE(RESOURCE_FONT, ResourceFlags::LoadFromAssets, ResourceExtensions("ttf"))

	static constexpr usize MinimumGlyphCount = 256;
	static constexpr usize DefaultFontSize = 32;

	struct Glyph
	{
		Rect2D src_rect;
		Vector2 advance;
		Vector2 size;
	};

	struct FontTheme
	{
		Collections::Array<Glyph> glyphs;
		i32 font_size;
		Vector2 atlas_size;
		Basic::GPUTextureID font_atlas;
		GPU::TextureViewID font_atlas_view;
		Basic::GPUResourceManager& gpu_resource_manager;

		FontTheme(Mem::Allocator& allocator, Basic::GPUResourceManager& gpu_resource_manager);
		~FontTheme();
	};

	Collections::Array<FontTheme> themes;

	Font(const ResourceCreateInfo& info);
	virtual ~Font() override;

	Error load(Collections::StringView _path);

	[[nodiscard]] const FontTheme& get_font_theme(i32 font_size);

	const FontTheme& _theme_with_size(i32 font_size);
};

}
