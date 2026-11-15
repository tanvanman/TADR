#pragma once

#include <cstddef>
#include <cstdint>

// Human build menus are GUI gadgets, not the AI-only UnitDef CANBUILD list.
// This bounded reader deliberately recognizes only complete build buttons.
namespace MexUpgradeMenu
{
    namespace Detail
    {
        struct Text { const char* data; std::size_t size; };
        inline char Lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c; }
        inline bool Equal(Text text, const char* value)
        {
            std::size_t i = 0;
            for (; i < text.size && value[i]; ++i)
                if (Lower(text.data[i]) != Lower(value[i])) return false;
            return i == text.size && value[i] == 0;
        }
        inline bool Space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v'; }
        inline bool Gadget(Text text)
        {
            const char prefix[] = "gadget";
            if (text.size <= sizeof(prefix) - 1) return false;
            for (std::size_t i = 0; i < sizeof(prefix) - 1; ++i)
                if (Lower(text.data[i]) != prefix[i]) return false;
            for (std::size_t i = sizeof(prefix) - 1; i < text.size; ++i)
                if (text.data[i] < '0' || text.data[i] > '9') return false;
            return true;
        }
        inline bool Integer(Text text, std::int32_t& value)
        {
            if (!text.size) return false;
            std::size_t i = 0;
            const bool negative = text.data[0] == '-';
            if (negative || text.data[0] == '+') ++i;
            if (i == text.size) return false;
            std::uint32_t number = 0;
            const std::uint32_t limit = negative ? 2147483648u : 2147483647u;
            for (; i < text.size; ++i)
            {
                const char digit = text.data[i];
                if (digit < '0' || digit > '9' || number > (limit - (digit - '0')) / 10) return false;
                number = number * 10 + static_cast<unsigned>(digit - '0');
            }
            value = negative ? static_cast<std::int32_t>(-static_cast<std::int64_t>(number))
                : static_cast<std::int32_t>(number);
            return true;
        }

        enum Kind { End, Word, OpenSection, CloseSection, OpenBody, CloseBody, Assign, Semicolon, Invalid };
        struct Token { Kind kind; Text text; };
        class Reader
        {
            const char* data;
            std::size_t length, position = 0;
        public:
            Reader(const char* bytes, std::size_t size) : data(bytes), length(size) {}
            Token Next()
            {
                for (;;)
                {
                    while (position < length && Space(data[position])) ++position;
                    if (position == length || !data[position]) return {End, {nullptr, 0}};
                    if (data[position] != '/' || position + 1 == length) break;
                    if (data[position + 1] == '/')
                    {
                        position += 2;
                        while (position < length && data[position] && data[position] != '\n' && data[position] != '\r') ++position;
                    }
                    else if (data[position + 1] == '*')
                    {
                        position += 2;
                        while (position + 1 < length && data[position]
                            && !(data[position] == '*' && data[position + 1] == '/')) ++position;
                        if (position + 1 >= length || !data[position]) return {Invalid, {nullptr, 0}};
                        position += 2;
                    }
                    else break;
                }
                const char c = data[position++];
                switch (c)
                {
                case '[': return {OpenSection, {nullptr, 0}};
                case ']': return {CloseSection, {nullptr, 0}};
                case '{': return {OpenBody, {nullptr, 0}};
                case '}': return {CloseBody, {nullptr, 0}};
                case '=': return {Assign, {nullptr, 0}};
                case ';': return {Semicolon, {nullptr, 0}};
                default: break;
                }
                const bool quoted = c == '"' || c == '\'';
                const std::size_t start = quoted ? position : position - 1;
                if (quoted)
                {
                    while (position < length && data[position])
                    {
                        if (data[position] == c)
                        {
                            const auto size = position++ - start;
                            return {Word, {data + start, size}};
                        }
                        if (data[position] == '\\' && position + 1 < length && data[position + 1] == c) position += 2;
                        else ++position;
                    }
                    return {Invalid, {nullptr, 0}};
                }
                while (position < length && data[position] && !Space(data[position]))
                {
                    const char next = data[position];
                    if (next == '[' || next == ']' || next == '{' || next == '}' || next == '=' || next == ';'
                        || next == '"' || next == '\''
                        || (next == '/' && position + 1 < length && (data[position + 1] == '/' || data[position + 1] == '*')))
                        break;
                    ++position;
                }
                return {Word, {data + start, position - start}};
            }
        };

        struct Button
        {
            unsigned seen = 0;
            bool valid = true, name = false;
            std::int32_t id = 0, active = 0, attribs = 0;
            bool Build() const { return valid && name && id == 1 && active != 0 && (attribs & 32) != 0; }
        };
        enum Context { Top, Other, GadgetBody, CommonBody };
        class Parser
        {
            Reader reader;
            const char* replacement;
            bool found = false;
            bool Body(Context context, unsigned depth, Button& button)
            {
                if (depth > 32) return false;
                unsigned commons = 0;
                bool matchingCommon = false;
                for (;;)
                {
                    const auto token = reader.Next();
                    if (token.kind == End) return context == Top;
                    if (token.kind == CloseBody)
                    {
                        if (context == Top) return false;
                        if (context == GadgetBody && commons == 1 && matchingCommon) found = true;
                        return true;
                    }
                    if (token.kind == Semicolon) continue;
                    if (token.kind == OpenSection)
                    {
                        const auto name = reader.Next();
                        if (name.kind != Word || reader.Next().kind != CloseSection || reader.Next().kind != OpenBody) return false;
                        Button child;
                        const bool common = context == GadgetBody && Equal(name.text, "common");
                        const auto childContext = common ? CommonBody
                            : (context == Top && Gadget(name.text) ? GadgetBody : Other);
                        if (!Body(childContext, depth + 1, child)) return false;
                        if (common) { ++commons; matchingCommon = child.Build(); }
                        continue;
                    }
                    if (token.kind != Word || reader.Next().kind != Assign) return false;
                    unsigned values = 0;
                    Token value = {Invalid, {nullptr, 0}};
                    for (;;)
                    {
                        const auto part = reader.Next();
                        if (part.kind == Semicolon) break;
                        if (part.kind != Word && part.kind != Assign) return false;
                        value = part;
                        ++values;
                    }
                    if (context != CommonBody) continue;
                    const unsigned field = Equal(token.text, "name") ? 1 : Equal(token.text, "id") ? 2
                        : Equal(token.text, "active") ? 4 : Equal(token.text, "attribs") ? 8 : 0;
                    if (!field) continue;
                    if ((button.seen & field) || values != 1 || value.kind != Word) { button.valid = false; continue; }
                    button.seen |= field;
                    if (field == 1) button.name = Equal(value.text, replacement);
                    else
                    {
                        std::int32_t number = 0;
                        if (!Integer(value.text, number)) { button.valid = false; continue; }
                        if (field == 2) button.id = number;
                        else if (field == 4) button.active = number;
                        else button.attribs = number;
                    }
                }
            }
        public:
            Parser(const char* data, std::size_t length, const char* name) : reader(data, length), replacement(name) {}
            bool Contains() { Button ignored; return Body(Top, 0, ignored) && found; }
        };
    }

    inline bool ContainsBuildButton(const char* data, std::size_t length, const char* replacement)
    {
        if (!data || !length || length > 1024 * 1024 || !replacement || !*replacement) return false;
        return Detail::Parser(data, length, replacement).Contains();
    }
}
