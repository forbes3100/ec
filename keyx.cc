// ****************************************************************************
// keyx.cc  key-input related system dependent routines
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

#include <unistd.h>

#include "termp.h"

char            keyInited;

// ----------------------------------------------------------------------------
// Initialize terminal-dependent variables, based on TERM setting.

void initTerm(termOptStr* termSave)
{
    TERMIO  n_termio;

#ifdef TERMIOS
    tcgetattr(0, &termSave->o_termio);  // save old terminal options
    tcgetattr(0, &n_termio);        // get a copy of terminal options
#else
    ioctl(0, MY_TCGETA, &termSave->o_termio);   // save old terminal options
    ioctl(0, MY_TCGETA, &n_termio);     // get a copy of terminal options
#endif
    n_termio.c_iflag = ISTRIP;          // no parity, etc.
    n_termio.c_oflag = 0;               // no output postprocessing
    n_termio.c_lflag = 0;               // non-cannonical input
    n_termio.c_cc[VMIN] = 1;            // getc: chars to read
    n_termio.c_cc[VTIME] = 1;           // getc: timeout in .1 sec incr
#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &n_termio);       // set new term options
#else
    ioctl(0, MY_TCSETAF, &n_termio);        // set new term options
#endif
    termSave->flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, termSave->flags | O_NDELAY); // set stdin for no delay
    keyInited = FALSE;                  // (returns -1 if no key)
}

// ----------------------------------------------------------------------------
// Restore terminal.

void restoreTerm(termOptStr* termSave)
{
    fcntl(0, F_SETFL, termSave->flags);
#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &termSave->o_termio); // restore old term options
#else
    ioctl(0, MY_TCSETAF, &termSave->o_termio); // restore old term options
#endif
}

// ----------------------------------------------------------------------------
// Check if a key is pressed, and get it if so.

void checkKey(signed char* key)
{
    if (*key <= 0 && keyInited)
        *key = getchar();
}

// ----------------------------------------------------------------------------
// Wait for a key to be pressed if not already gotten.

void waitKey(signed char* key)
{
    TERMIO  o_termio;
#ifdef TERMIOS
    tcgetattr(0, &o_termio);            // save current terminal options
#else
    ioctl(0, MY_TCGETA, &o_termio);     // save current terminal options
#endif
    TERMIO  n_termio;
    n_termio = o_termio;                // get a copy of terminal options
    n_termio.c_cc[VTIME] = 0;           // set no timeout
#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &n_termio);   // set new term options
#else
    ioctl(0, MY_TCSETAF, &n_termio);    // set new term options
#endif
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags & ~O_NDELAY);  // set stdin for delay

    while (*key <= 0)
        *key = getchar();
    keyInited = TRUE;

#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &o_termio);   // restore term options
#else
    ioctl(0, MY_TCSETAF, &o_termio);    // restore term options
#endif
    fcntl(0, F_SETFL, flags);
}
