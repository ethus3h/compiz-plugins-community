/**
 * Compiz Peek
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * Based on opacify.c and thumbnail.c:
 *
 * Copyright (c) 2006 Kristian Lyngstøl <kristian@beryl-project.org>
 * Ported to Compiz and BCOP usage by Danny Baumann <maniac@beryl-project.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Peek is a rip-off a Windows 7's peek feature, which makes windows
 * blocking the desktop transparent or the current taskbar window
 * the only visible window.
 * 
 */

#include <compiz-core.h>
#include <compiz-mousepoll.h>
#include "peek_options.h"

#define GET_PEEK_DISPLAY(d)                            \
    ((PeekDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define PEEK_DISPLAY(d)                                \
    PeekDisplay *pd = GET_PEEK_DISPLAY (d)
#define GET_PEEK_SCREEN(s, pd)                         \
    ((PeekScreen *) (s)->base.privates[(pd)->screenPrivateIndex].ptr)
#define PEEK_SCREEN(s)                                 \
    PeekScreen *ps = GET_PEEK_SCREEN (s, GET_PEEK_DISPLAY (s->display))
#define GET_PEEK_WINDOW(w, ps)                         \
    ((PeekWindow *) (w)->base.privates[(ps)->windowPrivateIndex].ptr)
#define PEEK_WINDOW(w)                                 \
    PeekWindow *pw = GET_PEEK_WINDOW (w,            \
	      	     GET_PEEK_SCREEN (w->screen,    \
		     GET_PEEK_DISPLAY (w->screen->display)))

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

/* window border painting is horribly broken currently */
#undef USE_BORDER_PAINT

static int displayPrivateIndex;

#ifdef USE_BORDER_PAINT
static char windowTex[4096] = {
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\2\0\0\0\2"
  "\0\0\0\2\0\0\0\2\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\2"
  "\0\0\0\2\0\0\0\3\0\0\0\3\0\0\0\3\0\0\0\4\0\0\0\4\0\0\0\4\0\0\0\4\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1"
  "\0\0\0\2\0\0\0\2\0\0\0\3\0\0\0\3\0\0\0\4\0\0\0\5\0\0\0\5\0\0\0\6\0\0\0\6"
  "\0\0\0\6\0\0\0\7\0\0\0\7\0\0\0\7\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\2\0\0\0\3\0\0\0\4\0\0\0\5\0\0\0\6"
  "\0\0\0\7\0\0\0\10\0\0\0\11\0\0\0\11\0\0\0\12\0\0\0\13\0\0\0\13\0\0\0\14\0"
  "\0\0\14\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\2\0"
  "\0\0\3\0\0\0\4\0\0\0\5\0\0\0\7\0\0\0\10\0\0\0\12\0\0\0\13\0\0\0\14\0\0\0"
  "\16\0\0\0\17\0\0\0\20\0\0\0\21\0\0\0\21\0\0\0\22\0\0\0\22\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\4\0\0\0\5\0\0\0\7\0\0\0\11"
  "\0\0\0\13\0\0\0\15\0\0\0\17\0\0\0\21\0\0\0\23\0\0\0\24\0\0\0\26\0\0\0\30"
  "\0\0\0\31\0\0\0\32\0\0\0\33\0\0\0\33\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0"
  "\0\2\0\0\0\3\0\0\0\4\0\0\0\6\0\0\0\10\0\0\0\12\0\0\0\15\0\0\0\20\0\0\0\22"
  "\0\0\0\25\0\0\0\30\0\0\0\33\0\0\0\35\0\0\0\37\0\0\0!\0\0\0#\0\0\0$\0\0\0"
  "%\0\0\0&\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\5\0\0\0\7\0\0\0\11"
  "\0\0\0\14\0\0\0\17\0\0\0\23\0\0\0\26\0\0\0\32\0\0\0\36\0\0\0\"\0\0\0%\0\0"
  "\0(\0\0\0+\0\0\0.\0\0\0""0\0\0\0""1\0\0\0""2\0\0\0""3\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0"
  "\0\2\0\0\0\3\0\0\0\5\0\0\0\7\0\0\0\12\0\0\0\15\0\0\0\21\0\0\0\25\0\0\0\32"
  "\0\0\0\36\0\0\0#\0\0\0(\0\0\0-\0\0\0""1\0\0\0""5\0\0\0""9\0\0\0<\0\0\0>\0"
  "\0\0@\0\0\0B\0\0\0C\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\5\0\0\0\7\0\0\0\12\0\0\0"
  "\16\0\0\0\22\0\0\0\27\0\0\0\34\0\0\0\"\0\0\0(\0\0\0/\0\0\0""5\0\0\0;\0\0"
  "\0@\0\0\0E\0\0\0I\0\0\0M\0\0\0P\0\0\0R\0\0\0T\0\0\0V\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0"
  "\0\5\0\0\0\7\0\0\0\12\0\0\0\16\0\0\0\22\0\0\0\30\0\0\0\36\0\0\0%\0\0\0,\0"
  "\0\0""4\0\0\0<\0\0\0C\0\0\0J\0\0\0Q\0\0\0V\0\0\0[\0\0\0`\0\0\0c\0\0\0f\0"
  "\0\0h\0\0\0i\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\1\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\6\0\0\0\11\0\0\0\15\0\0\0\22\0\0\0\30"
  "\0\0\0\36\0\0\0&\0\0\0/\0\0\0""8\0\0\0A\0\0\0J\0\0\0S\0\0\0[\0\0\0b\0\0\0"
  "i\0\0\0\231\0\0\0\354\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0"
  "\0\5\0\0\0\10\0\0\0\14\0\0\0\21\0\0\0\27\0\0\0\36\0\0\0&\0\0\0/\0\0\0""9"
  "\0\0\0D\0\0\0O\0\0\0Y\0\0\0c\0\0\0\377\0\0\0\377\0\0\0\377\0\0\0\377\361"
  "\361\361\353\361\361\361\353\361\361\361\353\361\361\361\353\361\361\361"
  "\353\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0"
  "\0\3\0\0\0\4\0\0\0\7\0\0\0\12\0\0\0\17\0\0\0\25\0\0\0\34\0\0\0$\0\0\0.\0"
  "\0\0""9\0\0\0E\0\0\0Q\0\0\0]\0\0\0\234\0\0\0\377\0\0\0\377\361\361\361\353"
  "\361\361\361\353\361\361\361\353\361\361\361\353\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\5\0\0\0\10\0\0\0\15\0\0\0"
  "\22\0\0\0\31\0\0\0\"\0\0\0,\0\0\0""7\0\0\0C\0\0\0Q\0\0\0^\0\0\0\377\0\0\0"
  "\377\361\361\361\353\361\361\361\353\361\361\361\353\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\1\0\0\0\2\0\0\0\4\0\0\0\7\0\0\0\12\0\0\0\17\0\0\0\26\0\0\0\36\0\0\0'\0"
  "\0\0""3\0\0\0@\0\0\0N\0\0\0]\0\0\0\377\0\0\0\377\361\361\361\353\361\361"
  "\361\353\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3"
  "\0\0\0\5\0\0\0\10\0\0\0\14\0\0\0\22\0\0\0\31\0\0\0\"\0\0\0-\0\0\0:\0\0\0"
  "I\0\0\0X\0\0\0w\0\0\0\377\361\361\361\353\361\361\361\353\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\6"
  "\0\0\0\11\0\0\0\16\0\0\0\24\0\0\0\35\0\0\0'\0\0\0""3\0\0\0A\0\0\0Q\0\0\0"
  "b\0\0\0\377\361\361\361\353\361\361\361\353\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\7"
  "\0\0\0\13\0\0\0\20\0\0\0\27\0\0\0\40\0\0\0+\0\0\0""9\0\0\0H\0\0\0Y\0\0\0"
  "\225\0\0\0\377\361\361\361\353\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\3\0\0\0\5\0\0"
  "\0\10\0\0\0\14\0\0\0\22\0\0\0\31\0\0\0#\0\0\0""0\0\0\0>\0\0\0N\0\0\0`\0\0"
  "\0\377\361\361\361\353\361\361\361\353\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341"
  "\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\3\0\0"
  "\0\5\0\0\0\10\0\0\0\15\0\0\0\23\0\0\0\34\0\0\0&\0\0\0""3\0\0\0B\0\0\0T\0"
  "\0\0f\0\0\0\377\361\361\361\353\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0"
  "\0\3\0\0\0\6\0\0\0\11\0\0\0\16\0\0\0\25\0\0\0\36\0\0\0)\0\0\0""7\0\0\0F\0"
  "\0\0X\0\0\0k\0\0\0\377\361\361\361\353\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0"
  "\0\2\0\0\0\3\0\0\0\6\0\0\0\12\0\0\0\17\0\0\0\26\0\0\0\37\0\0\0+\0\0\0""9"
  "\0\0\0J\0\0\0\\\0\0\0o\0\0\0\377\361\361\361\353\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\6\0\0\0\12\0\0\0\20\0\0\0\27\0\0\0!\0\0\0"
  "-\0\0\0<\0\0\0L\0\0\0_\0\0\0s\0\0\0\377\361\361\361\353\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341"
  "\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341"
  "\341\341\326\341\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\7\0\0\0\13\0\0\0\20\0\0\0\30\0\0\0\"\0"
  "\0\0.\0\0\0=\0\0\0O\0\0\0b\0\0\0v\0\0\0\377\361\361\361\353\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\7\0\0\0\13\0\0\0\21\0\0\0\31\0\0\0"
  "#\0\0\0""0\0\0\0?\0\0\0P\0\0\0c\0\0\0x\0\0\0\377\361\361\361\353\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\0\0\0\0\0\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\4\0\0\0\7\0\0\0\13\0\0\0\21\0\0\0"
  "\31\0\0\0#\0\0\0""0\0\0\0@\0\0\0Q\0\0\0e\0\0\0y\0\0\0\377\361\361\361\353"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341"
  "\341\326\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
  "\341\341\341\326\341\341\341\326\341\341\341\326\341\341\341\326"
};
#endif

typedef struct _PeekDisplay
{
    int screenPrivateIndex;

    HandleEventProc handleEvent;
    MousePollFunc   *mpFunc;

    CompTimeoutHandle timeoutHandle;
} PeekDisplay;

typedef struct _PeekScreen
{
    int windowPrivateIndex;

    PaintWindowProc paintWindow;
    PaintOutputProc paintOutput;

    CompWindow  *activeWindow;
#ifdef USE_BORDER_PAINT
    CompTexture windowTex;
#endif

    PositionPollingHandle pollHandle;
} PeekScreen;

typedef struct _PeekWindow
{
    Bool peekCandidate;
    Bool transparent;
    int  opacity;
} PeekWindow;

/* Core peek functions. These do the real work. ---------------------*/

/* Sets the real opacity and damages the window if actual opacity and
 * requested opacity differs. */
static void
setOpacity (CompWindow *w,
	    int        opacity)
{
    Bool transparent;

    PEEK_WINDOW (w);

    transparent = opacity != w->paint.opacity;

    if (transparent != pw->transparent || (w->lastPaint.opacity != opacity))
	addWindowDamage (w);

    pw->opacity = opacity;
    pw->transparent = transparent;
}

static Bool
checkPosition (CompWindow *w)
{
    if (peekGetCurrentViewport (w->screen))
    {
	if (w->serverX >= w->screen->width    ||
	    w->serverX + w->serverWidth <= 0  ||
	    w->serverY >= w->screen->height   ||
	    w->serverY + w->serverHeight <= 0)
	{
	    return FALSE;
	}
    }

    return TRUE;
}

static CompWindow *
peekFindWindowAt (CompScreen *s,
		  int        x,
		  int        y)
{
    CompWindow *cw;

    for (cw = s->windows; cw; cw = cw->next)
    {
	if (!cw->iconGeometrySet)
	    continue;

	if (cw->attrib.map_state != IsViewable)
	    continue;
	
	if (cw->state & CompWindowStateSkipTaskbarMask)
	    continue;

	if (cw->state & CompWindowStateSkipPagerMask)
	    continue;

	if (!cw->managed)
	    continue;

	if (!cw->texture->pixmap)
	    continue;

	if (x >= cw->iconGeometry.x                          &&
	    x < cw->iconGeometry.x + cw->iconGeometry.width  &&
	    y >= cw->iconGeometry.y                          &&
	    y < cw->iconGeometry.y + cw->iconGeometry.height &&
	    checkPosition (cw))
	    return cw;
    }

    return NULL;
}

static CompWindow *
peekFindDockAt (CompScreen *s,
		int        x,
		int        y)
{
    CompWindow *dock = s->windows;
    CompWindow *found = NULL;

    for (; dock && !found; dock = dock->next)
    {
	if (dock->type & CompWindowTypeDockMask)
	{
	    if ((x >= WIN_X (dock)) &&
		(x <= WIN_X (dock) + WIN_W (dock)) &&
		(y >= WIN_Y (dock)) &&
		(y <= WIN_Y (dock) + WIN_H (dock)))
	    {
		found = dock;
	    }
	}
    }

    return found;
}

static void
handleDockLeave (CompScreen *s)
{
    CompWindow *w;

    PEEK_SCREEN (s);
    PEEK_DISPLAY (s->display);

    for (w = s->windows; w; w = w->next)
    {
	PEEK_WINDOW (w);

    	if (pw->peekCandidate)
	    setOpacity (w, w->paint.opacity);
	pw->peekCandidate = FALSE;
    }

    ps->activeWindow = NULL;

    if (ps->pollHandle)
    {
	(*pd->mpFunc->removePositionPolling) (s, ps->pollHandle);
	ps->pollHandle = 0;
    }
}

static void
positionUpdate (CompScreen *s,
		int        x,
		int        y)
{
    CompWindow *w, *hovered, *dock;

    PEEK_SCREEN (s);

    dock = peekFindDockAt (s, x, y);
    if (dock)
    {
	hovered = peekFindWindowAt (s, x, y);
	if (hovered != ps->activeWindow)
	{
	    ps->activeWindow = hovered;

	    for (w = s->windows; w; w = w->next)
	    {
		PEEK_WINDOW (w);

		if (pw->peekCandidate)
		{
		    int opacity;

		    if (w == hovered)
		    {
			opacity = MIN (OPAQUE * peekGetActiveOpacity (s) / 100,
				       w->paint.opacity);
		    }
		    else
		    {
			opacity = MIN (OPAQUE * peekGetPassiveOpacity (s) / 100,
				       w->paint.opacity);
		    }

		    setOpacity (w, opacity);
		}
	    }
	}
    }
    else
    {
	handleDockLeave (s);
    }
}

static void
peekHandleDockEnter (CompScreen *s)
{
    CompWindow *w;

    PEEK_SCREEN (s);
    PEEK_DISPLAY (s->display);

    for (w = s->windows; w; w = w->next)
    {
	PEEK_WINDOW (w);

	pw->peekCandidate = !w->shaded && w->managed &&
			    !(w->type & CompWindowTypeDockMask) &&
			    matchEval (peekGetWindowMatch (s), w);

	if (pw->peekCandidate)
	{
	    int opacity;

	    opacity = MIN (OPAQUE * peekGetPassiveOpacity (s) / 100,
			   w->paint.opacity);

	    setOpacity (w, opacity);
	}
    }

    if (!ps->pollHandle)
    {
	ps->pollHandle = (*pd->mpFunc->addPositionPolling) (s, positionUpdate);
    }
}

static Bool
handleTimeout (void *data)
{
    int        x, y;
    CompWindow *w = (CompWindow *) data;
    CompWindow *dock;
    CompScreen *s = w->screen;

    PEEK_DISPLAY (s->display);

    pd->timeoutHandle = 0;

    (*pd->mpFunc->getCurrentPosition) (s, &x, &y);
    dock = peekFindDockAt (s, x, y);
    if (dock && dock == w)
    {
	peekHandleDockEnter (s);
	positionUpdate (s, x, y);
    }

    return FALSE;
}

#ifdef USE_BORDER_PAINT
static void
peekPaintBorder (CompWindow *w)
{
    CompScreen *s = w->screen;
    int        wx = WIN_X (w);
    int        wy = WIN_Y (w);
    int        width = WIN_W (w);
    int        height = WIN_H (w);
    int        off = 5;

    PEEK_SCREEN (s);
    PEEK_WINDOW (w);

    glEnable (GL_BLEND);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glColor4f (1.0, 1.0, 1.0, (float) pw->opacity / OPAQUE);
    enableTexture (s, &ps->windowTex, COMP_TEXTURE_FILTER_GOOD);

    glBegin (GL_QUADS);

    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glVertex2f (wx, wy + height);
    glVertex2f (wx + width, wy + height);
    glVertex2f (wx + width, wy);

    glTexCoord2f (0, 1);
    glVertex2f (wx - off, wy - off);
    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy - off);

    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy - off);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy);
    glTexCoord2f (0, 1);
    glVertex2f (wx + width + off, wy - off);

    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy + height);
    glTexCoord2f (0, 1);
    glVertex2f (wx - off, wy + height + off);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy + height + off);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);

    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy + height + off);
    glTexCoord2f (0, 1);
    glVertex2f (wx + width + off, wy + height + off);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy + height);

    glTexCoord2f (1, 1);
    glVertex2f (wx, wy - off);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy - off);

    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);
    glTexCoord2f (1, 1);
    glVertex2f (wx, wy + height + off);
    glTexCoord2f (1, 1);
    glVertex2f (wx + width, wy + height + off);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);

    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy);
    glTexCoord2f (0, 0);
    glVertex2f (wx - off, wy + height);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy + height);
    glTexCoord2f (1, 0);
    glVertex2f (wx, wy);

    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy);
    glTexCoord2f (1, 0);
    glVertex2f (wx + width, wy + height);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy + height);
    glTexCoord2f (0, 0);
    glVertex2f (wx + width + off, wy);

    glEnd ();
    disableTexture (s, &ps->windowTex);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

static Bool
peekPaintOutput (CompScreen              *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform     *transform,
		  Region                  region,
		  CompOutput              *output,
		  unsigned int            mask)
{
    Bool status;

    PEEK_SCREEN (s);

    UNWRAP (ps, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region,
	 			output, mask);
    WRAP (ps, s, paintOutput, peekPaintOutput);

    if (ps->pollHandle)
    {
	CompTransform sTransform = *transform;
	CompWindow    *w;

        transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA,
				&sTransform);
	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	for (w = s->windows; w; w = w->next)
	{
	    PEEK_WINDOW (w);

	    if (pw->peekCandidate)
		peekPaintBorder (w);
	}

	/* Repaint the active window on top of all the bordered windows */
	if (ps->activeWindow)
	    (*s->paintWindow) (ps->activeWindow, &ps->activeWindow->paint,
			       &sTransform, region, 0);

	glPopMatrix ();
    }

    return status;
}
#endif

static Bool
peekPaintWindow (CompWindow              *w,
		 const WindowPaintAttrib *attrib,
		 const CompTransform     *transform,
		 Region                  region,
		 unsigned int            mask)
{
    Bool       status;
    CompScreen *s = w->screen;

    PEEK_SCREEN (s);
    PEEK_WINDOW (w);

    if (pw->transparent)
    {
	WindowPaintAttrib wAttrib = *attrib;

	wAttrib.opacity = pw->opacity;

	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, &wAttrib, transform, region, mask);
	WRAP (ps, s, paintWindow, peekPaintWindow);

    }
    else
    {
	UNWRAP (ps, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ps, s, paintWindow, peekPaintWindow);
    }

    return status;
}

static void
peekHandleEvent (CompDisplay *d,
		    XEvent      *event)
{
    CompWindow *w;

    PEEK_DISPLAY (d);

    UNWRAP (pd, d, handleEvent);
    (*d->handleEvent) (d, event);
    WRAP (pd, d, handleEvent, peekHandleEvent);

    switch (event->type) {
    case EnterNotify:
	if (pd->timeoutHandle)
	{
	    compRemoveTimeout (pd->timeoutHandle);
	    pd->timeoutHandle = 0;
	}

	w = findWindowAtDisplay (d, event->xcrossing.window);
	if (w)
	{
	    PEEK_SCREEN (w->screen);

	    if (w->type & CompWindowTypeDockMask)
	    {
		pd->timeoutHandle = compAddTimeout (peekGetTimeout (d),
						    (float)
						    peekGetTimeout (d) * 1.2,
						    handleTimeout, w);
	    }
	    else if (ps->pollHandle)
	    {
		handleDockLeave (w->screen);
	    }
	}
	break;
    default:
	break;
    }
}

/* Configuration, initialization, boring stuff. ----------------------- */

static Bool
peekInitWindow (CompPlugin *p,
		CompWindow *w)
{
    PeekWindow *pw;

    PEEK_SCREEN (w->screen);

    pw = malloc (sizeof (PeekWindow));
    if (!pw)
	return FALSE;

    pw->peekCandidate = FALSE;
    pw->transparent = FALSE;
    pw->opacity = OPAQUE;

    w->base.privates[ps->windowPrivateIndex].ptr = pw;

    return TRUE;
}

static void
peekFiniWindow (CompPlugin *p,
		   CompWindow *w)
{
    PEEK_WINDOW (w);
    PEEK_SCREEN (w->screen);

    if (w == ps->activeWindow)
	ps->activeWindow = NULL;

    free (pw);
}

static Bool
peekInitScreen (CompPlugin *p,
		   CompScreen *s)
{
    PeekScreen *ps;

    PEEK_DISPLAY (s->display);

    ps = calloc (1, sizeof (PeekScreen));
    if (!ps)
	return FALSE;

    ps->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ps->windowPrivateIndex < 0)
    {
	free (ps);
	return FALSE;
    }

    WRAP (ps, s, paintWindow, peekPaintWindow);

    s->base.privates[pd->screenPrivateIndex].ptr = ps;
    ps->pollHandle = 0;

#ifdef USE_BORDER_PAINT
    WRAP (ps, s, paintOutput, peekPaintOutput);

    initTexture (s, &ps->windowTex);

    imageDataToTexture (s, &ps->windowTex, windowTex, 32, 32,
			GL_RGBA, GL_UNSIGNED_BYTE);
#endif

    return TRUE;
}

static void
peekFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    PEEK_SCREEN (s);
    PEEK_DISPLAY (s->display);

    UNWRAP (ps, s, paintWindow);

#ifdef USE_BORDER_PAINT
    UNWRAP (ps, s, paintOutput);

    finiTexture (s, &ps->windowTex);
#endif

    if (ps->pollHandle)
	(*pd->mpFunc->removePositionPolling) (s, ps->pollHandle);

    free (ps);
}

static Bool
peekInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    PeekDisplay *pd;
    int         index;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
	!checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "mousepoll", &index))
	return FALSE;

    pd = malloc (sizeof (PeekDisplay));
    if (!pd)
	return FALSE;

    pd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (pd->screenPrivateIndex < 0)
    {
	free (pd);
	return FALSE;
    }
    d->base.privates[displayPrivateIndex].ptr = pd;

    pd->timeoutHandle = 0;
    pd->mpFunc = d->base.privates[index].ptr;

    WRAP (pd, d, handleEvent, peekHandleEvent);

    return TRUE;
}

static void
peekFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    PEEK_DISPLAY (d);

    UNWRAP (pd, d, handleEvent);

    if (pd->timeoutHandle)
	compRemoveTimeout(pd->timeoutHandle);

    freeScreenPrivateIndex (d, pd->screenPrivateIndex);

    free (pd);
}

static CompBool
peekInitObject (CompPlugin *p,
		   CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) peekInitDisplay,
	(InitPluginObjectProc) peekInitScreen,
	(InitPluginObjectProc) peekInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
peekFiniObject (CompPlugin *p,
		   CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) peekFiniDisplay,
	(FiniPluginObjectProc) peekFiniScreen,
	(FiniPluginObjectProc) peekFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
peekInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
peekFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable peekVTable = {
    "peek",
    0,
    peekInit,
    peekFini,
    peekInitObject,
    peekFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &peekVTable;
}
