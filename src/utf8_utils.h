// UTF-8 helpers for insertion, cursor movement, and codepoint-aware editing.

#pragma once

#include <raylib.h>

#include <cctype>
#include <string>

namespace Utf8 {

inline bool IsContinuationByte(unsigned char ch) {
    return (ch & 0xC0) == 0x80;
}

inline size_t PrevCodepointStart(const std::string& text, size_t byteIndex) {
    if (byteIndex == 0) return 0;

    size_t i = byteIndex - 1;
    while (i > 0 && IsContinuationByte((unsigned char)text[i])) {
        --i;
    }
    return i;
}

inline size_t NextCodepointStart(const std::string& text, size_t byteIndex) {
    if (byteIndex >= text.size()) return text.size();

    int codepointSize = 0;
    GetCodepoint(text.c_str() + byteIndex, &codepointSize);
    if (codepointSize <= 0) codepointSize = 1;

    size_t next = byteIndex + (size_t)codepointSize;
    if (next > text.size()) next = text.size();
    return next;
}

inline int DecodeCodepointAt(const std::string& text, size_t byteIndex, int* outSize = nullptr) {
    if (byteIndex >= text.size()) {
        if (outSize) *outSize = 0;
        return 0;
    }

    int codepointSize = 0;
    int cp = GetCodepoint(text.c_str() + byteIndex, &codepointSize);
    if (codepointSize <= 0) codepointSize = 1;
    if (outSize) *outSize = codepointSize;
    return cp;
}

inline bool IsWhitespaceCodepoint(int cp) {
    return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r';
}

inline bool IsWordCodepoint(int cp) {
    if (cp == '_') return true;
    if (cp >= 128) return !IsWhitespaceCodepoint(cp);
    return std::isalnum((unsigned char)cp) != 0;
}

inline void AppendCodepoint(std::string& text, int cp) {
    int bytes = 0;
    const char* utf8 = CodepointToUTF8(cp, &bytes);
    if (utf8 && bytes > 0) {
        text.append(utf8, utf8 + bytes);
    }
}

inline size_t CodepointCount(const std::string& text) {
    size_t count = 0;
    size_t i = 0;
    while (i < text.size()) {
        ++count;
        i = NextCodepointStart(text, i);
    }
    return count;
}

inline void ErasePrevCodepoint(std::string& text, size_t& cursorByteIndex) {
    if (cursorByteIndex == 0 || text.empty()) return;

    const size_t start = PrevCodepointStart(text, cursorByteIndex);
    text.erase(start, cursorByteIndex - start);
    cursorByteIndex = start;
}

inline void EraseNextCodepoint(std::string& text, size_t& cursorByteIndex) {
    if (cursorByteIndex >= text.size() || text.empty()) return;

    const size_t end = NextCodepointStart(text, cursorByteIndex);
    text.erase(cursorByteIndex, end - cursorByteIndex);
}

} // namespace Utf8
