# ec
A portable terminal-based text editor using the WordStar diamond, with macro capability

## Getting Started

If you're not familiar with the WordStar diamond (the E S D X keys), the first step is to insure
that your keyboard has the Control key in the right place, just to the left of the 'A' key. On
modern keyboards means remapping the Caps Lock key to it. You'll then find that the WordStar
diamond keys fall naturally under your left index finger while your pinkie is on the Control key,
and that this is very close to home typing position.

To install, first clone the repository:

```
git clone https://github.com/forbes3100/ec.git
cd ec
```

Then build it:

```
make
```

## Usage

Test it first by just running it with no file:

```
./ec
```

And read in the ec.cc file by typing a ^KR (control-K, R), entering "ec.cc", and typing Return.

Type a ^KH at any time for a help screen (shown below), and any key to get back.

Use the WordStar diamond keys (^E, ^S, ^D, ^X) to move the cursor by a character in the corresponding direction, and the keys next to them (^R, ^A, ^F, ^C) to move faster: back/forward a word or up/down a half-screen. A ^Q prefix moves all the way in that direction (^QR, ^QS, ^QD, ^QC).

There are 10 separate buffers, each of which may have a file open or be used as scratch. A ^QW toggles a split view showing two buffers, during which a ^N toggles which window is being edited, and a ^B0 through ^B9 selects the buffer to edit.

```
------ Editor Help ------  control key summary:
-------------------------------------------------------------------------------
|   Q   |   W   |   E   |   R   |   T   |   Y   |   U   |   I   |   O   |   P  |
| Q-cmd | Q-cmd |   ^   |  ^ 12 |delete |delete |paste  | (TAB) |cut to |paste |
|       |       | line  | lines | word  |to EOL |clipbd |       |clipbd |clipbd|
-------------------------------------------------------------------------------
   |   A   |   S   |   D   |   F   |   G   |   H   |   J   |   K   |   L   |
   |   <   |   <   |   >   |   >   |delete |delete | mark  | K-cmd |repeat |
   | word  | char  | char  | word  | char  | char  |location       |long cmd
   -------------------------------------------------------------------------
      |   Z   |   X   |   C   |   V   |   B   |   N   |   M   |
      |   v   |   v   |  v 12 |toggle |select |toggle | (CR)  |
      |scroll | line  | lines |INS/RPL|buffer |window |       |
      ---------------------------------------------------------

Cursor movement:
  ^D fwd character,  ^S back character,  ^F fwd word,      ^A back word,
  ^X down line,      ^E up line,         ^C down 12 lines, ^R up 12 lines,
  ^Z scroll down,    ^QC end of file,    ^QR top file,
  ^QS begin of line, ^QD end of line,    ^QG"n" goto line n



Insert / Delete:
  ^H  delete character backward         ^T  delete word
  ^G  delete character forward          ^Y  delete line
  ^J  tag current position              ^V  toggle insert/replace mode
  ^O  cut   -or-  ^QO  copy   tag-to-cursor text to clipboard
  ^P or ^U  paste clipboard at cursor
Find / Replace:
  ^QF<options>"string"   find string  ('"' can be any delimiter)
    (Control chars in string are entered with a '^'. ex: '^M' for return)
  ^QA<options>"string1"string2"   find string1 and replace with string2
  ^L            repeat last find, replace, or ^Q<space> command
Buffer management:   (i is buffer 0 - 9)
  ^Bi  edit buffer i             ^Qi  execute buffer i as macro
  ^QW  split screen mode on/off  ^N   select other window
File I/O:
  ^KO"filename"   open file,       ^KS             save file
  ^KR"filename"   insert file at cursor
  ^KW"filename"   write tag-to-cursor to new file
Other:
 ^QT"n"   set TAB spacing for this file (n= 4, 8, etc.)
 ^Q<space>  enter a short macro, repeatable with ^L
 ^KD,   ^KX  save buffer 0 and exit editor,   ^QQ  exit the editor
 ^KE  save buf 0, exit, and make
 ^KA  toggle black-on-white

Macros may be any sequence of the above commands, entered as letters
 (upper or lower case) into any buffer. Executed with ^Qi (i=buffer).
 <n>[<commands>]   repeat <commands> <n> times
 [<commands>]      repeat <commands> until error or escape
 QI"string"        insert string at cursor; same rules as in 'find'

 Macro examples:
 5[ x 3[ qf"/*" j qf"*/" o ] ]   deletes 3 comments from 5 lines of c
```


## Contributing

Please read [CONTRIBUTING.md](https://github.com/forbes3100/ec.git/blob/master/CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Authors

* **Scott Forbes** - *Initial work* - [forbes3100](https://github.com/forbes3100)

See also the list of [contributors](https://github.com/forbes3100/ec.git/graphs/contributors) who participated in this project.

## License

This project is licensed under the GNU General Public License.
