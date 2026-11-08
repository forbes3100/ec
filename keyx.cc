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
#include <errno.h>
#include <poll.h>
#include <stdio.h>

#include "termp.h"
#include "utf8.h"

char            keyInited;

static char inputBytes[4];
static size_t inputLength;
static size_t inputExpected;

// Add one terminal byte to the current UTF-8 sequence.  Return true only
// when a complete key is available.  Invalid input becomes U+FFFD.
static bool acceptInputByte(unsigned char byte, int* key)
{
    if (inputLength == 0)
    {
        if (byte < 0x80)
        {
            *key = byte;
            return true;
        }
        if (byte >= 0xc2 && byte <= 0xdf)
            inputExpected = 2;
        else if (byte >= 0xe0 && byte <= 0xef)
            inputExpected = 3;
        else if (byte >= 0xf0 && byte <= 0xf4)
            inputExpected = 4;
        else
        {
            *key = 0xfffd;
            return true;
        }
        inputBytes[inputLength++] = (char)byte;
        return false;
    }

    if ((byte & 0xc0) != 0x80)
    {
        inputLength = 0;
        inputExpected = 0;
        *key = 0xfffd;
        return true;
    }

    inputBytes[inputLength++] = (char)byte;
    if (inputLength < inputExpected)
        return false;

    uint32_t codePoint;
    size_t length = utf8Decode(inputBytes, inputBytes + inputLength,
                               &codePoint);
    inputLength = 0;
    inputExpected = 0;
    *key = length > 1 ? (int)codePoint : 0xfffd;
    return true;
}

static void readKey(int* key, bool blocking)
{
    unsigned char byte;
    while (*key == NO_KEY)
    {
        if (!blocking)
        {
            struct pollfd input;
            input.fd = STDIN_FILENO;
            input.events = POLLIN;
            input.revents = 0;
            if (poll(&input, 1, 0) <= 0 || !(input.revents & POLLIN))
                return;
        }
        ssize_t count = read(STDIN_FILENO, &byte, 1);
        if (count == 1)
        {
            if (acceptInputByte(byte, key))
                return;
        }
        else if (count < 0 && errno == EINTR)
            continue;
        else if (!blocking)
            return;
    }
}

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
    // UTF-8 uses all eight bits of each input byte.  ISTRIP would clear the
    // high bit and corrupt every non-ASCII character before the editor sees
    // it.  Keep input otherwise raw as before, including disabling software
    // flow control so ^Q and ^S remain editor commands.
    n_termio.c_iflag = 0;
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

void checkKey(int* key)
{
    if (*key == NO_KEY && keyInited)
        readKey(key, false);
}

// ----------------------------------------------------------------------------
// Wait for a key to be pressed if not already gotten.

void waitKey(int* key)
{
    // Cursor-motion and terminal-control sequences use stdio.  Make sure the
    // user sees the completed frame and hardware cursor before input blocks.
    fflush(stdout);

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

    readKey(key, true);
    keyInited = TRUE;

#ifdef TERMIOS
    tcsetattr(0, TCSANOW, &o_termio);   // restore term options
#else
    ioctl(0, MY_TCSETAF, &o_termio);    // restore term options
#endif
    fcntl(0, F_SETFL, flags);
}
