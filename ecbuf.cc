// ****************************************************************************
// ecbuf.cc  Macro Screen Editor buffer routines
//
// Copyright (C) 2023 Scott Forbes
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "termp.h"
#include "ec.h"
#include "utf8.h"

#define COLORS      // put out ANSI color-change escape sequences
                    // (use "xterm +cm" or "xterm*colorMode: true")
#undef TERM_COLORS

#define COMMENT_COLOR MAGENTA

int  row, col;      // current screen row and column position
ScreenCell* ip;     // pointer to current cell in screenImage
bool  cursorGood;   // TRUE if ip matches real screen cursor position
int curDispAttr;    // current char attributes

// ----------------------------------------------------------------------------
// Clear screen using proper colors.

void clearScreenC()
{
    normalMode();
#ifdef COLORS
    if (ansiColors)
        printf("\e[3%d;4%dm", WHITE, BLACK);    // set colors to black on white
#endif
    clearScreen();
    for (int r = 0; r < SCRMAXHT; r++)
        for (int c = 0; c < SCRMAXWD; c++)
        {
            screenImage[r][c].codePoint = ' ';
            screenImage[r][c].attributes = 0;
        }
}

// ----------------------------------------------------------------------------
// Clear a line on screen using proper colors.

void clearLineC()
{
    normalMode();
#ifdef COLORS
    if (ansiColors)
        printf("\e[3%d;4%dm", WHITE, BLACK);    // set colors to black on white
#endif
    clearLine();
}

// ----------------------------------------------------------------------------
// Draw one Unicode code point at screen position ip and advance one cell.

void putAttrChar(uint32_t codePoint, int cellAttributes)
{
    if (ip->codePoint != codePoint || ip->attributes != cellAttributes)
    {
        int needAttrOn = attrib;
        int needAttrOff = curDispAttr & ~attrib;

        if (!cursorGood)
        {
            gotoxy(col, row);
            cursorGood = TRUE;
        }

        if (needAttrOff)
        {
            normalMode();
#ifdef TERM_COLORS
            setForeColor(BLACK);
#endif
#ifdef COLORS
            if (ansiColors)
                printf("\e[3%d;4%dm", WHITE, BLACK);    // set colors to black on white
#endif
        }
        else
            needAttrOn &= ~curDispAttr;

        if (needAttrOn & AT_BOLD)
        {
        if (!(needAttrOn & AT_REVERSE))
#ifdef TERM_COLORS
            setForeColor(COMMENT_COLOR);
#endif
#ifdef COLORS
            printf("\e[3%dm", COMMENT_COLOR); // set foreground color
#endif
            boldMode();
        }
        if (needAttrOn & AT_REVERSE)
            reverseMode();
        if (needAttrOn & AT_UNDERLINE)
            underlineMode();

        curDispAttr = attrib;
        char encoded[4];
        size_t length = utf8Encode(codePoint, encoded);
        if (!length)
            length = utf8Encode(0xfffd, encoded);
        fwrite(encoded, 1, length, stdout);
        ip->codePoint = codePoint;
        ip->attributes = (unsigned short)cellAttributes;
    }
    else
        cursorGood = FALSE;

    ip++;
    col++;
}

// ----------------------------------------------------------------------------
// Update the screen as necessary, given the text and screen window pos
// aborts update if a key is pressed.

void update(const char* atopPos, int hScroll, int tabSize, int atopRow,
            int abotRow)
{
#ifdef TERM_COLORS
    setForeColor(BLACK);
    setBackColor(WHITE);
#endif
#ifdef COLORS
    if (ansiColors)
        printf("\e[3%d;4%dm", WHITE, BLACK);    // set colors to black on white
#endif
    curDispAttr = 0;
    row = atopRow;
    col = -hScroll;
    cursorGood = FALSE;
    bool atEOT = FALSE;
    int comment1Line = FALSE;
    ip = &screenImage[row][0];
    const char* textEnd = atopPos + strlen(atopPos);

    const char* p;
    for (p = atopPos; row <= abotRow; )
    {
        if (p == bcursPos)
            if (!atEOT)
            {
                cursRow = row;
                cursCol = col;
            }
        size_t charLength = 1;
        if ((unsigned char)*p < ' ')
        {
            if ((*p == '\n') || *p == 0)        // '\n' or EOF
            {
                ScreenCell* nextRow = &screenImage[row+1][0];
                bool dirty = FALSE;
                if (!(attrib & AT_REVERSE) && comment1Line)
                    attrib &= ~AT_BOLD;

                while (ip < nextRow)
                {
                    if (ip->codePoint != ' ' || ip->attributes != 0)
                    {
                        ip->codePoint = ' ';
                        ip->attributes = 0;
                        dirty = TRUE;
                    }
                    ip++;
                }
                if (dirty)
                {
                    if (!cursorGood)
                        gotoxy(max(col, 0), row);
                    cursorGood = TRUE;
                    if (row < screenHt-1 || col < screenWd-1)
                    {
                        clearLineC();
                        curDispAttr = 0;
                    }
                    if (row < screenHt-1)
                        gotoxy(0, row + 1);
                }
                else
                    cursorGood = FALSE;
                row++;
                col = -hScroll;
            }
            else if (*p == '\t')                // TAB
            {
                int tabcol = col + hScroll + tabSize;
                tabcol = tabcol - (tabcol % tabSize) - hScroll;
                do {
                    if (col >= 0 && col < screenWd-1)
                        putAttrChar(' ', 0);
                    else
                    {
                        cursorGood = FALSE;
                        col++;
                    }
                } while (col < tabcol);
            }
            else                    // it's a generic control character
            {
                if (col >= 0 && col < screenWd-2)
                {
                    putAttrChar('^', 0);
                    putAttrChar(*p + 0x40, 0);
                }
                else
                {
                    cursorGood = FALSE;
                    col += 2;
                }
            }
        }
        else
        {
            uint32_t codePoint;
            charLength = utf8Decode(p, textEnd, &codePoint);
            if (!charLength)
                charLength = 1;
            if (col >= 0 && col < screenWd-1)       // other text
            {
                int cAttr = attrib;

                // turn on bold attribute if a comment
                if (*p == '/' && *(p+1) == '/')
                {
                    attrib |= AT_BOLD;
                    comment1Line = TRUE;
                }
                else if (*p == '/' && *(p+1) == '*')
                {
                    attrib |= AT_BOLD;
                    comment1Line = FALSE;
                }

                putAttrChar(codePoint, cAttr);

                if (p > atopPos && *(p-1) == '*' && *p == '/')
                    attrib &= ~AT_BOLD;

            }
            else
            {
                cursorGood = FALSE;
                attrib &= ~AT_BOLD;
                col++;
            }
        }
        if (*p != 0)
            p += charLength;
        else
            atEOT = TRUE;
    }

    normalMode();
#ifdef TERM_COLORS
    setForeColor(BLACK);
#endif
#ifdef COLORS
    if (ansiColors)
        printf("\e[0m"); // reset colors
#endif
}

// ----------------------------------------------------------------------------
// Move memory: size bytes of src to dest.

void movec(const char* src, char* dest, int size)
{
    for ( ; size > 0; size--)
        *dest++ = *src++;
}

// ----------------------------------------------------------------------------
// Clear buffer b.

void clearBuffer()
{
    bcursPos = bstart;
    beot = bstart;
    bcursPos = bstart;
    btagPos = bstart;
    btopRowPos = bstart;
    bhScroll = 0;
    *beot = 0;
    btabSize = givenTabSize;
    if (!btabSize)
        btabSize = 8;
    buffer[b].changed = FALSE;
    buffer[b].lineEnding = lEnd_Unix;
}

// ----------------------------------------------------------------------------
// Put buffer b pointers back.

void bToBuffer()
{
    BuffRec* p = &buffer[b];
    p->start = bstart;
    p->end = bend;
    p->eot = beot;
    p->cursPos = bcursPos;
    p->tagPos = btagPos;
    p->topRowPos = btopRowPos;
    p->hScroll = bhScroll;
    p->tabSize = btabSize;
}

// ----------------------------------------------------------------------------
// Select buffer as buffer b.

void selectBuffer(int newb)
{
    bToBuffer();
    b = newb;
    BuffRec* p = &buffer[b];
    bstart = p->start;
    if (bstart)
    {
        bend = p->end;
        beot = p->eot;
        bcursPos = p->cursPos;
        btagPos = p->tagPos;
        btopRowPos = p->topRowPos;
        bhScroll = p->hScroll;
        btabSize = p->tabSize;
    }
    else
    {
        bstart = (char*)malloc((size_t)(ELBOW+2));
        if (!bstart)
        {
            bend = bstart;
            throw new Error("out of memory");
        }
        bend = bstart + ELBOW;
        clearBuffer();
    }
    lastTopPos = 0;
}

// ----------------------------------------------------------------------------
// Extract file name proper in file pathname for buffer b.

void makeFName(int b)
{
    BuffRec* buf = &buffer[b];
    char* p;
    for (p = buf->fpath + strlen(buf->fpath) - 1; p > buf->fpath; p--)
        if (*p == '/')
        {
            p++;
            break;
        }
    buf->fname = p;
}

// ----------------------------------------------------------------------------
// Cursor position to line#, char# for status line.
// Exits early if a key is pressed.

void cursToLineChar()
{
    int i = 1;
    char* p = bstart;
    while (p < bcursPos)
        if (*p++ == '\n')
        {
            i++;
        }
    lineNum = i;
    p = bcursPos;
    while (p > bstart && *(p - 1) != '\n')
        p--;
    charNum = (int)utf8Count(p, bcursPos) + 1;
}

// ----------------------------------------------------------------------------
// Find string in buffer starting at bcursPos, forwards or backwards search.

const char* find(const char* str, int len)
{
    bool caseSens = FALSE;
    const char* p;
    for (p = str; *p; p++)
        if (isupper((unsigned char)*p))
        {
            caseSens = TRUE;
            break;
        }

    if (len > 0)
    {
        char ch0 = str[0];

        if (findForward)
        {
            char* last = beot - len;
            int i = 0;
            for (p = bcursPos; p < last; )
            {
                char c = *p++;
                if (!caseSens && isupper((unsigned char)c))
                    c = (char)tolower((unsigned char)c);
                if (c == ch0)
                {
                    const char* p2 = p;
                    for (i = 1; i <= len;)
                    {
                        c = *p2++;
                        if (!caseSens && isupper((unsigned char)c))
                            c = (char)tolower((unsigned char)c);
                        if (str[i++] != c)
                            break;
                    }
                    if (i > len)
                        return p-1;
                }
            }
        }
        else
        {
            char* last = bstart;
            int i = 0;
            for (p = bcursPos; p > last; )
            {
                char c = *(p--);
                if (!caseSens && isupper((unsigned char)c))
                    c = (char)tolower((unsigned char)c);
                if (c == ch0)
                {
                    const char* p2 = p+1;
                    for (i = 1; i <= len;)
                    {
                        c = *p2++;
                        if (!caseSens && isupper((unsigned char)c))
                            c = (char)tolower((unsigned char)c);
                        if (str[i++] != c)
                            break;
                    }
                    if (i > len)
                        return p;
                }
            }
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Insert string into buffer b, at p.  If str is zero, it doesn't copy
//  any text.

void insert(char* p, const char* str, int n)
{
    if (buffer[b].readOnly)
        throw new Error("%s is a read-only file", buffer[b].fname);

    if (beot+n > bend)      // if no more room in buffer block, expand it
    {
        int newSize = beot - bstart + n + ELBOW + 1;
        char* newp = (char*)malloc((size_t)newSize);
        if (newp)
        {
            char* op = newp;
            // first copy buffer text before insertion point into new buf
            const char* ip = bstart;
            while (ip < p)
                *op++ = *ip++;

            // then add new text string (n bytes at str)
            if (str)
            {
                ip = str;
                for (int i = n; i > 0; i--)
                    *op++ = *ip++;
            }
            else
                op += n;

            /* finally, append buffer text after insertion point, and
               trailing zero. */
            if (p)
            {
                for (ip = p; ip < beot; )
                    *op++ = *ip++;
            }
            *op = 0;

            free(bstart);
            // ptrdiff_t insures 64-bit pointer offsets are handled
            ptrdiff_t offset = newp - bstart;
            bstart = newp;
            bend = bstart + newSize - 1;
            beot = bend - ELBOW;
            bcursPos += offset;
            btopRowPos += offset;
        }
        else
            throw new Error("out of memory");
    }
    else        // enough room in buffer, insert string
    {
        // first, move text after insertion point up by n
        const char* ip = beot;
        char* op = beot + n;
        int i;
        for (i = beot-p; i >= 0; i--)
            *op-- = *ip--;

        // then copy in new string, if any
        if (str)
        {
            ip = str;
            op = (char*)p;
            for (i = n; i > 0; i--)
                *op++ = *ip++;
        }
        beot += n;
    }
    buffer[b].changed = TRUE;
}

// ----------------------------------------------------------------------------
// Delete n characters in buffer b at p.

void del(char* p, int n)
{
    if (buffer[b].readOnly)
        throw new Error("read-only file");

    if (beot >= p+n)
    {
        movec(p+n, p, beot+1-p-n);
        beot -= n;
    }
    else
        beot = p;
    buffer[b].changed = TRUE;
}

// ----------------------------------------------------------------------------
// Replace one UTF-8 character in buffer b at p with code point c.

void replace(char* p, int c)
{
    char repString[4];
    size_t repLength = utf8Encode((uint32_t)c, repString);

    if (buffer[b].readOnly)
        throw new Error("read-only file");
    if (!repLength)
        return;

    if (p < beot)
    {
        char* next = (char*)utf8Next(p, beot);
        int oldLength = (int)(next - p);
        if (oldLength == (int)repLength)
        {
            movec(repString, p, (int)repLength);
            buffer[b].changed = TRUE;
        }
        else
        {
            del(p, oldLength);
            insert(p, repString, (int)repLength);
        }
    }
    else
        insert(p, repString, (int)repLength);
}

// ----------------------------------------------------------------------------
// Check the current buffer's filename, and set tab size to 4 if a c file.
// A .exrc file tabsize overrides this.

void setTabSizeFromType()
{
    if (givenTabSize)
    {
        btabSize = givenTabSize;
        return;
    }
    char* p = buffer[b].fname;
    if (!p)
        return;
    p = p + strlen(p) - 2;
    while (*p != '.' && p > buffer[b].fname)
        p--;
    if (p[0] == '.' && (p[1] == 'c' || p[1] == 'h' || p[1] == 'v'))
        btabSize = 4;
}

// ----------------------------------------------------------------------------
// Read a file and insert it into buffer b at cursor.

bool insertFile(const char* fileName, InsertMode mode)
{
    if (buffer[b].readOnly)
        throw new Error("read-only file");
    bool wasEmpty = (beot == bstart);

    FILE* fp = fopen(fileName, "r");
    if (fp == NULL && mode == READ)
        throw new Error("can't find file '%s'", fileName);
    buffer[b].readOnly = FALSE;
    if (fp == NULL)
        buffer[b].newFile = TRUE;
    else
    {
        buffer[b].newFile = FALSE;
        int size = 0;
        if (!(fseek(fp, (long)0, 2) == 0 && (size = ftell(fp)) != EOF
            && fseek(fp, (long)0, 0) == 0))
            throw new Error("can't position file '%s'", fileName);

        insert(bcursPos, 0, size);          // add space for text
        char* p = bcursPos;
        while (size > 0)                    // read text into space
        {
            int freadSize = size;
            if (size > 32000)
                freadSize = 32000;
            fread(p, 1, freadSize, fp);
            size -= freadSize;
            p += freadSize;
        }
        char* pEnd = p;
        fclose(fp);

        // convert line endings to Unix style
        char lineEnding = lEnd_Unix;
        char* p2 = bcursPos;
        for (p = bcursPos; p < pEnd; p++)
            if (*p == '\r')
            {
                if (p[1] == '\n')
                {
                    lineEnding = lEnd_PC;
                    p++;
                }
                else
                    lineEnding = lEnd_Mac;
                *p2++ = '\n';
            }
            else
                *p2++ = *p;
        del(p2, p - p2);
        buffer[b].lineEnding = lineEnding;

        if (mode == OPEN)
            buffer[b].readOnly = access(fileName, W_OK);
    }

    if (wasEmpty)
    {                   // if buffer was empty, set its TAB spacing
        setTabSizeFromType();
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// Write text in current buffer to a file, with backup and line ending fix.

void writeToFile(const char* fName, const char* fPath, char* start, char* end)
{
    // if file already exists, back it up first
    struct stat fileStats;
    char backupName[MAX_LINE+10];
    bool haveStats = FALSE;
    FILE* fp = fopen(fPath, "r");
    if (fp)
    {
        fclose(fp);
        stat(fPath, &fileStats);
        haveStats = TRUE;
        snprintf(backupName, sizeof(backupName), ".~%s", fPath);
        rename(fPath, backupName);
    }

    // make sure it's writable
    fp = fopen(fPath, "w");
    if (!fp)
        throw new Error("can't write file '%s'", fName);

    // convert line endings to file's style
    char lineEnding = buffer[b].lineEnding;
    if (lineEnding != lEnd_Unix)
    {
        // offsets needed because insert() may change bstart
        ptrdiff_t startOffs = start - bstart;
        ptrdiff_t endOffs = end - beot;
        ptrdiff_t i = startOffs;
        char* p;
        while ((p = bstart+i) < beot + endOffs)
        {
            if (*p == '\n')
                switch (lineEnding)
                {
                    case lEnd_PC:
                        insert(p, "\r", 1);
                        i++;
                        break;
                    case lEnd_Mac:
                        *p = '\r';
                        break;
                }
            i++;
        }
        start = bstart + startOffs;
        end = beot + endOffs;
    }

    // write text
    int size = end - start;
    char* p = start;
    while (size > 0)
    {
        int writeSize = size;
        if (size > 32000)
            writeSize = 32000;
        fwrite(p, 1, writeSize, fp);
        size -= writeSize;
        p += writeSize;
    }
    fclose(fp);

    // convert line endings back
    for (p = start; p < end; p++)
        if (*p == '\r')
        {
            if (p[1] == '\n')
            {
                del(p+1, 1);
                end--;
            }
            *p = '\n';
        }

    if (haveStats)
        chmod(fPath, fileStats.st_mode);
    buffer[b].changed = FALSE;
    buffer[b].newFile = FALSE;

    // if backups disabled, delete backup
    if (!makeBak)
        unlink(backupName);
}

// ----------------------------------------------------------------------------
// Save buffer b file, if open.

void saveBuffer()
{
    if (buffer[b].readOnly)
        throw new Error("read-only file");
    if (!buffer[b].open)
        throw new Error("no file open");

    writeToFile(buffer[b].fname, buffer[b].fpath, bstart, beot);
}

// ----------------------------------------------------------------------------
// Write tag-to-cursor to a file.

void writeTaggedToFile(const char* fName)
{
    if (btagPos != bcursPos)
    {
        char* start;
        char* end;
        if (bcursPos > btagPos)
        {
            start = btagPos;
            end = bcursPos;
        }
        else
        {
            start = bcursPos;
            end = btagPos;
        }
        writeToFile(fName, fName, start, end);
    }
}

// ----------------------------------------------------------------------------
// Move pointer p to beginning of line.

void beginLine(char** p)
{
    char* rp = *p - 1;
    if (rp < bstart)
        return;
    while (rp > bstart && *rp != '\n')
        rp--;
    if (*rp != '\n')
        *p = bstart;
    else
        *p = rp + 1;
}

// ----------------------------------------------------------------------------
// Move pointer p back n characters.

void backChar(char** p, int n)
{
    while (n-- > 0 && *p > bstart)
        *p = (char*)utf8Prev(bstart, *p);
}

// ----------------------------------------------------------------------------
// Move pointer p forward n characters.

void fwdChar(char** p, int n)
{
    while (n-- > 0 && *p < beot)
        *p = (char*)utf8Next(*p, beot);
}

// ----------------------------------------------------------------------------
// Move pointer p back n words.

void backWord(char** p, int n)
{
    char* rp = *p;
    for ( ; n > 0 && rp > bstart; n--)
    {
        uint32_t codePoint = 0;
        while (rp > bstart)             // first back up over whitespace
        {
            rp = (char*)utf8Prev(bstart, rp);
            utf8Decode(rp, beot, &codePoint);
            if (codePoint > ' ' || codePoint == '\n')
                break;
        }
        if (codePoint != '\n')          // not end of prev line
        {
            bool word = (codePoint == '_') ||
                (codePoint < 0x80 && isalnum((unsigned char)codePoint)) ||
                (codePoint >= 0x0370 && codePoint <= 0x03ff);
            while (rp > bstart)
            {
                char* previous = (char*)utf8Prev(bstart, rp);
                uint32_t previousCodePoint;
                utf8Decode(previous, beot, &previousCodePoint);
                bool previousWord = (previousCodePoint == '_') ||
                    (previousCodePoint < 0x80 &&
                     isalnum((unsigned char)previousCodePoint)) ||
                    (previousCodePoint >= 0x0370 &&
                     previousCodePoint <= 0x03ff);
                if (previousCodePoint <= ' ' || previousWord != word)
                    break;
                rp = previous;
            }
        }
    }
    *p = rp;
}

// ----------------------------------------------------------------------------
// Move pointer p forward n words.

void fwdWord(char** p, int n)
{
    char* rp = *p;
    for ( ; n > 0 && rp < beot; n--)
    {
        uint32_t codePoint;
        utf8Decode(rp, beot, &codePoint);
        if (codePoint == '\n')
        {
            rp = (char*)utf8Next(rp, beot);
            if (rp >= beot)
                break;
            utf8Decode(rp, beot, &codePoint);
        }
        if (codePoint > ' ')
        {
            bool word = (codePoint == '_') ||
                (codePoint < 0x80 && isalnum((unsigned char)codePoint)) ||
                (codePoint >= 0x0370 && codePoint <= 0x03ff);
            do
            {
                rp = (char*)utf8Next(rp, beot);
                if (rp >= beot)
                    break;
                utf8Decode(rp, beot, &codePoint);
                bool nextWord = (codePoint == '_') ||
                    (codePoint < 0x80 &&
                     isalnum((unsigned char)codePoint)) ||
                    (codePoint >= 0x0370 && codePoint <= 0x03ff);
                if (codePoint <= ' ' || nextWord != word)
                    break;
            } while (true);
        }
        while (rp < beot)           // find next word
        {
            utf8Decode(rp, beot, &codePoint);
            if (codePoint > ' ' || codePoint == '\n')
                break;
            rp = (char*)utf8Next(rp, beot);
        }
    }
    *p = rp;
}

// ----------------------------------------------------------------------------
// Move pointer p back n lines.

void backLine(char** p, int n)
{
    if (*p > bstart)
    {
        char* rp = *p - 1;
        while (rp > bstart && n > 0)
        {
            rp--;
            if (*rp == '\n')
                n--;
        }
        *p = rp;
        if (rp > bstart || (rp == bstart && n == 0))
            fwdChar(p, 1);
    }
}

// ----------------------------------------------------------------------------
// Move pointer p forward n lines.

void fwdLine(char** p, int n)
{
    char* rp = *p;
    while (rp < beot && n > 0)
        if (*(rp++) == '\n')
            n--;
    *p = rp;
}

// ----------------------------------------------------------------------------
// Move pointer p geometrically up n lines.

void upLine(char** p, int n)
{
    char* rp = *p;
    beginLine(p);
    int col = 0;
    char* rp2 = *p;
    while (rp2 < rp)
        if (*rp2 == '\t')
        {
            rp2++;
            col += btabSize - ((col + btabSize) % btabSize);
        }
        else
        {
            rp2 = (char*)utf8Next(rp2, rp);
            col++;
        }
    backLine(p, n);
    rp = *p;
    int i = 0;
    while (rp < beot && i < col && *rp != '\n')
        if (*rp == '\t')
        {
            rp++;
            i += btabSize - ((i + btabSize) % btabSize);
        }
        else
        {
            rp = (char*)utf8Next(rp, beot);
            i++;
        }
    *p = rp;
}

// ----------------------------------------------------------------------------
// Move pointer p geometrically down n lines.

void downLine(char** p, int n)
{
    char* rp = *p;
    beginLine(p);
    int col = 0;
    char* rp2 = *p;
    while (rp2 < rp)
        if (*rp2 == '\t')
        {
            rp2++;
            col += btabSize - ((col + btabSize) % btabSize);
        }
        else
        {
            rp2 = (char*)utf8Next(rp2, rp);
            col++;
        }
    fwdLine(p, n);
    rp = *p;
    int i = 0;
    while (rp < beot && i < col && *rp != '\n')
        if (*rp == '\t')
        {
            rp++;
            i += btabSize - ((i + btabSize) % btabSize);
        }
        else
        {
            rp = (char*)utf8Next(rp, beot);
            i++;
        }
    *p = rp;
}
