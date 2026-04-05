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

    // 1. Detekce myši pro scrollování
    if (CheckCollisionPointRec(GetMousePosition(), m_Bounds)) {
        isHovered = true;
        float wheel = GetMouseWheelMove();

        // Rychlost scrollování je 25 pixelù na jedno otoèení
        if (wheel != 0.0f) {
            m_ScrollY += wheel * 25.0f;
        }
    }
    else {
        isHovered = false;
    }

    // 2. Omezení scrollování (aby nešlo odscrollovat do prázdna)
    float maxScroll = (m_ContentHeight > m_Bounds.height) ? (m_ContentHeight - m_Bounds.height) : 0.0f;

    if (m_ScrollY < -maxScroll) m_ScrollY = -maxScroll; // Spodní limit
    if (m_ScrollY > 0.0f) m_ScrollY = 0.0f;             // Horní limit
}

void Label::Draw(TextRenderer* renderer) {
    if (!isVisible || !renderer) return;

    if (!wordWrap) {
        renderer->DrawSimpleText(text, { m_Bounds.x, m_Bounds.y }, fontSize, color);
    }
    else {
        // Zalamování textu
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

                        // FIX: Pokud je SAMOTNÉ SLOVO delší než okno (napø. 200 znakù bez mezery)
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
                            currentLine = chunk; // Zbytek dlouhého slova
                        }
                        else {
                            currentLine = word; // Bìžné slovo, co se jen nevešlo na pøedchozí øádek
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

        // Výpoèet celkové výšky textu (pro limity scrollování)
        m_ContentHeight = m_WrappedLines.size() * (fontSize + 5);

        // Zapneme SCISSOR MODE (oøízne vše, co by chtìlo pøetéct pøes náš Vector2 size)
        BeginScissorMode((int)m_Bounds.x, (int)m_Bounds.y, (int)m_Bounds.width, (int)m_Bounds.height);

        float yOffset = m_Bounds.y + m_ScrollY; // Pøièteme odscrollovanou pozici

        for (const auto& line : m_WrappedLines) {
            if (!line.empty()) {
                // Vykreslíme øádek (Raylib už sám zahodí pixely, co jsou mimo ScissorBox)
                renderer->DrawSimpleText(line, { m_Bounds.x, yOffset }, fontSize, color);
            }
            yOffset += (fontSize + 5);
        }

        EndScissorMode(); // Vypneme oøezávání, zbytek UI se vykreslí normálnì
    }
}