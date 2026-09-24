// Implementation of the IconRenderer class.
// Uses sprite textures (.png/.ico) for rendering UI icons.

#include "iconRenderer.h"
#include <algorithm>
#include <cmath>

namespace {
Texture2D LoadTintableTexture(const std::string& path)
{
    Image image = LoadImage(path.c_str());
    if (!image.data) {
        return Texture2D{};
    }

    Texture2D texture{};
    Color* pixels = LoadImageColors(image);
    if (pixels) {
        const int count = image.width * image.height;
        for (int i = 0; i < count; ++i) {
            if (pixels[i].a > 0) {
                pixels[i].r = 255;
                pixels[i].g = 255;
                pixels[i].b = 255;
            }
        }

        Image processed{};
        processed.data = pixels;
        processed.width = image.width;
        processed.height = image.height;
        processed.mipmaps = 1;
        processed.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

        texture = LoadTextureFromImage(processed);
        UnloadImageColors(pixels);
    }

    UnloadImage(image);
    return texture;
}
}

IconRenderer::IconRenderer() {
    InitIconPaths();
    LoadIcons();
}

IconRenderer::~IconRenderer() {
    for (auto& [_, texture] : m_Icons) {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
}

void IconRenderer::InitIconPaths() {
    auto addPaths = [this](IconType icon, std::initializer_list<const char*> files) {
        auto& paths = m_IconPaths[static_cast<int>(icon)];
        paths.reserve(files.size());
        for (const char* f : files) {
            paths.emplace_back(f);
        }
    };

    addPaths(IconType::Settings, {
        "assets/icons/settings.png", "assets/icons/settings.ico",
        "assets/icons/icon-settings.png", "assets/icons/icon-settings.ico",
        "assets/settings.png", "assets/settings.ico"
    });
    addPaths(IconType::Filter, {
        "assets/icons/filter.png", "assets/icons/filter.ico",
        "assets/icons/filter_list.png", "assets/icons/filter_list.ico",
        "assets/icons/icon-filter.png", "assets/icons/icon-filter.ico",
        "assets/filter.png", "assets/filter.ico"
    });
    addPaths(IconType::FilterActive, {
        "assets/icons/filter_active.png", "assets/icons/filter_active.ico",
        "assets/icons/filter-filled.png", "assets/icons/filter-filled.ico",
        "assets/icons/filter.png", "assets/icons/filter.ico"
    });

    addPaths(IconType::Plus, {
        "assets/icons/plus.png", "assets/icons/plus.ico"
    });
    addPaths(IconType::Edit, {
        "assets/icons/edit.png", "assets/icons/edit.ico"
    });
    addPaths(IconType::Search, {
        "assets/icons/search.png", "assets/icons/search.ico"
    });
    addPaths(IconType::Close, {
        "assets/icons/close.png", "assets/icons/close.ico"
    });
    addPaths(IconType::Lock, {
        "assets/icons/lock.png", "assets/icons/lock.ico"
    });
    addPaths(IconType::Unlock, {
        "assets/icons/unlock.png", "assets/icons/unlock.ico"
    });
    addPaths(IconType::Download, {
        "assets/icons/download.png", "assets/icons/download.ico"
    });
    addPaths(IconType::Upload, {
        "assets/icons/upload.png", "assets/icons/upload.ico"
    });
    addPaths(IconType::Save, {
        "assets/icons/save.png", "assets/icons/save.ico"
    });
    addPaths(IconType::Chart, {
        "assets/icons/chart.png", "assets/icons/chart.ico"
    });
    addPaths(IconType::Grid, {
        "assets/icons/grid.png", "assets/icons/grid.ico"
    });
    addPaths(IconType::Goals, {
        "assets/icons/goals.png", "assets/icons/goals.ico"
    });
    addPaths(IconType::Dice, {
        "assets/icons/dice.png", "assets/icons/dice.ico"
    });
    addPaths(IconType::Back, {
        "assets/icons/arrow-left.png", "assets/icons/arrow-left.ico"
    });
    addPaths(IconType::Next, {
        "assets/icons/arrow-right.png", "assets/icons/arrow-right.ico"
    });
    addPaths(IconType::Left, {
        "assets/icons/arrow-left.png", "assets/icons/arrow-left.ico"
    });
    addPaths(IconType::Right, {
        "assets/icons/arrow-right.png", "assets/icons/arrow-right.ico"
    });
    addPaths(IconType::Calendar, {
        "assets/icons/calendar.png", "assets/icons/calendar.ico"
    });
}

void IconRenderer::LoadIcons() {
    m_AnyIconLoaded = false;

    for (const auto& [iconKey, candidates] : m_IconPaths) {
        Texture2D loaded{};

        for (const auto& path : candidates) {
            if (!FileExists(path.c_str())) {
                continue;
            }

            loaded = LoadTintableTexture(path);
            if (loaded.id == 0) {
                loaded = LoadTexture(path.c_str());
            }
            if (loaded.id != 0) {
                break;
            }
        }

        if (loaded.id != 0) {
            m_Icons[iconKey] = loaded;
            m_AnyIconLoaded = true;
        }
    }
}

const Texture2D* IconRenderer::GetIconTexture(IconType icon) const {
    auto it = m_Icons.find(static_cast<int>(icon));
    if (it != m_Icons.end() && it->second.id != 0) {
        return &it->second;
    }

    if (icon == IconType::FilterActive) {
        it = m_Icons.find(static_cast<int>(IconType::Filter));
        if (it != m_Icons.end() && it->second.id != 0) {
            return &it->second;
        }
    }

    return nullptr;
}

void IconRenderer::DrawIcon(IconType icon, Vector2 pos, float size, Color color) const {
    DrawIconCentered(icon, { pos.x + size * 0.5f, pos.y + size * 0.5f }, size, color);
}

void IconRenderer::DrawIconCentered(IconType icon, Vector2 centerPos, float size, Color color) const {
    const Texture2D* texture = GetIconTexture(icon);
    if (texture) {
        const Rectangle src = { 0.0f, 0.0f, static_cast<float>(texture->width), static_cast<float>(texture->height) };
        const Rectangle dst = { centerPos.x, centerPos.y, size, size };
        DrawTexturePro(*texture, src, dst, { size * 0.5f, size * 0.5f }, 0.0f, color);
        return;
    }

    DrawFallbackIcon(icon, centerPos, size, color);
}

void IconRenderer::DrawFallbackIcon(IconType icon, Vector2 centerPos, float size, Color color) const {
    const float half = size * 0.5f;

    if (icon == IconType::Filter || icon == IconType::FilterActive) {
        const Vector2 topLeft{ centerPos.x - half * 0.75f, centerPos.y - half * 0.65f };
        const Vector2 topRight{ centerPos.x + half * 0.75f, centerPos.y - half * 0.65f };
        const Vector2 mid{ centerPos.x, centerPos.y - half * 0.05f };
        const Vector2 stemLeft{ centerPos.x - half * 0.18f, centerPos.y + half * 0.55f };
        const Vector2 stemRight{ centerPos.x + half * 0.18f, centerPos.y + half * 0.55f };

        DrawTriangle(topLeft, topRight, mid, color);
        DrawTriangle(mid, stemLeft, stemRight, color);
        return;
    }

    if (icon == IconType::Settings) {
        DrawCircleLinesV(centerPos, half * 0.45f, color);
        for (int i = 0; i < 8; ++i) {
            const float angle = (PI * 2.0f / 8.0f) * static_cast<float>(i);
            const Vector2 inner{ centerPos.x + std::cosf(angle) * (half * 0.55f), centerPos.y + std::sinf(angle) * (half * 0.55f) };
            const Vector2 outer{ centerPos.x + std::cosf(angle) * (half * 0.85f), centerPos.y + std::sinf(angle) * (half * 0.85f) };
            DrawLineEx(inner, outer, std::max(1.0f, size * 0.08f), color);
        }
        return;
    }

    DrawCircleV(centerPos, half * 0.30f, color);
}
