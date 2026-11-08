// Minimal UTF-8 helpers used by ec's input and byte-oriented text buffer.

#ifndef utf8_h_
#define utf8_h_

#include <stddef.h>
#include <stdint.h>

// Decode the character at p.  Invalid input consumes one byte and produces
// U+FFFD, allowing arbitrary files to remain navigable without reading past
// end.  Returns the number of bytes consumed, or zero at end.
size_t utf8Decode(const char* p, const char* end, uint32_t* codePoint);

// Encode one Unicode scalar value.  Returns zero for an invalid value.
size_t utf8Encode(uint32_t codePoint, char out[4]);

const char* utf8Next(const char* p, const char* end);
const char* utf8Prev(const char* begin, const char* p);
size_t utf8Count(const char* begin, const char* end);

#endif // utf8_h_
