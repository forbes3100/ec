// ****************************************************************************
// ec.h  Macro Screen Editor Common Defines
//
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
// ****************************************************************************

#ifndef ec_h_
#define ec_h_

#include <string.h>

// standard types and constants

#ifndef TRUE
#define TRUE        (1)
#define FALSE       (0)
#endif

// storage limits

#define ELBOW       256 // elbow room in text buffers
#define SCRMAXHT    200
#define SCRMAXWD    250
#define MAX_LINE    256

extern inline int max(int a, int b) { return a > b ? a : b; }

// Temporary name string class

const int max_nameLen = 100;

class TmpName
{
    char    name[max_nameLen];

public:
    TmpName&    operator = (char* name)
        { strncpy(this->name, name, max_nameLen); return* this; }
    TmpName&    operator += (char* s)
        { strncat(this->name, s, max_nameLen); return* this; }
    TmpName();
    TmpName(char* fmt, ...);
    operator char* ()   { return this->name; }
};

// Throwable error message

class Error
{
public:
    char*   message;
    
            Error(const char* fmt, ...);
            ~Error();
    void    report();
};

typedef struct
{
    char*   start;          // start of buffer
    char*   end;            // end of buffer
    char*   eot;            // end of text in buffer, points to NULL
    char*   cursPos;        // cursor position
    char*   tagPos;         // tag position
    char*   topRowPos;      // character position of top screen line
    short   hScroll;        // horizontal scroll amount
    short   tabSize;        // tab spacing
    bool    open;           // file open flag
    bool    newFile;        // new file flag
    bool    readOnly;       // read-only file flag
    bool    changed;        // changed flag
    char    fpath[MAX_LINE]; // file full pathname string, if open
    char*   fname;          // file name string, if open
    char    lineEnding;     // file line-ending type
} BuffRec;

#define longCmdBuff 10

enum InsertMode { READ=0, READEXRC, OPEN }; // for insertFile 'mode' argument

enum CheckMode { NOCHECKKEY=0, CHECKKEY };  // for update 'check' argument

// file line ending types
enum LEnd {lEnd_Unix, lEnd_Mac, lEnd_PC};

// character attribute codes in screenImage[]

#define AT_BOLD         0x0100
#define AT_REVERSE      0x0200
#define AT_UNDERLINE    0x0400
#define AT_ALL          0xff00

extern BuffRec  buffer[];
extern short screenImage[SCRMAXHT][SCRMAXWD]; // internal image of screen
extern char statusLine[];                   // status line string
extern char divideLine[];                   // split window dividing line string
extern char insString[];                    // insert string temp storage
extern int  attrib;                         // current character display mode
extern int  b;                              // current buffer number
extern char* bstart;                        // current buffer locs
extern char* bend;
extern char* beot;
extern char* bcursPos;
extern char* btagPos;
extern char* btopRowPos;
extern char* bbotRowPos;                    // char pos of top & bottom of wind
extern short bhScroll;                      // horiz scroll amount
extern short btabSize;                      // current buffer TAB spacing
extern int  topRow, botRow;                 // screen row #s
extern int  topA, botA;                     // window A row #s
extern int  topB, botB;                     // window B row #s
extern int  buffA, buffB;                   // window buffer #s
extern char* lastTopPos;                    // previous top char position
extern int  ansiColors;                     // TRUE to force ANSI black on white

extern char command, cmdChar2;
extern signed char key;
extern int  cmdState;                       // doCommand() state
extern int  lineNum, charNum;               // cursor loc in line, char
extern int  cursRow, cursCol;               // cursor loc on screen
extern bool insertMode, quitting, doMake, doPMake;
extern bool splitMode, topWindow, longCommand, makeBak;
extern const char* insMsg;                  // INSERT, REPLACE string ptr
extern char* clipBoard;                     // clipboard data pointer
extern long clipSize;                       // clipboard char size
extern char theString[];                    // parsed string in a command
extern int  theStrLen;                      // parsed string length
extern bool findGlobal, findForward, findSingle, findAsk;   // find/replace options
extern char delimChar;                      // char used for QF, QA delimiter
extern int  macroLevel;                     // macro recursion level
extern int  givenTabSize;                   // default tab spacing

void update (const char* atopPos, int hScroll, int tabSize, int atopRow,
                    int abotRow);
void movec (const char* src, char* dest, int size);
void clearBuffer (void);
void bToBuffer (void);
void selectBuffer (int newb);
void makeFName (int b);
void cursToLineChar ();
const char* find (const char* str, int len);
void insert (char* p, const char* str, int n);
void del (char* p, int n);
void replace (char* p, int c);
void saveIfOpen (void);
void setTabSizeFromType (void);
bool insertFile (const char* fileName, InsertMode mode);
void writeToFile (const char* fName, const char* fPath, char* start, char* end);
void saveBuffer (void);
void writeTaggedToFile (const char* fName);
void beginLine (char** p);
void backChar (char** p, int n);
void fwdChar (char** p, int n);
void backWord (char** p, int n);
void fwdWord (char** p, int n);
void backLine (char** p, int n);
void fwdLine (char** p, int n);
void upLine (char** p, int n);
void downLine (char** p, int n);
void clearScreenC (void);
void clearLineC (void);

#endif // ec_h_
