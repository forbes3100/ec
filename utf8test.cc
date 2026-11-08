#include <assert.h>
#include <string.h>

#include "utf8.h"

static void roundTrip(uint32_t expected)
{
    char encoded[4];
    size_t length = utf8Encode(expected, encoded);
    assert(length != 0);

    uint32_t actual = 0;
    assert(utf8Decode(encoded, encoded + length, &actual) == length);
    assert(actual == expected);
}

int main()
{
    roundTrip('A');
    roundTrip(0x03b1);       // Greek alpha
    roundTrip(0x2014);       // em dash
    roundTrip(0x10ffff);     // largest Unicode scalar value

    const char text[] = "A\xce\xb1\xe2\x80\x94";
    const char* begin = text;
    const char* end = text + strlen(text);
    assert(utf8Count(begin, end) == 3);

    const char* p = begin;
    p = utf8Next(p, end);
    assert(p == begin + 1);
    p = utf8Next(p, end);
    assert(p == begin + 3);
    p = utf8Next(p, end);
    assert(p == end);
    assert(utf8Prev(begin, p) == begin + 3);
    assert(utf8Prev(begin, begin + 3) == begin + 1);
    assert(utf8Prev(begin, begin + 1) == begin);

    const char malformed[] = "\x80\xc0\xaf";
    uint32_t codePoint = 0;
    assert(utf8Decode(malformed, malformed + sizeof(malformed) - 1,
                      &codePoint) == 1);
    assert(codePoint == 0xfffd);
    assert(utf8Count(malformed, malformed + sizeof(malformed) - 1) == 3);

    char encoded[4];
    assert(utf8Encode(0xd800, encoded) == 0);
    assert(utf8Encode(0x110000, encoded) == 0);
}
