# Build lcurses documentation
# (c) Peter Billam 2011

# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


open(I, 'lcurses.c') or die; my @I = (<I>); close I;

# make list of the lc_ and lcw_ routines
my %lc_routine = ();
my %lcw_routine = ();
foreach (@I) {
        s/&/&amp;/g; s/>/&gt;/g; s/</&lt;/g;
        if ($_ =~ /^static\s.+\s(lc_\w+)/) {
                $_ = "<A NAME=\"$1\"></A>$_";
                $lc_routine{$1} = 1;
        } elsif ($_ =~ /^static\s.+\s(lcw_\w+)/) {
                $_ = "<A NAME=\"$1\"></A>$_";
                $lcw_routine{$1} = 1;
        }
}

open(O, '>', 'lcurses_c.html') or die;
print O <<EOT;
<HTML><HEAD><TITLE>lcurses.c</TITLE></HEAD><BODY BGCOLOR="#FFFFFF">
<CENTER><H1><A NAME="top">lcurses.c</A></H1></CENTER><PRE>
EOT
foreach my $line (@I) {
        if ($line =~ /^#define CC\(/) {
                print O '<A NAME="constants"></A>',$line;
        } elsif ($line =~ /^#define EWF\(/) {
                print O '<A NAME="window_methods"></A>',$line;
        } elsif ($line =~ /^#define ECF\(/) {
                print O '<A NAME="curses_functions"></A>',$line;
        } else {
                print O $line;
        }
}
print O <<EOT;
</PRE></BODY></HTML>
EOT
close O;

my @P = (<DATA>);  close P;
open(O, '>', 'lcurses.html') or die;
foreach my $line (@P) {
        if ($line =~ /^<code>(\w+)\(/) {
                my $s = $1;
                if ($lc_routine{"lc_$s"}) {
                        print O $line,
                          "&nbsp; see <A HREF=\"lcurses_c.html#lc_$s\">lcurses.c</A><BR>\n";
                } elsif ($lcw_routine{"lcw_$s"}) {
                        print O $line,
                          "(see <A HREF=\"lcurses_c.html#lcw_$s\">lcurses.c</A>)<BR>\n";
                } else {
                        print O $line,'<BR>';
                }
        } elsif ($line =~ /XXWINDOWLIBXX/) {
                my %wl = ();
                my $i = $[; while ($i <= $#I) {
                        if ($I[$i] =~ /static const luaL_reg windowlib/) { last; }
                        $i += 1;
                }
                while ($i <= $#I) {
                        if ($I[$i] =~ / misc /) { last; }
                        if ($I[$i] =~ /EWF\((\w+)\)/) { $wl{$1} = "lcw_$1";
                        } elsif ($I[$i] =~ /"(\w+)", (lcw_\w+)/) { $wl{$1} = $2;
                        }
                        $i += 1;
                }
                foreach my $k (sort keys %wl) {
                        my $v = $wl{$k};
                        if ($lcw_routine{$v}) {
                                print O "<CODE>$k()</CODE> &nbsp; see "
                                  . "<A HREF=\"lcurses_c.html#$wl{$k}\">lcurses.c</A><BR>\n";
                        } else {
                                print O "<CODE>$k()</CODE><BR>\n";
                        }
                }
        } else {
                my $url = 'http://search.cpan.org/perldoc?Curses';
                $line =~ s#Perl Curses#<A HREF="$url">Perl Curses</A>#;
                print O $line;
        }
}
close O;
__END__
<?xml version="1.0" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<title>lcurses documentation</title>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<link rev="made" href="mailto:root@localhost" />
<STYLE type="text/css"><!--
DIV.txt  { margin-left: 4% }
--></STYLE>
</head>

<body style="background-color: white">

<CENTER><H1>
<FONT COLOR="#800000"><I>lcurses documentation</I></FONT>
</H1></CENTER>

<!-- INDEX BEGIN -->
<div name="index">
<p><a name="__index__"></a></p>

<TABLE ALIGN="center" WIDTH="85%" BORDER=0 CELLSPACING=0>
<TR><TD ALIGN="left">
<ul>
    <li><a href="#name">NAME</a></li>
    <li><a href="#synopsis">SYNOPSIS</a></li>
    <li><a href="#description">DESCRIPTION</a></li>
    <li><a href="#curses_functions">CURSES FUNCTIONS</a></li>
    <li><a href="#window_methods">WINDOW METHODS</a></li>
</ul>
</TD><TD ALIGN="left">
<ul>
    <li><a href="#constants">CONSTANTS</a></li>
    <li><a href="#compatibility">COMPATIBILITY</a></li>
    <li><a href="#installation">INSTALLATION</a></li>
    <li><a href="#author">AUTHOR</a></li>
    <li><a href="#see_also">SEE ALSO</a></li>
</ul>
</TD></TR></TABLE></div>

<hr name="index" />
</div>
<!-- INDEX END -->

<p>
</p>
<h2><a name="name">NAME</a></h2><DIV CLASS="txt">
<p><strong>lcurses</strong> &nbsp; - &nbsp;
a C library for Lua 5.1 that wraps the curses API.
</p><p></p>
</DIV><h2><a name="synopsis">SYNOPSIS</a></h2><DIV CLASS="txt">
<pre>require 'curses'
curses.initscr()
curses.cbreak()
curses.echo(0)  -- not noecho !
curses.nl(0)    -- not nonl !
local stdscr = curses.stdscr()  -- it's a userdatum
stdscr:clear()
local a = {};  for k in pairs(curses) do a[#a+1]=k end
stdscr:mvaddstr(15,20,'print out curses table (y/n) ? ')
stdscr:refresh()
local c = stdscr:getch()
if c &lt; 256 then c = string.char(c) end
curses.endwin()
if c == 'y' then
    table.sort(a)
    for i,k in ipairs(a) do print(type(curses[k])..'  '..k) end
end</pre>
<p>
</p>
</DIV><h2><a name="description">DESCRIPTION</a></h2><DIV CLASS="txt">
<p><code>lcurses</code> is the interface between Lua and your system's <code>ncurses</code>
or <code>curses</code> library.  For descriptions on the usage of a given function
or constant, consult your system's documentation, starting with
<code>man ncurses</code> or <code>man curses</code></p>
<p>
</p>
</DIV><h2><a name="curses_functions">CURSES FUNCTIONS</a></h2><DIV CLASS="txt">
<p>This list of functions can be seen by</p>
<pre>
  require 'curses' ; local a = {};
  for k in pairs(curses) do
    if type(curses[k]) == 'function' then a[#a+1]=k end
  end
  table.sort(a) ; for i,k in ipairs(a) do print(k) end</pre>
<p>and are defined in the
<A HREF="./lcurses_c.html#curses_functions">
static const luaL_reg curseslib</A>
section of lcurses.c
</p><p>
<code>baudrate()</code>
<code>beep()</code>
<code>cbreak()</code>
<code>color_pair()</code>
<code>color_pairs()</code>
<code>colors()</code>
<code>cols()</code>
<code>curs_set()</code>
<code>delay_output()</code>
<code>doupdate()</code>
<code>echo()</code>
<code>endwin()</code>
<code>erasechar()</code>
<code>flash()</code>
<code>flushinp()</code>
<code>halfdelay()</code>
<code>has_colors()</code>
<code>has_ic()</code>
<code>has_il()</code>
<code>init_pair()</code>
<code>initscr()</code>
<code>isendwin()</code>
<code>keyname()</code>
<code>killchar()</code>
<code>lines()</code>
<code>longname()</code>
<code>napms()</code>
<code>new_chstr()</code>
<code>newpad()</code>
<code>newwin()</code>
<code>nl()</code>
<code>pair_content()</code>
<code>raw()</code>
<code>ripoffline()</code>
<code>slk_attroff()</code>
<code>slk_attron()</code>
<code>slk_attrset()</code>
<code>slk_clear()</code>
<code>slk_init()</code>
<code>slk_label()</code>
<code>slk_noutrefresh()</code>
<code>slk_refresh()</code>
<code>slk_restore()</code>
<code>slk_set()</code>
<code>slk_touch()</code>
<code>start_color()</code>
<code>stdscr()</code>
<code>termattrs()</code>
<code>termname()</code>
<code>unctrl()</code>
<code>ungetch()</code>
</p>
<p></p>
</DIV><h2><a name="window_methods">WINDOW METHODS</a></h2><DIV CLASS="txt">
<p>These are defined in the
<A HREF="./lcurses_c.html#window_methods">
static const luaL_reg windowlib</A>
section of lcurses.c
</p><p>
XXWINDOWLIBXX
</p><p></p>
</DIV><h2><a name="constants">CONSTANTS</a></h2><DIV CLASS="txt">
<p>These constants only appear after curses.initscr() is called;
they can be seen by:</p>
<pre>
  require 'curses' ; curses.initscr() ; local a = {};
  for k in pairs(curses) do
    if type(curses[k]) == 'number' then a[#a+1]=k end
  end
  curses.endwin()
  table.sort(a) ; for i,k in ipairs(a) do print(k) end</pre>
<p>and are defined in the
<A HREF="./lcurses_c.html#constants">
static void register_curses_constants</A>
section of lcurses.c</p>
<p>ACS_BLOCK
ACS_BOARD
ACS_BTEE
ACS_BULLET
ACS_CKBOARD
ACS_DARROW
ACS_DEGREE
ACS_DIAMOND
ACS_HLINE
ACS_LANTERN
ACS_LARROW
ACS_LLCORNER
ACS_LRCORNER
ACS_LTEE
ACS_PLMINUS
ACS_PLUS
ACS_RARROW
ACS_RTEE
ACS_S1
ACS_S9
ACS_TTEE
ACS_UARROW
ACS_ULCORNER
ACS_URCORNER
ACS_VLINE</p>
<p>A_ALTCHARSET
A_ATTRIBUTES
A_BLINK
A_BOLD
A_CHARTEXT
A_DIM
A_INVIS
A_NORMAL
A_PROTECT
A_REVERSE
A_STANDOUT
A_UNDERLINE</p>
<p>COLOR_BLACK
COLOR_BLUE
COLOR_CYAN
COLOR_GREEN
COLOR_MAGENTA
COLOR_RED
COLOR_WHITE
COLOR_YELLOW</p>
<p>KEY_A1
KEY_A3
KEY_B2
KEY_BACKSPACE
KEY_BEG
KEY_BREAK
KEY_BTAB
KEY_C1
KEY_C3
KEY_CANCEL
KEY_CATAB
KEY_CLEAR
KEY_CLOSE
KEY_COMMAND
KEY_COPY
KEY_CREATE
KEY_CTAB
KEY_DC
KEY_DL
KEY_DOWN
KEY_EIC
KEY_END
KEY_ENTER
KEY_EOL
KEY_EOS
KEY_EXIT
KEY_F0
KEY_F1
KEY_F10
KEY_F11
KEY_F12
KEY_F2
KEY_F3
KEY_F4
KEY_F5
KEY_F6
KEY_F7
KEY_F8
KEY_F9
KEY_FIND
KEY_HELP
KEY_HOME
KEY_IC
KEY_IL
KEY_LEFT
KEY_LL
KEY_MARK
KEY_MESSAGE
KEY_MOUSE
KEY_MOVE
KEY_NEXT
KEY_NPAGE
KEY_OPEN
KEY_OPTIONS
KEY_PPAGE
KEY_PREVIOUS
KEY_PRINT
KEY_REDO
KEY_REFERENCE
KEY_REFRESH
KEY_REPLACE
KEY_RESET
KEY_RESIZE
KEY_RESTART
KEY_RESUME
KEY_RIGHT
KEY_SAVE
KEY_SBEG
KEY_SCANCEL
KEY_SCOMMAND
KEY_SCOPY
KEY_SCREATE
KEY_SDC
KEY_SDL
KEY_SELECT
KEY_SEND
KEY_SEOL
KEY_SEXIT
KEY_SF
KEY_SFIND
KEY_SHELP
KEY_SHOME
KEY_SIC
KEY_SLEFT
KEY_SMESSAGE
KEY_SMOVE
KEY_SNEXT
KEY_SOPTIONS
KEY_SPREVIOUS
KEY_SPRINT
KEY_SR
KEY_SREDO
KEY_SREPLACE
KEY_SRESET
KEY_SRIGHT
KEY_SRSUME
KEY_SSAVE
KEY_SSUSPEND
KEY_STAB
KEY_SUNDO
KEY_SUSPEND
KEY_UNDO
KEY_UP</p>
<p>
</p>
</DIV><h2><a name="compatibility">COMPATIBILITY</a></h2><DIV CLASS="txt">
<p>In the C library, the following functions:</p>
<pre>
    getstr()   (and wgetstr(), mvgetstr(), and mvwgetstr())
    inchstr()  (and winchstr(), mvinchstr(), and mvwinchstr())
    instr()    (and winstr(), mvinstr(), and mvwinstr())</pre>
<p>are subject to buffer overflow attack.  This is because you pass
in the buffer to be filled in, which has to be of finite length.
But in this Lua module, a buffer is assigned automatically
and the function returns the string, so there is no security issue.
You may still use the alternate functions:</p>
<pre>   s = stdscr:getnstr()
   s = stdscr:inchnstr()
   s = stdscr:innstr()</pre>
<p>which take an extra &quot;size of buffer&quot; argument,
in order to impose a maximum length on the string the user may type in.</p>
<p>Some of the C functions beginning with &quot;<code>no</code>&quot; do not exist in Lua.
You should use <code>curses.nl(0)</code> and <code>curses.nl(1)</code>
instead of <code>nonl()</code> and <code>nl()</code>,
and likewise <code>curses.echo(0)</code> and <code>curses.echo(1)</code>
instead of <code>noecho()</code> and <code>echo()</code> .
</p><p>
In this Lua module the <code>stdscr:getch()</code> function always returns an
integer. In C, a single character is an integer, but in Lua
(and Perl) a single character is a short string.
The Perl Curses
function <code>getch()</code> returns a char if it was a char,
and a number if it was a constant; to get this behaviour in Lua
you have to convert explicitly, e.g.:<BR>
 &nbsp; <CODE> if c &lt; 256 then c = string.char(c) end</CODE>
</p><p>
Some Lua functions take a different set of parameters
than their C counterparts; for example, you should
use <code>str = stdscr.getstr()</code> and <code>y, x = stdscr.getyx()</code> instead
of <code>getstr(str)</code> and <code>getyx(y, x)</code>,
and likewise
for <CODE>getbegyx</CODE>
and <CODE>getmaxyx</CODE>
and <CODE>getparyx</CODE>
and <CODE>pair_content</CODE>.
The Perl Curses module now uses the C-compatible parameters,
so be aware of this difference when translating code from Perl into Lua,
as well as from C into Lua.
</p><p>
Many curses functions have variants starting with the prefixes
<code>w-</code>, <code>mv-</code>, and/or <code>wmv-</code>.
These variants differ only in the explicit addition of a window,
or by the addition of two coordinates
that are used to move the cursor first.
For example, <code>addch()</code> has three other
variants: <code>waddch()</code>, <code>mvaddch()</code>,
and <code>mvwaddch()</code>.
The Perl Curses module combines these variants into one
&quot;Unified Function&quot;
which detects how many arguments it has and behaves accordingly.
But with this Lua module you must always specify the window
(because it is the Object)
e.g.: <code>stdscr:addch()</code>,
and so the <code>mv-</code> variant is retained.
If you are translating Perl code into Lua you may wish to use
<A HREF="uf.txt">some wrapper functions</A>.
</p><p></p>
</DIV><h2><a name="installation">INSTALLATION</a></h2><DIV CLASS="txt">
<pre>
  luarocks install lcurses</pre>
<p></p>
</DIV><h2><a name="author">AUTHOR</a></h2><DIV CLASS="txt">
<p><B>lcurses</B> was written by &nbsp; Reuben Thomas &nbsp;
<a href="http://github.com/rrthomas/lcurses">
http://github.com/rrthomas/lcurses</a><BR>
This documentation was created by
<A HREF="make_lcurses_doc">a script</A>
written by
<A HREF="http://www.pjb.com.au">Peter Billam</A>.
</p><p></p>
</DIV><h2><a name="see_also">SEE ALSO</a></h2><DIV CLASS="txt">
<pre><a href="http://github.com/rrthomas/lcurses">http://github.com/rrthomas/lcurses</a>
<a href="http://luaforge.net/projects/lcurses/">http://luaforge.net/projects/lcurses/</a>
<a href="http://luarocks.org/repositories/rocks/index.html#lcurses">http://luarocks.org/repositories/rocks/index.html#lcurses</a>
<a href="http://luaforge.net/frs/download.php/4823/lcurses-6.tar.gz">http://luaforge.net/frs/download.php/4823/lcurses-6.tar.gz</a>
<a href="http://git.savannah.gnu.org/cgit/zile.git/tree/src/term_curses.lua?h=lua">http://git.savannah.gnu.org/cgit/zile.git/tree/src/term_curses.lua?h=lua</a>
<a href="http://search.cpan.org/perldoc?Curses">http://search.cpan.org/perldoc?Curses</a>
man ncurses
man curses</pre>

</body>

</html>
