// Icon rendering system using image sprites (.png/.ico).

#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

class IconRenderer {
public:
    enum class IconType {
        Settings,
        Filter,
        FilterActive,
        Plus,
        Minus,
        Edit,
        Delete,
        Search,
        Menu,
        Close,
        Checkmark,
        Book,
        Rating,
        Lock,
        Unlock,
        Download,
        Upload,
        Save,
        Refresh,
        Chart,
        Grid,
        Goals,
        Dice,
        Back,
        Next,
        Left,
        Right,
        Up,
        Down,
        Home,
        Calendar,
        Clock,
        User,
        Heart,
        Star,
        Copy,
        Paste,
        Cut,
        Undo,
        Redo
    };

    IconRenderer();
    ~IconRenderer();

    void DrawIcon(IconType icon, Vector2 pos, float size, Color color) const;

    void DrawIconCentered(IconType icon, Vector2 centerPos, float size, Color color) const;

    float GetIconWidth(float size) const { return size; }
    float GetIconHeight(float size) const { return size; }

    bool IsReady() const { return m_AnyIconLoaded; }

private:
    std::unordered_map<int, Texture2D> m_Icons;
    std::unordered_map<int, std::vector<std::string>> m_IconPaths;
    bool m_AnyIconLoaded = false;

    void InitIconPaths();
    void LoadIcons();
    const Texture2D* GetIconTexture(IconType icon) const;
    void DrawFallbackIcon(IconType icon, Vector2 centerPos, float size, Color color) const;
};
