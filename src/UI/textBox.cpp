#include "textBox.h"
#include "../textRenderer.h"
#include <algorithm>
#include <sstream>
#include <cctype> 

TextBox::TextBox(Rectangle r, std::string ph)
    : Widget(r), placeholder(ph)
{}


bool IsWordChar(char c) {
    return std::isalnum(c) || c == '_';
}

void TextBox::MoveLeft(bool jumpWord) {
    if (cursorIndex == 0) return;

    if (!jumpWord) {
        cursorIndex--;
    }
    else {
        // Ctrl+Left
        while (cursorIndex > 0 && !IsWordChar(text[cursorIndex - 1])) {
            cursorIndex--;
        }
        while (cursorIndex > 0 && IsWordChar(text[cursorIndex - 1])) {
            cursorIndex--;
        }
    }
}

void TextBox::MoveRight(bool jumpWord) {
    if (cursorIndex >= (int)text.length()) return;

    if (!jumpWord) {
        cursorIndex++;
    }
    else {
        // Ctrl+Right
        while (cursorIndex < (int)text.length() && IsWordChar(text[cursorIndex])) {
            cursorIndex++;
        }
        while (cursorIndex < (int)text.length() && !IsWordChar(text[cursorIndex])) {
            cursorIndex++;
        }
    }
}

//Key hold to move cursor
void TextBox::HandleKeyRepeat(int key, bool jumpWord, void (TextBox::* moveFunc)(bool)) {
    if (IsKeyPressed(key)) {
        (this->*moveFunc)(jumpWord);
        lastKeyPressed = key;
        keyRepeatTimer = 0.0f;
    }
    else if (IsKeyDown(key) && lastKeyPressed == key) {
        keyRepeatTimer += GetFrameTime();
        if (keyRepeatTimer > KEY_REPEAT_DELAY) {
            // Apply movement multiple times if frame time > repeat rate
            while (keyRepeatTimer > KEY_REPEAT_DELAY + KEY_REPEAT_RATE) {
                (this->*moveFunc)(jumpWord);
                keyRepeatTimer -= KEY_REPEAT_RATE;
            }
        }
    }
    else if (IsKeyReleased(key) && lastKeyPressed == key) {
        lastKeyPressed = -1;
        keyRepeatTimer = 0.0f;
    }
}

void TextBox::Update() {
    if (!isVisible) return;

    Vector2 mouse = GetMousePosition();
    isHovered = CheckCollisionPointRec(mouse, bounds);

  
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        bool wasFocused = isFocused;
        isFocused = isHovered;
       
        if (isFocused && !wasFocused) cursorIndex = (int)text.length();
    }

    
    
   
    if (isHovered || isFocused) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) scrollY -= wheel * 20.0f;
    }

   
    if (isFocused) {


        Widget::DesiredCursor = MOUSE_CURSOR_IBEAM;
        

        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

     
        HandleKeyRepeat(KEY_LEFT, ctrl, &TextBox::MoveLeft);
        HandleKeyRepeat(KEY_RIGHT, ctrl, &TextBox::MoveRight);

       
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (text.length() < maxLength)) {
                text.insert(cursorIndex, 1, (char)key);
                cursorIndex++;
            }
            key = GetCharPressed();
        }

       
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (cursorIndex > 0 && !text.empty()) {
                int deleteCount = 1;
                int startIndex = cursorIndex - 1;

                if (ctrl) {
                    // Ctrl+Backspace
                    int tempIndex = cursorIndex;
                    while (tempIndex > 0 && !IsWordChar(text[tempIndex - 1])) tempIndex--;
                    while (tempIndex > 0 && IsWordChar(text[tempIndex - 1])) tempIndex--;
                    startIndex = tempIndex;
                    deleteCount = cursorIndex - tempIndex;
                }

                text.erase(startIndex, deleteCount);
                cursorIndex = startIndex;
            }
        }

      
        if (IsKeyPressed(KEY_DELETE)) {
            if (cursorIndex < (int)text.length()) {
                text.erase(cursorIndex, 1);
            }
        }

       
        if (IsKeyPressed(KEY_ENTER)) {
            text.insert(cursorIndex, 1, '\n');
            cursorIndex++;
        }

    }
    else if (isHovered) {
        Widget::DesiredCursor = MOUSE_CURSOR_POINTING_HAND;
    }
}

void TextBox::Draw(TextRenderer* renderer) {
    if (!isVisible) return;

    //Background & Border
    DrawRectangleRec(bounds, RAYWHITE);
    Color borderColor = isFocused ? RED : (isHovered ? DARKGRAY : LIGHTGRAY);
    DrawRectangleLinesEx(bounds, 2, borderColor);

    if (!renderer) return;

    //Text to Draw
    std::string textToDraw = text;
    bool showPlaceholder = text.empty() && !isFocused;
    if (showPlaceholder) textToDraw = placeholder;

    
    const float fontSize = 20.0f;
    const float padding = 5.0f;
    const float lineHeight = 24.0f;
    const float contentWidth = bounds.width - (padding * 2);

    float currentX = 0.0f;
    float currentY = 0.0f;

    // Track cursor visualization
    Vector2 cursorPos = { -1, -1 };
    bool cursorFound = false;

    // Track word wrapping state
    int lineStartIndex = 0;
    int lastSpaceIndex = -1; // Index within the whole string

    // Helper to "commit" a line to drawing
    auto finishLine = [&](int endIndex, bool newline) {
        float absoluteY = bounds.y + padding + currentY - scrollY;
        float absoluteX = bounds.x + padding;

        // Draw if visible
        if (absoluteY + lineHeight > bounds.y && absoluteY < bounds.y + bounds.height) {
            std::string lineStr = textToDraw.substr(lineStartIndex, endIndex - lineStartIndex);
            renderer->DrawSimpleText(lineStr, { absoluteX, absoluteY }, fontSize, showPlaceholder ? GRAY : BLACK);
        }

        // Check if cursor is on this line
        if (isFocused && !showPlaceholder && !cursorFound && cursorIndex >= lineStartIndex && cursorIndex <= endIndex) {
            
            std::string sub = textToDraw.substr(lineStartIndex, cursorIndex - lineStartIndex);
            float subWidth = renderer->Measure(sub, fontSize);
            cursorPos = { absoluteX + subWidth, absoluteY };
            cursorFound = true;
        }

        currentY += lineHeight;
        currentX = 0.0f;
        };

    BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height);

    for (int i = 0; i < (int)textToDraw.length(); i++) {
        char c = textToDraw[i];

        // Handle newline
        if (c == '\n') {
            finishLine(i, true);
            lineStartIndex = i + 1;
            lastSpaceIndex = -1;
            continue;
        }

       
        std::string charStr(1, c);
        float charWidth = renderer->Measure(charStr, fontSize);

        // Check Wrap
        if (currentX + charWidth > contentWidth) {
            if (lastSpaceIndex != -1) {
                
                finishLine(lastSpaceIndex, false);
                i = lastSpaceIndex; 
                lineStartIndex = lastSpaceIndex + 1;
                lastSpaceIndex = -1;
            }
            else {
                // Force wrap
                finishLine(i, false);
                lineStartIndex = i;
                i--; 
            }
        }
        else {
            currentX += charWidth;
            if (c == ' ') lastSpaceIndex = i;
        }
    }

  
    finishLine((int)textToDraw.length(), false);

    // 4. Draw Cursor
    if (isFocused && cursorFound) {
       
        float relY = cursorPos.y - bounds.y;
        if (relY < 0) scrollY += relY;
        if (relY + lineHeight > bounds.height) scrollY += (relY + lineHeight - bounds.height);

        // Blinking
        if (((int)(GetTime() * 2) % 2) == 0) {
            renderer->DrawSimpleText("|", { cursorPos.x - 1, cursorPos.y }, fontSize, BLACK);
        }
    }

    EndScissorMode();

    //Scrollbar Logic
    float totalHeight = currentY + lineHeight;
    maxScrollY = std::max(0.0f, totalHeight - (bounds.height - padding * 2));

    // Clamp scroll
    if (scrollY < 0) scrollY = 0;
    if (scrollY > maxScrollY) scrollY = maxScrollY;

    if (maxScrollY > 0) {
        float scrollPerc = scrollY / maxScrollY;
        float barHeight = std::max(20.0f, (bounds.height / totalHeight) * bounds.height);
        float barY = bounds.y + (scrollPerc * (bounds.height - barHeight));
        DrawRectangle((int)(bounds.x + bounds.width - 6), (int)barY, 4, (int)barHeight, Fade(GRAY, 0.5f));
    }
}