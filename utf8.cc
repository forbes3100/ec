// Minimal, locale-independent UTF-8 support.

#include "utf8.h"

static bool continuation(unsigned char c)
{
    return (c & 0xc0) == 0x80;
}

size_t utf8Decode(const char* p, const char* end, uint32_t* codePoint)
{
    if (p >= end)
        return 0;

    const unsigned char c0 = (unsigned char)p[0];
    if (c0 < 0x80)
    {
        *codePoint = c0;
        return 1;
    }

    size_t length;
    uint32_t value;
    uint32_t minimum;
    if (c0 >= 0xc2 && c0 <= 0xdf)
    {
        length = 2;
        value = c0 & 0x1f;
        minimum = 0x80;
    }
    else if (c0 >= 0xe0 && c0 <= 0xef)
    {
        length = 3;
        value = c0 & 0x0f;
        minimum = 0x800;
    }
    else if (c0 >= 0xf0 && c0 <= 0xf4)
    {
        length = 4;
        value = c0 & 0x07;
        minimum = 0x10000;
    }
    else
    {
        *codePoint = 0xfffd;
        return 1;
    }

    if ((size_t)(end - p) < length)
    {
        *codePoint = 0xfffd;
        return 1;
    }

    for (size_t i = 1; i < length; i++)
    {
        const unsigned char c = (unsigned char)p[i];
        if (!continuation(c))
        {
            *codePoint = 0xfffd;
            return 1;
        }
        value = (value << 6) | (c & 0x3f);
    }

    if (value < minimum || value > 0x10ffff ||
        (value >= 0xd800 && value <= 0xdfff))
    {
        *codePoint = 0xfffd;
        return 1;
    }

    *codePoint = value;
    return length;
}

size_t utf8Encode(uint32_t codePoint, char out[4])
{
    if (codePoint <= 0x7f)
    {
        out[0] = (char)codePoint;
        return 1;
    }
    if (codePoint <= 0x7ff)
    {
        out[0] = (char)(0xc0 | (codePoint >> 6));
        out[1] = (char)(0x80 | (codePoint & 0x3f));
        return 2;
    }
    if (codePoint >= 0xd800 && codePoint <= 0xdfff)
        return 0;
    if (codePoint <= 0xffff)
    {
        out[0] = (char)(0xe0 | (codePoint >> 12));
        out[1] = (char)(0x80 | ((codePoint >> 6) & 0x3f));
        out[2] = (char)(0x80 | (codePoint & 0x3f));
        return 3;
    }
    if (codePoint <= 0x10ffff)
    {
        out[0] = (char)(0xf0 | (codePoint >> 18));
        out[1] = (char)(0x80 | ((codePoint >> 12) & 0x3f));
        out[2] = (char)(0x80 | ((codePoint >> 6) & 0x3f));
        out[3] = (char)(0x80 | (codePoint & 0x3f));
        return 4;
    }
    return 0;
}

const char* utf8Next(const char* p, const char* end)
{
    uint32_t codePoint;
    return p + utf8Decode(p, end, &codePoint);
}

const char* utf8Prev(const char* begin, const char* p)
{
    if (p <= begin)
        return begin;

    const char* candidate = p - 1;
    for (int i = 0; i < 3 && candidate > begin &&
         continuation((unsigned char)*candidate); i++)
        candidate--;

    uint32_t codePoint;
    size_t length = utf8Decode(candidate, p, &codePoint);
    if (candidate + length == p)
        return candidate;

    // The byte immediately before p is not part of a valid character.
    return p - 1;
}

size_t utf8Count(const char* begin, const char* end)
{
    size_t count = 0;
    while (begin < end)
    {
        begin = utf8Next(begin, end);
        count++;
    }
    return count;
}
