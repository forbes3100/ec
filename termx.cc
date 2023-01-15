// ****************************************************************************
// termx.cc  Macro Screen Editor terminal-dependent routines
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "termp.h"
#include "ec.h"


#ifdef TERMCAPS     // Termcaps globals:

char            tcapbuf[TCAPSLEN];      // buffer for extracted termcap strings
char            PC_;                    // pad character
char*           CL;                     // clear screen string pointer
char*           CM;                     // cursor motion string pointer
char*           CE;                     // clear to end-of-line string pointer
char*           MD;                     // bold mode string pointer
char*           MR;                     // reverse mode string pointer
char*           US;                     // underline mode string pointer
char*           ME;                     // end bold,etc. modes string pointer
char*           AF;                     // set ANSI foreground color
char*           AB;                     // set ANSI background color
int             CLlength;               // clear screen string length
int             CElength;               // clear to end-of-line string length

#else               // Terminfo globals:

char            myClrScr[20];
char            myClrEol[20];
char            myCurAdr[40];
char            myBold[20];
char            myRev[20];
char            myUnder[20];
char            myNorm[20];
char            myAF[20];
char            myAB[20];
#endif

int             screenHt, screenWd;     // screen dimensions

// ----------------------------------------------------------------------------
// Get (new) screen dimensions to screenHt, screenWd.

void getScreenSize()
{
#ifdef TERMCAPS

    const char* term_type = getenv("TERM");
    if (!term_type)
        term_type = "vt100";

    char tcbuf[1024];
    if (tgetent(tcbuf, term_type) <= 0)
        throw new Error("unknown terminal type '%s'\n", term_type);

    screenHt = tgetnum((char*)"li");
    screenWd = tgetnum((char*)"co");

#else       // Terminfo version:

    setupterm((char*)0, 1, (int*)0);    // terminfo setup
    screenHt = lines;
    screenWd = columns - 1;
#endif
}

// ----------------------------------------------------------------------------
// Initialize terminal-dependent variables, based on TERM setting.

void iTermCaps(termOptStr* termSave)
{
#ifdef TERMCAPS

    const char* term_type = getenv("TERM");
    if (!term_type)
        term_type = "vt100";

    char tcbuf[1024];
    if (tgetent(tcbuf, term_type) <= 0)
        throw new Error("unknown terminal type '%s'\n", term_type);
    char* ptr = tcapbuf;

    char* temp = tgetstr((char*)"PC", &ptr);
    if (temp)
        PC_ = *temp;
    CL = tgetstr((char*)"cl", &ptr);    // get parameters from termcap library
    CM = tgetstr((char*)"cm", &ptr);
    CE = tgetstr((char*)"ce", &ptr);
    MD = tgetstr((char*)"md", &ptr);
    MR = tgetstr((char*)"mr", &ptr);
    US = tgetstr((char*)"us", &ptr);
    ME = tgetstr((char*)"me", &ptr);
    AF = tgetstr((char*)"setaf", &ptr);
    AB = tgetstr((char*)"setab", &ptr);
    screenHt = tgetnum((char*)"li");
    screenWd = tgetnum((char*)"co");

    if (!(CE && CL && CM))
        throw new Error("incomplete termcap entry");

    CLlength = strlen(CL);      // measure CL and CE strings
    CElength = strlen(CE);

#else       // Terminfo version

    setupterm((char*)0, 1, (int*)0);    // terminfo setup
    screenHt = lines;
    screenWd = columns - 1;
    strcpy(myClrScr, clear_screen);     // just copy strings
    strcpy(myClrEol, clr_eol);
    strcpy(myCurAdr, cursor_address);
    strcpy(myBold, enter_bold_mode);
    strcpy(myRev, enter_reverse_mode);
    strcpy(myUnder, enter_underline_mode);
    strcpy(myNorm, exix_attribute_mode);
    strcpy(myAF, set_a_foreground);
    strcpy(myAB, set_a_background);
    reset_shell_mode();                 // and restore term
#endif

#ifdef TERMIOS
    tcgetattr(0, &termSave->o_termio);  // save old terminal options
#else
    ioctl(0, MY_TCGETA, &termSave->o_termio);   // save old terminal
#endif

    TERMIO n_termio;
#ifdef TERMIOS
    tcgetattr(0, &n_termio);            // get a copy of terminal options
#else
    ioctl(0, MY_TCGETA, &n_termio);     // get a copy of terminal options
#endif

    n_termio.c_iflag = ISTRIP;          // no parity, etc.
    n_termio.c_oflag = 0;               // no output postprocessing
    n_termio.c_lflag = 0;               // non-cannonical input
    n_termio.c_cc[VMIN] = 1;            // getc: chars to read
    n_termio.c_cc[VTIME] = 1;           // getc: timeout in .1 sec incr
#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &n_termio);   // set new term options
#else
    ioctl(0, MY_TCSETAF, &n_termio);    // set new term options
#endif
    termSave->flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, termSave->flags | O_NDELAY); // set stdin for no delay
    keyInited = FALSE;                  // (returns -1 if no key)
}

// ----------------------------------------------------------------------------
// Print a character, fast.

int tputc(tputc_t ch)
{
    return putchar(ch);
}

#ifndef TERMCAPS    // terminfo-based screen routines:

// ----------------------------------------------------------------------------
// Clear screen.

void clearScreen()
{
    tputs(myClrScr, 1, tputc);
}

// ----------------------------------------------------------------------------
// Clear to end-of-line.

void clearLine()
{
    tputs(myClrEol, 1, tputc);
}

// ----------------------------------------------------------------------------
// Enter bold mode

void boldMode()
{
    tputs(myBold, 1, tputc);
}

// ----------------------------------------------------------------------------
// Enter reverse mode

void reverseMode()
{
    tputs(myRev, 1, tputc);
}

// ----------------------------------------------------------------------------
// Enter underline mode

void underlineMode()
{
    tputs(myUnder, 1, tputc);
}

// ----------------------------------------------------------------------------
// Exit attributes modes

void normalMode()
{
    tputs(myNorm, 1, tputc);
}

// ----------------------------------------------------------------------------
// Move screen cursor to row,col.

void gotoxy(int col, int row)
{
    tputs(tparm(myCurAdr, row, col), 1, tputc);
}

// ----------------------------------------------------------------------------
// Set foreground color.

void setForeColor(int color)
{
    tputs(tparm(myAF, color), 1, tputc);
}

// ----------------------------------------------------------------------------
// Set background color.

void setBackColor(int color)
{
    tputs(tparm(myAB, color), 1, tputc);
}
#endif
