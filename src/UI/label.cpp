
// Implementation of the Label widget class.
// Renders text with optional word wrapping and custom styling.


#include "label.h"
#include "../textRenderer.h"
#include <sstream>

Label::Label(Anchor anchor, Vector2 offset, Vector2 size, std::string t, int fSize, Color c, bool wrap)
    : Widget(anchor, offset, size), text(t), fontSize(fSize), color(c), wordWrap(wrap)
{
    OnWindowResize(GetScreenWidth(), GetScreenHeight());
}

void Label::Update() {
    if (!isVisible) return;

   
    if (CheckCollisionPointRec(GetMousePosition(), m_Bounds)) {
        isHovered = true;
        float wheel = GetMouseWheelMove();

        
        if (wheel != 0.0f) {
            m_ScrollY += wheel * 25.0f;
        }
    }
    else {
        isHovered = false;
    }

   
    // Scroll range depends on rendered content height computed in Draw().
    float maxScroll = (m_ContentHeight > m_Bounds.height) ? (m_ContentHeight - m_Bounds.height) : 0.0f;

    if (m_ScrollY < -maxScroll) m_ScrollY = -maxScroll; 
    if (m_ScrollY > 0.0f) m_ScrollY = 0.0f;           
}

void Label::Draw(TextRenderer* renderer) {
    if (!isVisible || !renderer) return;

    if (!wordWrap) {
        renderer->DrawSimpleText(text, { m_Bounds.x, m_Bounds.y }, fontSize, color);
    }
    else {
        // Recompute wrapped lines only when text changes to keep per-frame cost low.
        if (text != m_LastText) {
            m_LastText = text;
            m_WrappedLines.clear();

            std::stringstream ss(text);
            std::string paragraph;

            while (std::getline(ss, paragraph, '\n')) {
                if (paragraph.empty()) {
                    m_WrappedLines.push_back("");
                    continue;
                }

                std::stringstream wordStream(paragraph);
                std::string word;
                std::string currentLine = "";

                while (wordStream >> word) {
                    std::string testLine = currentLine.empty() ? word : currentLine + " " + word;

                    if (renderer->Measure(testLine, fontSize) > m_Bounds.width) {
                        if (!currentLine.empty()) {
                            m_WrappedLines.push_back(currentLine);
                            currentLine = "";
                        }

                        // If one token is wider than the label, split it into character chunks.
                        if (renderer->Measure(word, fontSize) > m_Bounds.width) {
                            std::string chunk = "";
                            for (char c : word) {
                                if (renderer->Measure(chunk + c, fontSize) > m_Bounds.width) {
                                    m_WrappedLines.push_back(chunk);
                                    chunk = std::string(1, c);
                                }
                                else {
                                    chunk += c;
                                }
                            }
                            currentLine = chunk; 
                        }
                        else {
                            currentLine = word; 
                        }
                    }
                    else {
                        currentLine = testLine;
                    }
                }
                if (!currentLine.empty()) {
                    m_WrappedLines.push_back(currentLine);
                }
            }
        }

        
        m_ContentHeight = m_WrappedLines.size() * (fontSize + 5);

        // Clip rendering to bounds so long text does not bleed into neighboring widgets.
        BeginScissorMode((int)m_Bounds.x, (int)m_Bounds.y, (int)m_Bounds.width, (int)m_Bounds.height);

        float yOffset = m_Bounds.y + m_ScrollY; 

        for (const auto& line : m_WrappedLines) {
            if (!line.empty()) {
                renderer->DrawSimpleText(line, { m_Bounds.x, yOffset }, fontSize, color);
            }
            yOffset += (fontSize + 5);
        }

        EndScissorMode(); 
    }
}