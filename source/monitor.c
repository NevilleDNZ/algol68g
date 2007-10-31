/*
\file monitor.c
\brief low-level monitor for the interpreter
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2007 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


/*
This is a basic monitor for Algol68G. It activates when the interpreter
receives SIGINT (CTRL-C, for instance) or when PROC VOID break, debug or
evaluate is called, or when a runtime error occurs and --debug is selected.

The monitor allows single stepping (unit-wise through serial/enquiry
clauses) and has basic means for inspecting call-frame stack and heap. 
*/

#include "algol68g.h"
#include "genie.h"
#include "mp.h"
#include "transput.h"

#if defined ENABLE_TERMINFO
#include <term.h>
extern char term_buffer[];
char *term_type;
#endif

static int mon_errors = 0;
static int current_frame = 0;

#define QUIT_ON_ERROR\
  if (mon_errors > 0) {\
    return;\
  }

#define PARSE_CHECK(f, p, d)\
  parse ((f), (p), (d));\
  QUIT_ON_ERROR;

#define SCAN_CHECK(f, p)\
  scan_sym((f), (p));\
  QUIT_ON_ERROR;

#define WRITE(f, s) io_write_string ((f), (s));

#define WRITELN(f, s)\
  io_close_tty_line ();\
  WRITE (f, s);

#define MAX_ROW_ELEMS 24
#define STACK_SIZE 3
#define NO_VALUE " uninitialised value"
#define CANNOT_SHOW " unprintable value or uninitialised value"
#define TOP_MODE (m_stack[m_sp - 1])
#define LOGOUT_STRING "exit"

BOOL_T in_monitor = A68_FALSE;

static int tabs = 0;
static int max_row_elems = MAX_ROW_ELEMS;

static char prompt[BUFFER_SIZE] = { '=', '=', '=', '>', BLANK_CHAR, NULL_CHAR };

static BOOL_T check_initialisation (NODE_T *, BYTE_T *, MOID_T *, BOOL_T *);
static void parse (FILE_T, NODE_T *, int);

/*!
\brief ask whether we really want to quit
\return 
*/

static BOOL_T confirm_exit (void)
{
  char *cmd;
  int k;
  snprintf (output_line, BUFFER_SIZE, "++++ Terminate %s (yes|no): ", A68G_NAME);
  WRITELN (STDOUT_FILENO, output_line);
  cmd = read_string_from_tty (NULL);
  if (TO_UCHAR (cmd[0]) == TO_UCHAR (EOF_CHAR)) {
    return (confirm_exit ());
  }
  for (k = 0; cmd[k] != NULL_CHAR; k++) {
    cmd[k] = TO_LOWER (cmd[k]);
  }
  if (strcmp (cmd, "y") == 0) {
    return (A68_TRUE);
  }
  if (strcmp (cmd, "yes") == 0) {
    return (A68_TRUE);
  }
  if (strcmp (cmd, "n") == 0) {
    return (A68_FALSE);
  }
  if (strcmp (cmd, "no") == 0) {
    return (A68_FALSE);
  }
  return (confirm_exit ());
}

/*!
\brief give a monitor error message
*/

void monitor_error (char *msg, char *info)
{
  char edit[BUFFER_SIZE];
  QUIT_ON_ERROR;
  mon_errors++;
  bufcpy (edit, msg, BUFFER_SIZE);
  WRITELN (STDOUT_FILENO, "++++ Monitor error. ");
  edit[0] = TO_UPPER (edit[0]);
  WRITE (STDOUT_FILENO, edit);
  if (info != NULL) {
    WRITE (STDOUT_FILENO, " (");
    WRITE (STDOUT_FILENO, info);
    WRITE (STDOUT_FILENO, ")");
  }
  WRITE (STDOUT_FILENO, ".");
}

static char expr[BUFFER_SIZE];
static int pos, attr;
static char *symbol;
static MOID_T *m_stack[STACK_SIZE];
static int m_sp;

/*!
\brief scan symbol from input
\param f 
\param p
*/

static void scan_sym (FILE_T f, NODE_T * p)
{
  int k = 0;
  char buffer[BUFFER_SIZE];
  (void) f;
  (void) p;
  buffer[k] = NULL_CHAR;
  attr = 0;
  QUIT_ON_ERROR;
  while (IS_SPACE (expr[pos])) {
    pos++;
  }
  if (expr[pos] == NULL_CHAR) {
    attr = 0;
    symbol = NULL;
    return;
  } else if (expr[pos] == ':') {
    if (strncmp (&(expr[pos]), ":=:", 3) == 0) {
      pos += 3;
      bufcpy (buffer, ":=:", BUFFER_SIZE);
      attr = IS_SYMBOL;
    } else if (strncmp (&(expr[pos]), ":/=:", 4) == 0) {
      pos += 4;
      bufcpy (buffer, ":/=:", BUFFER_SIZE);
      attr = ISNT_SYMBOL;
    } else if (strncmp (&(expr[pos]), ":=", 2) == 0) {
      pos += 2;
      bufcpy (buffer, ":=", BUFFER_SIZE);
      attr = ASSIGN_SYMBOL;
    } else {
      pos++;
      bufcpy (buffer, ":", BUFFER_SIZE);
      attr = COLON_SYMBOL;
    }
    symbol = add_token (&top_token, buffer)->text;
    return;
  } else if (expr[pos] == QUOTE_CHAR) {
    BOOL_T cont = A68_TRUE;
    pos++;
    while (cont) {
      while (expr[pos] != QUOTE_CHAR) {
        buffer[k++] = expr[pos++];
      }
      if (expr[++pos] == QUOTE_CHAR) {
        buffer[k++] = QUOTE_CHAR;
      } else {
        cont = A68_FALSE;
      }
    }
    buffer[k] = NULL_CHAR;
    symbol = add_token (&top_token, buffer)->text;
    attr = ROW_CHAR_DENOTER;
    return;
  } else if (IS_LOWER (expr[pos])) {
    while (IS_LOWER (expr[pos]) || IS_DIGIT (expr[pos]) || IS_SPACE (expr[pos])) {
      if (IS_SPACE (expr[pos])) {
        pos++;
      } else {
        buffer[k++] = expr[pos++];
      }
    }
    buffer[k] = NULL_CHAR;
    symbol = add_token (&top_token, buffer)->text;
    attr = IDENTIFIER;
    return;
  } else if (IS_UPPER (expr[pos])) {
    KEYWORD_T *kw;
    while (IS_UPPER (expr[pos])) {
      buffer[k++] = expr[pos++];
    }
    buffer[k] = NULL_CHAR;
    kw = find_keyword (top_keyword, buffer);
    if (kw != NULL) {
      attr = ATTRIBUTE (kw);
      symbol = TEXT (kw);
    } else {
      attr = OPERATOR;
      symbol = add_token (&top_token, buffer)->text;
    }
    return;
  } else if (IS_DIGIT (expr[pos])) {
    while (IS_DIGIT (expr[pos])) {
      buffer[k++] = expr[pos++];
    }
    if (expr[pos] == 'r') {
      buffer[k++] = expr[pos++];
      while (IS_XDIGIT (expr[pos])) {
        buffer[k++] = expr[pos++];
      }
      buffer[k] = NULL_CHAR;
      symbol = add_token (&top_token, buffer)->text;
      attr = BITS_DENOTER;
      return;
    }
    if (expr[pos] != POINT_CHAR && expr[pos] != 'e' && expr[pos] != 'E') {
      buffer[k] = NULL_CHAR;
      symbol = add_token (&top_token, buffer)->text;
      attr = INT_DENOTER;
      return;
    }
    if (expr[pos] == POINT_CHAR) {
      buffer[k++] = expr[pos++];
      while (IS_DIGIT (expr[pos])) {
        buffer[k++] = expr[pos++];
      }
    }
    if (expr[pos] != 'e' && expr[pos] != 'E') {
      buffer[k] = NULL_CHAR;
      symbol = add_token (&top_token, buffer)->text;
      attr = REAL_DENOTER;
      return;
    }
    buffer[k++] = TO_UPPER (expr[pos++]);
    if (expr[pos] == '+' || expr[pos] == '-') {
      buffer[k++] = expr[pos++];
    }
    while (IS_DIGIT (expr[pos])) {
      buffer[k++] = expr[pos++];
    }
    buffer[k] = NULL_CHAR;
    symbol = add_token (&top_token, buffer)->text;
    attr = REAL_DENOTER;
    return;
  } else if (a68g_strchr (MONADS, expr[pos]) != NULL || a68g_strchr (NOMADS, expr[pos]) != NULL) {
    buffer[k++] = expr[pos++];
    if (a68g_strchr (NOMADS, expr[pos]) != NULL) {
      buffer[k++] = expr[pos++];
    }
    if (expr[pos] == ':') {
      buffer[k++] = expr[pos++];
      if (expr[pos] == '=') {
        buffer[k++] = expr[pos++];
      } else {
        buffer[k] = NULL_CHAR;
        monitor_error ("operator symbol error", buffer);
      }
    } else if (expr[pos] == '=') {
      buffer[k++] = expr[pos++];
      if (expr[pos] == ':') {
        buffer[k++] = expr[pos++];
      } else {
        buffer[k] = NULL_CHAR;
        monitor_error ("operator symbol error", buffer);
      }
    }
    buffer[k] = NULL_CHAR;
    symbol = add_token (&top_token, buffer)->text;
    attr = OPERATOR;
    return;
  } else if (expr[pos] == '(') {
    pos++;
    symbol = add_token (&top_token, "(")->text;
    attr = OPEN_SYMBOL;
    return;
  } else if (expr[pos] == ')') {
    pos++;
    symbol = add_token (&top_token, ")")->text;
    attr = CLOSE_SYMBOL;
    return;
  } else if (expr[pos] == '[') {
    pos++;
    symbol = add_token (&top_token, "[")->text;
    attr = SUB_SYMBOL;
    return;
  } else if (expr[pos] == ']') {
    pos++;
    symbol = add_token (&top_token, "]")->text;
    attr = BUS_SYMBOL;
    return;
  } else if (expr[pos] == ',') {
    pos++;
    symbol = add_token (&top_token, ",")->text;
    attr = COMMA_SYMBOL;
    return;
  } else if (expr[pos] == ';') {
    pos++;
    symbol = add_token (&top_token, ";")->text;
    attr = SEMI_SYMBOL;
    return;
  }
}

/*!
\brief return priority for symbol at input
\return 
*/

static int prio (FILE_T f, NODE_T * p)
{
  TAG_T *s = find_tag_global (stand_env, PRIO_SYMBOL, symbol);
  (void) p;
  (void) f;
  if (s == NULL) {
    monitor_error ("unknown operator", symbol);
    return (0);
  }
  return (PRIO (s));
}

/*!
\brief push a mode on the stack
\param f
\param m
*/

static void push_mode (FILE_T f, MOID_T * m)
{
  (void) f;
  if (m_sp < STACK_SIZE) {
    m_stack[m_sp++] = m;
  } else {
    monitor_error ("expression too complex", NULL);
  }
}

/*!
\brief dereference, WEAK or otherwise
\param k
\param context
\return whether value can be dereferenced further
*/

static BOOL_T deref_condition (int k, int context)
{
  MOID_T *u = m_stack[k];
  if (context == WEAK && SUB (u) != NULL) {
    MOID_T *v = SUB (u);
    BOOL_T stowed = WHETHER (v, FLEX_SYMBOL) || WHETHER (v, ROW_SYMBOL) || WHETHER (v, STRUCT_SYMBOL);
    return (WHETHER (u, REF_SYMBOL) && !stowed);
  } else {
    return (WHETHER (u, REF_SYMBOL));
  }
}

/*!
\brief weak dereferencing
*/

static void deref (NODE_T * p, int k, int context)
{
  while (deref_condition (k, context)) {
    A68_REF z;
    POP_REF (p, &z);
    CHECK_NIL (p, z, m_stack[k]);
    CHECK_INIT (p, INITIALISED (&z), m_stack[k]);
    m_stack[k] = SUB (m_stack[k]);
    PUSH (p, ADDRESS (&z), MOID_SIZE (m_stack[k]));
  }
}

/*!
\brief search mode that matches indicant
\param refs
\param leng
\param indy
\return moid
**/

static MOID_T *search_mode (int refs, int leng, char *indy)
{
  MOID_LIST_T *l = top_moid_list;
  MOID_T *z = A68_FALSE;
  for (l = top_moid_list; l != NULL; FORWARD (l)) {
    MOID_T *m = MOID (l);
    if (NODE (m) != NULL) {
      if (indy == SYMBOL (NODE (m)) && leng == DIMENSION (m)) {
        z = m;
        while (EQUIVALENT (z) != NULL) {
          z = EQUIVALENT (z);
        }
      }
    }
  }
  if (z == NULL) {
    monitor_error ("unknown indicant", indy);
    return (NULL);
  }
  for (l = top_moid_list; l != NULL; FORWARD (l)) {
    MOID_T *m = MOID (l);
    int k = 0;
    while (WHETHER (m, REF_SYMBOL)) {
      k++;
      m = SUB (m);
    }
    if (k == refs && m == z) {
      z = MOID (l);
      while (EQUIVALENT (z) != NULL) {
        z = EQUIVALENT (z);
      }
      return (z);
    }
  }
  return (NULL);
}

/*!
\brief search operator X SYM Y
\param sym
\param x
\param y
\return
*/

static TAG_T *search_operator (char *sym, MOID_T * x, MOID_T * y)
{
  TAG_T *t;
  for (t = stand_env->operators; t != NULL; FORWARD (t)) {
    if (SYMBOL (NODE (t)) == sym) {
      PACK_T *p = PACK (MOID (t));
      if (x == MOID (p)) {
        FORWARD (p);
        if (p == NULL && y == NULL) {
/* Matched in case of a monad. */
          return (t);
        } else if (p != NULL && y != NULL && y == MOID (p)) {
/* Matched in case of a nomad. */
          return (t);
        }
      }
    }
  }
/* Not found yet, try dereferencing. */
  if (WHETHER (x, REF_SYMBOL)) {
    return (search_operator (sym, SUB (x), y));
  }
  if (y != NULL && WHETHER (y, REF_SYMBOL)) {
    return (search_operator (sym, x, SUB (y)));
  }
/* Not found. Grrrr. Give a message. */
  if (y == NULL) {
    snprintf (edit_line, BUFFER_SIZE, "%s %s", sym, moid_to_string (x, MOID_WIDTH));
  } else {
    snprintf (edit_line, BUFFER_SIZE, "%s %s %s", moid_to_string (x, MOID_WIDTH), sym, moid_to_string (y, MOID_WIDTH));
  }
  monitor_error ("cannot find operator in standard environ", edit_line);
  return (NULL);
}

/*!
\brief search identifier in frame stack and push value
*/

static void search_identifier (FILE_T f, NODE_T * p, ADDR_T link, char *sym)
{
  if (link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    if (current_frame == 0 || (current_frame == FRAME_NUMBER (link))) {
      NODE_T *u = FRAME_TREE (link);
      if (u != NULL) {
        SYMBOL_TABLE_T *q = SYMBOL_TABLE (u);
        TAG_T *i = q->identifiers;
        for (; i != NULL; FORWARD (i)) {
          if (sym == SYMBOL (NODE (i))) {
            ADDR_T pos = link + FRAME_INFO_SIZE + i->offset;
            MOID_T *m = MOID (i);
            PUSH (p, FRAME_ADDRESS (pos), MOID_SIZE (m));
            push_mode (f, m);
            return;
          }
        }
      }
    }
    search_identifier (f, p, dynamic_link, sym);
  } else {
    SYMBOL_TABLE_T *q = stand_env;
    TAG_T *i = q->identifiers;
    for (; i != NULL; FORWARD (i)) {
      if (sym == SYMBOL (NODE (i))) {
        if (WHETHER (MOID (i), PROC_SYMBOL)) {
          static A68_PROCEDURE z;
          z.status = (INITIALISED_MASK | STANDENV_PROC_MASK);
          z.body = (void *) i->procedure;
          z.environ = 0;
          z.locale = NULL;
          z.proc_mode = MOID (i);
          PUSH_PROCEDURE (p, z);
        } else {
          (i->procedure) (p);
        }
        push_mode (f, MOID (i));
        return;
      }
    }
    monitor_error ("cannot find identifier", sym);
  }
}

/*!
\brief coerce arguments in a call
*/

static void coerce_arguments (FILE_T f, NODE_T * p, MOID_T * proc, int bot, int top, int top_sp)
{
  int k;
  PACK_T *u;
  ADDR_T sp_2 = top_sp;
  (void) f;
  if ((top - bot) != DIMENSION (proc)) {
    monitor_error ("procedure argument count", NULL);
  }
  QUIT_ON_ERROR;
  for (k = bot, u = PACK (proc); k < top; k++, u = NEXT (u)) {
    if (m_stack[k] == MOID (u)) {
      PUSH (p, STACK_ADDRESS (sp_2), MOID_SIZE (MOID (u)));
      sp_2 += MOID_SIZE (MOID (u));
    } else if (WHETHER (m_stack[k], REF_SYMBOL)) {
      A68_REF *v = (A68_REF *) STACK_ADDRESS (sp_2);
      PUSH_REF (p, *v);
      sp_2 += ALIGNED_SIZEOF (A68_REF);
      deref (p, k, STRONG);
      if (m_stack[k] != MOID (u)) {
        snprintf (edit_line, BUFFER_SIZE, "%s to %s", moid_to_string (m_stack[k], MOID_WIDTH), moid_to_string (MOID (u), MOID_WIDTH));
        monitor_error ("argument mode error", edit_line);
      }
    } else {
      snprintf (edit_line, BUFFER_SIZE, "%s to %s", moid_to_string (m_stack[k], MOID_WIDTH), moid_to_string (MOID (u), MOID_WIDTH));
      monitor_error ("cannot coerce argument", edit_line);
    }
    QUIT_ON_ERROR;
  }
  MOVE (STACK_ADDRESS (top_sp), STACK_ADDRESS (sp_2), stack_pointer - sp_2);
  stack_pointer = top_sp + (stack_pointer - sp_2);
}

/*!
\brief perform a selection
*/

static void selection (FILE_T f, NODE_T * p, char *field)
{
  BOOL_T name;
  MOID_T *moid;
  PACK_T *u, *v;
  SCAN_CHECK (f, p);
  if (attr != IDENTIFIER && attr != OPEN_SYMBOL) {
    monitor_error ("selection syntax error", NULL);
  }
  QUIT_ON_ERROR;
  PARSE_CHECK (f, p, MAX_PRIORITY + 1);
  deref (p, m_sp - 1, WEAK);
  if (WHETHER (TOP_MODE, REF_SYMBOL)) {
    name = A68_TRUE;
    u = PACK (NAME (TOP_MODE));
    moid = SUB (m_stack[--m_sp]);
    v = PACK (moid);
  } else {
    name = A68_FALSE;
    moid = m_stack[--m_sp];
    u = PACK (moid);
    v = PACK (moid);
  }
  if (WHETHER_NOT (moid, STRUCT_SYMBOL)) {
    monitor_error ("selection mode error", moid_to_string (moid, MOID_WIDTH));
  }
  QUIT_ON_ERROR;
  for (; u != NULL; FORWARD (u), FORWARD (v)) {
    if (field == u->text) {
      if (name) {
        A68_REF *z = (A68_REF *) (STACK_OFFSET (-ALIGNED_SIZEOF (A68_REF)));
        CHECK_NIL (p, *z, moid);
        z->offset += v->offset;
      } else {
        DECREMENT_STACK_POINTER (p, MOID_SIZE (moid));
        MOVE (STACK_TOP, STACK_OFFSET (v->offset), (unsigned) MOID_SIZE (MOID (u)));
        INCREMENT_STACK_POINTER (p, MOID_SIZE (MOID (u)));
      }
      push_mode (f, MOID (u));
      return;
    }
  }
  monitor_error ("field name error", field);
}

/*!
\brief perform a call
*/

static void call (FILE_T f, NODE_T * p, int depth)
{
  A68_PROCEDURE z;
  NODE_T q;
  int args, old_m_sp;
  ADDR_T top_sp;
  MOID_T *proc;
  (void) depth;
  QUIT_ON_ERROR;
  deref (p, m_sp - 1, STRONG);
  proc = m_stack[--m_sp];
  old_m_sp = m_sp;
  if (WHETHER_NOT (proc, PROC_SYMBOL)) {
    monitor_error ("procedure mode error", moid_to_string (proc, MOID_WIDTH));
  }
  QUIT_ON_ERROR;
  POP_PROCEDURE (p, &z);
  args = m_sp;
  top_sp = stack_pointer;
  if (attr == OPEN_SYMBOL) {
    do {
      SCAN_CHECK (f, p);
      PARSE_CHECK (f, p, 0);
    } while (attr == COMMA_SYMBOL);
    if (attr != CLOSE_SYMBOL) {
      monitor_error ("unmatched parenthesis", NULL);
    }
    SCAN_CHECK (f, p);
  }
  coerce_arguments (f, p, proc, args, m_sp, top_sp);
  if (z.status & STANDENV_PROC_MASK) {
    MOID (&q) = m_stack[--m_sp];
    INFO (&q) = INFO (p);
    SYMBOL (&q) = SYMBOL (p);
    (void) ((GENIE_PROCEDURE *) z.body) (&q);
    m_sp = old_m_sp;
    push_mode (f, SUB (z.proc_mode));
  } else {
    monitor_error ("can only call standard environ routines", NULL);
  }
}

/*!
\brief perform a slice
*/

static void slice (FILE_T f, NODE_T * p, int depth)
{
  MOID_T *moid, *res;
  A68_REF z;
  A68_ARRAY *x;
  ADDR_T ref_heap, address;
  int dim, k, index, args;
  BOOL_T name;
  (void) depth;
  QUIT_ON_ERROR;
  deref (p, m_sp - 1, WEAK);
  if (WHETHER (TOP_MODE, REF_SYMBOL)) {
    name = A68_TRUE;
    res = NAME (TOP_MODE);
    deref (p, m_sp - 1, STRONG);
    moid = m_stack[--m_sp];
  } else {
    name = A68_FALSE;
    moid = m_stack[--m_sp];
    res = SUB (moid);
  }
  if (WHETHER_NOT (moid, ROW_SYMBOL) && WHETHER_NOT (moid, FLEX_SYMBOL)) {
    monitor_error ("row mode error", moid_to_string (moid, MOID_WIDTH));
  }
  QUIT_ON_ERROR;
/* Get descriptor. */
  POP_REF (p, &z);
  CHECK_NIL (p, z, moid);
  x = (A68_ARRAY *) ADDRESS (&z);
  if (WHETHER (moid, FLEX_SYMBOL)) {
    dim = DIMENSION (SUB (moid));
  } else {
    dim = DIMENSION (moid);
  }
/* Get indexer. */
  ref_heap = REF_HANDLE (&z)->offset + ALIGNED_SIZEOF (A68_ARRAY) + (dim - 1) * ALIGNED_SIZEOF (A68_TUPLE);
  args = m_sp;
  if (attr == SUB_SYMBOL) {
    do {
      SCAN_CHECK (f, p);
      PARSE_CHECK (f, p, 0);
    } while (attr == COMMA_SYMBOL);
    if (attr != BUS_SYMBOL) {
      monitor_error ("unmatched parenthesis", NULL);
    }
    SCAN_CHECK (f, p);
  }
  if ((m_sp - args) != dim) {
    monitor_error ("slice index count error", NULL);
  }
  QUIT_ON_ERROR;
  for (k = 0, index = 0; k < dim; k++, ref_heap -= sizeof (A68_TUPLE), m_sp--) {
    A68_TUPLE *t = (A68_TUPLE *) HEAP_ADDRESS (ref_heap);
    A68_INT i;
    deref (p, m_sp - 1, MEEK);
    if (TOP_MODE != MODE (INT)) {
      monitor_error ("indexer mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
    }
    QUIT_ON_ERROR;
    POP_OBJECT (p, &i, A68_INT);
    if (i.value < t->lower_bound || i.value > t->upper_bound) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_INDEX_OUT_OF_BOUNDS);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    QUIT_ON_ERROR;
    index += t->span * i.value - t->shift;
  }
  address = ROW_ELEMENT (x, index);
  if (name) {
    z = x->array;
    z.offset += address;
    SET_REF_SCOPE (&z, PRIMAL_SCOPE);
    PUSH_REF (p, z);
  } else {
    PUSH (p, ADDRESS (&(x->array)) + address, MOID_SIZE (res));
  }
  push_mode (f, res);
}

/*!
\brief perform a call or a slice
*/

static void call_or_slice (FILE_T f, NODE_T * p, int depth)
{
  while (attr == OPEN_SYMBOL || attr == SUB_SYMBOL) {
    QUIT_ON_ERROR;
    if (attr == OPEN_SYMBOL) {
      call (f, p, depth);
    } else if (attr == SUB_SYMBOL) {
      slice (f, p, depth);
    }
  }
}

/*!
\brief parse expression on input
*/

static void parse (FILE_T f, NODE_T * p, int depth)
{
  QUIT_ON_ERROR;
  if (depth <= MAX_PRIORITY) {
    if (depth == 0) {
/* Identity relations. */
      PARSE_CHECK (f, p, 1);
      while (attr == IS_SYMBOL || attr == ISNT_SYMBOL) {
        A68_REF x, y;
        BOOL_T res;
        int op = attr;
        if (TOP_MODE != MODE (HIP) && WHETHER_NOT (TOP_MODE, REF_SYMBOL)) {
          monitor_error ("identity relation operand must yield a name", moid_to_string (TOP_MODE, MOID_WIDTH));
        }
        SCAN_CHECK (f, p);
        PARSE_CHECK (f, p, 1);
        if (TOP_MODE != MODE (HIP) && WHETHER_NOT (TOP_MODE, REF_SYMBOL)) {
          monitor_error ("identity relation operand must yield a name", moid_to_string (TOP_MODE, MOID_WIDTH));
        }
        QUIT_ON_ERROR;
        if (TOP_MODE != MODE (HIP) && m_stack[m_sp - 2] != MODE (HIP)) {
          if (TOP_MODE != m_stack[m_sp - 2]) {
            monitor_error ("identity relation operand mode error", NULL);
          }
        }
        QUIT_ON_ERROR;
        m_sp -= 2;
        POP_REF (p, &y);
        POP_REF (p, &x);
        res = (ADDRESS (&x) == ADDRESS (&y));
        PUSH_PRIMITIVE (p, (op == IS_SYMBOL ? res : !res), A68_BOOL);
        push_mode (f, MODE (BOOL));
      }
    } else {
/* Dyadic expressions. */
      PARSE_CHECK (f, p, depth + 1);
      while (attr == OPERATOR && prio (f, p) == depth) {
        int args;
        ADDR_T top_sp;
        NODE_T q;
        TAG_T *opt;
        char *op = symbol;
        args = m_sp - 1;
        top_sp = stack_pointer - MOID_SIZE (m_stack[args]);
        SCAN_CHECK (f, p);
        PARSE_CHECK (f, p, depth + 1);
        opt = search_operator (op, m_stack[m_sp - 2], TOP_MODE);
        QUIT_ON_ERROR;
        coerce_arguments (f, p, MOID (opt), args, m_sp, top_sp);
        m_sp -= 2;
        MOID (&q) = MOID (opt);
        INFO (&q) = INFO (p);
        SYMBOL (&q) = SYMBOL (p);
        (void) ((GENIE_PROCEDURE *) (opt->procedure)) (&q);
        push_mode (f, SUB (MOID (opt)));
      }
    }
  } else if (attr == OPERATOR) {
    int args;
    ADDR_T top_sp;
    NODE_T q;
    TAG_T *opt;
    char *op = symbol;
    args = m_sp;
    top_sp = stack_pointer;
    SCAN_CHECK (f, p);
    PARSE_CHECK (f, p, depth);
    opt = search_operator (op, TOP_MODE, NULL);
    QUIT_ON_ERROR;
    coerce_arguments (f, p, MOID (opt), args, m_sp, top_sp);
    m_sp--;
    MOID (&q) = MOID (opt);
    INFO (&q) = INFO (p);
    SYMBOL (&q) = SYMBOL (p);
    (void) ((GENIE_PROCEDURE *) (opt->procedure)) (&q);
    push_mode (f, SUB (MOID (opt)));
  } else if (attr == REF_SYMBOL) {
    int refs = 0, length = 0;
    MOID_T *m = NULL;
    while (attr == REF_SYMBOL) {
      refs++;
      SCAN_CHECK (f, p);
    }
    while (attr == LONG_SYMBOL) {
      length++;
      SCAN_CHECK (f, p);
    }
    m = search_mode (refs, length, symbol);
    QUIT_ON_ERROR;
    if (m == NULL) {
      monitor_error ("unknown reference to mode", NULL);
    }
    SCAN_CHECK (f, p);
    if (attr != OPEN_SYMBOL) {
      monitor_error ("cast expects open-symbol", NULL);
    }
    SCAN_CHECK (f, p);
    PARSE_CHECK (f, p, 0);
    if (attr != CLOSE_SYMBOL) {
      monitor_error ("cast expects close-symbol", NULL);
    }
    SCAN_CHECK (f, p);
    while (WHETHER (TOP_MODE, REF_SYMBOL) && TOP_MODE != m) {
      MOID_T *sub = SUB (TOP_MODE);
      A68_REF z;
      POP_REF (p, &z);
      CHECK_NIL (p, z, TOP_MODE);
      PUSH (p, ADDRESS (&z), MOID_SIZE (sub));
      TOP_MODE = sub;
    }
    if (TOP_MODE != m) {
      monitor_error ("cast mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
    }
  } else if (attr == LONG_SYMBOL) {
    int length = 0;
    MOID_T *m;
    while (attr == LONG_SYMBOL) {
      length++;
      SCAN_CHECK (f, p);
    }
/* Cast L INT -> L REAL. */
    if (attr == REAL_SYMBOL) {
      MOID_T *i = (length == 1 ? MODE (LONG_INT) : MODE (LONGLONG_INT));
      MOID_T *r = (length == 1 ? MODE (LONG_REAL) : MODE (LONGLONG_REAL));
      SCAN_CHECK (f, p);
      if (attr != OPEN_SYMBOL) {
        monitor_error ("cast expects open-symbol", NULL);
      }
      SCAN_CHECK (f, p);
      PARSE_CHECK (f, p, 0);
      if (attr != CLOSE_SYMBOL) {
        monitor_error ("cast expects close-symbol", NULL);
      }
      SCAN_CHECK (f, p);
      if (TOP_MODE != i) {
        monitor_error ("cast argument mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
      }
      QUIT_ON_ERROR;
      TOP_MODE = r;
      return;
    }
/* L INT or L REAL denoter. */
    if (attr == INT_DENOTER) {
      m = (length == 1 ? MODE (LONG_INT) : MODE (LONGLONG_INT));
    } else if (attr == REAL_DENOTER) {
      m = (length == 1 ? MODE (LONG_REAL) : MODE (LONGLONG_REAL));
    } else if (attr == BITS_DENOTER) {
      m = (length == 1 ? MODE (LONG_BITS) : MODE (LONGLONG_BITS));
    } else {
      m = NULL;
    }
    if (m != NULL) {
      int digits = get_mp_digits (m);
      MP_DIGIT_T *z;
      STACK_MP (z, p, digits);
      if (genie_string_to_value_internal (p, m, symbol, (BYTE_T *) z) == A68_FALSE) {
        diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTER, m);
        exit_genie (p, A68_RUNTIME_ERROR);
      }
      z[0] = INITIALISED_MASK | CONSTANT_MASK;
      push_mode (f, m);
      SCAN_CHECK (f, p);
    } else {
      monitor_error ("invalid mode", NULL);
    }
  } else if (attr == INT_DENOTER) {
    A68_INT z;
    if (genie_string_to_value_internal (p, MODE (INT), symbol, (BYTE_T *) & z) == A68_FALSE) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTER, MODE (INT));
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    PUSH_PRIMITIVE (p, VALUE (&z), A68_INT);
    push_mode (f, MODE (INT));
    SCAN_CHECK (f, p);
  } else if (attr == REAL_DENOTER) {
    A68_REAL z;
    if (genie_string_to_value_internal (p, MODE (REAL), symbol, (BYTE_T *) & z) == A68_FALSE) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTER, MODE (REAL));
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    PUSH_PRIMITIVE (p, VALUE (&z), A68_REAL);
    push_mode (f, MODE (REAL));
    SCAN_CHECK (f, p);
  } else if (attr == BITS_DENOTER) {
    A68_BITS z;
    if (genie_string_to_value_internal (p, MODE (BITS), symbol, (BYTE_T *) & z) == A68_FALSE) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_IN_DENOTER, MODE (BITS));
      exit_genie (p, A68_RUNTIME_ERROR);
    }
    PUSH_PRIMITIVE (p, VALUE (&z), A68_BITS);
    push_mode (f, MODE (BITS));
    SCAN_CHECK (f, p);
  } else if (attr == ROW_CHAR_DENOTER) {
    if (strlen (symbol) == 1) {
      PUSH_PRIMITIVE (p, symbol[0], A68_CHAR);
      push_mode (f, MODE (CHAR));
    } else {
      A68_REF z;
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      z = c_to_a_string (p, symbol);
      GET_DESCRIPTOR (arr, tup, &z);
      PROTECT_SWEEP_HANDLE (&z);
      PROTECT_SWEEP_HANDLE (&(arr->array));
      PUSH_REF (p, z);
      push_mode (f, MODE (STRING));
    }
    SCAN_CHECK (f, p);
  } else if (attr == TRUE_SYMBOL) {
    PUSH_PRIMITIVE (p, A68_TRUE, A68_BOOL);
    push_mode (f, MODE (BOOL));
    SCAN_CHECK (f, p);
  } else if (attr == FALSE_SYMBOL) {
    PUSH_PRIMITIVE (p, A68_FALSE, A68_BOOL);
    push_mode (f, MODE (BOOL));
    SCAN_CHECK (f, p);
  } else if (attr == NIL_SYMBOL) {
    PUSH_REF (p, nil_ref);
    push_mode (f, MODE (HIP));
    SCAN_CHECK (f, p);
  } else if (attr == REAL_SYMBOL) {
    A68_INT k;
    SCAN_CHECK (f, p);
    if (attr != OPEN_SYMBOL) {
      monitor_error ("cast expects open-symbol", NULL);
    }
    SCAN_CHECK (f, p);
    PARSE_CHECK (f, p, 0);
    if (attr != CLOSE_SYMBOL) {
      monitor_error ("cast expects close-symbol", NULL);
    }
    SCAN_CHECK (f, p);
    if (TOP_MODE != MODE (INT)) {
      monitor_error ("cast argument mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
    }
    QUIT_ON_ERROR;
    POP_OBJECT (p, &k, A68_INT);
    PUSH_PRIMITIVE (p, k.value, A68_REAL);
    TOP_MODE = MODE (REAL);
  } else if (attr == IDENTIFIER) {
    ADDR_T old_sp = stack_pointer;
    MOID_T *moid;
    char *name = symbol;
    SCAN_CHECK (f, p);
    if (attr == OF_SYMBOL) {
      selection (f, p, name);
    } else {
      search_identifier (f, p, frame_pointer, name);
      QUIT_ON_ERROR;
      call_or_slice (f, p, depth);
    }
    moid = TOP_MODE;
    QUIT_ON_ERROR;
    if (check_initialisation (p, STACK_ADDRESS (old_sp), moid, NULL) == A68_FALSE) {
      monitor_error ("cannot process value of mode", moid_to_string (moid, MOID_WIDTH));
    }
  } else if (attr == OPEN_SYMBOL) {
    do {
      SCAN_CHECK (f, p);
      PARSE_CHECK (f, p, 0);
    } while (attr == COMMA_SYMBOL);
    if (attr != CLOSE_SYMBOL) {
      monitor_error ("unmatched parenthesis", NULL);
    }
    SCAN_CHECK (f, p);
    call_or_slice (f, p, depth);
  } else {
    monitor_error ("expression syntax error", NULL);
  }
}

/*!
\brief perform assignment
*/

static void assign (FILE_T f, NODE_T * p)
{
  PARSE_CHECK (f, p, 0);
  if (attr == ASSIGN_SYMBOL) {
    MOID_T *m = m_stack[--m_sp];
    A68_REF z;
    if (WHETHER_NOT (m, REF_SYMBOL)) {
      monitor_error ("destination mode error", moid_to_string (m, MOID_WIDTH));
    }
    QUIT_ON_ERROR;
    POP_REF (p, &z);
    CHECK_NIL (p, z, m);
    SCAN_CHECK (f, p);
    assign (f, p);
    QUIT_ON_ERROR;
    while (WHETHER (TOP_MODE, REF_SYMBOL) && TOP_MODE != SUB (m)) {
      MOID_T *sub = SUB (TOP_MODE);
      A68_REF y;
      POP_REF (p, &y);
      CHECK_NIL (p, y, TOP_MODE);
      PUSH (p, ADDRESS (&y), MOID_SIZE (sub));
      TOP_MODE = sub;
    }
    if (TOP_MODE != SUB (m) && TOP_MODE != MODE (HIP)) {
      monitor_error ("source mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
    }
    QUIT_ON_ERROR;
    POP (p, ADDRESS (&z), MOID_SIZE (TOP_MODE));
    PUSH_REF (p, z);
    TOP_MODE = m;
  }
}

/*!
\brief evaluate expression on input
*/

static void evaluate (FILE_T f, NODE_T * p, char *str)
{
  m_sp = 0;
  m_stack[0] = NULL;
  pos = 0;
  bufcpy (expr, str, BUFFER_SIZE);
  SCAN_CHECK (f, p);
  QUIT_ON_ERROR;
  assign (f, p);
  if (attr != 0) {
    monitor_error ("trailing character in expression", symbol);
  }
}

/*!
\brief convert string to int
\param num
\return int value or -1 on error
*/

static int argval (char *num, char **rest)
{
  char *end;
  int k;
  if (rest != NULL) {
    *rest = NULL;
  }
  if (num == NULL) {
    return (0);
  }
  while (!IS_SPACE (num[0]) && num[0] != NULL_CHAR) {
    num++;
  }
  while (IS_SPACE (num[0]) && num[0] != NULL_CHAR) {
    num++;
  }
  if (num[0] != NULL_CHAR) {
    RESET_ERRNO;
    k = a68g_strtoul (num, &end, 10);
    if (end != num && errno == 0) {
      if (rest != NULL) {
        *rest = end;
      }
      return (k);
    } else {
      monitor_error (strerror (errno), NULL);
      return (-1);
    }
  } else {
    return (0);
  }
}

/*!
\brief whether item at "w" of mode "q" is initialised
\param p
\param w
\param q
\param result
\return TRUE or FALSE
**/

static BOOL_T check_initialisation (NODE_T * p, BYTE_T * w, MOID_T * q, BOOL_T * result)
{
  BOOL_T initialised = A68_FALSE, recognised = A68_FALSE;
  switch (q->short_id) {
  case MODE_NO_CHECK:
  case UNION_SYMBOL:
    {
      initialised = A68_TRUE;
      recognised = A68_TRUE;
      break;
    }
  case REF_SYMBOL:
    {
      A68_REF *z = (A68_REF *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case PROC_SYMBOL:
    {
      A68_PROCEDURE *z = (A68_PROCEDURE *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_INT:
    {
      A68_INT *z = (A68_INT *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_REAL:
    {
      A68_REAL *z = (A68_REAL *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_COMPLEX:
    {
      A68_REAL *r = (A68_REAL *) w;
      A68_REAL *i = (A68_REAL *) (w + ALIGNED_SIZEOF (A68_REAL));
      initialised = (INITIALISED (r) && INITIALISED (i));
      recognised = A68_TRUE;
      break;
    }
  case MODE_LONG_INT:
  case MODE_LONG_REAL:
  case MODE_LONG_BITS:
    {
      MP_DIGIT_T *z = (MP_DIGIT_T *) w;
      initialised = (int) z[0] & INITIALISED_MASK;
      recognised = A68_TRUE;
      break;
    }
  case MODE_LONGLONG_INT:
  case MODE_LONGLONG_REAL:
  case MODE_LONGLONG_BITS:
    {
      MP_DIGIT_T *z = (MP_DIGIT_T *) w;
      initialised = (int) z[0] & INITIALISED_MASK;
      recognised = A68_TRUE;
      break;
    }
  case MODE_LONG_COMPLEX:
    {
      MP_DIGIT_T *r = (MP_DIGIT_T *) w;
      MP_DIGIT_T *i = (MP_DIGIT_T *) (w + size_long_mp ());
      initialised = ((int) r[0] & INITIALISED_MASK) && ((int) i[0] & INITIALISED_MASK);
      recognised = A68_TRUE;
      break;
    }
  case MODE_LONGLONG_COMPLEX:
    {
      MP_DIGIT_T *r = (MP_DIGIT_T *) w;
      MP_DIGIT_T *i = (MP_DIGIT_T *) (w + size_long_mp ());
      initialised = ((int) r[0] & INITIALISED_MASK) && ((int) i[0] & INITIALISED_MASK);
      recognised = A68_TRUE;
      break;
    }
  case MODE_BOOL:
    {
      A68_BOOL *z = (A68_BOOL *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_CHAR:
    {
      A68_CHAR *z = (A68_CHAR *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_BITS:
    {
      A68_BITS *z = (A68_BITS *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_BYTES:
    {
      A68_BYTES *z = (A68_BYTES *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_LONG_BYTES:
    {
      A68_LONG_BYTES *z = (A68_LONG_BYTES *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_FILE:
    {
      A68_FILE *z = (A68_FILE *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_FORMAT:
    {
      A68_FORMAT *z = (A68_FORMAT *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
      break;
    }
  case MODE_PIPE:
    {
      A68_REF *read = (A68_REF *) w;
      A68_REF *write = (A68_REF *) (w + ALIGNED_SIZEOF (A68_REF));
      A68_INT *pid = (A68_INT *) (w + 2 * ALIGNED_SIZEOF (A68_REF));
      initialised = (INITIALISED (read) && INITIALISED (write) && INITIALISED (pid));
      recognised = A68_TRUE;
      break;
    }
  case MODE_SOUND:
    {
      A68_SOUND *z = (A68_SOUND *) w;
      initialised = INITIALISED (z);
      recognised = A68_TRUE;
    }
  }
  if (result == NULL) {
    if (recognised == A68_TRUE && initialised == A68_FALSE) {
      diagnostic_node (A68_RUNTIME_ERROR, p, ERROR_EMPTY_VALUE_FROM, q);
      exit_genie (p, A68_RUNTIME_ERROR);
    }
  } else {
    *result = initialised;
  }
  return (recognised);
}

/*!
\brief show value of object
\param p
\param f
\param item
\param mode
**/

void print_item (NODE_T * p, FILE_T f, BYTE_T * item, MOID_T * mode)
{
  A68_REF nil_file = nil_ref;
  reset_transput_buffer (UNFORMATTED_BUFFER);
  genie_write_standard (p, mode, item, nil_file);
  if (get_transput_buffer_index (UNFORMATTED_BUFFER) > 0) {
    if (mode == MODE (CHAR) || mode == MODE (ROW_CHAR) || mode == MODE (STRING)) {
      snprintf (output_line, BUFFER_SIZE, " \"%s\"", get_transput_buffer (UNFORMATTED_BUFFER));
      WRITE (f, output_line);
    } else {
      char *str = get_transput_buffer (UNFORMATTED_BUFFER);
      while (IS_SPACE (str[0])) {
        str++;
      }
      snprintf (output_line, BUFFER_SIZE, " %s", str);
      WRITE (f, output_line);
    }
  } else {
    WRITE (f, CANNOT_SHOW);
  }
}

/*!
\brief indented indent_crlf
\param f
**/

static void indent_crlf (FILE_T f)
{
  int k;
  io_close_tty_line ();
  for (k = 0; k < tabs; k++) {
    WRITE (f, "     ");
  }
}

/*!
\brief show value of object
\param p
\param f
\param item
\param mode
**/

static void show_item (FILE_T f, NODE_T * p, BYTE_T * item, MOID_T * mode)
{
  if (WHETHER (mode, REF_SYMBOL)) {
    A68_REF *z = (A68_REF *) item;
    if (IS_NIL (*z)) {
      if (INITIALISED (z)) {
        WRITE (STDOUT_FILENO, " = NIL");
      } else {
        WRITE (STDOUT_FILENO, NO_VALUE);
      }
    } else {
      if (INITIALISED (z)) {
        ADDR_T addr = z->offset;
        WRITE (STDOUT_FILENO, " refers to");
        if (IS_IN_HEAP (z)) {
          addr += REF_HANDLE (z)->offset;
          WRITE (STDOUT_FILENO, " heap");
        } else if (IS_IN_FRAME (z)) {
          WRITE (STDOUT_FILENO, " frame");
        } else if (IS_IN_STACK (z)) {
          WRITE (STDOUT_FILENO, " stack");
        } else if (IS_IN_HANDLE (z)) {
          WRITE (STDOUT_FILENO, " handle");
        }
        snprintf (output_line, BUFFER_SIZE, "(%d)", addr);
        WRITE (STDOUT_FILENO, output_line);
      } else {
        WRITE (STDOUT_FILENO, NO_VALUE);
      }
    }
  } else if (mode == MODE (STRING)) {
    if (!INITIALISED ((A68_REF *) item)) {
      WRITE (STDOUT_FILENO, NO_VALUE);
    } else {
      print_item (p, f, item, mode);
    }
  } else if ((WHETHER (mode, ROW_SYMBOL) || WHETHER (mode, FLEX_SYMBOL)) && mode != MODE (STRING)) {
    MOID_T *deflexed = DEFLEX (mode);
    int old_tabs = tabs;
    tabs += 2;
    if (!INITIALISED ((A68_REF *) item)) {
      WRITE (STDOUT_FILENO, NO_VALUE);
    } else {
      A68_ARRAY *arr;
      A68_TUPLE *tup;
      int count = 0, act_count = 0, elems;
      GET_DESCRIPTOR (arr, tup, (A68_REF *) item);
      elems = get_row_size (tup, arr->dimensions);
      snprintf (output_line, BUFFER_SIZE, ", %d element(s)", elems);
      WRITE (f, output_line);
      if (get_row_size (tup, arr->dimensions) != 0) {
        BYTE_T *base_addr = ADDRESS (&arr->array);
        BOOL_T done = A68_FALSE;
        initialise_internal_index (tup, arr->dimensions);
        while (!done && ++count <= (max_row_elems + 1)) {
          if (count <= max_row_elems) {
            ADDR_T index = calculate_internal_index (tup, arr->dimensions);
            ADDR_T elem_addr = ROW_ELEMENT (arr, index);
            BYTE_T *elem = &base_addr[elem_addr];
            indent_crlf (f);
            WRITE (f, "[");
            print_internal_index (f, tup, arr->dimensions);
            WRITE (f, "]");
            show_item (f, p, elem, SUB (deflexed));
            act_count++;
            done = increment_internal_index (tup, arr->dimensions);
          }
        }
        indent_crlf (f);
        snprintf (output_line, BUFFER_SIZE, " %d element(s) written (%d%%)", act_count, (int) ((100.0 * act_count) / elems));
        WRITE (f, output_line);
      }
    }
    tabs = old_tabs;
  } else if (WHETHER (mode, STRUCT_SYMBOL)) {
    PACK_T *q = PACK (mode);
    tabs++;
    for (; q != NULL; q = NEXT (q)) {
      BYTE_T *elem = &item[q->offset];
      indent_crlf (f);
      snprintf (output_line, BUFFER_SIZE, "     %s \"%s\"", moid_to_string (MOID (q), MOID_WIDTH), TEXT (q));
      WRITE (STDOUT_FILENO, output_line);
      show_item (f, p, elem, MOID (q));
    }
    tabs--;
  } else if (WHETHER (mode, UNION_SYMBOL)) {
    A68_UNION *z = (A68_UNION *) item;
    snprintf (output_line, BUFFER_SIZE, " united-moid %s", moid_to_string ((MOID_T *) (z->value), MOID_WIDTH));
    WRITE (STDOUT_FILENO, output_line);
    show_item (f, p, &item[ALIGNED_SIZEOF (A68_UNION)], (MOID_T *) (z->value));
  } else if (mode == MODE (SIMPLIN)) {
    A68_UNION *z = (A68_UNION *) item;
    snprintf (output_line, BUFFER_SIZE, " united-moid %s", moid_to_string ((MOID_T *) (z->value), MOID_WIDTH));
    WRITE (STDOUT_FILENO, output_line);
  } else if (mode == MODE (SIMPLOUT)) {
    A68_UNION *z = (A68_UNION *) item;
    snprintf (output_line, BUFFER_SIZE, " united-moid %s", moid_to_string ((MOID_T *) (z->value), MOID_WIDTH));
    WRITE (STDOUT_FILENO, output_line);
  } else {
    BOOL_T init;
    if (check_initialisation (p, item, mode, &init)) {
      if (init) {
        if (WHETHER (mode, PROC_SYMBOL)) {
          A68_PROCEDURE *z = (A68_PROCEDURE *) item;
          if (z != NULL && z->status & STANDENV_PROC_MASK) {
            char *fname = standard_environ_proc_name (*(GENIE_PROCEDURE *) & z->body);
            WRITE (STDOUT_FILENO, " standenv procedure");
            if (fname != NULL) {
              WRITE (STDOUT_FILENO, " (");
              WRITE (STDOUT_FILENO, fname);
              WRITE (STDOUT_FILENO, ")");
            }
          } else if (z != NULL && z->status & SKIP_PROCEDURE_MASK) {
            WRITE (STDOUT_FILENO, " skip procedure");
          } else if (z != NULL && z->body != NULL) {
            snprintf (output_line, BUFFER_SIZE, " line %d, environ at frame(%d)", LINE ((NODE_T *) z->body)->number, z->environ);
            WRITE (STDOUT_FILENO, output_line);
          } else {
            WRITE (STDOUT_FILENO, " cannot show value");
          }
        } else if (mode == MODE (FORMAT)) {
          A68_FORMAT *z = (A68_FORMAT *) item;
          if (z != NULL && z->body != NULL) {
            snprintf (output_line, BUFFER_SIZE, " line %d, environ at frame(%d)", LINE (z->body)->number, z->environ);
            WRITE (STDOUT_FILENO, output_line);
          } else {
            WRITE (STDOUT_FILENO, CANNOT_SHOW);
          }
        } else if (mode == MODE (SOUND)) {
          A68_SOUND *z = (A68_SOUND *) item;
          if (z != NULL) {
            snprintf (output_line, BUFFER_SIZE, "% d channels, %d bits, %d rate, %d samples", z->num_channels, z->bits_per_sample, z->sample_rate, z->num_samples);
            WRITE (STDOUT_FILENO, output_line);

          } else {
            WRITE (STDOUT_FILENO, CANNOT_SHOW);
          }
        } else {
          print_item (p, f, item, mode);
        }
      } else {
        WRITE (STDOUT_FILENO, NO_VALUE);
      }
    } else {
      snprintf (output_line, BUFFER_SIZE, " mode %s, %s", moid_to_string (mode, MOID_WIDTH), CANNOT_SHOW);
      WRITE (STDOUT_FILENO, output_line);
    }
  }
}

/*!
\brief overview of frame item
\param f
\param p
\param link
\param q
\param modif
**/

static void show_frame_item (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * q, int modif)
{
  ADDR_T addr = link + FRAME_INFO_SIZE + q->offset;
  ADDR_T loc = FRAME_INFO_SIZE + q->offset;
  (void) p;
  indent_crlf (STDOUT_FILENO);
  if (modif != ANONYMOUS) {
    snprintf (output_line, BUFFER_SIZE, "     frame(%d=%d+%d) %s \"%s\"", addr, link, loc, moid_to_string (MOID (q), MOID_WIDTH), SYMBOL (NODE (q)));
    WRITE (STDOUT_FILENO, output_line);
    show_item (f, p, FRAME_ADDRESS (addr), MOID (q));
  } else {
    switch (PRIO (q)) {
    case GENERATOR:
      {
        snprintf (output_line, BUFFER_SIZE, "     frame(%d=%d+%d) LOC %s", addr, link, loc, moid_to_string (MOID (q), MOID_WIDTH));
        WRITE (STDOUT_FILENO, output_line);
        break;
      }
    default:
      {
        snprintf (output_line, BUFFER_SIZE, "     frame(%d=%d+%d) internal %s", addr, link, loc, moid_to_string (MOID (q), MOID_WIDTH));
        WRITE (STDOUT_FILENO, output_line);
        break;
      }
    }
    show_item (f, p, FRAME_ADDRESS (addr), MOID (q));
  }
}

/*!
\brief overview of frame items
\param f
\param p
\param link
\param q
\param modif
**/

static void show_frame_items (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * q, int modif)
{
  (void) p;
  for (; q != NULL; q = NEXT (q)) {
    show_frame_item (f, p, link, q, modif);
  }
}

/*!
\brief introduce stack frame
\param f
\param p
\param link
\param printed
**/

static void intro_frame (FILE_T f, NODE_T * p, ADDR_T link, int *printed)
{
  SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
  if (*printed > 0) {
    WRITELN (f, "++++");
  }
  (*printed)++;
  where (f, p);
  snprintf (output_line, BUFFER_SIZE, "++++ Stack frame %d at frame(%d), level=%d, size=%d bytes", FRAME_NUMBER (link), link, q->level, FRAME_INCREMENT (link) + FRAME_INFO_SIZE);
  WRITELN (f, output_line);
}

/*!
\brief view contents of stack frame
\param f
\param p
\param link
\param printed
**/

void show_stack_frame (FILE_T f, NODE_T * p, ADDR_T link, int *printed)
{
/* show the frame starting at frame pointer 'link', using symbol table from p as a map. */
  if (p != NULL) {
    SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
    intro_frame (f, p, link, printed);
    snprintf (output_line, BUFFER_SIZE, "++++ Dynamic link=frame(%d), static link=frame(%d)", FRAME_DYNAMIC_LINK (link), FRAME_STATIC_LINK (link));
    WRITELN (STDOUT_FILENO, output_line);
    snprintf (output_line, BUFFER_SIZE, "++++ Procedure frame=%s", (FRAME_PROC_FRAME (link) ? "yes" : "no"));
    WRITELN (STDOUT_FILENO, output_line);
#if defined ENABLE_PAR_CLAUSE
    snprintf (output_line, BUFFER_SIZE, "++++ Thread id=%d", (unsigned) FRAME_THREAD_ID (link));
    WRITELN (STDOUT_FILENO, output_line);
#endif
    show_frame_items (f, p, link, q->identifiers, IDENTIFIER);
    show_frame_items (f, p, link, q->operators, OPERATOR);
    show_frame_items (f, p, link, q->anonymous, ANONYMOUS);
  }
}

/*!
\brief shows line where 'p' is at and draws a '-' beneath the position
\param f
\param p
**/

void where (FILE_T f, NODE_T * p)
{
  write_source_line (f, LINE (p), p, A68_NO_DIAGNOSTICS);
}

/*!
\brief shows lines around the line where 'p' is at
\param f
\param p
\param depth
**/

static void list (FILE_T f, NODE_T * p, int n, int m)
{
  if (p != NULL && INFO (p)->module != NULL) {
    if (m == 0) {
      SOURCE_LINE_T *r = INFO (p)->line;
      SOURCE_LINE_T *l = INFO (p)->module->top_line;
      for (; l != NULL; l = NEXT (l)) {
        if (l->number > 0 && abs (r->number - l->number) <= n) {
          write_source_line (f, l, NULL, A68_TRUE);
        }
      }
    } else {
      SOURCE_LINE_T *l = INFO (p)->module->top_line;
      for (; l != NULL; l = NEXT (l)) {
        if (l->number > 0 && l->number >= n && l->number <= m) {
          write_source_line (f, l, NULL, A68_TRUE);
        }
      }
    }
  }
}

/*!
\brief overview of the heap
\param f
\param p
\param z
**/

void show_heap (FILE_T f, NODE_T * p, A68_HANDLE * z, int top, int n)
{
  int k = 0, m = n;
  (void) p;
  snprintf (output_line, BUFFER_SIZE, "     size=%d available=%d garbage collections=%d", heap_size, heap_available (), garbage_collects);
  WRITELN (f, output_line);
  for (; z != NULL; z = NEXT (z), k++) {
    if (z->offset <= top && n > 0) {
      n--;
      indent_crlf (f);
      snprintf (output_line, BUFFER_SIZE, "     heap(%d-%d) %s", z->offset, z->offset + z->size, moid_to_string (MOID (z), MOID_WIDTH));
      WRITE (f, output_line);
    }
  }
  snprintf (output_line, BUFFER_SIZE, "printed %d out of %d handles", m, k);
  WRITELN (f, output_line);
}

/*!
\brief search current frame and print it
\param f
\param link
**/

void stack_dump_current (FILE_T f, ADDR_T link)
{
  if (link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    NODE_T *p = FRAME_TREE (link);
    if (p != NULL && SYMBOL_TABLE (p)->level > 3) {
      if (FRAME_NUMBER (link) == current_frame) {
        int printed = 0;
        show_stack_frame (f, p, link, &printed);
      } else {
        stack_dump_current (f, dynamic_link);
      }
    }
  }
}

/*!
\brief overview of the stack
\param f
\param link
\param depth
**/

void stack_dump (FILE_T f, ADDR_T link, int depth, int *printed)
{
  if (depth > 0 && link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    NODE_T *p = FRAME_TREE (link);
    if (p != NULL && SYMBOL_TABLE (p)->level > 3) {
      show_stack_frame (f, p, link, printed);
      stack_dump (f, dynamic_link, depth - 1, printed);
    }
  }
}

/*!
\brief overview of the stack
\param f
\param link
\param depth
**/

void stack_trace (FILE_T f, ADDR_T link, int depth, int *printed)
{
  if (depth > 0 && link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    if (FRAME_PROC_FRAME (link)) {
      NODE_T *p = FRAME_TREE (link);
      show_stack_frame (f, p, link, printed);
      stack_trace (f, dynamic_link, depth - 1, printed);
    } else {
      stack_trace (f, dynamic_link, depth, printed);
    }
  }
}

/*!
\brief examine tags
\param f
\param p
\param link
\param i
\param sym
**/

void examine_tags (FILE_T f, NODE_T * p, ADDR_T link, TAG_T * q, char *sym, int *printed)
{
  for (; q != NULL; q = NEXT (q)) {
    if (NODE (q) != NULL && SYMBOL (NODE (q)) == sym) {
      intro_frame (f, p, link, printed);
      show_frame_item (f, p, link, q, PRIO (q));
    }
  }
}

/*!
\brief search symbol in stack
\param f
\param link
\param sym
**/

void examine_stack (FILE_T f, ADDR_T link, char *sym, int *printed)
{
  if (link > 0) {
    int dynamic_link = FRAME_DYNAMIC_LINK (link);
    NODE_T *p = FRAME_TREE (link);
    if (p != NULL) {
      SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
      examine_tags (f, p, link, q->identifiers, sym, printed);
      examine_tags (f, p, link, q->operators, sym, printed);
    }
    examine_stack (f, dynamic_link, sym, printed);
  }
}

/*!
\brief set or reset breakpoints
\param p
\param set
\param num
**/

static void breakpoints (NODE_T * p, BOOL_T set, int num, char *expr)
{
  for (; p != NULL; p = NEXT (p)) {
    breakpoints (SUB (p), set, num, expr);
    if (set) {
      if (LINE (p)->number == num) {
        MASK (p) |= BREAKPOINT_MASK;
        INFO (p)->expr = expr;
      }
    } else {
      MASK (p) &= ~BREAKPOINT_MASK;
      INFO (p)->expr = NULL;
    }
  }
}

/*!
\brief monitor command overview to tty
\param f
**/

static void genie_help (FILE_T f)
{
  WRITELN (f, "Commands can be abbreviated");
  WRITELN (f, "");
  WRITELN (f, "BREAKPOINT n [expression]");
  WRITELN (f, "   Set breakpoint on units in line \"n\"");
  WRITELN (f, "   For a break to occur, expression must evaluate to TRUE");
  WRITELN (f, "BREAKPOINT");
  WRITELN (f, "   Clear all breakpoints");
  WRITELN (f, "CALLS [n]");
  WRITELN (f, "   Print \"n\" frames in the call stack (default n=3)");
  WRITELN (f, "CONTINUE, RESUME");
  WRITELN (f, "   Continue execution");
  WRITELN (f, "DO command, EXEC command");
  WRITELN (f, "   Pass \"command\" to the shell and print return code");
  WRITELN (f, "ELEMS n");
  WRITELN (f, "   Print first \"n\" elements of rows (default n=24)");
  WRITELN (f, "EVALUATE expression, X expression");
  WRITELN (f, "   Print result of \"expression\"");
  WRITELN (f, "EXAMINE n");
  WRITELN (f, "   Print value of symbols named \"n\" in the call stack");
  WRITELN (f, "EXIT, HX, QUIT");
  WRITELN (f, "   Terminates the program");
  WRITELN (f, "FRAME [n]");
  WRITELN (f, "   Print contents of the current stack frame if n is not specified");
  WRITELN (f, "   Set current stack frame to \"n\" if \"n\" is specified");
  WRITELN (f, "   Set current stack frame to top of frame stack if \"0\" is specified");
  WRITELN (f, "HEAP [n]");
  WRITELN (f, "   Print contents of the heap with address not greater than \"n\"");
  WRITELN (f, "HELP");
  WRITELN (f, "   Print brief help text");
  WRITELN (f, "HT");
  WRITELN (f, "   Halts typing to standard output");
  WRITELN (f, "LIST [n]");
  WRITELN (f, "   Show \"n\" lines around the interrupted line (default n=10)");
  WRITELN (f, "PROMPT s");
  WRITELN (f, "   Set prompt to \"s\"");
  WRITELN (f, "RT");
  WRITELN (f, "   Resumes typing to standard output");
  WRITELN (f, "SIZES");
  WRITELN (f, "   Print size of memory segments");
  WRITELN (f, "STACK [n]");
  WRITELN (f, "   Print \"n\" frames in the stack (default n=3)");
  WRITELN (f, "NEXT, STEP");
  WRITELN (f, "   Resume execution to next interruptable point");
  WRITELN (f, "WHERE");
  WRITELN (f, "   Print the interrupted line");
  WRITELN (f, "XREF n");
  WRITELN (f, "   Give detailed information on source line \"n\"");
}

/*!
\brief execute monitor command
\param p
\param cmd
\return TRUE if execution can be continued, FALSE otherwise
**/

static BOOL_T single_stepper (NODE_T * p, char *cmd)
{
  mon_errors = 0;
  errno = 0;
  if (strlen (cmd) == 0) {
    return (A68_FALSE);
  }
  while (IS_SPACE (cmd[strlen (cmd) - 1])) {
    cmd[strlen (cmd) - 1] = NULL_CHAR;
  }
  if (match_string (cmd, "CAlls", BLANK_CHAR)) {
    int k = argval (cmd, NULL);
    int printed = 0;
    if (k > 0) {
      stack_trace (STDOUT_FILENO, frame_pointer, k, &printed);
    } else if (k == 0) {
      stack_trace (STDOUT_FILENO, frame_pointer, 3, &printed);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "Continue", NULL_CHAR)) {
    return (A68_TRUE);
  } else if (match_string (cmd, "DO", BLANK_CHAR) || match_string (cmd, "EXEC", BLANK_CHAR)) {
    char *sym = cmd;
    while (!IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    while (IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    if (sym[0] != NULL_CHAR) {
      snprintf (output_line, BUFFER_SIZE, "return code %d", system (sym));
      WRITELN (STDOUT_FILENO, output_line);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "ELems", BLANK_CHAR)) {
    int k = argval (cmd, NULL);
    if (k > 0) {
      max_row_elems = k;
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "Evaluate", BLANK_CHAR) || match_string (cmd, "X", BLANK_CHAR)) {
    char *sym = cmd;
    while (!IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    while (IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    if (sym[0] != NULL_CHAR) {
      ADDR_T old_sp = stack_pointer;
      evaluate (STDOUT_FILENO, p, sym);
      if (mon_errors == 0 && m_sp > 0) {
        MOID_T *res;
        BOOL_T cont = A68_TRUE;
        while (cont) {
          res = m_stack[0];
          WRITELN (STDOUT_FILENO, "(");
          WRITE (STDOUT_FILENO, moid_to_string (res, MOID_WIDTH));
          WRITE (STDOUT_FILENO, ")");
          show_item (STDOUT_FILENO, p, STACK_ADDRESS (old_sp), res);
          cont = (WHETHER (res, REF_SYMBOL) && !IS_NIL (*(A68_REF *) STACK_ADDRESS (old_sp)));
          if (cont) {
            A68_REF z;
            POP_REF (p, &z);
            m_stack[0] = SUB (m_stack[0]);
            PUSH (p, ADDRESS (&z), MOID_SIZE (m_stack[0]));
          }
        }
      }
      stack_pointer = old_sp;
      m_sp = 0;
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "EXamine", BLANK_CHAR)) {
    char *sym = cmd;
    while (!IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    while (IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    if (sym[0] != NULL_CHAR && (IS_LOWER (sym[0]) || IS_UPPER (sym[0]))) {
      int printed = 0;
      examine_stack (STDOUT_FILENO, frame_pointer, add_token (&top_token, sym)->text, &printed);
      if (printed == 0) {
        monitor_error ("tag not found", sym);
      }
    } else {
      monitor_error ("tag expected", NULL);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "EXIt", NULL_CHAR) || match_string (cmd, "HX", NULL_CHAR) || match_string (cmd, "Quit", NULL_CHAR) || strcmp (cmd, LOGOUT_STRING) == 0) {
    if (confirm_exit ()) {
      exit_genie (p, A68_RUNTIME_ERROR + A68_FORCE_QUIT);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "Frame", NULL_CHAR)) {
    if (current_frame == 0) {
      int printed = 0;
      stack_dump (STDOUT_FILENO, frame_pointer, 1, &printed);
    } else {
      stack_dump_current (STDOUT_FILENO, frame_pointer);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "Frame", BLANK_CHAR)) {
    int n = argval (cmd, NULL);
    current_frame = (n > 0 ? n : 0);
    stack_dump_current (STDOUT_FILENO, frame_pointer);
    return (A68_FALSE);
  } else if (match_string (cmd, "HEAp", BLANK_CHAR)) {
    int top = argval (cmd, NULL);
    if (top <= 0) {
      top = heap_size;
    }
#if defined ENABLE_TERMINFO
    {
      char *term_type = getenv ("TERM");
      int term_lines;
      if (term_type == NULL) {
        term_lines = 24;
      } else if (tgetent (term_buffer, term_type) < 0) {
        term_lines = 24;
      } else {
        term_lines = tgetnum ("li");
      }
      show_heap (STDOUT_FILENO, p, busy_handles, top, term_lines - 4);
    }
#else
    show_heap (STDOUT_FILENO, p, busy_handles, top, 20);
#endif
    return (A68_FALSE);
  } else if (match_string (cmd, "HELp", NULL_CHAR)) {
    genie_help (STDOUT_FILENO);
    return (A68_FALSE);
  } else if (match_string (cmd, "HT", NULL_CHAR)) {
    halt_typing = A68_TRUE;
    return (A68_TRUE);
  } else if (match_string (cmd, "RT", NULL_CHAR)) {
    halt_typing = A68_FALSE;
    return (A68_TRUE);
  } else if (match_string (cmd, "Breakpoint", BLANK_CHAR)) {
    char *expr = NULL;
    int k = argval (cmd, &expr);
    if (k > 0) {
      while (expr[0] != NULL_CHAR && IS_SPACE (expr[0])) {
        expr++;
      }
      breakpoints (INFO (p)->module->top_node, A68_TRUE, k, new_string (expr));
    } else if (k == 0) {
      breakpoints (INFO (p)->module->top_node, A68_FALSE, 0, NULL);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "List", BLANK_CHAR)) {
    char *where;
    int n = argval (cmd, &where);
    int m = argval (where, NULL);
    if (m == 0) {
      if (n > 0) {
        list (STDOUT_FILENO, p, n, 0);
      } else if (n == 0) {
        list (STDOUT_FILENO, p, 10, 0);
      }
    } else if (n > 0 && m > 0 && n <= m) {
      list (STDOUT_FILENO, p, n, m);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "PROmpt", BLANK_CHAR)) {
    char *sym = cmd;
    while (!IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    while (IS_SPACE (sym[0]) && sym[0] != NULL_CHAR) {
      sym++;
    }
    if (sym[0] != NULL_CHAR) {
      if (sym[0] == QUOTE_CHAR) {
        sym++;
      }
      if (sym[strlen (sym) - 1] == QUOTE_CHAR) {
        sym[strlen (sym) - 1] = NULL_CHAR;
      }
      bufcpy (prompt, sym, BUFFER_SIZE);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "Resume", NULL_CHAR)) {
    return (A68_TRUE);
  } else if (match_string (cmd, "STAck", BLANK_CHAR)) {
    int k = argval (cmd, NULL);
    int printed = 0;
    if (k > 0) {
      stack_dump (STDOUT_FILENO, frame_pointer, k, &printed);
    } else if (k == 0) {
      stack_dump (STDOUT_FILENO, frame_pointer, 3, &printed);
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "STEp", NULL_CHAR)) {
    sys_request_flag = A68_TRUE;
    return (A68_TRUE);
  } else if (match_string (cmd, "Next", NULL_CHAR)) {
    sys_request_flag = A68_TRUE;
    return (A68_TRUE);
  } else if (match_string (cmd, "Where", NULL_CHAR)) {
    where (STDOUT_FILENO, p);
    return (A68_FALSE);
  } else if (strcmp (cmd, "?") == 0) {
    genie_help (STDOUT_FILENO);
    return (A68_FALSE);
  } else if (match_string (cmd, "Sizes", NULL_CHAR)) {
    snprintf (output_line, BUFFER_SIZE, "Frame stack pointer=%d available=%d", frame_pointer, frame_stack_size - frame_pointer);
    WRITELN (STDOUT_FILENO, output_line);
    snprintf (output_line, BUFFER_SIZE, "Expression stack pointer=%d available=%d", stack_pointer, expr_stack_size - stack_pointer);
    WRITELN (STDOUT_FILENO, output_line);
    snprintf (output_line, BUFFER_SIZE, "Heap size=%d available=%d", heap_size, heap_available ());
    WRITELN (STDOUT_FILENO, output_line);
    snprintf (output_line, BUFFER_SIZE, "Garbage collections=%d", garbage_collects);
    WRITELN (STDOUT_FILENO, output_line);
    return (A68_FALSE);
  } else if (match_string (cmd, "XRef", NULL_CHAR)) {
    int k = NUMBER (LINE (p));
    SOURCE_LINE_T *line = a68_prog.top_line;
    for (; line != NULL; FORWARD (line)) {
      if (line->number > 0 && line->number == k) {
        list_source_line (STDOUT_FILENO, &a68_prog, line, A68_TRUE);
      }
    }
    return (A68_FALSE);
  } else if (match_string (cmd, "XRef", BLANK_CHAR)) {
    int k = argval (cmd, NULL);
    SOURCE_LINE_T *line = a68_prog.top_line;
    for (; line != NULL; FORWARD (line)) {
      if (line->number > 0 && line->number == k) {
        list_source_line (STDOUT_FILENO, &a68_prog, line, A68_TRUE);
      }
    }
    return (A68_FALSE);
  } else if (strlen (cmd) == 0) {
    return (A68_FALSE);
  } else {
    monitor_error ("unrecognised command", NULL);
    return (A68_FALSE);
  }
}

/*!
\brief evaluate conditional breakpoint expression
\param p
\return whether expression evaluates to TRUE
**/

BOOL_T breakpoint_expression (NODE_T * p)
{
  ADDR_T top_sp = stack_pointer;
  volatile BOOL_T res = A68_FALSE;
  mon_errors = 0;
  if (INFO (p)->expr != NULL) {
    evaluate (STDOUT_FILENO, p, INFO (p)->expr);
    if (m_sp != 1) {
      monitor_error ("invalid breakpoint expression", NULL);
    }
    if (TOP_MODE == MODE (BOOL)) {
      A68_BOOL z;
      POP_OBJECT (p, &z, A68_BOOL);
      res = (STATUS (&z) == INITIALISED_MASK && VALUE (&z) == A68_TRUE);
    } else {
      monitor_error ("breakpoint expression mode error", moid_to_string (TOP_MODE, MOID_WIDTH));
    }
  }
  stack_pointer = top_sp;
  return (res);
}

/*!
\brief execute monitor
\param p
**/

void single_step (NODE_T * p, BOOL_T sigint, BOOL_T breakpoint)
{
  volatile BOOL_T do_cmd = A68_TRUE;
  ADDR_T top_sp = stack_pointer;
#if defined ENABLE_CURSES
  genie_curses_end (NULL);
#endif
  in_monitor = A68_TRUE;
  sys_request_flag = A68_FALSE;
  UP_SWEEP_SEMA;
  if (sigint) {
    WRITE (STDOUT_FILENO, NEWLINE_STRING);
    where (STDOUT_FILENO, p);
    if (confirm_exit ()) {
      exit_genie (p, A68_RUNTIME_ERROR + A68_FORCE_QUIT);
    }
  }
  if (breakpoint) {
    if (INFO (p)->expr != NULL) {
      snprintf (output_line, BUFFER_SIZE, "\n++++ Breakpoint (%s)", INFO (p)->expr);
    } else {
      snprintf (output_line, BUFFER_SIZE, "\n++++ Breakpoint");
    }
    where (STDOUT_FILENO, p);
  }
  while (do_cmd) {
    char *cmd;
    stack_pointer = top_sp;
    io_close_tty_line ();
    while (strlen (cmd = read_string_from_tty (prompt)) == 0) {;
    }
    if (TO_UCHAR (cmd[0]) == TO_UCHAR (EOF_CHAR)) {
      bufcpy (cmd, LOGOUT_STRING, BUFFER_SIZE);
      WRITE (STDOUT_FILENO, LOGOUT_STRING);
      WRITE (STDOUT_FILENO, NEWLINE_STRING);
    }
    m_sp = 0;
    do_cmd = !single_stepper (p, cmd);
  }
  stack_pointer = top_sp;
  in_monitor = A68_FALSE;
  DOWN_SWEEP_SEMA;
}

/*!
\brief PROC debug = VOID
\param p
**/

void genie_debug (NODE_T * p)
{
  single_step (p, A68_FALSE, A68_FALSE);
}

/*!
\brief PROC break = VOID
\param p
**/

void genie_break (NODE_T * p)
{
  (void) p;
  sys_request_flag = A68_TRUE;
}

/*!
\brief PROC evaluate = (STRING) STRING
\param p
*/

void genie_evaluate (NODE_T * p)
{
  A68_REF z;
  volatile ADDR_T top_sp;
/* Pop argument. */
  POP_REF (p, (A68_REF *) & z);
  top_sp = stack_pointer;
  CHECK_INIT (p, INITIALISED (&z), MODE (STRING));
  CHECK_NIL (p, z, MODE (STRING));
  reset_transput_buffer (UNFORMATTED_BUFFER);
  add_a_string_transput_buffer (p, UNFORMATTED_BUFFER, (BYTE_T *) & z);
/* Evaluate in the monitor. */
  in_monitor = A68_TRUE;
  mon_errors = 0;
  evaluate (STDOUT_FILENO, p, get_transput_buffer (UNFORMATTED_BUFFER));
  in_monitor = A68_FALSE;
  if (m_sp != 1) {
    monitor_error ("invalid expression", NULL);
  }
  z = empty_string (p);
  if (mon_errors == 0) {
    MOID_T *res;
    BOOL_T cont = A68_TRUE;
    while (cont) {
      res = TOP_MODE;
      cont = (WHETHER (res, REF_SYMBOL) && !IS_NIL (*(A68_REF *) STACK_ADDRESS (top_sp)));
      if (cont) {
        A68_REF z;
        POP_REF (p, &z);
        TOP_MODE = SUB (TOP_MODE);
        PUSH (p, ADDRESS (&z), MOID_SIZE (TOP_MODE));
      }
    }
    reset_transput_buffer (UNFORMATTED_BUFFER);
    genie_write_standard (p, TOP_MODE, STACK_ADDRESS (top_sp), nil_ref);
    z = c_to_a_string (p, get_transput_buffer (UNFORMATTED_BUFFER));
  }
  stack_pointer = top_sp;
  PUSH_REF (p, z);
}
