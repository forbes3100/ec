// ****************************************************************************
// ec.cc  Macro Screen Editor
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
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdarg.h>

#include "termp.h"
#include "ec.h"

#define NO_UPDATE_DURING_INPUT

static const char* Intro =
    "Macro Editor  6.9  " __DATE__ "  Type ctrl-K then H for help\n";

// character codes

const signed char CH_CR =       0x0d;   // ASCII carriage return char
const signed char CH_LF =       0x0a;   // ASCII line feed char
const signed char CH_BELL =     0x07;   // ASCII bell char
const signed char CH_BS =       0x08;   // ASCII backspace char
const signed char CH_ESC =      0x1b;   // ASCII escape char
const signed char CH_RUB =      0x7f;   // ASCII rubout char

class Cancel
{
};

void updateWindows ();
void centerCursor (void);
void sayWait(void);
char* currOptions(void);
void findReplace (const char* findStr, int findStrLen,
                  const char* replStr, int replStrLen);
void doQcommand (int ch);
void doKcommand (int ch);
void doCommand (int ch);
const char* execString (const char* pcStart, const char *pcEnd, int rep);
void execBuffer (int exb);
void adjustTopRow (void);
void initCmdBuf (void);
void getCommand (const char* msg, bool isFile = false);


// ----------------------------------------------------------------------------
// global variables

termOptStr  termSave;                   // saved terminal characteristics
BuffRec buffer[12];                     // 12 edit buffers
short   screenImage[SCRMAXHT][SCRMAXWD]; // internal image of screen
char    statusLine[SCRMAXWD];           // status line string
char    divideLine[SCRMAXWD];           // split window dividing line string
char    insString[MAX_LINE];            // insert string temp storage
int     attrib;                         // current character display mode
int     b;                              // current buffer number
char*   bstart;                         // current buffer locs
char*   bend;
char*   beot;
char*   bcursPos;
char*   btagPos;
char*   btopRowPos;
char*   bbotRowPos;                     // char pos of top & bottom of wind
short   bhScroll;                       // horiz scroll amount
short   btabSize;                       // current buffer TAB spacing
int     topRow, botRow;                 // screen row #s
int     topA, botA;                     // window A row #s
int     topB, botB;                     // window B row #s
int     buffA, buffB;                   // window buffer #s
char*   lastTopPos;                     // previous top char position
int     ansiColors;                     // TRUE to force ANSI black on white

char    command, cmdChar2;
signed char key;
int     cmdState;                       // doCommand() state
int     lineNum, charNum;               // cursor loc in line, char
int     cursRow, cursCol;               // cursor loc on screen
int     curRowSv, curColSv;             // cursor loc saved during long cmd
bool    screenReady, insertMode, quitting, doMake;
bool    splitMode, topWindow, longCommand, makeBak;
const char* insMsg;                     // INSERT, REPLACE string ptr
char*   clipBoard;                      // clipboard data pointer
long    clipSize;                       // clipboard char size
char    theString[MAX_LINE];            // parsed string in a command
int     theStrLen;                      // parsed string length
char    fstString[MAX_LINE];            // saved first string in a command
int     fstStrLen;                      // saved first string length
bool    findGlobal, findForward, findSingle, findAsk;   // find/replace opts
bool    findBeeped;                     // true if find command at EOT
char    delimChar;                      // char used for QF, QA delimiter
char    lastFind[MAX_LINE];             // last QF, QA command
int     macroLevel;                     // macro recursion level
int     prevBuff;                       // buffer before getCommand()
int     givenTabSize;                   // given tab spacing (if any)

static const char* help[] = {
"------ Editor Help ------  control key summary:\n",
"------------------------------------------\
-------------------------------------\n",
"|   Q   |   W   |   E   |   R   |   T   | \
  Y   |   U   |   I   |   O   |   P  |\n",
"| Q-cmd | Q-cmd |   ^   |  ^ 12 |delete |d\
elete |paste  | (TAB) |cut to |paste |\n",
"|       |       | line  | lines | word  |t\
o EOL |clipbd |       |clipbd |clipbd|\n",
"------------------------------------------\
-------------------------------------\n",
"   |   A   |   S   |   D   |   F   |   G  \
 |   H   |   J   |   K   |   L   |\n",
"   |   <   |   <   |   >   |   >   |delete\
 |delete | mark  | K-cmd |repeat |\n",
"   | word  | char  | char  | word  | char \
 | char  |location       |long cmd\n",
"   ---------------------------------------\
----------------------------------\n",
"      |   Z   |   X   |   C   |   V   |   B   |   N   |   M   |\n",
"      |   v   |   v   |  v 12 |toggle |select |toggle | (CR)  |\n",
"      |scroll | line  | lines |INS/RPL|buffer |window |       |\n",
"      ---------------------------------------------------------\n",
"\n",
"Cursor movement:\n",
"  ^D fwd character,  ^S back character,  ^F fwd word,      ^A back word,\n",
"  ^X down line,      ^E up line,         ^C down 12 lines, ^R up 12 lines,\n",
"  ^Z scroll down,    ^QC end of file,    ^QR top file,\n",
"  ^QS begin of line, ^QD end of line,    ^QG\"n\" goto line n\n",
"\n",
"\n",
"\n",
"Insert / Delete:\n",
"  ^H  delete character backward         ^T  delete word\n",
"  ^G  delete character forward          ^Y  delete line\n",
"  ^J  tag current position              ^V  toggle insert/replace mode\n",
"  ^O  cut   -or-  ^QO  copy   tag-to-cursor text to clipboard\n",
"  ^P or ^U  paste clipboard at cursor\n",
"Find / Replace:\n",
"  ^QF<options>\"string\"   find string  ('\"' can be any delimiter)\n",
"    (Control chars in string are entered with a '^'. ex: '^M' for return)\n",
"  ^QA<options>\"string1\"string2\"   find string1 and replace with string2\n",
"  ^L            repeat last find, replace, or ^Q<space> command\n",
"Buffer management:   (i is buffer 0 - 9)\n",
"  ^Bi  edit buffer i             ^Qi  execute buffer i as macro\n",
"  ^QW  split screen mode on/off  ^N   select other window\n",
"File I/O:\n",
"  ^KO\"filename\"   open file,       ^KS             save file\n",
"  ^KR\"filename\"   insert file at cursor\n",
"  ^KW\"filename\"   write tag-to-cursor to new file\n",
"Other:\n",
" ^QT\"n\"   set TAB spacing for this file (n= 4, 8, etc.)\n",
" ^Q<space>  enter a short macro, repeatable with ^L\n",
" ^KD,  ^KX  save buffer 0 and exit editor,   ^QQ  exit the editor\n",
" ^KE  save buf 0, exit, and make,    ^KP  like ^KE, but OS-9 pmake\n",
" ^KA  toggle black-on-white\n",
"\n",
"Macros may be any sequence of the above commands, entered as letters\n",
" (upper or lower case) into any buffer. Executed with ^Qi (i=buffer).\n",
" <n>[<commands>]   repeat <commands> <n> times\n",
" [<commands>]      repeat <commands> until error or escape\n",
" QI\"string\"        insert string at cursor; same rules as in 'find'\n",
"\n",
" Macro examples:\n",
" 5[ x 3[ qf\"/*\" j qf\"*/\" o ] ]   deletes 3 comments from 5 lines of c\n",
"\0" };

// ----------------------------------------------------------------------------
// Construct a throwable error from a printf-like format and args.

Error::Error(const char* fmt, ...)
{
    const int max_messageLen = 200;
    va_list ap;
    va_start(ap, fmt);
    message = new char[max_messageLen];
    vsnprintf(message, max_messageLen, fmt, ap);
    va_end(ap);
}

// ----------------------------------------------------------------------------
// Destroy error message.

Error::~Error()
{
    delete message;
}

// ----------------------------------------------------------------------------
// Error encountered: ring bell, display message, and return for command.

void Error::report()
{
    putchar(CH_BELL);
    char errLine[200];
    snprintf(errLine, 199, "\nERROR: %s\n", message);
    attrib = AT_REVERSE + AT_BOLD;
    update(errLine, 0, 4, screenHt-2, screenHt-1);
    attrib = 0;
}

// ----------------------------------------------------------------------------
// adjust bTopRowPos if the cursor is moved off screen.

void adjustTopRow()
{
    if (btopRowPos >= bcursPos)
    {
        btopRowPos = bcursPos;
        beginLine(&btopRowPos);
    }
    else
    {
        char* p = bcursPos;
        beginLine(&p);
        char* endRowPos = btopRowPos;
        fwdLine(&endRowPos, botRow-topRow);
        if (endRowPos < p)
        {
            btopRowPos = p;
            backLine(&btopRowPos, botRow-topRow);
        }
    }
    if (!splitMode)
    {
        char* p = btopRowPos;
        backLine(&p, 1);
        if (p == lastTopPos && lastTopPos != bstart)
        {
            gotoxy(0, screenHt-1);
            putchar(CH_LF);
            clearLineC();
            movec((char*)&screenImage[1][0], (char* )&screenImage[0][0],
                (long )(SCRMAXWD*(screenHt-1)) * sizeof(short));
            for (int i = 0; i < screenWd-1; i++)
                screenImage[screenHt-1][i] = ' ';
        }
    }
    lastTopPos = btopRowPos;
}

// ----------------------------------------------------------------------------
// Update screen display of current buffer(s).

void updateWindows()
{
    static char* lastCursPos;

    bool notUpdated;
    do
    {
        notUpdated = FALSE;
        adjustTopRow();
        bToBuffer();
        if (splitMode)
        {
            attrib = AT_REVERSE + AT_BOLD;
            char* p;
            for (p = divideLine; p < divideLine + screenWd - 1; p++)
                *p = '-';
            *p = 0;
            char s[MAX_LINE];
            sprintf(s, " buffer=%d ", buffB);
            memcpy(divideLine + screenWd/2 - 5, s, strlen(s));
            update(divideLine, 0, 0, screenHt/2, screenHt/2);
            attrib = 0;
            if (topWindow)
            {
                update(buffer[buffB].topRowPos, buffer[buffB].hScroll,
                 buffer[buffB].tabSize, topB, botB);
                update(buffer[buffA].topRowPos, buffer[buffA].hScroll,
                 buffer[buffA].tabSize, topA, botA);
            }
            else
            {
                update(buffer[buffA].topRowPos, buffer[buffA].hScroll,
                 buffer[buffA].tabSize, topA, botA);
                update(buffer[buffB].topRowPos, buffer[buffB].hScroll,
                 buffer[buffB].tabSize, topB, botB);
            }
        }
        else
        {
            attrib = 0;
            update(btopRowPos, bhScroll, btabSize, topRow, botRow);
        }
        if (cursCol < 0)
        {
            bhScroll = max(bhScroll - screenWd/2, 0);
            notUpdated = TRUE;
        }
        if (cursCol >= screenWd-1)
        {
            bhScroll += screenWd/2;
            notUpdated = TRUE;
        }
    } while (notUpdated);

    gotoxy(cursCol, cursRow);
    const char* fileStat = "file";
    const char* curFileName = "-none-";
    if (buffer[b].open)
    {
        if (buffer[b].newFile)
            fileStat = " new";
        else if (buffer[b].readOnly)
            fileStat = "[RO]";
        curFileName = buffer[b].fpath;
    }

    char lEndMsg = ' ';
    switch (buffer[b].lineEnding)
    {
        case lEnd_Mac:  lEndMsg = 'M'; break;
        case lEnd_PC:   lEndMsg = 'P'; break;
    }

    sprintf(statusLine, "----- %c %4s: %-33s",
        command, fileStat, curFileName);
    char* p = statusLine + strlen(statusLine);
    while (p < statusLine + screenWd - 30)
        *p++ = ' ';
    sprintf(p, "buffer=%d [    ,   ] %s %c -----", buffA, insMsg, lEndMsg);

    if (bcursPos != lastCursPos)
    {
        attrib = AT_REVERSE + AT_BOLD;
        update(statusLine, 0, 0, 0, 0);
        attrib = 0;
        gotoxy(cursCol, cursRow);
        cursToLineChar();
    }
    lastCursPos = bcursPos;
    char s[MAX_LINE];
    snprintf(s, MAX_LINE, "%-4d,%-3d", lineNum, charNum);
    memcpy(statusLine + screenWd - 20, s, strlen(s));
    attrib = AT_REVERSE + AT_BOLD;
    update(statusLine, 0, 0, 0, 0);
    attrib = 0;
}

// ----------------------------------------------------------------------------
// Screen was just resized-- redraw it.

void screenRedraw(int sigRaised)
{
    // get new screen size
    getScreenSize();
    if (screenHt > SCRMAXHT)
        screenHt = SCRMAXHT;
    if (screenWd > SCRMAXWD)
        screenWd = SCRMAXWD;

    // adjust window pane sizes
    topB = screenHt/2 + 1;
    botB = screenHt-1;
    if (splitMode)
    {
        botA = screenHt/2 - 1;
        if (topWindow)
            botRow = botA;
        else
        {
            topRow = topB;
            botRow = botB;
        }
    }
    else
    {
        botA = screenHt-1;
        botRow = botA;
    }

    // refresh screen
    clearScreenC();
    updateWindows();
}

// ----------------------------------------------------------------------------
// If buffer b is open or has text in it ask if it is to be saved.

void saveIfOpen()
{
    if (buffer[b].changed)
    {
        do
        {
            key = NO_KEY;
            attrib = AT_REVERSE + AT_BOLD;
            char cmdLine[MAX_LINE];
            if (buffer[b].open)
                sprintf(cmdLine,"\n Save changes to '%s' (Yes/No/Cancel)?\n",
                 buffer[b].fpath);
            else
            {
                centerCursor();
                key = NO_KEY;
                updateWindows();
                attrib = AT_REVERSE + AT_BOLD;
                sprintf(cmdLine,"\n Save buffer %d (Yes/No/Cancel)?\n", b);
            }
            update(cmdLine, 0, 0, screenHt-2, screenHt-1);
            attrib = 0;
            gotoxy(strlen(cmdLine)-2, screenHt-1);
            waitKey(&key);                  // wait for key
            key = toupper(key);
        } while (!(key == 'Y' || key == 'N' || key == 'C'));

        if (key == 'C')
        {
            quitting = FALSE;
            throw new Cancel;
        }
        else if (key == 'Y')
        {
            if (buffer[b].open)
            {
                saveBuffer();
                buffer[b].open = FALSE;
            }
            else
            {
                initCmdBuf();
                getCommand("Save buffer as:");
                insert(bstart, "QRJQCKW\4", (long)8);
                insert(beot, "\4", 1);
                selectBuffer(prevBuff);
                execBuffer(longCmdBuff);
            }
        }
        key = NO_KEY;
    }
    clearBuffer();
}

// ----------------------------------------------------------------------------
// Save any open buffers (may be cancelled).

void saveAllBuffers()
{
    bToBuffer();
    for (int i = 0; i < 10; i++)
    {
        if (buffer[i].start)
        {
            selectBuffer(i);
            saveIfOpen();
        }
    }
}

// ----------------------------------------------------------------------------
// Center the screen about the cursor for Find commands.

void centerCursor()
{
    btopRowPos = bcursPos;
    backLine(&btopRowPos, (botRow-topRow)/2);
}

// ----------------------------------------------------------------------------
// Display wait message.

void sayWait()
{
    update("WAIT\n", 0, 8, screenHt-1, screenHt-1);
}

// ----------------------------------------------------------------------------
// Return options question string with current option selections.

char* currOptions()
{
    static char options[MAX_LINE];

    if (findGlobal)
        strcpy(options, "options: GLOBAL/Local, ");
    else
        strcpy(options, "options: Global/LOCAL, ");
    if (findForward)
        strcat(options, "FWD/Back, ");
    else
        strcat(options, "Fwd/BACK, ");
    if (findSingle)
        strcat(options, "SINGLE/Mult, ");
    else
        strcat(options, "Single/MULT, ");
    if (findAsk)
        strcat(options, "ASK/Don't");
    else
        strcat(options, "Ask/DON'T");
    return options;
}

// ----------------------------------------------------------------------------
// Find and Replace with options.

void findReplace(const char* findStr, int findStrLen, const char* replStr, int replStrLen)
{
    if (findGlobal)
    {
        if (findForward)
            bcursPos = bstart;
        else
            bcursPos = beot - findStrLen;
    }
    else
    {
        if (bcursPos == beot || findBeeped)
            bcursPos = bstart;
    }
    findBeeped = FALSE;

    if (!(findSingle || findAsk))
        sayWait();

    bool found = FALSE;
    do
    {
        if (!findForward)
            bcursPos = bcursPos - findStrLen;
        const char* findPos = find(findStr, findStrLen);
        if (findPos)
            {
                bcursPos = (char*)(findPos + findStrLen);
                found = TRUE;
                if (replStr)
                {
                    bool replace = TRUE;
                    if (findAsk)
                    {
                        centerCursor();
                        key = NO_KEY;
                        updateWindows();
                        attrib = AT_REVERSE + AT_BOLD;
                        update("\nReplace (Y/N/C)?\n", 0, 0, screenHt-2, screenHt-1);
                        attrib = 0;
                        gotoxy(cursCol, cursRow);
                        waitKey(&key);
                        key = toupper(key);
                        if (key == 'C')
                            break;
                        if (key != 'Y')
                            replace = FALSE;
                    }
                    if (replace)
                    {
                        bcursPos -= findStrLen;
                        del(bcursPos, (long )findStrLen);
                        insert(bcursPos, replStr, (long )replStrLen);
                        bcursPos += replStrLen;
                    }
                }
            }
        else
            break;
    } while (!findSingle && replStr);

    if (found)
    {
        if (findAsk && replStr)     // multiple replacing: beep when done
            putchar(CH_BELL);
        centerCursor();
    }
    else
    {
        findBeeped = TRUE;
        throw new Error("can't find '%s'", findStr);
    }
}

// ----------------------------------------------------------------------------
// Do a ^Q command.

void doQcommand(int ch)
{
    switch(cmdState)
    {
        case 1:         // execute buffer i as a macro
            if (ch >= '0' && ch <= '9')
            {
                cmdState = 0;
                execBuffer(ch-'0');
            }
            else
            {
                ch = (ch & 0x1f) + 0x40;
                switch(ch)
                {
                    case 'A':           // replace
                    case 'F':           // find
                    case 'I':           // insert
                    case 'G':           // goto line
                    case 'T':           // tab size
                        cmdChar2 = ch;
                        cmdState = 2;
                        break;

                    case 'C':           // go to bottom
                        bcursPos = beot;
                        cmdState = 0;
                        break;

                    case 'R':           // go to top
                        bcursPos = bstart;
                        cmdState = 0;
                        break;

                    case 'S':           // go to beginning of line
                        beginLine(&bcursPos);
                        cmdState = 0;
                        break;

                    case 'D':           // go to end of line
                        beginLine(&bcursPos);
                        fwdLine(&bcursPos,1);
                        backChar(&bcursPos,1);
                        cmdState = 0;
                        break;

                    case 'O':       // copy tag-to-cursor into clipboard
                    {
                        char* p;
                        if (btagPos > bcursPos)
                        {
                            p = bcursPos;
                            clipSize = btagPos - bcursPos;
                        }
                        else
                        {
                            p = btagPos;
                            clipSize = bcursPos - btagPos;
                        }
                        if (clipBoard)
                            free(clipBoard);
                        clipBoard = (char*)malloc((size_t)(clipSize+1));
                        if (!clipBoard)
                            throw new Error("no space for clipboard");
                        movec(p, clipBoard, clipSize);
                        cmdState = 0;
                        break;
                    }
                    case 'W':           // split windows toggle
                        if (splitMode)
                        {
                            splitMode = FALSE;
                            botA = screenHt-1;
                            selectBuffer(buffA);
                            topRow = topA;
                            botRow = botA;
                        }
                        else
                        {
                            splitMode = TRUE;
                            topWindow = FALSE;
                            botA = screenHt/2 - 1;
                            selectBuffer(buffB);
                            topRow = topB;
                            botRow = botB;
                        }
                        cmdState = 0;
                        break;

                    default:
                        cmdState = 0;
                        break;
                }
            }
            break;

        case 2:             // get options or string delimiter char
                            // alpha characters are find/replace options
            switch (ch)
            {
                case 'G':
                case 'g':
                    findGlobal = TRUE;
                    break;

                case 'L':
                case 'l':
                    findGlobal = FALSE;
                    break;

                case 'F':
                case 'f':
                    findForward = TRUE;
                    break;

                case 'B':
                case 'b':
                    findForward = FALSE;
                    break;

                case 'A':
                case 'a':
                    findAsk = TRUE;
                    break;

                case 'D':
                case 'd':
                    findAsk = FALSE;
                    break;

                case 'S':
                case 's':
                    findSingle = TRUE;
                    break;

                case 'M':
                case 'm':
                    findSingle = FALSE;
                    break;

                default:    // otherwise, it may be a delimiter
                    if (!isalpha(ch))
                    {
                        delimChar = ch;
                        theStrLen = 0;
                        cmdState = 3;
                    }
                    break;
            }
            break;

        case 3:
        case 4:             // get string
            if (ch != delimChar)
            {
                if (theStrLen > 0 && theString[theStrLen-1] == '^')
                {
                    if (ch != '^')
                        theString[theStrLen-1] = ch & 0x1f;
                }
                else if (theStrLen < MAX_LINE-10)
                    theString[theStrLen++] = ch;
            }
            else
            {
                theString[theStrLen] = 0;
                switch (cmdChar2)
                {
                    case 'F':           // find the string
                        findReplace(theString, theStrLen, NULL, 0);
                        cmdState = 0;
                        break;

                    case 'A':           // find and replace the string
                        if (cmdState == 3)
                        {
                            strcpy(fstString, theString);
                            fstStrLen = theStrLen;
                            theStrLen = 0;
                            cmdState = 4;
                        }
                        else
                        {
                            findReplace(fstString, fstStrLen, theString,
                                 theStrLen);
                            cmdState = 0;
                        }
                        break;

                    case 'I':           // insert the string
                        insert(bcursPos, theString, (long )theStrLen);
                        bcursPos += theStrLen;
                        cmdState = 0;
                        break;

                    case 'G':           // goto line
                        bcursPos = bstart;
                        downLine(&bcursPos, atoi(theString)-1);
                        centerCursor();
                        cmdState = 0;
                        break;

                    case 'T':           // set tab size
                        btabSize = max(atoi(theString), 1);
                        cmdState = 0;
                        break;
                }
            }
            break;
    }
}

// ----------------------------------------------------------------------------
// Do a ^K command.

void doKcommand(int ch)
{
    switch(cmdState)
    {
        case 1:
            ch = (ch & 0x1f) + 0x40;
            switch(ch)
            {
                case 'A':
                    ansiColors = !ansiColors;
                    clearScreenC();
                    cmdState = 0;
                    break;

                case 'E':           // exit editor and run MAKE
                    doMake = TRUE;
                    goto doexit;
                case 'D':
                case 'X':           // save file and exit editor
doexit:
                    sayWait();
                    quitting = TRUE;
                    cmdState = 0;
                    selectBuffer(0);
                    if (buffer[b].changed)
                        saveBuffer();
                    saveAllBuffers();
                    break;

                case 'H':           // display help screens
                {
                    cmdState = 0;
                    int line = 0;
                    bool more = FALSE;
                    do
                    {
                        int row = 0;
                        for ( ; row < screenHt-1 &&
                         (more = (help[line][0] != '\0')); row++, line++)
                            update(help[line], 0, 8,
                                row, row);
                        if (more)
                            update(" -- more -- (ESC to cancel)\n", 0, 8, row,
                                    row);
                        else
                            for ( ; row < screenHt; row++)
                                update("\n", 0, 8, row, row);
                        key = NO_KEY;
                        waitKey(&key);
                    } while (key != CH_ESC && more);
                    break;
                }
                case 'P':           // exit editor and run PMAKE
                    doMake = TRUE;
                    goto doexit;
                case 'O':           // open file
                case 'R':           // read in file
                case 'W':           // write file
                    cmdChar2 = ch;
                    cmdState = 2;
                    break;

                case 'Q':           // exit editor
                    quitting = TRUE;
                    cmdState = 0;
                    saveAllBuffers();
                    break;

                case 'S':           // save file
                    sayWait();
                    saveBuffer();
                    cmdState = 0;
                    break;

                default:
                    cmdState = 0;
                    break;
            }
            break;

        case 2:             // get file name delimiter char
            delimChar = ch;
            theStrLen = 0;
            cmdState = 3;
            break;

        case 3:
        case 4:             // get file name
            if (ch != delimChar)
            {
                if (theStrLen < MAX_LINE-10)
                    theString[theStrLen++] = ch;
            }
            else
            {
                theString[theStrLen] = 0;
                switch (cmdChar2)
                {
                    case 'O':       // open file
                        sayWait();
                        saveIfOpen();
                        clearBuffer();
                        buffer[b].changed = FALSE;
                        buffer[b].readOnly = FALSE;

                        if (insertFile(theString, OPEN))
                        {
                            strcpy(buffer[b].fpath, theString);
                            makeFName(b);
                            buffer[b].open = TRUE;
                            buffer[b].changed = FALSE;
                        }
                        setTabSizeFromType();
                        break;

                    case 'R':       // read file and insert at cursor
                        sayWait();
                        insertFile(theString, READ);
                        buffer[b].readOnly = FALSE;
                        break;

                    case 'W':       // write tag-to-cursor to file
                        sayWait();
                        writeTaggedToFile(theString);
                        break;
                }
            cmdState = 0;
            }
            break;
    }
}

// ----------------------------------------------------------------------------
// Execute one command character, either from keyboard or from a macro.

void doCommand(int ch)
{
    if (cmdState)
        switch(command)         // if second char of 2 char command:
        {
            case 'B':           // ^Bi  select buffer i
                if (ch >= '0' && ch <= '9')
                {
                    selectBuffer(ch - '0');
                    if (splitMode && !topWindow)
                        buffB = b;
                    else
                        buffA = b;
                }
                cmdState = 0;
                break;

            case 'Q':           // ^Qx
                doQcommand(ch);
                break;

            case 'K':           // ^Kx
                doKcommand(ch);
                break;
        }
    else
    {
        ch = (ch & 0x1f) + 0x40;        // one char commands:
        switch(ch)
        {
            case 'A':           // back a word
                backWord(&bcursPos, 1);
                break;

            case 'C':           // down 12 lines
                downLine(&bcursPos, screenHt/2);
                break;

            case 'D':           // forward one char
                fwdChar(&bcursPos, 1);
                break;

            case 'E':           // up one line
                upLine(&bcursPos, 1);
                break;

            case 'F':           // forward a word
                fwdWord(&bcursPos, 1);
                break;

            case 'G':           // delete char at cursor
                if (insertMode)
                    del(bcursPos, 1);
                else
                    replace(bcursPos, ' ');
                break;

            case '_':           // RUB, ^H: delete last char
            case 'H':
                backChar(&bcursPos, 1);
                if (insertMode)
                    del(bcursPos, 1);
                else
                    replace(bcursPos, ' ');
                break;

            case 'J':           // tag cursor location
                btagPos = bcursPos;
                break;

            case 'B':           // all multiple char commands
            case 'K':
            case 'Q':
                cmdState = 1;
                command = ch;
                break;

            case 'L':           // resend last find/replace command
            {
                prevBuff = b;
                selectBuffer(longCmdBuff);
                clearBuffer();
                insert(bcursPos, lastFind, (long)strlen(lastFind));
                selectBuffer(prevBuff);
                char* p = buffer[longCmdBuff].start;
                if (p)
                {
                    if (*p++ == 'Q' && (*p == 'F' || *p == 'A'))
                    {
                        for (p++; isalpha(*p); p++)
                            if (*p == 'G' || *p == 'g')
                                *p = 'L'; // change GLOBAL to LOCAL option
                    }
                    execBuffer(longCmdBuff);
                }
                break;
            }
            case 'N':           // select other window, if split
                if (splitMode)
                {
                    if (topWindow)
                    {
                        topWindow = FALSE;
                        selectBuffer(buffB);
                        topRow = topB;
                        botRow = botB;
                    }
                    else
                    {
                        topWindow = TRUE;
                        selectBuffer(buffA);
                        topRow = topA;
                        botRow = botA;
                    }
                }
                break;

            case 'O':           // cut tag-to-cursor into clipboard
            {
                char* p;
                if (btagPos > bcursPos)
                {
                    p = bcursPos;
                    clipSize = btagPos - bcursPos;
                }
                else
                {
                    p = btagPos;
                    clipSize = bcursPos - btagPos;
                }
                if (clipBoard)
                    free(clipBoard);
                clipBoard = (char*)malloc((size_t)(clipSize+1));
                if (!clipBoard)
                    throw new Error("no space for clipboard");
                movec(p, clipBoard, clipSize);
                del(p, clipSize);
                bcursPos = p;
                break;
            }
            case 'U':
            case 'P':           // paste clipboard at cursor
                insert(bcursPos, clipBoard, clipSize);
                bcursPos += clipSize;
                break;

            case 'R':           // up 12 lines
                upLine(&bcursPos, screenHt/2);
                break;

            case 'S':           // back one char
                backChar(&bcursPos, 1);
                break;

            case 'T':           // delete next word
            {
                char* p = bcursPos;
                fwdWord(&p, 1);
                del(bcursPos, p-bcursPos);
                break;
            }
#if 0
            case 'U':           // update (refresh) screen
                clearScreenC();
                break;
#endif
            case 'V':           // toggle insert/replace mode
                if (insertMode)
                {
                    insertMode = FALSE;
                    insMsg = "R";
                }
                else
                {
                    insertMode = TRUE;
                    insMsg = " ";
                }
                break;

            case 'W':           // scroll screen up
                backLine(&btopRowPos,1);
                break;

            case 'X':           // down one line
                downLine(&bcursPos, 1);
                break;

            case 'Y':           // delete line or to end-of-line
            {
                char* p = bcursPos;
                fwdLine(&p, 1);
                if (bcursPos != bstart && *(bcursPos-1) != '\n')
                    p--;
                clipSize = p - bcursPos;
                if (clipBoard)
                    free(clipBoard);
                clipBoard = (char*)malloc((size_t)(clipSize+1));
                if (!clipBoard)
                    throw new Error("no space for clipboard");
                movec(bcursPos, clipBoard, clipSize);
                del(bcursPos, clipSize);
                break;
            }
            case 'Z':           // scroll screen down
                fwdLine(&btopRowPos,1);
                break;

            default:            // just ignore other commands
                break;
        }
    }
    if (cmdState == 0)
        command = ' ';
}

// ----------------------------------------------------------------------------
// Execute macro string.

const char* execString(const char* pcStart, const char* pcEnd, int rep)
{
    if (macroLevel++ > 9)
        throw new Error("macro recursion more than 9 deep");

    const char* pc = pcStart;
    int i;
    for (i = rep; i > 0; )
    {
        if (cmdState == 0)
        {
            checkKey(&key);
            if (key == CH_ESC)
                    throw new Cancel;
            if (*pc == ']' || pc >= pcEnd)
            {
                pc++;
                if (i-- <= 0)       // if last loop, leave pc after ']'
                    break;
                pc = pcStart;
                continue;
            }
            int num = 0;
            while (*pc >='0' && *pc <= '9')
            {
                num *= 10;
                num += (*pc++) - '0';
            }
            if (*pc == '[')
            {
                pc++;
                if (num == 0)
                    num = 32000;
                pc = execString(pc, pcEnd, num);
            }
            else if (num > 0)
            {
                execString(pc, pc+1, num);
                pc++;
            }
            else
                doCommand(*pc++);
        }
        else
            doCommand(*pc++);
    }
    macroLevel--;
    return pc;
}

// ----------------------------------------------------------------------------
// Execute buffer as a macro, with limited recursion.

void execBuffer(int exb)
{
    BuffRec* p = &buffer[exb];
    if (p->start)
        execString(p->start, p->eot, 1);
}

// ----------------------------------------------------------------------------
// Stop the current command and prepare to accept a command line in
// longCmdBuff.

void initCmdBuf()
{
    doCommand(' ');  // this aborts the command
    prevBuff = b;
    selectBuffer(longCmdBuff);
    longCommand = TRUE;
    curRowSv = cursRow;
    curColSv = cursCol;
}

// ----------------------------------------------------------------------------
// Prompt user for a string on an editable line and put in current buffer.

void getCommand(const char* msg, bool isFile)
{
    key = NO_KEY;
    char cmdLine[2*SCRMAXWD + 50];
    char* p;
    for (p = cmdLine; p < cmdLine + screenWd - 1; p++)
        *p = '-';
    sprintf(p, "\n%s  (<esc> to cancel)\n", msg);
    attrib = AT_REVERSE + AT_BOLD;
    update(cmdLine, 0, 8, screenHt-3, screenHt-1);
    attrib = 0;
    clearBuffer();
    do {
        update(bstart, 0, 8, screenHt-1, screenHt-1);
        gotoxy(cursCol, cursRow);
        waitKey(&key);                  // wait for key
        if (key == CH_CR)
            break;
        if (key == CH_ESC)
            throw new Cancel;
        if (cmdState)
            doCommand(' ');
        else if (key == '\t')
        {
            if (isFile)
            {
                // tab-- try to complete a filename
                *bcursPos = 0;
                char lsCmd[MAX_LINE];
                snprintf(lsCmd, MAX_LINE, "ls -dF %s* 2>&1", bstart);
                fflush(stdout);
                FILE* ls = popen(lsCmd, "r");
                if (!ls)
                    putchar(CH_BELL);
                else
                {
                    char* buf = new char[32000];
                    int n = fread(buf, 1, 32000, ls);
                    if (!pclose(ls) && n > 0)
                    {
                        int baseLen = bcursPos - bstart;
                        char* p = buf;
                        char* addB1 = p + baseLen;
                        char* addE1 = 0;
                        char* addB2 = 0;
                        int col = 0;
                        int line = 0;
                        char* base = bstart;
                        for ( ; *p; p++)
                        {
                            if (col == baseLen)
                                addB2 = p;
                            if (!*p || (col < baseLen && *p != *base))
                                break;
                            if (*p == '\n')
                            {
                                if (col <= baseLen)
                                    break;
                                char* p1 = addB1;
                                char* p2 = addB2;
                                while (*p1 == *p2 && *p1 != '\n')
                                {
                                    p1++;
                                    p2++;
                                }
                                if (!addE1 || p1 < addE1)
                                    addE1 = p1;
                                col = 0;
                                base = bstart;
                                line++;
                            }
                            else
                            {
                                col++;
                                base++;
                            }
                        }
                        if (line != 1)
                            putchar(CH_BELL);
                        if (line > 0)
                        {
                            int addLen = addE1 - addB1;
                            insert(bcursPos, addB1, addLen);
                            bcursPos += addLen;
                        }
                    }
                }
            }
        }
        else if (key < 0x20 || key == CH_RUB)
            doCommand(key);
        else
        {
            insString[0] = key;
            insert(bcursPos, insString, 1);
            bcursPos++;
        }
        key = NO_KEY;
    } while (TRUE);
    cmdState = 0;
    key = NO_KEY;
}

// ----------------------------------------------------------------------------
// main program

int main(int argc, const char** argv)
{
    screenReady = FALSE;
    char clipName[MAX_LINE];

    try
    {
        // set up window-resize signal
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGWINCH);
        struct sigaction resizeSA;
        resizeSA.sa_handler = screenRedraw;
        resizeSA.sa_mask = sigset;
        resizeSA.sa_flags = 0;
        struct sigaction oldSA;
        sigaction(SIGWINCH, &resizeSA, &oldSA);
        sigset_t osigset;
        sigprocmask(SIG_UNBLOCK, &sigset, &osigset);
    
        iTermCaps(&termSave);
        if (screenHt > SCRMAXHT)
            screenHt = SCRMAXHT;
        if (screenWd > SCRMAXWD)
            screenWd = SCRMAXWD;
        ansiColors = FALSE;
    
        cmdState = 0;                   // initialize everything
        command = ' ';
        key = NO_KEY;
        doMake = FALSE;
        makeBak = FALSE;
        quitting = FALSE;
        insertMode = TRUE;
        insMsg = " ";
        longCommand = FALSE;
        strcpy(lastFind, "");
        findGlobal = FALSE;
        findForward = TRUE;
        findSingle = FALSE;
        findAsk = TRUE;
        findBeeped = FALSE;
        clipName[0] = 0;
        clipBoard = 0;
        int i;
        for (i = 0; i < 10; i++)
        {
            buffer[i].start = 0;
            buffer[i].open = FALSE;
        }
        b = 0;
        bstart = 0;
        buffA = 0;
        buffB = 1;
        topA = 1;
        botA = screenHt-1;
        topB = screenHt/2 + 1;
        botB = screenHt-1;
        splitMode = FALSE;
        topRow = topA;
        botRow = botA;
    
        // scroll existing terminal lines up to save any error messages, etc.
        for (i = screenHt-1; i > 0; i--)
            putchar('\n');
    
        clearScreenC();
    
        buffer[0].readOnly = FALSE;
        if (insertFile(".exrc", READEXRC))  // read in settings file, if any
        {
            buffer[0].readOnly = FALSE;
            insert(beot, "\0", 1);
            for (char* p = bstart; *p; p++)
            {
                while (*p == ' ' || *p == '\t')
                    p++;
                if (*p == 's' && *(p+1) == 'e' && *(p+2) == 't')
                {
                    p += 3;
                    while (*p == ' ' || *p == '\t')
                        p++;
                    if (*p == 't' && *(p+1) == 'a' && *(p+2) == 'b')
                    {
                        while (*p >= 'a' && *p <= 'z')
                            p++;
                        while (*p == ' ' || *p == '\t')
                            p++;
                        if (*p == '=')
                        {
                            p++;
                            while (*p == ' ' || *p == '\t')
                                p++;
                            givenTabSize = atoi(p);
                            if (givenTabSize <= 1)
                                givenTabSize = 0;
                        }
                    }
                    else if (*p == 'b' && *(p+1) == 'a')
                    {
                        while (*p >= 'a' && *p <= 'z')
                            p++;
                        while (*p == ' ' || *p == '\t')
                            p++;
                        if (*p == '=')
                        {
                            p++;
                            while (*p == ' ' || *p == '\t')
                                p++;
                            if (*p == 'y')
                                makeBak = TRUE;
                            else if (*p == 'n')
                                makeBak = FALSE;
                        }
                    }
                }
                while (p < bend && *p != '\n')
                    p++;
            }
        }
        clearBuffer();
    
        char* home = getenv("HOME");
        if (home)
            strncpy(clipName, home, MAX_LINE);
        else
            strncpy(clipName, "/tmp", MAX_LINE);
        strncat(clipName, "/.clipboard", MAX_LINE);
    
        buffer[0].readOnly = FALSE;
        if (insertFile(clipName, READEXRC)) // read in clipboard file, if any
        {
            buffer[0].readOnly = FALSE;
            clipSize = beot - bstart;
            if ((clipBoard  = (char*)malloc((size_t)(clipSize+1))) != 0)
                movec(bstart, clipBoard, clipSize);
        }
        clearBuffer();
    
        int fbuf = 0;
        int startLine = 0;
        for (i = 1; i < argc; i++)
        {
            const char* arg = argv[i];
            if (*arg == '-')
            {
                int size = atoi(arg+1);
                if (size > 0 && size < 25)          // -<n> is tab size
                    givenTabSize = size;
            }
            else if (*arg == '+')
                startLine = atoi(arg+1);        // +<l> is starting line no.
            else try
            {
                selectBuffer(fbuf);
                buffer[b].readOnly = FALSE;
                screenReady = TRUE;
                attrib = 0;
                if (insertFile(arg, OPEN))
                        // if filename given to edit
                {
                    strncpy(buffer[b].fpath, arg, MAX_LINE);    // open file in next buffer
                    makeFName(b);
                    buffer[b].open = TRUE;
                    buffer[b].changed = FALSE;
                    fbuf++;
                }
                setTabSizeFromType();
            
            } catch (Error* error)
            {
            }
        }
                        // start out editing first (if any) file
        if (startLine)
        {
            initCmdBuf();
            char sTmp[SCRMAXWD + 10];
            sprintf(sTmp, "QG/%d/", startLine);
            insert(bstart, sTmp, strlen(sTmp));
            selectBuffer(0);
            execBuffer(longCmdBuff);
        }
    
        selectBuffer(0);
    } catch(Error* error)
    {
        // errors caught during intialization are fatal
        error->report();
        exit(-1);
    }
    updateWindows();

    // loop once per character typed
    bool startup = TRUE;
    bool noqsMode = TRUE;
    do
    {
        try
        {
            screenReady = TRUE;
            macroLevel = 0;
            longCommand = FALSE;

            if (startup)
            {
                attrib = AT_REVERSE + AT_BOLD;
                char sTmp[SCRMAXWD + 10];
                char* p;
                for (p = sTmp; p < sTmp + screenWd - 1; p++)
                    *p = '-';
                *p++ = '\n';
                *p = 0;
                update(sTmp, 0, 8, screenHt-2, screenHt-1);
                attrib = 0;
                update(Intro, 0, 8, screenHt-1, screenHt-1);
                startup = FALSE;
            }

            gotoxy(cursCol, cursRow);
            waitKey(&key);      // wait for key if we don't have one

            if (cmdState)
            {
                char cmd1 = command;
                char cmd2 = ((key >= 'A' || key < ' ') ? (key & 0x1f) + 0x40 : key);
    
                switch (cmd1)
                {
                    case 'Q':
                        switch (cmd2)
                        {
                            case 'F':
                                initCmdBuf();
                                getCommand("find:");
                                sprintf(lastFind, "\4%s\4", bstart);
                                getCommand(currOptions());
                                insert(bstart, "QF", (long)2);
                                insert(beot, lastFind, (long)strlen(lastFind));
                                strcpy(lastFind, bstart);
                                break;
                            case 'A':
                                initCmdBuf();
                                getCommand("find:");
                                sprintf(lastFind, "\4%s\4", bstart);
                                getCommand("replace with:");
                                strcat(lastFind, bstart);
                                getCommand(currOptions());
                                insert(bstart, "QA", (long)2);
                                insert(beot, lastFind, (long)strlen(lastFind));
                                insert(beot, "\4", 1);
                                strcpy(lastFind, bstart);
                                break;
                            case 'G':
                                initCmdBuf();
                                getCommand("go to line:");
                                insert(bstart, "QG\4", (long)3);
                                insert(beot, "\4", 1);
                                break;
                            case 'T':
                                initCmdBuf();
                                getCommand("tab size:");
                                insert(bstart, "QT\4", (long)3);
                                insert(beot, "\4", 1);
                                break;
                            case ' ':
                                initCmdBuf();
                                getCommand("enter command: (^L repeats)");
                                strcpy(lastFind, bstart);
                                break;
                            default:
                                doCommand(cmd2);
                                break;
                        }
                        break;
    
                    case 'K':
                        switch (cmd2)
                        {
                            case 'O':
                                initCmdBuf();
                                getCommand("Open file:", true);
                                insert(bstart, "KO\4", (long)3);
                                insert(beot, "\4", 1);
                                break;
                            case 'R':
                                initCmdBuf();
                                getCommand("Read file:", true);
                                insert(bstart, "KR\4", (long)3);
                                insert(beot, "\4", 1);
                                break;
                            case 'W':
                                initCmdBuf();
                                getCommand("Write selected text to file:", true);
                                insert(bstart, "KW\4", (long)3);
                                insert(beot, "\4", 1);
                                break;
                            default:
                                doCommand(cmd2);
                            break;
                        }
                        break;
    
                    default:
                        doCommand(key);
                        break;
                }
    
                if (longCommand)
                {
                    cursRow = curRowSv;
                    cursCol = curColSv;
                    selectBuffer(prevBuff);
                    execBuffer(longCmdBuff);  // resend entire command
                }
    
            }
            else if ((key < 0x20 || key == CH_RUB) && key != CH_CR && key != '\t')
            {
                if (noqsMode)       // if no ^Q or ^S has been seen:
                {                   // keep mapping ^W to ^Q and ^Z to ^S.
                    if (key == 'Q'-64 || key == 'S'-64)
                        noqsMode = FALSE;
                    else if (key == 'W'-64)
                        key = 'Q'-64;
                    else if (key == 'Z'-64)
                        key = 'S'-64;
                }
                if (key != 'L'-64)
                    findBeeped = FALSE;
    
                doCommand(key);
            }
    
            else
            {
                if (insertMode)             // enter one character
                {
                    if (key == CH_CR)       // CR auto-indents
                    {
                        char* lineStart = bcursPos;
                        beginLine(&lineStart);
                        char* p = lineStart;
                        while ((*p == ' ' || *p == '\t') && (p < bcursPos))
                            p++;
                        int whiteSpLen = p - lineStart;
                        insert(bcursPos, "\n", 1);
                        if (whiteSpLen)
                        {
                            insert(bcursPos+1, lineStart, whiteSpLen);
                            bcursPos += whiteSpLen;
                        }
                    }
                    else
                    {
                        insString[0] = key;
                        insert(bcursPos, insString, 1);
                    }
                }
                else
                    replace(bcursPos, key);
    
                if (cursCol < screenWd-1 && key >= ' ')
                {                   // do quick update of screen first
                    putchar(key);
                    screenImage[cursRow][cursCol] = key;
                    cursCol++;
                }
                bcursPos++;
            }
#ifdef NO_UPDATE_DURING_INPUT   // FIX: this breaks Find command
            if (quitting)
                key = NO_KEY;
            else
            {
                // Don't update screen while we still have text to insert.
                // This cures paste-into-window problem.
                key = NO_KEY;
                checkKey(&key);
                if (key == NO_KEY)
                    updateWindows();
            }
#else
            if (!quitting)
                updateWindows();
#endif
        } catch (Cancel* c)
        {
            delete c;
            cmdState = 0;
            selectBuffer(prevBuff);
            updateWindows();
#ifdef NO_UPDATE_DURING_INPUT
            key = NO_KEY;
#endif

        } catch (Error* error)
        {
            error->report();
            delete error;
            cmdState = 0;
#ifdef NO_UPDATE_DURING_INPUT
            key = NO_KEY;
#endif
        }
#ifndef NO_UPDATE_DURING_INPUT
        key = NO_KEY;
#endif
    } while (!quitting);        // end of character main loop

    if (clipBoard)
    {
        // write the clipboard contents to file $HOME/.clipboard
        char* p = clipBoard;
        FILE* fp = fopen(clipName, "w");
        if (!fp)
            printf("\nError: can't write file '%s'\n", clipName);
        else
        {
            while (clipSize > 0)
            {
                int writeSize;
                if (clipSize > 32000)
                    writeSize = 32000;
                else
                    writeSize = clipSize;
                fwrite(p, 1, writeSize, fp);
                clipSize -= writeSize;
                p += writeSize;
            }
            fclose(fp);
        }
    }

    gotoxy(0, screenHt-1);
    clearLineC();
    putchar(CH_LF);

    restoreTerm(&termSave);
    if (doMake)     // ^KE: chain to 'make' program
    {
        execlp("make", "make", (char*)0);
        exit(1);    // error if it returns!
    }
    exit(0);
}
