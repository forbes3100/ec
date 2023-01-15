// ****************************************************************************
// termp.h  Terminal-dependent definitions
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

#ifndef termp_h_
#define termp_h_

// UNIX terminal-dependent routines

#ifdef SunOS
#define bool CURSES_BOOL
#include <curses.h>
#undef bool
extern "C" { char* tgoto(char* cap, int col, int row); }
typedef char tputc_t;
#else
#include <curses.h>
typedef int tputc_t;
#define TERMIOS
#endif

#ifdef TERMIOS
#include <termios.h>
typedef struct termios TERMIO;
#else
#include <termio.h>
typedef struct termio TERMIO;
#endif

#include <term.h>
#include <fcntl.h>

#define putChar(ch)         putchar(ch)
#ifdef SV3              // Unix 5.3 using terminfo:
    extern int tputc();     // fast putchar()
#else                   // other Unixes using termcaps:
#define TERMCAPS
#endif

// this fix isn't needed for Solaris
#ifdef SUN4
#define MY_TCGETA  (0x40000000|((sizeof(struct termio) & 0xff)<<16)|('T'<<8)|1)
#define MY_TCSETAF (0x80000000|((sizeof(struct termio) & 0xff)<<16)|('T'<<8)|4)
#else
#define MY_TCGETA  TCGETA
#define MY_TCSETAF TCSETAF
#endif

// Terminal-dependent routines using Termcaps

#define TCAPSLEN    400
extern char tcapbuf[];          // buffer for extracted termcap strings

#ifdef TERMCAPS
    extern char* CL;            // clear screen string pointer
    extern char* CM;            // cursor motion string pointer
    extern char* CE;            // clear to end-of-line string pointer
    extern char*    MD;         // bold mode string pointer
    extern char*    MR;         // reverse mode string pointer
    extern char*    US;         // underline mode string pointer
    extern char*    ME;         // end bold,etc. modes string pointer
    extern char*    AF;         // set ANSI foreground color
    extern char*    AB;         // set ANSI background color
    extern int CLlength;        // clear screen string length
    extern int CElength;        // clear to end-of-line string length
extern "C" { int tputc(tputc_t ch); }   // fast putchar()
inline void gotoxy(int col, int row) { tputs(tgoto(CM, col, row), 1, tputc); }
#define clearLine()         tputs(CE, 1, tputc)
#define clearScreen()       tputs(CL, 1, tputc)
#define boldMode()          tputs(MD, 1, tputc)
#define reverseMode()       tputs(MR, 1, tputc)
#define underlineMode()     tputs(US, 1, tputc)
#define normalMode()        tputs(ME, 1, tputc)
#define setForeColor(c)     tputs(tparam(AF, tcapbuf, TCAPSLEN, c), 1, tputc)
#define setBackColor(c)     tputs(tparam(AB, tcapbuf, TCAPSLEN, c), 1, tputc)

#else   // terminfo-based screen routines:
    int tputc(char ch);
    void clearScreen();
    void clearLine();
    void boldMode();
    void reverseMode();
    void underlineMode();
    void normalMode();
    void gotoxy(int col, int row);
    void setForeColor(int color);
    void setBackColor(int color);
#endif

// ANSI terminal color codes
#define WHITE   0
#define RED     1
#define BLACK   7

#define NO_KEY      -1      // used by checkKey(), waitKey()

struct termOptStr       // saved terminal settings
{
    TERMIO      o_termio;
    int         flags;
};

extern char     keyInited;
extern int screenHt, screenWd;      // screen dimensions

void iTermCaps (termOptStr* termSave);  // initialize full terminal emulation
void initTerm (termOptStr* termSave);   // initialize for checkKey
void restoreTerm (termOptStr* termSave); // conclude terminal emulation
void checkKey (signed char* key);               // check key pressed: defd in key.c
void waitKey (signed char* key);                // wait for key: defined in key.c
void getScreenSize();

#endif // termp_h_
