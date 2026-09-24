
// Implementation of the TextRenderer class.
// Handles font loading and text rendering with various fitting strategies.


#include "textRenderer.h"

#include "utf8_utils.h"

#include <vector>
#include <cmath>
#include <algorithm>


TextRenderer::TextRenderer() {
    InitFont();
}

TextRenderer::~TextRenderer() {
    // Unload all pre-rasterized font atlases.
    for (auto &f : m_Fonts) {
        UnloadFont(f);
    }
}

void TextRenderer::InitFont()
{
    std::vector<int> codepoints;
    auto addRange = [&codepoints](int begin, int end) {
        for (int cp = begin; cp <= end; ++cp) {
            codepoints.push_back(cp);
        }
    };

    // Broad multilingual + symbols set (Latin, Greek, Cyrillic, punctuation, icons).
    addRange(0x0020, 0x007E); // Basic Latin
    addRange(0x00A0, 0x00FF); // Latin-1 Supplement
    addRange(0x0100, 0x017F); // Latin Extended-A
    addRange(0x0180, 0x024F); // Latin Extended-B
    addRange(0x1E00, 0x1EFF); // Latin Extended Additional
    addRange(0x0370, 0x03FF); // Greek and Coptic
    addRange(0x0400, 0x04FF); // Cyrillic
    addRange(0x2000, 0x206F); // General punctuation
    addRange(0x20A0, 0x20CF); // Currency symbols
    addRange(0x2190, 0x21FF); // Arrows
    addRange(0x2600, 0x26FF); // Misc symbols
    addRange(0x2700, 0x27BF); // Dingbats


    // Dense UI range + larger headings to minimize runtime scaling artifacts.
    m_FontSizes = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 30, 32, 36, 40, 48, 64, 96 };
    m_Fonts.clear();
    m_Fonts.reserve(m_FontSizes.size());

    for (int sz : m_FontSizes) {
        Font f = LoadFontEx("assets/Nunito-Regular.ttf", sz, codepoints.data(), (int)codepoints.size());
        if (f.texture.id == 0) {
            f = GetFontDefault();
        }
        // Keep UI text crisp; zoom-aware atlas selection handles world text scaling.
        SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
        m_Fonts.push_back(f);
    }
}

float TextRenderer::Measure(const std::string& text, float fontSize, float renderScale) const {
    const float sampleSize = std::max(1.0f, fontSize * renderScale);
    const Font* f = GetFontForSize(sampleSize);
    if (!f) return 0.0f;
    return MeasureTextEx(*f, text.c_str(), fontSize, m_Spacing).x;
}

std::string TextRenderer::FitTextToWidth(const std::string& text, float maxWidth, float fontSize, float renderScale) {
    if (Measure(text, fontSize, renderScale) <= maxWidth) {
        return text;
    }


    std::string result = text;
    std::string ellipsis = "...";
    float ellipsisW = Measure(ellipsis, fontSize, renderScale);

    // Remove trailing codepoints, not bytes, to keep UTF-8 valid.
    while (!result.empty() && (Measure(result, fontSize, renderScale) + ellipsisW) > maxWidth) {
        size_t cursor = result.size();
        Utf8::ErasePrevCodepoint(result, cursor);
    }

    return result + ellipsis;
}

const Font* TextRenderer::GetFontForSize(float fontSize) const
{
    if (m_Fonts.empty() || m_FontSizes.empty()) return nullptr;
    // Pick the closest pre-rasterized size to reduce visual artifacts.
    size_t bestIdx = 0;
    float bestDiff = std::abs((float)m_FontSizes[0] - fontSize);
    for (size_t i = 1; i < m_FontSizes.size(); ++i) {
        float diff = std::abs((float)m_FontSizes[i] - fontSize);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = i;
        }
    }
    return &m_Fonts[bestIdx];
}

void TextRenderer::DrawSimpleText(const std::string& text, Vector2 pos, float fontSize, Color col, float renderScale)
{
    // Keep positions aligned to pixels to reduce shimmer during movement.
    pos.x = std::round(pos.x);
    pos.y = std::round(pos.y);
    const float sampleSize = std::max(1.0f, fontSize * renderScale);
    const Font* f = GetFontForSize(sampleSize);
    if (!f) return;
    DrawTextEx(*f, text.c_str(), pos, fontSize, m_Spacing, col);
}

void TextRenderer::DrawTextCentered(const std::string& text, Vector2 centerPos, float fontSize, Color col, float renderScale)
{
    const float sampleSize = std::max(1.0f, fontSize * renderScale);
    const Font* f = GetFontForSize(sampleSize);
    if (!f) return;
    Vector2 size = MeasureTextEx(*f, text.c_str(), fontSize, m_Spacing);
    Vector2 drawPos = {
        std::round(centerPos.x - size.x / 2.0f),
        std::round(centerPos.y - size.y / 2.0f)
    };
    DrawTextEx(*f, text.c_str(), drawPos, fontSize, m_Spacing, col);
}

void TextRenderer::DrawTextFitted(const std::string& text, Vector2 centerPos, float maxWidth, float fontSize, Color col, float renderScale)
{
    std::string toDraw = FitTextToWidth(text, maxWidth, fontSize, renderScale);
    DrawTextCentered(toDraw, centerPos, fontSize, col, renderScale);
}