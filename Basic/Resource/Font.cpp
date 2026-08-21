#include "Basic/Resource/Font.hpp"

#include <External/stb_truetype.h>

#include "Basic/Resource/ResourceManagerInternal.hpp"
#include "Basic/Resource/ResourceManager.hpp"


namespace Basic
{

static void load_theme(Mem::Allocator& allocator, const Slice<u8>& font_file_content,
    stbtt_fontinfo* font, Font::FontTheme& theme, Basic::GPUResourceManager& gpu_resource_manager)
{
    stbtt_pack_context pack_context;
    stbtt_packedchar ranges[Font::MinimumGlyphCount] = {};
    
    bool success = false;
    i32 width = 512;
    while(success == false)
    {
        Slice pixels = allocator.alloc(i64(width * width), 16);

        stbtt_PackBegin(&pack_context, pixels.ptr(), width, width, width, 0, 0);

        i32 result = stbtt_PackFontRange(
            &pack_context, font_file_content.ptr(), 0, 
            f32(theme.font_size), 0, Font::MinimumGlyphCount, ranges
        );

        if(result != 1)
        {
            width *= 2;
            stbtt_PackEnd(&pack_context);
            allocator.free(pixels);
            continue;
        }

        success = true;

        for (u32 glyph_index = 0; glyph_index < Font::MinimumGlyphCount; glyph_index++)
        {
            Font::Glyph& glyph = theme.glyphs.get(glyph_index);

            glyph.advance.width = ranges[glyph_index].xadvance;

            i32 ascent;
            i32 descent;
            i32 line_gap;
            stbtt_GetFontVMetrics(font, &ascent, &descent, &line_gap);

            f32 scale = stbtt_ScaleForPixelHeight(font, f32(theme.font_size));
            f32 line_advance = f32(ascent - descent + line_gap) * scale;
            glyph.advance.height = line_advance;
            
            f32 u0 = f32(ranges[glyph_index].x0);
            f32 v0 = f32(ranges[glyph_index].y0);
            f32 u1 = f32(ranges[glyph_index].x1);
            f32 v1 = f32(ranges[glyph_index].y1);
            
            f32 glyph_width = u1 - u0;
            f32 glyph_height = v1 - v0;

            // top-left rect
            glyph.src_rect = Rect2D(
                Vector2(u0, v0),
                Vector2(glyph_width, glyph_height)
            );
        }

        theme.atlas_size = Vector2(width, width);
        theme.font_atlas = gpu_resource_manager.create_texture(
            {
                .type = GPU::TextureType::Texture2D,
                .format = GPU::TextureFormat::R8Unorm,
                .extent = Vector3U(width, width, 1),
                .pixels = pixels,
                .flags = Basic::TextureAllocateFlags::ViewR8All,
            }
        );
        theme.font_atlas_view = gpu_resource_manager.texture_get_texture_view(theme.font_atlas);

        stbtt_PackEnd(&pack_context);
        allocator.free(pixels);
    }
}

Font::FontTheme::FontTheme(Mem::Allocator& allocator, Basic::GPUResourceManager& gpu_resource_manager)
: glyphs(allocator, 4, {}), gpu_resource_manager(gpu_resource_manager)
{}

Font::FontTheme::~FontTheme()
{
    if (font_atlas != Basic::GPUTextureID::invalid())
    {
        gpu_resource_manager.destroy_texture(font_atlas);     
    }
}

Font::Font(const ResourceCreateInfo& info)
: Resource(info), themes(allocator, 4, {})
{}

Font::~Font()
{}

Error Font::load(Collections::StringView path)
{
    if (!IO::File::exists(allocator, path))
    {
        RMDebugInfo("Couldn't load the font '{}'", path);
        return MakeError(ErrorCode::FileNotFound);
    }

    Resource::path.set(path);

    Slice content = IO::File::read_all(allocator, path);

    stbtt_fontinfo font;
    stbtt_InitFont(&font, content.ptr(), stbtt_GetFontOffsetForIndex(content.ptr(), 0));

    FontTheme& default_theme = themes.emplace(allocator, resource_manager.get_gpu_resource_manager());
    default_theme.font_size = DefaultFontSize;

    default_theme.glyphs.resize(MinimumGlyphCount);
    load_theme(allocator, content, &font, default_theme, resource_manager.get_gpu_resource_manager());
    
    allocator.free(content);

    return ErrorCode::Ok;
}

const Font::FontTheme& Font::get_font_theme(i32 font_size)
{
    DebugAssert(font_size != 0, "invalid font size");

    for (FontTheme& theme : themes.iter())
    {
        if (theme.font_size == font_size)
        {
            return theme;
        }
    }

    return _theme_with_size(font_size);
}

const Font::FontTheme& Font::_theme_with_size(i32 font_size)
{
    Slice content = IO::File::read_all(allocator, path.view());

    stbtt_fontinfo font;
    stbtt_InitFont(&font, content.ptr(), stbtt_GetFontOffsetForIndex(content.ptr(), 0));

    FontTheme& new_theme = themes.emplace(allocator, resource_manager.get_gpu_resource_manager());
    new_theme.font_size = font_size;

    new_theme.glyphs.resize(MinimumGlyphCount);
    load_theme(allocator, content, &font, new_theme, resource_manager.get_gpu_resource_manager());

    allocator.free(content);

    return new_theme;
}

}
