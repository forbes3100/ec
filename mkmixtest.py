#!/usr/local/bin/python
#
# Create test files with mixed line endings

CR = chr(13)
LF = chr(10)
CRLF = CR + LF

f = open('f1', 'w')
f.write('Line1'); f.write(CR)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(CR)
f.close()

f = open('f2', 'w')
f.write('Line1'); f.write(CRLF)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(CR)
f.close()

f = open('f3', 'w')
f.write('Line1'); f.write(CR)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(CRLF)
f.close()

f = open('f4', 'w')
f.write('Line1'); f.write(LF)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(LF)
f.close()

f = open('f5', 'w')
f.write('Line1'); f.write(CRLF)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(LF)
f.close()

f = open('f6', 'w')
f.write('Line1'); f.write(LF)
f.write('Line2'); f.write(CRLF)
f.write('Line3'); f.write(CRLF)
f.close()

f = open('f7', 'w')
f.write('Line1'); f.write(LF)
f.write('Line2'); f.write(CR)
f.write('Line3'); f.write(LF)
f.close()

f = open('f8', 'w')
f.write('Line1'); f.write(CR)
f.write('Line2'); f.write(CR)
f.write('Line3'); f.write(LF)
f.close()

f = open('f9', 'w')
f.write('Line1'); f.write(LF)
f.write('Line2'); f.write(CR)
f.write('Line3'); f.write(CR)
f.close()
