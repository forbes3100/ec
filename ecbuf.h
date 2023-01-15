// **********************************************************************
// ecbuf.h  Macro Screen Editor Common Defines
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
// **********************************************************************

#ifndef _ECBUF_H_
#define _ECBUF_H_

#include "ec.h"

enum insertMode{ READ=0, READEXRC, OPEN };  // for insertFile 'mode' argument

const int ELBOW = 256;  // elbow room in text buffers

class Buffer
{
public:
    char    *start;         // start of buffer
    char    *end;           // end of buffer
    char    *eot;           // end of text in buffer, points to NULL
    char    *cursPos;       // cursor position
    char    *tagPos;        // tag position
    char    *topRowPos;     // character position of top screen line
    short   hScroll;        // horizontal scroll amount
    short   tabSize;        // tab spacing
    bool    open;           // file open flag
    bool    newFile;        // new file flag
    bool    readOnly;       // read-only file flag
    bool    changed;        // changed flag
    char*   fpath;          // file full pathname string, if open
    char    *fname;         // file name string, if open

        Buffer();
    void    clearIt(int tabSize);
    void    makeFName();
    void    setTabSizeFromType();
    void    cursToLineChar();
    char*   find(char *str, int len);
    void    findReplace(char *findStr, int findStrLen, char *replStr,
          int replStrLen);
    void    insert(char *p, const char *str, long n);
    void    del(char *p, long n);
    void    replace(char *p, int c);
    void    saveIfOpen(void);
    bool    insertFile(char *fileName, enum insertMode mode);
    void    saveBuffer(void);
    void    beginLine(char **p);
    void    backChar(char **p, int n);
    void    fwdChar(char **p, int n);
    void    backWord(char **p, int n);
    void    fwdWord(char **p, int n);
    void    backLine(char **p, int n);
    void    fwdLine(char **p, int n);
    void    upLine(char **p, int n);
    void    downLine(char **p, int n);
    void    exec();
};

extern int  lineNum, charNum;           // cursor loc in line, char
extern int  cursRow, cursCol;           // cursor loc on screen
extern bool findGlobal, findForward, findSingle, findAsk;   // find/replace options
extern bool makeBak;

// to be replaced with ncurses routines
void checkKey(char *key);
void waitKey(char *key);
void setColorPair(short pair);

#endif // _ECBUF_H_
