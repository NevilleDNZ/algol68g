/*!
\file prelude.c
\brief builds symbol table for standard prelude
*/

/*
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2005 J. Marcel van der Veer <algol68g@xs4all.nl>.

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


#include "algol68g.h"
#include "genie.h"
#include "transput.h"
#include "mp.h"
#include "gsl.h"

SYMBOL_TABLE_T *stand_env;

MOID_T *proc_int;
MOID_T *proc_real, *proc_real_real, *proc_real_real_real;
MOID_T *proc_complex_complex;
MOID_T *proc_bool;
MOID_T *proc_char;
MOID_T *proc_void;

#define INSERT_TAG(l, n) {NEXT (n) = *(l); *(l) = (n);}

/*!
\brief enter tag in standenv symbol table
\param a
\param n
\param c
\param m
\param p
\param q
**/

static void add_stand_env (int a, NODE_T * n, char *c, MOID_T * m, int p, GENIE_PROCEDURE * q)
{
  TAG_T *new_one = new_tag ();
  n->info->PROCEDURE_LEVEL = 0;
  n->info->PROCEDURE_NUMBER = 0;
  new_one->use = A_FALSE;
  HEAP (new_one) = HEAP_SYMBOL;
  SYMBOL_TABLE (new_one) = stand_env;
  NODE (new_one) = n;
  new_one->value = c != NULL ? add_token (&top_token, c)->text : NULL;
  PRIO (new_one) = p;
  new_one->procedure = q;
  new_one->stand_env_proc = q != NULL;
  new_one->unit = NULL;
  MOID (new_one) = m;
  NEXT (new_one) = NULL;
  ACCESS (new_one) = PRIVATE_SYMBOL;
  if (a == IDENTIFIER) {
    INSERT_TAG (&stand_env->identifiers, new_one);
  } else if (a == OP_SYMBOL) {
    INSERT_TAG (&stand_env->operators, new_one);
  } else if (a == PRIO_SYMBOL) {
    INSERT_TAG (&PRIO (stand_env), new_one);
  } else if (a == INDICANT) {
    INSERT_TAG (&stand_env->indicants, new_one);
  } else if (a == LABEL) {
    INSERT_TAG (&stand_env->labels, new_one);
  }
}

#undef INSERT_TAG

/*!
\brief compose PROC moid from arguments - first result, than arguments
\param m
\return
**/

static MOID_T *a68_proc (MOID_T * m, ...)
{
  MOID_T *y, **z = &(stand_env->moids);
  PACK_T *p = NULL, *q = NULL;
  va_list attribute;
  va_start (attribute, m);
  while ((y = va_arg (attribute, MOID_T *)) != NULL) {
    PACK_T *new_one = new_pack ();
    MOID (new_one) = y;
    new_one->text = NULL;
    NEXT (new_one) = NULL;
    if (q != NULL) {
      NEXT (q) = new_one;
    } else {
      p = new_one;
    }
    q = new_one;
  }
  va_end (attribute);
  add_mode (z, PROC_SYMBOL, count_pack_members (p), NULL, m, p);
  return (*z);
}

/*!
\brief enter an identifier in standenv
\param n
\param m
\param q
**/

static void a68_idf (char *n, MOID_T * m, GENIE_PROCEDURE * q)
{
  add_stand_env (IDENTIFIER, some_node (TEXT (add_token (&top_token, n))), NULL, m, 0, q);
}

/*!
\brief enter a MOID in standenv
\param p
\param t
\param m
**/

static void a68_mode (int p, char *t, MOID_T ** m)
{
  add_mode (&stand_env->moids, STANDARD, p, some_node (TEXT (find_keyword (top_keyword, t))), NULL, NULL);
  *m = stand_env->moids;
}

/*!
\brief enter a priority in standenv
\param p
\param b
**/

static void a68_prio (char *p, int b)
{
  add_stand_env (PRIO_SYMBOL, some_node (TEXT (add_token (&top_token, p))), NULL, NULL, b, NULL);
}

/*!
\brief enter operator in standenv
\param n
\param m
\param q
**/

static void a68_op (char *n, MOID_T * m, GENIE_PROCEDURE * q)
{
  add_stand_env (OP_SYMBOL, some_node (TEXT (add_token (&top_token, n))), NULL, m, 0, q);
}

/*!
\brief build the standard environ symbol table
**/

void make_standard_environ (void)
{
  MOID_T *m;
  PACK_T *z;
/* Primitive A68 moids. */
  a68_mode (0, "VOID", &MODE (VOID));
/* Standard precision. */
  a68_mode (0, "INT", &MODE (INT));
  a68_mode (0, "REAL", &MODE (REAL));
  a68_mode (0, "COMPLEX", &MODE (COMPLEX));
  a68_mode (0, "COMPL", &MODE (COMPL));
  a68_mode (0, "BITS", &MODE (BITS));
  a68_mode (0, "BYTES", &MODE (BYTES));
/* Multiple precision. */
  a68_mode (1, "INT", &MODE (LONG_INT));
  a68_mode (1, "REAL", &MODE (LONG_REAL));
  a68_mode (1, "COMPLEX", &MODE (LONG_COMPLEX));
  a68_mode (1, "COMPL", &MODE (LONG_COMPL));
  a68_mode (1, "BITS", &MODE (LONG_BITS));
  a68_mode (1, "BYTES", &MODE (LONG_BYTES));
  a68_mode (2, "REAL", &MODE (LONGLONG_REAL));
  a68_mode (2, "INT", &MODE (LONGLONG_INT));
  a68_mode (2, "COMPLEX", &MODE (LONGLONG_COMPLEX));
  a68_mode (2, "COMPL", &MODE (LONGLONG_COMPL));
  a68_mode (2, "BITS", &MODE (LONGLONG_BITS));
/* Other. */
  a68_mode (0, "BOOL", &MODE (BOOL));
  a68_mode (0, "CHAR", &MODE (CHAR));
  a68_mode (0, "STRING", &MODE (STRING));
  a68_mode (0, "FILE", &MODE (FILE));
  a68_mode (0, "CHANNEL", &MODE (CHANNEL));
  a68_mode (0, "PIPE", &MODE (PIPE));
  a68_mode (0, "FORMAT", &MODE (FORMAT));
  a68_mode (0, "SEMA", &MODE (SEMA));
/* ROWS. */
  add_mode (&stand_env->moids, ROWS_SYMBOL, 0, NULL, NULL, NULL);
  MODE (ROWS) = stand_env->moids;
/* REFs. */
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (INT), NULL);
  MODE (REF_INT) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (REAL), NULL);
  MODE (REF_REAL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (COMPLEX), NULL);
  MODE (REF_COMPLEX) = MODE (REF_COMPL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (BITS), NULL);
  MODE (REF_BITS) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (BYTES), NULL);
  MODE (REF_BYTES) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (FORMAT), NULL);
  MODE (REF_FORMAT) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (PIPE), NULL);
  MODE (REF_PIPE) = stand_env->moids;
/* Multiple precision. */
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONG_INT), NULL);
  MODE (REF_LONG_INT) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONG_REAL), NULL);
  MODE (REF_LONG_REAL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONG_COMPLEX), NULL);
  MODE (REF_LONG_COMPLEX) = MODE (REF_LONG_COMPL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONGLONG_INT), NULL);
  MODE (REF_LONGLONG_INT) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONGLONG_REAL), NULL);
  MODE (REF_LONGLONG_REAL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONGLONG_COMPLEX), NULL);
  MODE (REF_LONGLONG_COMPLEX) = MODE (REF_LONGLONG_COMPL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONG_BITS), NULL);
  MODE (REF_LONG_BITS) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONGLONG_BITS), NULL);
  MODE (REF_LONGLONG_BITS) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (LONG_BYTES), NULL);
  MODE (REF_LONG_BYTES) = stand_env->moids;
/* Other. */
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (BOOL), NULL);
  MODE (REF_BOOL) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (CHAR), NULL);
  MODE (REF_CHAR) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (FILE), NULL);
  MODE (REF_FILE) = stand_env->moids;
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (REF_FILE), NULL);
  MODE (REF_REF_FILE) = stand_env->moids;
/* [] REAL and alikes. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (REAL), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_REAL) = stand_env->moids;
  SLICE (MODE (ROW_REAL)) = MODE (REAL);
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (ROW_REAL), NULL);
  MODE (REF_ROW_REAL) = stand_env->moids;
  NAME (MODE (REF_ROW_REAL)) = MODE (REF_REAL);
  add_mode (&stand_env->moids, ROW_SYMBOL, 2, NULL, MODE (REAL), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROWROW_REAL) = stand_env->moids;
  SLICE (MODE (ROWROW_REAL)) = MODE (ROW_REAL);
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (ROWROW_REAL), NULL);
  MODE (REF_ROWROW_REAL) = stand_env->moids;
  NAME (MODE (REF_ROWROW_REAL)) = MODE (REF_ROW_REAL);
/* [] INT. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (INT), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_INT) = stand_env->moids;
  SLICE (MODE (ROW_INT)) = MODE (INT);
/* [] BOOL. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (BOOL), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_BOOL) = stand_env->moids;
  SLICE (MODE (ROW_BOOL)) = MODE (BOOL);
/* [] BITS. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (BITS), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_BITS) = stand_env->moids;
  SLICE (MODE (ROW_BITS)) = MODE (BITS);
/* [] LONG BITS. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (LONG_BITS), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_LONG_BITS) = stand_env->moids;
  SLICE (MODE (ROW_LONG_BITS)) = MODE (LONG_BITS);
/* [] LONG LONG BITS. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (LONGLONG_BITS), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_LONGLONG_BITS) = stand_env->moids;
  SLICE (MODE (ROW_LONGLONG_BITS)) = MODE (LONGLONG_BITS);
/* [] CHAR. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (CHAR), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_CHAR) = stand_env->moids;
  SLICE (MODE (ROW_CHAR)) = MODE (CHAR);
/* [][] CHAR. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (ROW_CHAR), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_ROW_CHAR) = stand_env->moids;
  SLICE (MODE (ROW_ROW_CHAR)) = MODE (ROW_CHAR);
/* MODE STRING = FLEX [] CHAR. */
  add_mode (&stand_env->moids, FLEX_SYMBOL, 0, NULL, MODE (ROW_CHAR), NULL);
  stand_env->moids->has_rows = A_TRUE;
  stand_env->moids->deflexed_mode = MODE (ROW_CHAR);
  stand_env->moids->trim = MODE (ROW_CHAR);
  EQUIVALENT (MODE (STRING)) = stand_env->moids;
  DEFLEXED (MODE (STRING)) = MODE (ROW_CHAR);
/* REF [] CHAR. */
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, MODE (ROW_CHAR), NULL);
  MODE (REF_ROW_CHAR) = stand_env->moids;
  NAME (MODE (REF_ROW_CHAR)) = MODE (REF_CHAR);
/* PROC [] CHAR. */
  add_mode (&stand_env->moids, PROC_SYMBOL, 0, NULL, MODE (ROW_CHAR), NULL);
  MODE (PROC_ROW_CHAR) = stand_env->moids;
/* REF STRING = REF FLEX [] CHAR. */
  add_mode (&stand_env->moids, REF_SYMBOL, 0, NULL, EQUIVALENT (MODE (STRING)), NULL);
  MODE (REF_STRING) = stand_env->moids;
  NAME (MODE (REF_STRING)) = MODE (REF_CHAR);
  DEFLEXED (MODE (REF_STRING)) = MODE (REF_ROW_CHAR);
  TRIM (MODE (REF_STRING)) = MODE (REF_ROW_CHAR);
/* [] STRING. */
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (STRING), NULL);
  stand_env->moids->has_rows = A_TRUE;
  MODE (ROW_STRING) = stand_env->moids;
  SLICE (MODE (ROW_STRING)) = MODE (STRING);
  DEFLEXED (MODE (ROW_STRING)) = MODE (ROW_ROW_CHAR);
/* PROC STRING. */
  add_mode (&stand_env->moids, PROC_SYMBOL, 0, NULL, MODE (STRING), NULL);
  MODE (PROC_STRING) = stand_env->moids;
  DEFLEXED (MODE (PROC_STRING)) = MODE (PROC_ROW_CHAR);
/* COMPLEX. */
  z = NULL;
  add_mode_to_pack (&z, MODE (REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  EQUIVALENT (MODE (COMPLEX)) = EQUIVALENT (MODE (COMPL)) = stand_env->moids;
  MODE (COMPLEX) = MODE (COMPL) = stand_env->moids;
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  NAME (MODE (REF_COMPLEX)) = NAME (MODE (REF_COMPL)) = stand_env->moids;
/* LONG COMPLEX. */
  z = NULL;
  add_mode_to_pack (&z, MODE (LONG_REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (LONG_REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  EQUIVALENT (MODE (LONG_COMPLEX)) = EQUIVALENT (MODE (LONG_COMPL)) = stand_env->moids;
  MODE (LONG_COMPLEX) = MODE (LONG_COMPL) = stand_env->moids;
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_LONG_REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_LONG_REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  NAME (MODE (REF_LONG_COMPLEX)) = NAME (MODE (REF_LONG_COMPL)) = stand_env->moids;
/* LONG_LONG COMPLEX. */
  z = NULL;
  add_mode_to_pack (&z, MODE (LONGLONG_REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (LONGLONG_REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  EQUIVALENT (MODE (LONGLONG_COMPLEX)) = EQUIVALENT (MODE (LONGLONG_COMPL)) = stand_env->moids;
  MODE (LONGLONG_COMPLEX) = MODE (LONGLONG_COMPL) = stand_env->moids;
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_LONGLONG_REAL), add_token (&top_token, "im")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_LONGLONG_REAL), add_token (&top_token, "re")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  NAME (MODE (REF_LONGLONG_COMPLEX)) = NAME (MODE (REF_LONGLONG_COMPL)) = stand_env->moids;
/* NUMBER. */
  z = NULL;
  add_mode_to_pack (&z, MODE (INT), NULL, NULL);
  add_mode_to_pack (&z, MODE (LONG_INT), NULL, NULL);
  add_mode_to_pack (&z, MODE (LONGLONG_INT), NULL, NULL);
  add_mode_to_pack (&z, MODE (REAL), NULL, NULL);
  add_mode_to_pack (&z, MODE (LONG_REAL), NULL, NULL);
  add_mode_to_pack (&z, MODE (LONGLONG_REAL), NULL, NULL);
  add_mode (&stand_env->moids, UNION_SYMBOL, count_pack_members (z), NULL, NULL, z);
  MODE (NUMBER) = stand_env->moids;
/* SEMA. */
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_INT), NULL, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  EQUIVALENT (MODE (SEMA)) = stand_env->moids;
  MODE (SEMA) = stand_env->moids;
/* PROC VOID. */
  z = NULL;
  add_mode (&stand_env->moids, PROC_SYMBOL, count_pack_members (z), NULL, MODE (VOID), z);
  MODE (PROC_VOID) = stand_env->moids;
/* IO: PROC (REF FILE) BOOL. */
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_FILE), NULL, NULL);
  add_mode (&stand_env->moids, PROC_SYMBOL, count_pack_members (z), NULL, MODE (BOOL), z);
  MODE (PROC_REF_FILE_BOOL) = stand_env->moids;
/* IO: PROC (REF FILE) VOID. */
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_FILE), NULL, NULL);
  add_mode (&stand_env->moids, PROC_SYMBOL, count_pack_members (z), NULL, MODE (VOID), z);
  MODE (PROC_REF_FILE_VOID) = stand_env->moids;
/* IO: SIMPLIN and SIMPLOUT. */
  add_mode (&stand_env->moids, IN_TYPE_MODE, 0, NULL, NULL, NULL);
  MODE (SIMPLIN) = stand_env->moids;
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (SIMPLIN), NULL);
  MODE (ROW_SIMPLIN) = stand_env->moids;
  SLICE (MODE (ROW_SIMPLIN)) = MODE (SIMPLIN);
  add_mode (&stand_env->moids, OUT_TYPE_MODE, 0, NULL, NULL, NULL);
  MODE (SIMPLOUT) = stand_env->moids;
  add_mode (&stand_env->moids, ROW_SYMBOL, 1, NULL, MODE (SIMPLOUT), NULL);
  MODE (ROW_SIMPLOUT) = stand_env->moids;
  SLICE (MODE (ROW_SIMPLOUT)) = MODE (SIMPLOUT);
/* PIPE. */
  z = NULL;
  add_mode_to_pack (&z, MODE (INT), add_token (&top_token, "pid")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_FILE), add_token (&top_token, "write")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_FILE), add_token (&top_token, "read")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  EQUIVALENT (MODE (PIPE)) = stand_env->moids;
  MODE (PIPE) = stand_env->moids;
  z = NULL;
  add_mode_to_pack (&z, MODE (REF_INT), add_token (&top_token, "pid")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_REF_FILE), add_token (&top_token, "write")->text, NULL);
  add_mode_to_pack (&z, MODE (REF_REF_FILE), add_token (&top_token, "read")->text, NULL);
  add_mode (&stand_env->moids, STRUCT_SYMBOL, count_pack_members (z), NULL, NULL, z);
  NAME (MODE (REF_PIPE)) = stand_env->moids;
/* Identifiers. */
  a68_idf ("intlengths", MODE (INT), genie_int_lengths);
  a68_idf ("intshorts", MODE (INT), genie_int_shorts);
  a68_idf ("maxint", MODE (INT), genie_max_int);
  a68_idf ("maxreal", MODE (REAL), genie_max_real);
  a68_idf ("smallreal", MODE (REAL), genie_small_real);
  a68_idf ("reallengths", MODE (INT), genie_real_lengths);
  a68_idf ("realshorts", MODE (INT), genie_real_shorts);
  a68_idf ("compllengths", MODE (INT), genie_complex_lengths);
  a68_idf ("complshorts", MODE (INT), genie_complex_shorts);
  a68_idf ("bitslengths", MODE (INT), genie_bits_lengths);
  a68_idf ("bitsshorts", MODE (INT), genie_bits_shorts);
  a68_idf ("bitswidth", MODE (INT), genie_bits_width);
  a68_idf ("longbitswidth", MODE (INT), genie_long_bits_width);
  a68_idf ("longlongbitswidth", MODE (INT), genie_longlong_bits_width);
  a68_idf ("maxbits", MODE (BITS), genie_max_bits);
  a68_idf ("longmaxbits", MODE (LONG_BITS), genie_long_max_bits);
  a68_idf ("longlongmaxbits", MODE (LONGLONG_BITS), genie_longlong_max_bits);
  a68_idf ("byteslengths", MODE (INT), genie_bytes_lengths);
  a68_idf ("bytesshorts", MODE (INT), genie_bytes_shorts);
  a68_idf ("byteswidth", MODE (INT), genie_bytes_width);
  a68_idf ("maxabschar", MODE (INT), genie_max_abs_char);
  a68_idf ("pi", MODE (REAL), genie_pi);
  a68_idf ("dpi", MODE (LONG_REAL), genie_pi_long_mp);
  a68_idf ("longpi", MODE (LONG_REAL), genie_pi_long_mp);
  a68_idf ("qpi", MODE (LONGLONG_REAL), genie_pi_long_mp);
  a68_idf ("longlongpi", MODE (LONGLONG_REAL), genie_pi_long_mp);
  a68_idf ("intwidth", MODE (INT), genie_int_width);
  a68_idf ("realwidth", MODE (INT), genie_real_width);
  a68_idf ("expwidth", MODE (INT), genie_exp_width);
  a68_idf ("longintwidth", MODE (INT), genie_long_int_width);
  a68_idf ("longlongintwidth", MODE (INT), genie_longlong_int_width);
  a68_idf ("longrealwidth", MODE (INT), genie_long_real_width);
  a68_idf ("longlongrealwidth", MODE (INT), genie_longlong_real_width);
  a68_idf ("longexpwidth", MODE (INT), genie_long_exp_width);
  a68_idf ("longlongexpwidth", MODE (INT), genie_longlong_exp_width);
  a68_idf ("longmaxint", MODE (LONG_INT), genie_long_max_int);
  a68_idf ("longlongmaxint", MODE (LONGLONG_INT), genie_longlong_max_int);
  a68_idf ("longsmallreal", MODE (LONG_REAL), genie_long_small_real);
  a68_idf ("longlongsmallreal", MODE (LONGLONG_REAL), genie_longlong_small_real);
  a68_idf ("longmaxreal", MODE (LONG_REAL), genie_long_max_real);
  a68_idf ("longlongmaxreal", MODE (LONGLONG_REAL), genie_longlong_max_real);
  a68_idf ("longbyteswidth", MODE (INT), genie_long_bytes_width);
  a68_idf ("seconds", MODE (REAL), genie_seconds);
  a68_idf ("clock", MODE (REAL), genie_cputime);
  a68_idf ("cputime", MODE (REAL), genie_cputime);
  m = proc_int = a68_proc (MODE (INT), NULL);
  a68_idf ("collections", m, genie_garbage_collections);
  m = a68_proc (MODE (LONG_INT), NULL);
  a68_idf ("garbage", m, genie_garbage_freed);
  m = proc_real = a68_proc (MODE (REAL), NULL);
  a68_idf ("collectseconds", m, genie_garbage_seconds);
  a68_idf ("stackpointer", MODE (INT), genie_stack_pointer);
  a68_idf ("systemstackpointer", MODE (INT), genie_system_stack_pointer);
  a68_idf ("systemstacksize", MODE (INT), genie_system_stack_size);
  a68_idf ("actualstacksize", MODE (INT), genie_stack_pointer);
  m = proc_void = a68_proc (MODE (VOID), NULL);
  a68_idf ("sweepheap", m, genie_sweep_heap);
  a68_idf ("preemptivesweepheap", m, genie_preemptive_sweep_heap);
  a68_idf ("break", m, genie_break);
  m = a68_proc (MODE (INT), MODE (STRING), NULL);
  a68_idf ("system", m, genie_system);
  m = a68_proc (MODE (STRING), MODE (STRING), NULL);
  a68_idf ("vmsacronym", m, genie_idle);
/* BITS procedures. */
  m = a68_proc (MODE (BITS), MODE (ROW_BOOL), NULL);
  a68_idf ("bitspack", m, genie_bits_pack);
  m = a68_proc (MODE (LONG_BITS), MODE (ROW_BOOL), NULL);
  a68_idf ("longbitspack", m, genie_long_bits_pack);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (ROW_BOOL), NULL);
  a68_idf ("longlongbitspack", m, genie_long_bits_pack);
/* IO procedures. */
  a68_idf ("errorchar", MODE (CHAR), genie_error_char);
  a68_idf ("expchar", MODE (CHAR), genie_exp_char);
  a68_idf ("flip", MODE (CHAR), genie_flip_char);
  a68_idf ("flop", MODE (CHAR), genie_flop_char);
  a68_idf ("blankcharacter", MODE (CHAR), genie_blank_char);
  a68_idf ("blankchar", MODE (CHAR), genie_blank_char);
  a68_idf ("blank", MODE (CHAR), genie_blank_char);
  a68_idf ("nullcharacter", MODE (CHAR), genie_null_char);
  a68_idf ("nullchar", MODE (CHAR), genie_null_char);
  a68_idf ("newlinecharacter", MODE (CHAR), genie_newline_char);
  a68_idf ("newlinechar", MODE (CHAR), genie_newline_char);
  a68_idf ("formfeedcharacter", MODE (CHAR), genie_formfeed_char);
  a68_idf ("formfeedchar", MODE (CHAR), genie_formfeed_char);
  a68_idf ("tabcharacter", MODE (CHAR), genie_tab_char);
  a68_idf ("tabchar", MODE (CHAR), genie_tab_char);
  m = a68_proc (MODE (STRING), MODE (NUMBER), MODE (INT), NULL);
  a68_idf ("whole", m, genie_whole);
  m = a68_proc (MODE (STRING), MODE (NUMBER), MODE (INT), MODE (INT), NULL);
  a68_idf ("fixed", m, genie_fixed);
  m = a68_proc (MODE (STRING), MODE (NUMBER), MODE (INT), MODE (INT), MODE (INT), NULL);
  a68_idf ("float", m, genie_float);
  a68_idf ("standin", MODE (REF_FILE), genie_stand_in);
  a68_idf ("standout", MODE (REF_FILE), genie_stand_out);
  a68_idf ("standback", MODE (REF_FILE), genie_stand_back);
  a68_idf ("standerror", MODE (REF_FILE), genie_stand_error);
  a68_idf ("standinchannel", MODE (CHANNEL), genie_stand_in_channel);
  a68_idf ("standoutchannel", MODE (CHANNEL), genie_stand_out_channel);
  a68_idf ("standdrawchannel", MODE (CHANNEL), genie_stand_draw_channel);
  a68_idf ("standbackchannel", MODE (CHANNEL), genie_stand_back_channel);
  a68_idf ("standerrorchannel", MODE (CHANNEL), genie_stand_error_channel);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (STRING), NULL);
  a68_idf ("maketerm", m, genie_make_term);
  m = a68_proc (MODE (BOOL), MODE (CHAR), MODE (REF_INT), MODE (STRING), NULL);
  a68_idf ("charinstring", m, genie_char_in_string);
  a68_idf ("lastcharinstring", m, genie_last_char_in_string);
  m = a68_proc (MODE (BOOL), MODE (STRING), MODE (REF_INT), MODE (STRING), NULL);
  a68_idf ("stringinstring", m, genie_string_in_string);
  m = a68_proc (MODE (STRING), MODE (REF_FILE), NULL);
  a68_idf ("idf", m, genie_idf);
  a68_idf ("term", m, genie_term);
  m = a68_proc (MODE (STRING), NULL);
  a68_idf ("programidf", m, genie_program_idf);
/* Event routines. */
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (PROC_REF_FILE_BOOL), NULL);
  a68_idf ("onfileend", m, genie_on_file_end);
  a68_idf ("onpageeend", m, genie_on_page_end);
  a68_idf ("onlineend", m, genie_on_line_end);
  a68_idf ("onlogicalfileend", m, genie_on_file_end);
  a68_idf ("onphysicalfileend", m, genie_on_file_end);
  a68_idf ("onformatend", m, genie_on_format_end);
  a68_idf ("onformaterror", m, genie_on_format_error);
  a68_idf ("onvalueerror", m, genie_on_value_error);
  a68_idf ("onopenerror", m, genie_on_open_error);
  a68_idf ("ontransputerror", m, genie_on_transput_error);
/* Enquiries on files. */
  a68_idf ("putpossible", MODE (PROC_REF_FILE_BOOL), genie_put_possible);
  a68_idf ("getpossible", MODE (PROC_REF_FILE_BOOL), genie_get_possible);
  a68_idf ("binpossible", MODE (PROC_REF_FILE_BOOL), genie_bin_possible);
  a68_idf ("setpossible", MODE (PROC_REF_FILE_BOOL), genie_set_possible);
  a68_idf ("resetpossible", MODE (PROC_REF_FILE_BOOL), genie_reset_possible);
  a68_idf ("drawpossible", MODE (PROC_REF_FILE_BOOL), genie_draw_possible);
  a68_idf ("compressible", MODE (PROC_REF_FILE_BOOL), genie_compressible);
/* Handling of files. */
  m = a68_proc (MODE (INT), MODE (REF_FILE), MODE (STRING), MODE (CHANNEL), NULL);
  a68_idf ("open", m, genie_open);
  a68_idf ("establish", m, genie_establish);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (REF_STRING), NULL);
  a68_idf ("associate", m, genie_associate);
  m = a68_proc (MODE (INT), MODE (REF_FILE), MODE (CHANNEL), NULL);
  a68_idf ("create", m, genie_create);
  a68_idf ("close", MODE (PROC_REF_FILE_VOID), genie_close);
  a68_idf ("lock", MODE (PROC_REF_FILE_VOID), genie_lock);
  a68_idf ("scratch", MODE (PROC_REF_FILE_VOID), genie_erase);
  a68_idf ("erase", MODE (PROC_REF_FILE_VOID), genie_erase);
  a68_idf ("reset", MODE (PROC_REF_FILE_VOID), genie_reset);
  a68_idf ("scratch", MODE (PROC_REF_FILE_VOID), genie_erase);
  a68_idf ("newline", MODE (PROC_REF_FILE_VOID), genie_new_line);
  a68_idf ("newpage", MODE (PROC_REF_FILE_VOID), genie_new_page);
  a68_idf ("space", MODE (PROC_REF_FILE_VOID), genie_space);
  m = a68_proc (MODE (VOID), MODE (ROW_SIMPLIN), NULL);
  a68_idf ("read", m, genie_read);
  a68_idf ("readf", m, genie_read_format);
  m = a68_proc (MODE (VOID), MODE (ROW_SIMPLOUT), NULL);
  a68_idf ("print", m, genie_write);
  a68_idf ("write", m, genie_write);
  a68_idf ("printf", m, genie_write_format);
  a68_idf ("writef", m, genie_write_format);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (ROW_SIMPLIN), NULL);
  a68_idf ("get", m, genie_read_file);
  a68_idf ("getf", m, genie_read_file_format);
  a68_idf ("getbin", m, genie_read_bin_file);
  a68_idf ("readbin", m, genie_read_bin_file);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (ROW_SIMPLOUT), NULL);
  a68_idf ("put", m, genie_write_file);
  a68_idf ("putf", m, genie_write_file_format);
  a68_idf ("putbin", m, genie_write_bin_file);
  a68_idf ("printbin", m, genie_write_bin_file);
  a68_idf ("writebin", m, genie_write_bin_file);
/* ALGOL68C type procs. */
  m = proc_int;
  a68_idf ("readint", m, genie_read_int);
  m = a68_proc (MODE (VOID), MODE (INT), NULL);
  a68_idf ("printint", m, genie_print_int);
  m = a68_proc (MODE (LONG_INT), NULL);
  a68_idf ("readlongint", m, genie_read_long_int);
  m = a68_proc (MODE (VOID), MODE (LONG_INT), NULL);
  a68_idf ("printlongint", m, genie_print_long_int);
  m = a68_proc (MODE (LONGLONG_INT), NULL);
  a68_idf ("readlonglongint", m, genie_read_longlong_int);
  m = a68_proc (MODE (VOID), MODE (LONGLONG_INT), NULL);
  a68_idf ("printlonglongint", m, genie_print_longlong_int);
  m = proc_real;
  a68_idf ("readreal", m, genie_read_real);
  m = a68_proc (MODE (VOID), MODE (REAL), NULL);
  a68_idf ("printreal", m, genie_print_real);
  m = a68_proc (MODE (LONG_REAL), NULL);
  a68_idf ("readlongreal", m, genie_read_long_real);
  a68_idf ("readdouble", m, genie_read_long_real);
  m = a68_proc (MODE (VOID), MODE (LONG_REAL), NULL);
  a68_idf ("printlongreal", m, genie_print_long_real);
  a68_idf ("printdouble", m, genie_print_long_real);
  m = a68_proc (MODE (LONGLONG_REAL), NULL);
  a68_idf ("readlonglongreal", m, genie_read_longlong_real);
  a68_idf ("readquad", m, genie_read_longlong_real);
  m = a68_proc (MODE (VOID), MODE (LONGLONG_REAL), NULL);
  a68_idf ("printlonglongreal", m, genie_print_longlong_real);
  a68_idf ("printquad", m, genie_print_longlong_real);
  m = a68_proc (MODE (COMPLEX), NULL);
  a68_idf ("readcompl", m, genie_read_complex);
  a68_idf ("readcomplex", m, genie_read_complex);
  m = a68_proc (MODE (VOID), MODE (COMPLEX), NULL);
  a68_idf ("printcompl", m, genie_print_complex);
  a68_idf ("printcomplex", m, genie_print_complex);
  m = a68_proc (MODE (LONG_COMPLEX), NULL);
  a68_idf ("readlongcompl", m, genie_read_long_complex);
  a68_idf ("readlongcomplex", m, genie_read_long_complex);
  m = a68_proc (MODE (VOID), MODE (LONG_COMPLEX), NULL);
  a68_idf ("printlongcompl", m, genie_print_long_complex);
  a68_idf ("printlongcomplex", m, genie_print_long_complex);
  m = a68_proc (MODE (LONGLONG_COMPLEX), NULL);
  a68_idf ("readlonglongcompl", m, genie_read_longlong_complex);
  a68_idf ("readlonglongcomplex", m, genie_read_longlong_complex);
  m = a68_proc (MODE (VOID), MODE (LONGLONG_COMPLEX), NULL);
  a68_idf ("printlonglongcompl", m, genie_print_longlong_complex);
  a68_idf ("printlonglongcomplex", m, genie_print_longlong_complex);
  m = proc_bool = a68_proc (MODE (BOOL), NULL);
  a68_idf ("readbool", m, genie_read_bool);
  m = a68_proc (MODE (VOID), MODE (BOOL), NULL);
  a68_idf ("printbool", m, genie_print_bool);
  m = a68_proc (MODE (BITS), NULL);
  a68_idf ("readbits", m, genie_read_bits);
  m = a68_proc (MODE (LONG_BITS), NULL);
  a68_idf ("readlongbits", m, genie_read_long_bits);
  m = a68_proc (MODE (LONGLONG_BITS), NULL);
  a68_idf ("readlonglongbits", m, genie_read_longlong_bits);
  m = a68_proc (MODE (VOID), MODE (BITS), NULL);
  a68_idf ("printbits", m, genie_print_bits);
  m = a68_proc (MODE (VOID), MODE (LONG_BITS), NULL);
  a68_idf ("printlongbits", m, genie_print_long_bits);
  m = a68_proc (MODE (VOID), MODE (LONGLONG_BITS), NULL);
  a68_idf ("printlonglongbits", m, genie_print_longlong_bits);
  m = proc_char = a68_proc (MODE (CHAR), NULL);
  a68_idf ("readchar", m, genie_read_char);
  m = a68_proc (MODE (VOID), MODE (CHAR), NULL);
  a68_idf ("printchar", m, genie_print_char);
  a68_idf ("readstring", MODE (PROC_STRING), genie_read_string);
  m = a68_proc (MODE (VOID), MODE (STRING), NULL);
  a68_idf ("printstring", m, genie_print_string);
#ifdef HAVE_PLOTUTILS
/* Drawing. */
  m = a68_proc (MODE (BOOL), MODE (REF_FILE), MODE (STRING), MODE (STRING), NULL);
  a68_idf ("drawdevice", m, genie_make_device);
  a68_idf ("makedevice", m, genie_make_device);
  m = a68_proc (MODE (REAL), MODE (REF_FILE), NULL);
  a68_idf ("drawaspect", m, genie_draw_aspect);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), NULL);
  a68_idf ("drawclear", m, genie_draw_clear);
  a68_idf ("drawerase", m, genie_draw_clear);
  a68_idf ("drawflush", m, genie_draw_show);
  a68_idf ("drawshow", m, genie_draw_show);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (INT), NULL);
  a68_idf ("drawfillstyle", m, genie_draw_filltype);
  m = a68_proc (MODE (STRING), MODE (INT), NULL);
  a68_idf ("drawgetcolourname", m, genie_draw_get_colour_name);
  a68_idf ("drawgetcolorname", m, genie_draw_get_colour_name);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (REAL), MODE (REAL), MODE (REAL), NULL);
  a68_idf ("drawcolor", m, genie_draw_colour);
  a68_idf ("drawcolour", m, genie_draw_colour);
  a68_idf ("drawbackgroundcolor", m, genie_draw_background_colour);
  a68_idf ("drawbackgroundcolour", m, genie_draw_background_colour);
  a68_idf ("drawcircle", m, genie_draw_circle);
  a68_idf ("drawball", m, genie_draw_atom);
  a68_idf ("drawstar", m, genie_draw_star);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (REAL), MODE (REAL), NULL);
  a68_idf ("drawpoint", m, genie_draw_point);
  a68_idf ("drawline", m, genie_draw_line);
  a68_idf ("drawmove", m, genie_draw_move);
  a68_idf ("drawrect", m, genie_draw_rect);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (CHAR), MODE (CHAR), MODE (ROW_CHAR), NULL);
  a68_idf ("drawtext", m, genie_draw_text);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (ROW_CHAR), NULL);
  a68_idf ("drawlinestyle", m, genie_draw_linestyle);
  a68_idf ("drawfontname", m, genie_draw_fontname);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (REAL), NULL);
  a68_idf ("drawlinewidth", m, genie_draw_linewidth);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (INT), NULL);
  a68_idf ("drawfontsize", m, genie_draw_fontsize);
  a68_idf ("drawtextangle", m, genie_draw_textangle);
  m = a68_proc (MODE (VOID), MODE (REF_FILE), MODE (STRING), NULL);
  a68_idf ("drawcolorname", m, genie_draw_colour_name);
  a68_idf ("drawcolourname", m, genie_draw_colour_name);
  a68_idf ("drawbackgroundcolorname", m, genie_draw_background_colour_name);
  a68_idf ("drawbackgroundcolourname", m, genie_draw_background_colour_name);
#endif
/* RNG procedures. */
  m = a68_proc (MODE (VOID), MODE (INT), NULL);
  a68_idf ("firstrandom", m, genie_first_random);
  m = proc_real;
  a68_idf ("nextrandom", m, genie_next_random);
  a68_idf ("random", m, genie_next_random);
  m = a68_proc (MODE (LONG_REAL), NULL);
  a68_idf ("longnextrandom", m, genie_long_next_random);
  a68_idf ("longrandom", m, genie_long_next_random);
  m = a68_proc (MODE (LONGLONG_REAL), NULL);
  a68_idf ("longlongnextrandom", m, genie_long_next_random);
  a68_idf ("longlongrandom", m, genie_long_next_random);
/* Priorities. */
  a68_prio ("+:=", 1);
  a68_prio ("-:=", 1);
  a68_prio ("*:=", 1);
  a68_prio ("/:=", 1);
  a68_prio ("%:=", 1);
  a68_prio ("%*:=", 1);
  a68_prio ("+=:", 1);
  a68_prio ("PLUSAB", 1);
  a68_prio ("MINUSAB", 1);
  a68_prio ("TIMESAB", 1);
  a68_prio ("DIVAB", 1);
  a68_prio ("OVERAB", 1);
  a68_prio ("MODAB", 1);
  a68_prio ("PLUSTO", 1);
  a68_prio ("OR", 2);
  a68_prio ("AND", 3);
  a68_prio ("&", 3);
  a68_prio ("XOR", 3);
  a68_prio ("=", 4);
  a68_prio ("/=", 4);
  a68_prio ("~=", 4);
  a68_prio ("^=", 4);
  a68_prio ("<", 5);
  a68_prio ("<=", 5);
  a68_prio (">", 5);
  a68_prio (">=", 5);
  a68_prio ("EQ", 4);
  a68_prio ("NE", 4);
  a68_prio ("LT", 5);
  a68_prio ("LE", 5);
  a68_prio ("GT", 5);
  a68_prio ("GE", 5);
  a68_prio ("+", 6);
  a68_prio ("-", 6);
  a68_prio ("*", 7);
  a68_prio ("/", 7);
  a68_prio ("OVER", 7);
  a68_prio ("%", 7);
  a68_prio ("MOD", 7);
  a68_prio ("%*", 7);
  a68_prio ("ELEM", 7);
  a68_prio ("**", 8);
  a68_prio ("SHL", 8);
  a68_prio ("SHR", 8);
  a68_prio ("UP", 8);
  a68_prio ("DOWN", 8);
  a68_prio ("^", 8);
  a68_prio ("ELEMS", 8);
  a68_prio ("LWB", 8);
  a68_prio ("UPB", 8);
  a68_prio ("I", 9);
  a68_prio ("+*", 9);
/* INT ops. */
  m = a68_proc (MODE (INT), MODE (INT), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_int);
  a68_op ("ABS", m, genie_abs_int);
  a68_op ("SIGN", m, genie_sign_int);
  m = a68_proc (MODE (BOOL), MODE (INT), NULL);
  a68_op ("ODD", m, genie_odd_int);
  m = a68_proc (MODE (BOOL), MODE (INT), MODE (INT), NULL);
  a68_op ("=", m, genie_eq_int);
  a68_op ("/=", m, genie_ne_int);
  a68_op ("~=", m, genie_ne_int);
  a68_op ("^=", m, genie_ne_int);
  a68_op ("<", m, genie_lt_int);
  a68_op ("<=", m, genie_le_int);
  a68_op (">", m, genie_gt_int);
  a68_op (">=", m, genie_ge_int);
  a68_op ("EQ", m, genie_eq_int);
  a68_op ("NE", m, genie_ne_int);
  a68_op ("LT", m, genie_lt_int);
  a68_op ("LE", m, genie_le_int);
  a68_op ("GT", m, genie_gt_int);
  a68_op ("GE", m, genie_ge_int);
  m = a68_proc (MODE (INT), MODE (INT), MODE (INT), NULL);
  a68_op ("+", m, genie_add_int);
  a68_op ("-", m, genie_sub_int);
  a68_op ("*", m, genie_mul_int);
  a68_op ("OVER", m, genie_over_int);
  a68_op ("%", m, genie_over_int);
  a68_op ("MOD", m, genie_mod_int);
  a68_op ("%*", m, genie_mod_int);
  a68_op ("**", m, genie_pow_int);
  a68_op ("UP", m, genie_pow_int);
  a68_op ("^", m, genie_pow_int);
  m = a68_proc (MODE (REAL), MODE (INT), MODE (INT), NULL);
  a68_op ("/", m, genie_div_int);
  m = a68_proc (MODE (REF_INT), MODE (REF_INT), MODE (INT), NULL);
  a68_op ("+:=", m, genie_plusab_int);
  a68_op ("-:=", m, genie_minusab_int);
  a68_op ("*:=", m, genie_timesab_int);
  a68_op ("%:=", m, genie_overab_int);
  a68_op ("%*:=", m, genie_modab_int);
  a68_op ("PLUSAB", m, genie_plusab_int);
  a68_op ("MINUSAB", m, genie_minusab_int);
  a68_op ("TIMESAB", m, genie_timesab_int);
  a68_op ("OVERAB", m, genie_overab_int);
  a68_op ("MODAB", m, genie_modab_int);
/* REAL ops. */
  m = proc_real_real = a68_proc (MODE (REAL), MODE (REAL), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_real);
  a68_op ("ABS", m, genie_abs_real);
  a68_op ("NINT", m, genie_nint_real);
  m = a68_proc (MODE (INT), MODE (REAL), NULL);
  a68_op ("SIGN", m, genie_sign_real);
  a68_op ("ROUND", m, genie_round_real);
  a68_op ("ENTIER", m, genie_entier_real);
  m = a68_proc (MODE (BOOL), MODE (REAL), MODE (REAL), NULL);
  a68_op ("=", m, genie_eq_real);
  a68_op ("/=", m, genie_ne_real);
  a68_op ("~=", m, genie_ne_real);
  a68_op ("^=", m, genie_ne_real);
  a68_op ("<", m, genie_lt_real);
  a68_op ("<=", m, genie_le_real);
  a68_op (">", m, genie_gt_real);
  a68_op (">=", m, genie_ge_real);
  a68_op ("EQ", m, genie_eq_real);
  a68_op ("NE", m, genie_ne_real);
  a68_op ("LT", m, genie_lt_real);
  a68_op ("LE", m, genie_le_real);
  a68_op ("GT", m, genie_gt_real);
  a68_op ("GE", m, genie_ge_real);
  m = proc_real_real_real = a68_proc (MODE (REAL), MODE (REAL), MODE (REAL), NULL);
  a68_op ("+", m, genie_add_real);
  a68_op ("-", m, genie_sub_real);
  a68_op ("*", m, genie_mul_real);
  a68_op ("/", m, genie_div_real);
  a68_op ("**", m, genie_pow_real);
  a68_op ("UP", m, genie_pow_real);
  a68_op ("^", m, genie_pow_real);
  m = a68_proc (MODE (REAL), MODE (REAL), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_real_int);
  a68_op ("UP", m, genie_pow_real_int);
  a68_op ("^", m, genie_pow_real_int);
  m = a68_proc (MODE (REF_REAL), MODE (REF_REAL), MODE (REAL), NULL);
  a68_op ("+:=", m, genie_plusab_real);
  a68_op ("-:=", m, genie_minusab_real);
  a68_op ("*:=", m, genie_timesab_real);
  a68_op ("/:=", m, genie_overab_real);
  a68_op ("PLUSAB", m, genie_plusab_real);
  a68_op ("MINUSAB", m, genie_minusab_real);
  a68_op ("TIMESAB", m, genie_timesab_real);
  a68_op ("DIVAB", m, genie_overab_real);
  m = proc_real_real;
  a68_idf ("sqrt", m, genie_sqrt_real);
  a68_idf ("cbrt", m, genie_curt_real);
  a68_idf ("curt", m, genie_curt_real);
  a68_idf ("exp", m, genie_exp_real);
  a68_idf ("ln", m, genie_ln_real);
  a68_idf ("log", m, genie_log_real);
  a68_idf ("sin", m, genie_sin_real);
  a68_idf ("cos", m, genie_cos_real);
  a68_idf ("tan", m, genie_tan_real);
  a68_idf ("asin", m, genie_arcsin_real);
  a68_idf ("acos", m, genie_arccos_real);
  a68_idf ("atan", m, genie_arctan_real);
  a68_idf ("arcsin", m, genie_arcsin_real);
  a68_idf ("arccos", m, genie_arccos_real);
  a68_idf ("arctan", m, genie_arctan_real);
  a68_idf ("sinh", m, genie_sinh_real);
  a68_idf ("cosh", m, genie_cosh_real);
  a68_idf ("tanh", m, genie_tanh_real);
  a68_idf ("asinh", m, genie_arcsinh_real);
  a68_idf ("acosh", m, genie_arccosh_real);
  a68_idf ("atanh", m, genie_arctanh_real);
  a68_idf ("arcsinh", m, genie_arcsinh_real);
  a68_idf ("arccosh", m, genie_arccosh_real);
  a68_idf ("arctanh", m, genie_arctanh_real);
  a68_idf ("inverseerf", m, genie_inverf_real);
  a68_idf ("inverseerfc", m, genie_inverfc_real);
  m = proc_real_real_real;
  a68_idf ("arctan2", m, genie_atan2_real);
/* COMPLEX ops. */
  m = a68_proc (MODE (COMPLEX), MODE (REAL), MODE (REAL), NULL);
  a68_op ("I", m, genie_icomplex);
  a68_op ("+*", m, genie_icomplex);
  m = a68_proc (MODE (COMPLEX), MODE (INT), MODE (INT), NULL);
  a68_op ("I", m, genie_iint_complex);
  a68_op ("+*", m, genie_iint_complex);
  m = a68_proc (MODE (REAL), MODE (COMPLEX), NULL);
  a68_op ("RE", m, genie_re_complex);
  a68_op ("IM", m, genie_im_complex);
  a68_op ("ABS", m, genie_abs_complex);
  a68_op ("ARG", m, genie_arg_complex);
  m = proc_complex_complex = a68_proc (MODE (COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_complex);
  a68_op ("CONJ", m, genie_conj_complex);
  m = a68_proc (MODE (BOOL), MODE (COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("=", m, genie_eq_complex);
  a68_op ("/=", m, genie_ne_complex);
  a68_op ("~=", m, genie_ne_complex);
  a68_op ("^=", m, genie_ne_complex);
  a68_op ("EQ", m, genie_eq_complex);
  a68_op ("NE", m, genie_ne_complex);
  m = a68_proc (MODE (COMPLEX), MODE (COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("+", m, genie_add_complex);
  a68_op ("-", m, genie_sub_complex);
  a68_op ("*", m, genie_mul_complex);
  a68_op ("/", m, genie_div_complex);
  m = a68_proc (MODE (COMPLEX), MODE (COMPLEX), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_complex_int);
  a68_op ("UP", m, genie_pow_complex_int);
  a68_op ("^", m, genie_pow_complex_int);
  m = a68_proc (MODE (REF_COMPLEX), MODE (REF_COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("+:=", m, genie_plusab_complex);
  a68_op ("-:=", m, genie_minusab_complex);
  a68_op ("*:=", m, genie_timesab_complex);
  a68_op ("/:=", m, genie_divab_complex);
  a68_op ("PLUSAB", m, genie_plusab_complex);
  a68_op ("MINUSAB", m, genie_minusab_complex);
  a68_op ("TIMESAB", m, genie_timesab_complex);
  a68_op ("DIVAB", m, genie_divab_complex);
/* BOOL ops. */
  m = a68_proc (MODE (BOOL), MODE (BOOL), NULL);
  a68_op ("NOT", m, genie_not_bool);
  a68_op ("~", m, genie_not_bool);
  m = a68_proc (MODE (INT), MODE (BOOL), NULL);
  a68_op ("ABS", m, genie_abs_bool);
  m = a68_proc (MODE (BOOL), MODE (BOOL), MODE (BOOL), NULL);
  a68_op ("OR", m, genie_or_bool);
  a68_op ("AND", m, genie_and_bool);
  a68_op ("&", m, genie_and_bool);
  a68_op ("XOR", m, genie_xor_bool);
  a68_op ("=", m, genie_eq_bool);
  a68_op ("/=", m, genie_ne_bool);
  a68_op ("~=", m, genie_ne_bool);
  a68_op ("^=", m, genie_ne_bool);
  a68_op ("EQ", m, genie_eq_bool);
  a68_op ("NE", m, genie_ne_bool);
/* CHAR ops. */
  m = a68_proc (MODE (BOOL), MODE (CHAR), MODE (CHAR), NULL);
  a68_op ("=", m, genie_eq_char);
  a68_op ("/=", m, genie_ne_char);
  a68_op ("~=", m, genie_ne_char);
  a68_op ("^=", m, genie_ne_char);
  a68_op ("<", m, genie_lt_char);
  a68_op ("<=", m, genie_le_char);
  a68_op (">", m, genie_gt_char);
  a68_op (">=", m, genie_ge_char);
  a68_op ("EQ", m, genie_eq_char);
  a68_op ("NE", m, genie_ne_char);
  a68_op ("LT", m, genie_lt_char);
  a68_op ("LE", m, genie_le_char);
  a68_op ("GT", m, genie_gt_char);
  a68_op ("GE", m, genie_ge_char);
  m = a68_proc (MODE (INT), MODE (CHAR), NULL);
  a68_op ("ABS", m, genie_abs_char);
  m = a68_proc (MODE (CHAR), MODE (INT), NULL);
  a68_op ("REPR", m, genie_repr_char);
/* BITS ops. */
  m = a68_proc (MODE (INT), MODE (BITS), NULL);
  a68_op ("ABS", m, genie_idle);
  m = a68_proc (MODE (BITS), MODE (INT), NULL);
  a68_op ("BIN", m, genie_bin_int);
  m = a68_proc (MODE (BITS), MODE (BITS), NULL);
  a68_op ("NOT", m, genie_not_bits);
  a68_op ("~", m, genie_not_bits);
  m = a68_proc (MODE (BOOL), MODE (BITS), MODE (BITS), NULL);
  a68_op ("=", m, genie_eq_bits);
  a68_op ("/=", m, genie_ne_bits);
  a68_op ("~=", m, genie_ne_bits);
  a68_op ("^=", m, genie_ne_bits);
  a68_op ("<", m, genie_lt_bits);
  a68_op ("<=", m, genie_le_bits);
  a68_op (">", m, genie_gt_bits);
  a68_op (">=", m, genie_ge_bits);
  a68_op ("EQ", m, genie_eq_bits);
  a68_op ("NE", m, genie_ne_bits);
  a68_op ("LT", m, genie_lt_bits);
  a68_op ("LE", m, genie_le_bits);
  a68_op ("GT", m, genie_gt_bits);
  a68_op ("GE", m, genie_ge_bits);
  m = a68_proc (MODE (BITS), MODE (BITS), MODE (BITS), NULL);
  a68_op ("AND", m, genie_and_bits);
  a68_op ("&", m, genie_and_bits);
  a68_op ("OR", m, genie_or_bits);
  a68_op ("XOR", m, genie_xor_bits);
  m = a68_proc (MODE (BITS), MODE (BITS), MODE (INT), NULL);
  a68_op ("SHL", m, genie_shl_bits);
  a68_op ("UP", m, genie_shl_bits);
  a68_op ("SHR", m, genie_shr_bits);
  a68_op ("DOWN", m, genie_shr_bits);
  m = a68_proc (MODE (BOOL), MODE (INT), MODE (BITS), NULL);
  a68_op ("ELEM", m, genie_elem_bits);
/* LONG BITS ops. */
  m = a68_proc (MODE (LONG_INT), MODE (LONG_BITS), NULL);
  a68_op ("ABS", m, genie_idle);
  m = a68_proc (MODE (LONG_BITS), MODE (LONG_INT), NULL);
  a68_op ("BIN", m, genie_bin_long_mp);
  m = a68_proc (MODE (BITS), MODE (LONG_BITS), NULL);
  a68_op ("SHORTEN", m, genie_shorten_long_mp_to_bits);
  m = a68_proc (MODE (LONG_BITS), MODE (BITS), NULL);
  a68_op ("LENG", m, genie_lengthen_unsigned_to_long_mp);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONG_BITS), NULL);
  a68_op ("LENG", m, genie_lengthen_long_mp_to_longlong_mp);
  m = a68_proc (MODE (LONG_BITS), MODE (LONG_BITS), NULL);
  a68_op ("NOT", m, genie_not_long_mp);
  a68_op ("~", m, genie_not_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONG_BITS), MODE (LONG_BITS), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONG_BITS), MODE (LONG_BITS), MODE (LONG_BITS), NULL);
  a68_op ("AND", m, genie_and_long_mp);
  a68_op ("&", m, genie_and_long_mp);
  a68_op ("OR", m, genie_or_long_mp);
  a68_op ("XOR", m, genie_xor_long_mp);
  m = a68_proc (MODE (LONG_BITS), MODE (LONG_BITS), MODE (INT), NULL);
  a68_op ("SHL", m, genie_shl_long_mp);
  a68_op ("UP", m, genie_shl_long_mp);
  a68_op ("SHR", m, genie_shr_long_mp);
  a68_op ("DOWN", m, genie_shr_long_mp);
  m = a68_proc (MODE (BOOL), MODE (INT), MODE (LONG_BITS), NULL);
  a68_op ("ELEM", m, genie_elem_long_bits);
/* LONG LONG BITS. */
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_BITS), NULL);
  a68_op ("ABS", m, genie_idle);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONGLONG_INT), NULL);
  a68_op ("BIN", m, genie_bin_long_mp);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), NULL);
  a68_op ("NOT", m, genie_not_long_mp);
  a68_op ("~", m, genie_not_long_mp);
  m = a68_proc (MODE (LONG_BITS), MODE (LONGLONG_BITS), NULL);
  a68_op ("SHORTEN", m, genie_shorten_longlong_mp_to_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), NULL);
  a68_op ("AND", m, genie_and_long_mp);
  a68_op ("&", m, genie_and_long_mp);
  a68_op ("OR", m, genie_or_long_mp);
  a68_op ("XOR", m, genie_xor_long_mp);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), MODE (INT), NULL);
  a68_op ("SHL", m, genie_shl_long_mp);
  a68_op ("UP", m, genie_shl_long_mp);
  a68_op ("SHR", m, genie_shr_long_mp);
  a68_op ("DOWN", m, genie_shr_long_mp);
  m = a68_proc (MODE (BOOL), MODE (INT), MODE (LONGLONG_BITS), NULL);
  a68_op ("ELEM", m, genie_elem_longlong_bits);
/* BYTES ops. */
  m = a68_proc (MODE (BYTES), MODE (STRING), NULL);
  a68_idf ("bytespack", m, genie_bytespack);
  m = a68_proc (MODE (CHAR), MODE (INT), MODE (BYTES), NULL);
  a68_op ("ELEM", m, genie_elem_bytes);
  m = a68_proc (MODE (BYTES), MODE (BYTES), MODE (BYTES), NULL);
  a68_op ("+", m, genie_add_bytes);
  m = a68_proc (MODE (REF_BYTES), MODE (REF_BYTES), MODE (BYTES), NULL);
  a68_op ("+:=", m, genie_plusab_bytes);
  a68_op ("PLUSAB", m, genie_plusab_bytes);
  m = a68_proc (MODE (REF_BYTES), MODE (BYTES), MODE (REF_BYTES), NULL);
  a68_op ("+=:", m, genie_plusto_bytes);
  a68_op ("PLUSTO", m, genie_plusto_bytes);
  m = a68_proc (MODE (BOOL), MODE (BYTES), MODE (BYTES), NULL);
  a68_op ("=", m, genie_eq_bytes);
  a68_op ("/=", m, genie_ne_bytes);
  a68_op ("~=", m, genie_ne_bytes);
  a68_op ("^=", m, genie_ne_bytes);
  a68_op ("<", m, genie_lt_bytes);
  a68_op ("<=", m, genie_le_bytes);
  a68_op (">", m, genie_gt_bytes);
  a68_op (">=", m, genie_ge_bytes);
  a68_op ("EQ", m, genie_eq_bytes);
  a68_op ("NE", m, genie_ne_bytes);
  a68_op ("LT", m, genie_lt_bytes);
  a68_op ("LE", m, genie_le_bytes);
  a68_op ("GT", m, genie_gt_bytes);
  a68_op ("GE", m, genie_ge_bytes);
/* LONG BYTES ops. */
  m = a68_proc (MODE (LONG_BYTES), MODE (BYTES), NULL);
  a68_op ("LENG", m, genie_leng_bytes);
  m = a68_proc (MODE (BYTES), MODE (LONG_BYTES), NULL);
  a68_idf ("SHORTEN", m, genie_shorten_bytes);
  m = a68_proc (MODE (LONG_BYTES), MODE (STRING), NULL);
  a68_idf ("longbytespack", m, genie_long_bytespack);
  m = a68_proc (MODE (CHAR), MODE (INT), MODE (LONG_BYTES), NULL);
  a68_op ("ELEM", m, genie_elem_long_bytes);
  m = a68_proc (MODE (LONG_BYTES), MODE (LONG_BYTES), MODE (LONG_BYTES), NULL);
  a68_op ("+", m, genie_add_long_bytes);
  m = a68_proc (MODE (REF_LONG_BYTES), MODE (REF_LONG_BYTES), MODE (LONG_BYTES), NULL);
  a68_op ("+:=", m, genie_plusab_long_bytes);
  a68_op ("PLUSAB", m, genie_plusab_long_bytes);
  m = a68_proc (MODE (REF_LONG_BYTES), MODE (LONG_BYTES), MODE (REF_LONG_BYTES), NULL);
  a68_op ("+=:", m, genie_plusto_long_bytes);
  a68_op ("PLUSTO", m, genie_plusto_long_bytes);
  m = a68_proc (MODE (BOOL), MODE (LONG_BYTES), MODE (LONG_BYTES), NULL);
  a68_op ("=", m, genie_eq_long_bytes);
  a68_op ("/=", m, genie_ne_long_bytes);
  a68_op ("~=", m, genie_ne_long_bytes);
  a68_op ("^=", m, genie_ne_long_bytes);
  a68_op ("<", m, genie_lt_long_bytes);
  a68_op ("<=", m, genie_le_long_bytes);
  a68_op (">", m, genie_gt_long_bytes);
  a68_op (">=", m, genie_ge_long_bytes);
  a68_op ("EQ", m, genie_eq_long_bytes);
  a68_op ("NE", m, genie_ne_long_bytes);
  a68_op ("LT", m, genie_lt_long_bytes);
  a68_op ("LE", m, genie_le_long_bytes);
  a68_op ("GT", m, genie_gt_long_bytes);
  a68_op ("GE", m, genie_ge_long_bytes);
/* STRING ops. */
  m = a68_proc (MODE (BOOL), MODE (STRING), MODE (STRING), NULL);
  a68_op ("=", m, genie_eq_string);
  a68_op ("/=", m, genie_ne_string);
  a68_op ("~=", m, genie_ne_string);
  a68_op ("^=", m, genie_ne_string);
  a68_op ("<", m, genie_lt_string);
  a68_op ("<=", m, genie_le_string);
  a68_op (">=", m, genie_ge_string);
  a68_op (">", m, genie_gt_string);
  a68_op ("EQ", m, genie_eq_string);
  a68_op ("NE", m, genie_ne_string);
  a68_op ("LT", m, genie_lt_string);
  a68_op ("LE", m, genie_le_string);
  a68_op ("GE", m, genie_ge_string);
  a68_op ("GT", m, genie_gt_string);
  m = a68_proc (MODE (CHAR), MODE (INT), MODE (STRING), NULL);
  a68_op ("ELEM", m, genie_elem_string);
  m = a68_proc (MODE (STRING), MODE (CHAR), MODE (CHAR), NULL);
  a68_op ("+", m, genie_add_char);
  m = a68_proc (MODE (STRING), MODE (STRING), MODE (STRING), NULL);
  a68_op ("+", m, genie_add_string);
  m = a68_proc (MODE (REF_STRING), MODE (REF_STRING), MODE (STRING), NULL);
  a68_op ("+:=", m, genie_plusab_string);
  a68_op ("PLUSAB", m, genie_plusab_string);
  m = a68_proc (MODE (REF_STRING), MODE (REF_STRING), MODE (INT), NULL);
  a68_op ("*:=", m, genie_timesab_string);
  a68_op ("TIMESAB", m, genie_timesab_string);
  m = a68_proc (MODE (REF_STRING), MODE (STRING), MODE (REF_STRING), NULL);
  a68_op ("+=:", m, genie_plusto_string);
  a68_op ("PLUSTO", m, genie_plusto_string);
  m = a68_proc (MODE (STRING), MODE (STRING), MODE (INT), NULL);
  a68_op ("*", m, genie_times_string_int);
  m = a68_proc (MODE (STRING), MODE (INT), MODE (STRING), NULL);
  a68_op ("*", m, genie_times_int_string);
  m = a68_proc (MODE (STRING), MODE (INT), MODE (CHAR), NULL);
  a68_op ("*", m, genie_times_int_char);
  m = a68_proc (MODE (STRING), MODE (CHAR), MODE (INT), NULL);
  a68_op ("*", m, genie_times_char_int);
/* [] CHAR as cross term for STRING. */
  m = a68_proc (MODE (BOOL), MODE (ROW_CHAR), MODE (ROW_CHAR), NULL);
  a68_op ("=", m, genie_eq_string);
  a68_op ("/=", m, genie_ne_string);
  a68_op ("~=", m, genie_ne_string);
  a68_op ("^=", m, genie_ne_string);
  a68_op ("<", m, genie_lt_string);
  a68_op ("<=", m, genie_le_string);
  a68_op (">=", m, genie_ge_string);
  a68_op (">", m, genie_gt_string);
  a68_op ("EQ", m, genie_eq_string);
  a68_op ("NE", m, genie_ne_string);
  a68_op ("LT", m, genie_lt_string);
  a68_op ("LE", m, genie_le_string);
  a68_op ("GE", m, genie_ge_string);
  a68_op ("GT", m, genie_gt_string);
  m = a68_proc (MODE (CHAR), MODE (INT), MODE (ROW_CHAR), NULL);
  a68_op ("ELEM", m, genie_elem_string);
  m = a68_proc (MODE (STRING), MODE (ROW_CHAR), MODE (ROW_CHAR), NULL);
  a68_op ("+", m, genie_add_string);
  m = a68_proc (MODE (STRING), MODE (ROW_CHAR), MODE (INT), NULL);
  a68_op ("*", m, genie_times_string_int);
  m = a68_proc (MODE (STRING), MODE (INT), MODE (ROW_CHAR), NULL);
  a68_op ("*", m, genie_times_int_string);
/* SEMA ops. */
  m = a68_proc (MODE (SEMA), MODE (INT), NULL);
  a68_op ("LEVEL", m, genie_level_sema_int);
  m = a68_proc (MODE (INT), MODE (SEMA), NULL);
  a68_op ("LEVEL", m, genie_level_int_sema);
  m = a68_proc (MODE (VOID), MODE (SEMA), NULL);
  a68_op ("UP", m, genie_up_sema);
  a68_op ("DOWN", m, genie_down_sema);
/* ROWS ops. */
  m = a68_proc (MODE (INT), MODE (ROWS), NULL);
  a68_op ("ELEMS", m, genie_monad_elems);
  a68_op ("LWB", m, genie_monad_lwb);
  a68_op ("UPB", m, genie_monad_upb);
  m = a68_proc (MODE (INT), MODE (INT), MODE (ROWS), NULL);
  a68_op ("ELEMS", m, genie_dyad_elems);
  a68_op ("LWB", m, genie_dyad_lwb);
  a68_op ("UPB", m, genie_dyad_upb);
/* Binding for the multiple-precision library. */
/* LONG INT. */
  m = a68_proc (MODE (LONG_INT), MODE (INT), NULL);
  a68_op ("LENG", m, genie_lengthen_int_to_long_mp);
  m = a68_proc (MODE (LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_mp);
  a68_op ("ABS", m, genie_abs_long_mp);
  m = a68_proc (MODE (INT), MODE (LONG_INT), NULL);
  a68_op ("SHORTEN", m, genie_shorten_long_mp_to_int);
  a68_op ("SIGN", m, genie_sign_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONG_INT), NULL);
  a68_op ("ODD", m, genie_odd_long_mp);
  m = a68_proc (MODE (LONG_INT), MODE (LONG_REAL), NULL);
  a68_op ("ENTIER", m, genie_entier_long_mp);
  a68_op ("ROUND", m, genie_round_long_mp);
  m = a68_proc (MODE (LONG_INT), MODE (LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("+", m, genie_add_long_int);
  a68_op ("-", m, genie_minus_long_int);
  a68_op ("*", m, genie_mul_long_int);
  a68_op ("OVER", m, genie_over_long_mp);
  a68_op ("%", m, genie_over_long_mp);
  a68_op ("MOD", m, genie_mod_long_mp);
  a68_op ("%*", m, genie_mod_long_mp);
  m = a68_proc (MODE (REF_LONG_INT), MODE (REF_LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("+:=", m, genie_plusab_long_int);
  a68_op ("-:=", m, genie_minusab_long_int);
  a68_op ("*:=", m, genie_timesab_long_int);
  a68_op ("%:=", m, genie_overab_long_mp);
  a68_op ("%*:=", m, genie_modab_long_mp);
  a68_op ("PLUSAB", m, genie_plusab_long_int);
  a68_op ("MINUSAB", m, genie_minusab_long_int);
  a68_op ("TIMESAB", m, genie_timesab_long_int);
  a68_op ("OVERAB", m, genie_overab_long_mp);
  a68_op ("MODAB", m, genie_modab_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONG_REAL), MODE (LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("/", m, genie_div_long_mp);
  m = a68_proc (MODE (LONG_INT), MODE (LONG_INT), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_mp_int_int);
  a68_op ("^", m, genie_pow_long_mp_int_int);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_INT), MODE (LONG_INT), NULL);
  a68_op ("I", m, genie_idle);
  a68_op ("+*", m, genie_idle);
/* LONG REAL. */
  m = a68_proc (MODE (LONG_REAL), MODE (REAL), NULL);
  a68_op ("LENG", m, genie_lengthen_real_to_long_mp);
  m = a68_proc (MODE (REAL), MODE (LONG_REAL), NULL);
  a68_op ("SHORTEN", m, genie_shorten_long_mp_to_real);
  m = a68_proc (MODE (LONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_mp);
  a68_op ("ABS", m, genie_abs_long_mp);
  a68_idf ("longsqrt", m, genie_sqrt_long_mp);
  a68_idf ("longcbrt", m, genie_curt_long_mp);
  a68_idf ("longcurt", m, genie_curt_long_mp);
  a68_idf ("longexp", m, genie_exp_long_mp);
  a68_idf ("longln", m, genie_ln_long_mp);
  a68_idf ("longlog", m, genie_log_long_mp);
  a68_idf ("longsin", m, genie_sin_long_mp);
  a68_idf ("longcos", m, genie_cos_long_mp);
  a68_idf ("longtan", m, genie_tan_long_mp);
  a68_idf ("longasin", m, genie_asin_long_mp);
  a68_idf ("longacos", m, genie_acos_long_mp);
  a68_idf ("longatan", m, genie_atan_long_mp);
  a68_idf ("longarcsin", m, genie_asin_long_mp);
  a68_idf ("longarccos", m, genie_acos_long_mp);
  a68_idf ("longarctan", m, genie_atan_long_mp);
  a68_idf ("longsinh", m, genie_sinh_long_mp);
  a68_idf ("longcosh", m, genie_cosh_long_mp);
  a68_idf ("longtanh", m, genie_tanh_long_mp);
  a68_idf ("longasinh", m, genie_arcsinh_long_mp);
  a68_idf ("longacosh", m, genie_arccosh_long_mp);
  a68_idf ("longatanh", m, genie_arctanh_long_mp);
  a68_idf ("longarcsinh", m, genie_arcsinh_long_mp);
  a68_idf ("longarccosh", m, genie_arccosh_long_mp);
  a68_idf ("longarctanh", m, genie_arctanh_long_mp);
  a68_idf ("dsqrt", m, genie_sqrt_long_mp);
  a68_idf ("dcbrt", m, genie_curt_long_mp);
  a68_idf ("dcurt", m, genie_curt_long_mp);
  a68_idf ("dexp", m, genie_exp_long_mp);
  a68_idf ("dln", m, genie_ln_long_mp);
  a68_idf ("dlog", m, genie_log_long_mp);
  a68_idf ("dsin", m, genie_sin_long_mp);
  a68_idf ("dcos", m, genie_cos_long_mp);
  a68_idf ("dtan", m, genie_tan_long_mp);
  a68_idf ("dasin", m, genie_asin_long_mp);
  a68_idf ("dacos", m, genie_acos_long_mp);
  a68_idf ("datan", m, genie_atan_long_mp);
  a68_idf ("dsinh", m, genie_sinh_long_mp);
  a68_idf ("dcosh", m, genie_cosh_long_mp);
  a68_idf ("dtanh", m, genie_tanh_long_mp);
  a68_idf ("dasinh", m, genie_arcsinh_long_mp);
  a68_idf ("dacosh", m, genie_arccosh_long_mp);
  a68_idf ("datanh", m, genie_arctanh_long_mp);
  m = a68_proc (MODE (INT), MODE (LONG_REAL), NULL);
  a68_op ("SIGN", m, genie_sign_long_mp);
  m = a68_proc (MODE (LONG_REAL), MODE (LONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("+", m, genie_add_long_mp);
  a68_op ("-", m, genie_sub_long_mp);
  a68_op ("*", m, genie_mul_long_mp);
  a68_op ("/", m, genie_div_long_mp);
  a68_op ("**", m, genie_pow_long_mp);
  a68_op ("UP", m, genie_pow_long_mp);
  a68_op ("^", m, genie_pow_long_mp);
  m = a68_proc (MODE (REF_LONG_REAL), MODE (REF_LONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("+:=", m, genie_plusab_long_mp);
  a68_op ("-:=", m, genie_minusab_long_mp);
  a68_op ("*:=", m, genie_timesab_long_mp);
  a68_op ("/:=", m, genie_divab_long_mp);
  a68_op ("PLUSAB", m, genie_plusab_long_mp);
  a68_op ("MINUSAB", m, genie_minusab_long_mp);
  a68_op ("TIMESAB", m, genie_timesab_long_mp);
  a68_op ("DIVAB", m, genie_divab_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONG_REAL), MODE (LONG_REAL), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_mp_int);
  a68_op ("UP", m, genie_pow_long_mp_int);
  a68_op ("^", m, genie_pow_long_mp_int);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("I", m, genie_idle);
  a68_op ("+*", m, genie_idle);
/* LONG COMPLEX. */
  m = a68_proc (MODE (LONG_COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("LENG", m, genie_lengthen_complex_to_long_complex);
  m = a68_proc (MODE (COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("SHORTEN", m, genie_shorten_long_complex_to_complex);
  m = a68_proc (MODE (LONG_REAL), MODE (LONG_COMPLEX), NULL);
  a68_op ("RE", m, genie_re_long_complex);
  a68_op ("IM", m, genie_im_long_complex);
  a68_op ("ARG", m, genie_arg_long_complex);
  a68_op ("ABS", m, genie_abs_long_complex);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_complex);
  a68_op ("CONJ", m, genie_conj_long_complex);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("+", m, genie_add_long_complex);
  a68_op ("-", m, genie_sub_long_complex);
  a68_op ("*", m, genie_mul_long_complex);
  a68_op ("/", m, genie_div_long_complex);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_complex_int);
  a68_op ("UP", m, genie_pow_long_complex_int);
  a68_op ("^", m, genie_pow_long_complex_int);
  m = a68_proc (MODE (BOOL), MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("=", m, genie_eq_long_complex);
  a68_op ("EQ", m, genie_eq_long_complex);
  a68_op ("/=", m, genie_ne_long_complex);
  a68_op ("~=", m, genie_ne_long_complex);
  a68_op ("NE", m, genie_ne_long_complex);
  m = a68_proc (MODE (REF_LONG_COMPLEX), MODE (REF_LONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("+:=", m, genie_plusab_long_complex);
  a68_op ("-:=", m, genie_minusab_long_complex);
  a68_op ("*:=", m, genie_timesab_long_complex);
  a68_op ("/:=", m, genie_divab_long_complex);
  a68_op ("PLUSAB", m, genie_plusab_long_complex);
  a68_op ("MINUSAB", m, genie_minusab_long_complex);
  a68_op ("TIMESAB", m, genie_timesab_long_complex);
  a68_op ("DIVAB", m, genie_divab_long_complex);
/* LONG LONG INT. */
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONG_INT), NULL);
  a68_op ("LENG", m, genie_lengthen_long_mp_to_longlong_mp);
  m = a68_proc (MODE (LONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("SHORTEN", m, genie_shorten_longlong_mp_to_long_mp);
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_mp);
  a68_op ("ABS", m, genie_abs_long_mp);
  m = a68_proc (MODE (INT), MODE (LONGLONG_INT), NULL);
  a68_op ("SIGN", m, genie_sign_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONGLONG_INT), NULL);
  a68_op ("ODD", m, genie_odd_long_mp);
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_REAL), NULL);
  a68_op ("ENTIER", m, genie_entier_long_mp);
  a68_op ("ROUND", m, genie_round_long_mp);
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("+", m, genie_add_long_int);
  a68_op ("-", m, genie_minus_long_int);
  a68_op ("*", m, genie_mul_long_int);
  a68_op ("OVER", m, genie_over_long_mp);
  a68_op ("%", m, genie_over_long_mp);
  a68_op ("MOD", m, genie_mod_long_mp);
  a68_op ("%*", m, genie_mod_long_mp);
  m = a68_proc (MODE (REF_LONGLONG_INT), MODE (REF_LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("+:=", m, genie_plusab_long_int);
  a68_op ("-:=", m, genie_minusab_long_int);
  a68_op ("*:=", m, genie_timesab_long_int);
  a68_op ("%:=", m, genie_overab_long_mp);
  a68_op ("%*:=", m, genie_modab_long_mp);
  a68_op ("PLUSAB", m, genie_plusab_long_int);
  a68_op ("MINUSAB", m, genie_minusab_long_int);
  a68_op ("TIMESAB", m, genie_timesab_long_int);
  a68_op ("OVERAB", m, genie_overab_long_mp);
  a68_op ("MODAB", m, genie_modab_long_mp);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("/", m, genie_div_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_INT), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_mp_int_int);
  a68_op ("^", m, genie_pow_long_mp_int_int);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("I", m, genie_idle);
  a68_op ("+*", m, genie_idle);
/* LONG LONG REAL. */
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONG_REAL), NULL);
  a68_op ("LENG", m, genie_lengthen_long_mp_to_longlong_mp);
  m = a68_proc (MODE (LONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("SHORTEN", m, genie_shorten_longlong_mp_to_long_mp);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_mp);
  a68_op ("ABS", m, genie_abs_long_mp);
  a68_idf ("longlongsqrt", m, genie_sqrt_long_mp);
  a68_idf ("longlongcbrt", m, genie_curt_long_mp);
  a68_idf ("longlongcurt", m, genie_curt_long_mp);
  a68_idf ("longlongexp", m, genie_exp_long_mp);
  a68_idf ("longlongln", m, genie_ln_long_mp);
  a68_idf ("longlonglog", m, genie_log_long_mp);
  a68_idf ("longlongsin", m, genie_sin_long_mp);
  a68_idf ("longlongcos", m, genie_cos_long_mp);
  a68_idf ("longlongtan", m, genie_tan_long_mp);
  a68_idf ("longlongasin", m, genie_asin_long_mp);
  a68_idf ("longlongacos", m, genie_acos_long_mp);
  a68_idf ("longlongatan", m, genie_atan_long_mp);
  a68_idf ("longlongarcsin", m, genie_asin_long_mp);
  a68_idf ("longlongarccos", m, genie_acos_long_mp);
  a68_idf ("longlongarctan", m, genie_atan_long_mp);
  a68_idf ("longlongsinh", m, genie_sinh_long_mp);
  a68_idf ("longlongcosh", m, genie_cosh_long_mp);
  a68_idf ("longlongtanh", m, genie_tanh_long_mp);
  a68_idf ("longlongasinh", m, genie_arcsinh_long_mp);
  a68_idf ("longlongacosh", m, genie_arccosh_long_mp);
  a68_idf ("longlongatanh", m, genie_arctanh_long_mp);
  a68_idf ("longlongarcsinh", m, genie_arcsinh_long_mp);
  a68_idf ("longlongarccosh", m, genie_arccosh_long_mp);
  a68_idf ("longlongarctanh", m, genie_arctanh_long_mp);
  a68_idf ("qsqrt", m, genie_sqrt_long_mp);
  a68_idf ("qcbrt", m, genie_curt_long_mp);
  a68_idf ("qcurt", m, genie_curt_long_mp);
  a68_idf ("qexp", m, genie_exp_long_mp);
  a68_idf ("qln", m, genie_ln_long_mp);
  a68_idf ("qlog", m, genie_log_long_mp);
  a68_idf ("qsin", m, genie_sin_long_mp);
  a68_idf ("qcos", m, genie_cos_long_mp);
  a68_idf ("qtan", m, genie_tan_long_mp);
  a68_idf ("qasin", m, genie_asin_long_mp);
  a68_idf ("qacos", m, genie_acos_long_mp);
  a68_idf ("qatan", m, genie_atan_long_mp);
  a68_idf ("qsinh", m, genie_sinh_long_mp);
  a68_idf ("qcosh", m, genie_cosh_long_mp);
  a68_idf ("qtanh", m, genie_tanh_long_mp);
  a68_idf ("qasinh", m, genie_arcsinh_long_mp);
  a68_idf ("qacosh", m, genie_arccosh_long_mp);
  a68_idf ("qatanh", m, genie_arctanh_long_mp);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("+", m, genie_add_long_mp);
  a68_op ("-", m, genie_sub_long_mp);
  a68_op ("*", m, genie_mul_long_mp);
  a68_op ("/", m, genie_div_long_mp);
  a68_op ("**", m, genie_pow_long_mp);
  a68_op ("UP", m, genie_pow_long_mp);
  a68_op ("^", m, genie_pow_long_mp);
  m = a68_proc (MODE (REF_LONGLONG_REAL), MODE (REF_LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("+:=", m, genie_plusab_long_mp);
  a68_op ("-:=", m, genie_minusab_long_mp);
  a68_op ("*:=", m, genie_timesab_long_mp);
  a68_op ("/:=", m, genie_divab_long_mp);
  a68_op ("PLUSAB", m, genie_plusab_long_mp);
  a68_op ("MINUSAB", m, genie_minusab_long_mp);
  a68_op ("TIMESAB", m, genie_timesab_long_mp);
  a68_op ("DIVAB", m, genie_divab_long_mp);
  m = a68_proc (MODE (BOOL), MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("=", m, genie_eq_long_mp);
  a68_op ("EQ", m, genie_eq_long_mp);
  a68_op ("/=", m, genie_ne_long_mp);
  a68_op ("~=", m, genie_ne_long_mp);
  a68_op ("NE", m, genie_ne_long_mp);
  a68_op ("<", m, genie_lt_long_mp);
  a68_op ("LT", m, genie_lt_long_mp);
  a68_op ("<=", m, genie_le_long_mp);
  a68_op ("LE", m, genie_le_long_mp);
  a68_op (">", m, genie_gt_long_mp);
  a68_op ("GT", m, genie_gt_long_mp);
  a68_op (">=", m, genie_ge_long_mp);
  a68_op ("GE", m, genie_ge_long_mp);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_mp_int);
  a68_op ("UP", m, genie_pow_long_mp_int);
  a68_op ("^", m, genie_pow_long_mp_int);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("I", m, genie_idle);
  a68_op ("+*", m, genie_idle);
/* LONGLONG COMPLEX. */
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_op ("LENG", m, genie_lengthen_long_complex_to_longlong_complex);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("SHORTEN", m, genie_shorten_longlong_complex_to_long_complex);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("RE", m, genie_re_long_complex);
  a68_op ("IM", m, genie_im_long_complex);
  a68_op ("ARG", m, genie_arg_long_complex);
  a68_op ("ABS", m, genie_abs_long_complex);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("+", m, genie_idle);
  a68_op ("-", m, genie_minus_long_complex);
  a68_op ("CONJ", m, genie_conj_long_complex);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("+", m, genie_add_long_complex);
  a68_op ("-", m, genie_sub_long_complex);
  a68_op ("*", m, genie_mul_long_complex);
  a68_op ("/", m, genie_div_long_complex);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), MODE (INT), NULL);
  a68_op ("**", m, genie_pow_long_complex_int);
  a68_op ("UP", m, genie_pow_long_complex_int);
  a68_op ("^", m, genie_pow_long_complex_int);
  m = a68_proc (MODE (BOOL), MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("=", m, genie_eq_long_complex);
  a68_op ("EQ", m, genie_eq_long_complex);
  a68_op ("/=", m, genie_ne_long_complex);
  a68_op ("~=", m, genie_ne_long_complex);
  a68_op ("NE", m, genie_ne_long_complex);
  m = a68_proc (MODE (REF_LONGLONG_COMPLEX), MODE (REF_LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("+:=", m, genie_plusab_long_complex);
  a68_op ("-:=", m, genie_minusab_long_complex);
  a68_op ("*:=", m, genie_timesab_long_complex);
  a68_op ("/:=", m, genie_divab_long_complex);
  a68_op ("PLUSAB", m, genie_plusab_long_complex);
  a68_op ("MINUSAB", m, genie_minusab_long_complex);
  a68_op ("TIMESAB", m, genie_timesab_long_complex);
  a68_op ("DIVAB", m, genie_divab_long_complex);
/* Some "terminators" to handle the mapping of very short or very long modes.
   This allows you to write SHORT REAL z = SHORTEN pi while everything is
   silently mapped onto REAL. */
  m = a68_proc (MODE (LONGLONG_INT), MODE (LONGLONG_INT), NULL);
  a68_op ("LENG", m, genie_idle);
  m = a68_proc (MODE (LONGLONG_REAL), MODE (LONGLONG_REAL), NULL);
  a68_op ("LENG", m, genie_idle);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_op ("LENG", m, genie_idle);
  m = a68_proc (MODE (LONGLONG_BITS), MODE (LONGLONG_BITS), NULL);
  a68_op ("LENG", m, genie_idle);
  m = a68_proc (MODE (INT), MODE (INT), NULL);
  a68_op ("SHORTEN", m, genie_idle);
  m = a68_proc (MODE (REAL), MODE (REAL), NULL);
  a68_op ("SHORTEN", m, genie_idle);
  m = a68_proc (MODE (COMPLEX), MODE (COMPLEX), NULL);
  a68_op ("SHORTEN", m, genie_idle);
  m = a68_proc (MODE (BITS), MODE (BITS), NULL);
  a68_op ("SHORTEN", m, genie_idle);
/* Vector and matrix. */
  m = a68_proc (MODE (VOID), MODE (REF_ROW_REAL), MODE (REAL), NULL);
  a68_idf ("vectorset", m, genie_vector_set);
  m = a68_proc (MODE (VOID), MODE (REF_ROW_REAL), MODE (ROW_REAL), MODE (REAL), NULL);
  a68_idf ("vectortimesscalar", m, genie_vector_times_scalar);
  m = a68_proc (MODE (VOID), MODE (REF_ROW_REAL), MODE (ROW_REAL), NULL);
  a68_idf ("vectormove", m, genie_vector_move);
  m = a68_proc (MODE (VOID), MODE (REF_ROW_REAL), MODE (ROW_REAL), MODE (ROW_REAL), NULL);
  a68_idf ("vectorplus", m, genie_vector_add);
  a68_idf ("vectorminus", m, genie_vector_sub);
  a68_idf ("vectortimes", m, genie_vector_mul);
  a68_idf ("vectordiv", m, genie_vector_div);
  m = a68_proc (MODE (REAL), MODE (ROW_REAL), MODE (ROW_REAL), NULL);
  a68_idf ("vectorinnerproduct", m, genie_vector_inner_product);
  a68_idf ("vectorinproduct", m, genie_vector_inner_product);
  m = proc_complex_complex;
  a68_idf ("complexsqrt", m, genie_sqrt_complex);
  a68_idf ("csqrt", m, genie_sqrt_complex);
  a68_idf ("complexexp", m, genie_exp_complex);
  a68_idf ("cexp", m, genie_exp_complex);
  a68_idf ("complexln", m, genie_ln_complex);
  a68_idf ("cln", m, genie_ln_complex);
  a68_idf ("complexsin", m, genie_sin_complex);
  a68_idf ("csin", m, genie_sin_complex);
  a68_idf ("complexcos", m, genie_cos_complex);
  a68_idf ("ccos", m, genie_cos_complex);
  a68_idf ("complextan", m, genie_tan_complex);
  a68_idf ("ctan", m, genie_tan_complex);
  a68_idf ("complexasin", m, genie_arcsin_complex);
  a68_idf ("casin", m, genie_arcsin_complex);
  a68_idf ("complexacos", m, genie_arccos_complex);
  a68_idf ("cacos", m, genie_arccos_complex);
  a68_idf ("complexatan", m, genie_arctan_complex);
  a68_idf ("catan", m, genie_arctan_complex);
  a68_idf ("complexarcsin", m, genie_arcsin_complex);
  a68_idf ("carcsin", m, genie_arcsin_complex);
  a68_idf ("complexarccos", m, genie_arccos_complex);
  a68_idf ("carccos", m, genie_arccos_complex);
  a68_idf ("complexarctan", m, genie_arctan_complex);
  a68_idf ("carctan", m, genie_arctan_complex);
  m = a68_proc (MODE (LONG_COMPLEX), MODE (LONG_COMPLEX), NULL);
  a68_idf ("longcomplexsqrt", m, genie_sqrt_long_complex);
  a68_idf ("dcsqrt", m, genie_sqrt_long_complex);
  a68_idf ("longcomplexexp", m, genie_exp_long_complex);
  a68_idf ("dcexp", m, genie_exp_long_complex);
  a68_idf ("longcomplexln", m, genie_ln_long_complex);
  a68_idf ("dcln", m, genie_ln_long_complex);
  a68_idf ("longcomplexsin", m, genie_sin_long_complex);
  a68_idf ("dcsin", m, genie_sin_long_complex);
  a68_idf ("longcomplexcos", m, genie_cos_long_complex);
  a68_idf ("dccos", m, genie_cos_long_complex);
  a68_idf ("longcomplextan", m, genie_tan_long_complex);
  a68_idf ("dctan", m, genie_tan_long_complex);
  a68_idf ("longcomplexarcsin", m, genie_asin_long_complex);
  a68_idf ("dcasin", m, genie_asin_long_complex);
  a68_idf ("longcomplexarccos", m, genie_acos_long_complex);
  a68_idf ("dcacos", m, genie_acos_long_complex);
  a68_idf ("longcomplexarctan", m, genie_atan_long_complex);
  a68_idf ("dcatan", m, genie_atan_long_complex);
  m = a68_proc (MODE (LONGLONG_COMPLEX), MODE (LONGLONG_COMPLEX), NULL);
  a68_idf ("longlongcomplexsqrt", m, genie_sqrt_long_complex);
  a68_idf ("qcsqrt", m, genie_sqrt_long_complex);
  a68_idf ("longlongcomplexexp", m, genie_exp_long_complex);
  a68_idf ("qcexp", m, genie_exp_long_complex);
  a68_idf ("longlongcomplexln", m, genie_ln_long_complex);
  a68_idf ("qcln", m, genie_ln_long_complex);
  a68_idf ("longlongcomplexsin", m, genie_sin_long_complex);
  a68_idf ("qcsin", m, genie_sin_long_complex);
  a68_idf ("longlongcomplexcos", m, genie_cos_long_complex);
  a68_idf ("qccos", m, genie_cos_long_complex);
  a68_idf ("longlongcomplextan", m, genie_tan_long_complex);
  a68_idf ("qctan", m, genie_tan_long_complex);
  a68_idf ("longlongcomplexarcsin", m, genie_asin_long_complex);
  a68_idf ("qcasin", m, genie_asin_long_complex);
  a68_idf ("longlongcomplexarccos", m, genie_acos_long_complex);
  a68_idf ("qcacos", m, genie_acos_long_complex);
  a68_idf ("longlongcomplexarctan", m, genie_atan_long_complex);
  a68_idf ("qcatan", m, genie_atan_long_complex);
/* GNU scientific library. */
#ifdef HAVE_GSL
  a68_idf ("cgsspeedoflight", MODE (REAL), genie_cgs_speed_of_light);
  a68_idf ("cgsgravitationalconstant", MODE (REAL), genie_cgs_gravitational_constant);
  a68_idf ("cgsplanckconstant", MODE (REAL), genie_cgs_planck_constant_h);
  a68_idf ("cgsplanckconstantbar", MODE (REAL), genie_cgs_planck_constant_hbar);
  a68_idf ("cgsastronomicalunit", MODE (REAL), genie_cgs_astronomical_unit);
  a68_idf ("cgslightyear", MODE (REAL), genie_cgs_light_year);
  a68_idf ("cgsparsec", MODE (REAL), genie_cgs_parsec);
  a68_idf ("cgsgravaccel", MODE (REAL), genie_cgs_grav_accel);
  a68_idf ("cgselectronvolt", MODE (REAL), genie_cgs_electron_volt);
  a68_idf ("cgsmasselectron", MODE (REAL), genie_cgs_mass_electron);
  a68_idf ("cgsmassmuon", MODE (REAL), genie_cgs_mass_muon);
  a68_idf ("cgsmassproton", MODE (REAL), genie_cgs_mass_proton);
  a68_idf ("cgsmassneutron", MODE (REAL), genie_cgs_mass_neutron);
  a68_idf ("cgsrydberg", MODE (REAL), genie_cgs_rydberg);
  a68_idf ("cgsboltzmann", MODE (REAL), genie_cgs_boltzmann);
  a68_idf ("cgsbohrmagneton", MODE (REAL), genie_cgs_bohr_magneton);
  a68_idf ("cgsnuclearmagneton", MODE (REAL), genie_cgs_nuclear_magneton);
  a68_idf ("cgselectronmagneticmoment", MODE (REAL), genie_cgs_electron_magnetic_moment);
  a68_idf ("cgsprotonmagneticmoment", MODE (REAL), genie_cgs_proton_magnetic_moment);
  a68_idf ("cgsmolargas", MODE (REAL), genie_cgs_molar_gas);
  a68_idf ("cgsstandardgasvolume", MODE (REAL), genie_cgs_standard_gas_volume);
  a68_idf ("cgsminute", MODE (REAL), genie_cgs_minute);
  a68_idf ("cgshour", MODE (REAL), genie_cgs_hour);
  a68_idf ("cgsday", MODE (REAL), genie_cgs_day);
  a68_idf ("cgsweek", MODE (REAL), genie_cgs_week);
  a68_idf ("cgsinch", MODE (REAL), genie_cgs_inch);
  a68_idf ("cgsfoot", MODE (REAL), genie_cgs_foot);
  a68_idf ("cgsyard", MODE (REAL), genie_cgs_yard);
  a68_idf ("cgsmile", MODE (REAL), genie_cgs_mile);
  a68_idf ("cgsnauticalmile", MODE (REAL), genie_cgs_nautical_mile);
  a68_idf ("cgsfathom", MODE (REAL), genie_cgs_fathom);
  a68_idf ("cgsmil", MODE (REAL), genie_cgs_mil);
  a68_idf ("cgspoint", MODE (REAL), genie_cgs_point);
  a68_idf ("cgstexpoint", MODE (REAL), genie_cgs_texpoint);
  a68_idf ("cgsmicron", MODE (REAL), genie_cgs_micron);
  a68_idf ("cgsangstrom", MODE (REAL), genie_cgs_angstrom);
  a68_idf ("cgshectare", MODE (REAL), genie_cgs_hectare);
  a68_idf ("cgsacre", MODE (REAL), genie_cgs_acre);
  a68_idf ("cgsbarn", MODE (REAL), genie_cgs_barn);
  a68_idf ("cgsliter", MODE (REAL), genie_cgs_liter);
  a68_idf ("cgsusgallon", MODE (REAL), genie_cgs_us_gallon);
  a68_idf ("cgsquart", MODE (REAL), genie_cgs_quart);
  a68_idf ("cgspint", MODE (REAL), genie_cgs_pint);
  a68_idf ("cgscup", MODE (REAL), genie_cgs_cup);
  a68_idf ("cgsfluidounce", MODE (REAL), genie_cgs_fluid_ounce);
  a68_idf ("cgstablespoon", MODE (REAL), genie_cgs_tablespoon);
  a68_idf ("cgsteaspoon", MODE (REAL), genie_cgs_teaspoon);
  a68_idf ("cgscanadiangallon", MODE (REAL), genie_cgs_canadian_gallon);
  a68_idf ("cgsukgallon", MODE (REAL), genie_cgs_uk_gallon);
  a68_idf ("cgsmilesperhour", MODE (REAL), genie_cgs_miles_per_hour);
  a68_idf ("cgskilometersperhour", MODE (REAL), genie_cgs_kilometers_per_hour);
  a68_idf ("cgsknot", MODE (REAL), genie_cgs_knot);
  a68_idf ("cgspoundmass", MODE (REAL), genie_cgs_pound_mass);
  a68_idf ("cgsouncemass", MODE (REAL), genie_cgs_ounce_mass);
  a68_idf ("cgston", MODE (REAL), genie_cgs_ton);
  a68_idf ("cgsmetricton", MODE (REAL), genie_cgs_metric_ton);
  a68_idf ("cgsukton", MODE (REAL), genie_cgs_uk_ton);
  a68_idf ("cgstroyounce", MODE (REAL), genie_cgs_troy_ounce);
  a68_idf ("cgscarat", MODE (REAL), genie_cgs_carat);
  a68_idf ("cgsunifiedatomicmass", MODE (REAL), genie_cgs_unified_atomic_mass);
  a68_idf ("cgsgramforce", MODE (REAL), genie_cgs_gram_force);
  a68_idf ("cgspoundforce", MODE (REAL), genie_cgs_pound_force);
  a68_idf ("cgskilopoundforce", MODE (REAL), genie_cgs_kilopound_force);
  a68_idf ("cgspoundal", MODE (REAL), genie_cgs_poundal);
  a68_idf ("cgscalorie", MODE (REAL), genie_cgs_calorie);
  a68_idf ("cgsbtu", MODE (REAL), genie_cgs_btu);
  a68_idf ("cgstherm", MODE (REAL), genie_cgs_therm);
  a68_idf ("cgshorsepower", MODE (REAL), genie_cgs_horsepower);
  a68_idf ("cgsbar", MODE (REAL), genie_cgs_bar);
  a68_idf ("cgsstdatmosphere", MODE (REAL), genie_cgs_std_atmosphere);
  a68_idf ("cgstorr", MODE (REAL), genie_cgs_torr);
  a68_idf ("cgsmeterofmercury", MODE (REAL), genie_cgs_meter_of_mercury);
  a68_idf ("cgsinchofmercury", MODE (REAL), genie_cgs_inch_of_mercury);
  a68_idf ("cgsinchofwater", MODE (REAL), genie_cgs_inch_of_water);
  a68_idf ("cgspsi", MODE (REAL), genie_cgs_psi);
  a68_idf ("cgspoise", MODE (REAL), genie_cgs_poise);
  a68_idf ("cgsstokes", MODE (REAL), genie_cgs_stokes);
  a68_idf ("cgsfaraday", MODE (REAL), genie_cgs_faraday);
  a68_idf ("cgselectroncharge", MODE (REAL), genie_cgs_electron_charge);
  a68_idf ("cgsgauss", MODE (REAL), genie_cgs_gauss);
  a68_idf ("cgsstilb", MODE (REAL), genie_cgs_stilb);
  a68_idf ("cgslumen", MODE (REAL), genie_cgs_lumen);
  a68_idf ("cgslux", MODE (REAL), genie_cgs_lux);
  a68_idf ("cgsphot", MODE (REAL), genie_cgs_phot);
  a68_idf ("cgsfootcandle", MODE (REAL), genie_cgs_footcandle);
  a68_idf ("cgslambert", MODE (REAL), genie_cgs_lambert);
  a68_idf ("cgsfootlambert", MODE (REAL), genie_cgs_footlambert);
  a68_idf ("cgscurie", MODE (REAL), genie_cgs_curie);
  a68_idf ("cgsroentgen", MODE (REAL), genie_cgs_roentgen);
  a68_idf ("cgsrad", MODE (REAL), genie_cgs_rad);
  a68_idf ("cgssolarmass", MODE (REAL), genie_cgs_solar_mass);
  a68_idf ("cgsbohrradius", MODE (REAL), genie_cgs_bohr_radius);
  a68_idf ("cgsnewton", MODE (REAL), genie_cgs_newton);
  a68_idf ("cgsdyne", MODE (REAL), genie_cgs_dyne);
  a68_idf ("cgsjoule", MODE (REAL), genie_cgs_joule);
  a68_idf ("cgserg", MODE (REAL), genie_cgs_erg);
  a68_idf ("mksaspeedoflight", MODE (REAL), genie_mks_speed_of_light);
  a68_idf ("mksagravitationalconstant", MODE (REAL), genie_mks_gravitational_constant);
  a68_idf ("mksaplanckconstant", MODE (REAL), genie_mks_planck_constant_h);
  a68_idf ("mksaplanckconstantbar", MODE (REAL), genie_mks_planck_constant_hbar);
  a68_idf ("mksavacuumpermeability", MODE (REAL), genie_mks_vacuum_permeability);
  a68_idf ("mksaastronomicalunit", MODE (REAL), genie_mks_astronomical_unit);
  a68_idf ("mksalightyear", MODE (REAL), genie_mks_light_year);
  a68_idf ("mksaparsec", MODE (REAL), genie_mks_parsec);
  a68_idf ("mksagravaccel", MODE (REAL), genie_mks_grav_accel);
  a68_idf ("mksaelectronvolt", MODE (REAL), genie_mks_electron_volt);
  a68_idf ("mksamasselectron", MODE (REAL), genie_mks_mass_electron);
  a68_idf ("mksamassmuon", MODE (REAL), genie_mks_mass_muon);
  a68_idf ("mksamassproton", MODE (REAL), genie_mks_mass_proton);
  a68_idf ("mksamassneutron", MODE (REAL), genie_mks_mass_neutron);
  a68_idf ("mksarydberg", MODE (REAL), genie_mks_rydberg);
  a68_idf ("mksaboltzmann", MODE (REAL), genie_mks_boltzmann);
  a68_idf ("mksabohrmagneton", MODE (REAL), genie_mks_bohr_magneton);
  a68_idf ("mksanuclearmagneton", MODE (REAL), genie_mks_nuclear_magneton);
  a68_idf ("mksaelectronmagneticmoment", MODE (REAL), genie_mks_electron_magnetic_moment);
  a68_idf ("mksaprotonmagneticmoment", MODE (REAL), genie_mks_proton_magnetic_moment);
  a68_idf ("mksamolargas", MODE (REAL), genie_mks_molar_gas);
  a68_idf ("mksastandardgasvolume", MODE (REAL), genie_mks_standard_gas_volume);
  a68_idf ("mksaminute", MODE (REAL), genie_mks_minute);
  a68_idf ("mksahour", MODE (REAL), genie_mks_hour);
  a68_idf ("mksaday", MODE (REAL), genie_mks_day);
  a68_idf ("mksaweek", MODE (REAL), genie_mks_week);
  a68_idf ("mksainch", MODE (REAL), genie_mks_inch);
  a68_idf ("mksafoot", MODE (REAL), genie_mks_foot);
  a68_idf ("mksayard", MODE (REAL), genie_mks_yard);
  a68_idf ("mksamile", MODE (REAL), genie_mks_mile);
  a68_idf ("mksanauticalmile", MODE (REAL), genie_mks_nautical_mile);
  a68_idf ("mksafathom", MODE (REAL), genie_mks_fathom);
  a68_idf ("mksamil", MODE (REAL), genie_mks_mil);
  a68_idf ("mksapoint", MODE (REAL), genie_mks_point);
  a68_idf ("mksatexpoint", MODE (REAL), genie_mks_texpoint);
  a68_idf ("mksamicron", MODE (REAL), genie_mks_micron);
  a68_idf ("mksaangstrom", MODE (REAL), genie_mks_angstrom);
  a68_idf ("mksahectare", MODE (REAL), genie_mks_hectare);
  a68_idf ("mksaacre", MODE (REAL), genie_mks_acre);
  a68_idf ("mksabarn", MODE (REAL), genie_mks_barn);
  a68_idf ("mksaliter", MODE (REAL), genie_mks_liter);
  a68_idf ("mksausgallon", MODE (REAL), genie_mks_us_gallon);
  a68_idf ("mksaquart", MODE (REAL), genie_mks_quart);
  a68_idf ("mksapint", MODE (REAL), genie_mks_pint);
  a68_idf ("mksacup", MODE (REAL), genie_mks_cup);
  a68_idf ("mksafluidounce", MODE (REAL), genie_mks_fluid_ounce);
  a68_idf ("mksatablespoon", MODE (REAL), genie_mks_tablespoon);
  a68_idf ("mksateaspoon", MODE (REAL), genie_mks_teaspoon);
  a68_idf ("mksacanadiangallon", MODE (REAL), genie_mks_canadian_gallon);
  a68_idf ("mksaukgallon", MODE (REAL), genie_mks_uk_gallon);
  a68_idf ("mksamilesperhour", MODE (REAL), genie_mks_miles_per_hour);
  a68_idf ("mksakilometersperhour", MODE (REAL), genie_mks_kilometers_per_hour);
  a68_idf ("mksaknot", MODE (REAL), genie_mks_knot);
  a68_idf ("mksapoundmass", MODE (REAL), genie_mks_pound_mass);
  a68_idf ("mksaouncemass", MODE (REAL), genie_mks_ounce_mass);
  a68_idf ("mksaton", MODE (REAL), genie_mks_ton);
  a68_idf ("mksametricton", MODE (REAL), genie_mks_metric_ton);
  a68_idf ("mksaukton", MODE (REAL), genie_mks_uk_ton);
  a68_idf ("mksatroyounce", MODE (REAL), genie_mks_troy_ounce);
  a68_idf ("mksacarat", MODE (REAL), genie_mks_carat);
  a68_idf ("mksaunifiedatomicmass", MODE (REAL), genie_mks_unified_atomic_mass);
  a68_idf ("mksagramforce", MODE (REAL), genie_mks_gram_force);
  a68_idf ("mksapoundforce", MODE (REAL), genie_mks_pound_force);
  a68_idf ("mksakilopoundforce", MODE (REAL), genie_mks_kilopound_force);
  a68_idf ("mksapoundal", MODE (REAL), genie_mks_poundal);
  a68_idf ("mksacalorie", MODE (REAL), genie_mks_calorie);
  a68_idf ("mksabtu", MODE (REAL), genie_mks_btu);
  a68_idf ("mksatherm", MODE (REAL), genie_mks_therm);
  a68_idf ("mksahorsepower", MODE (REAL), genie_mks_horsepower);
  a68_idf ("mksabar", MODE (REAL), genie_mks_bar);
  a68_idf ("mksastdatmosphere", MODE (REAL), genie_mks_std_atmosphere);
  a68_idf ("mksatorr", MODE (REAL), genie_mks_torr);
  a68_idf ("mksameterofmercury", MODE (REAL), genie_mks_meter_of_mercury);
  a68_idf ("mksainchofmercury", MODE (REAL), genie_mks_inch_of_mercury);
  a68_idf ("mksainchofwater", MODE (REAL), genie_mks_inch_of_water);
  a68_idf ("mksapsi", MODE (REAL), genie_mks_psi);
  a68_idf ("mksapoise", MODE (REAL), genie_mks_poise);
  a68_idf ("mksastokes", MODE (REAL), genie_mks_stokes);
  a68_idf ("mksafaraday", MODE (REAL), genie_mks_faraday);
  a68_idf ("mksaelectroncharge", MODE (REAL), genie_mks_electron_charge);
  a68_idf ("mksagauss", MODE (REAL), genie_mks_gauss);
  a68_idf ("mksastilb", MODE (REAL), genie_mks_stilb);
  a68_idf ("mksalumen", MODE (REAL), genie_mks_lumen);
  a68_idf ("mksalux", MODE (REAL), genie_mks_lux);
  a68_idf ("mksaphot", MODE (REAL), genie_mks_phot);
  a68_idf ("mksafootcandle", MODE (REAL), genie_mks_footcandle);
  a68_idf ("mksalambert", MODE (REAL), genie_mks_lambert);
  a68_idf ("mksafootlambert", MODE (REAL), genie_mks_footlambert);
  a68_idf ("mksacurie", MODE (REAL), genie_mks_curie);
  a68_idf ("mksaroentgen", MODE (REAL), genie_mks_roentgen);
  a68_idf ("mksarad", MODE (REAL), genie_mks_rad);
  a68_idf ("mksasolarmass", MODE (REAL), genie_mks_solar_mass);
  a68_idf ("mksabohrradius", MODE (REAL), genie_mks_bohr_radius);
  a68_idf ("mksavacuumpermittivity", MODE (REAL), genie_mks_vacuum_permittivity);
  a68_idf ("mksanewton", MODE (REAL), genie_mks_newton);
  a68_idf ("mksadyne", MODE (REAL), genie_mks_dyne);
  a68_idf ("mksajoule", MODE (REAL), genie_mks_joule);
  a68_idf ("mksaerg", MODE (REAL), genie_mks_erg);
  a68_idf ("numfinestructure", MODE (REAL), genie_num_fine_structure);
  a68_idf ("numavogadro", MODE (REAL), genie_num_avogadro);
  a68_idf ("numyotta", MODE (REAL), genie_num_yotta);
  a68_idf ("numzetta", MODE (REAL), genie_num_zetta);
  a68_idf ("numexa", MODE (REAL), genie_num_exa);
  a68_idf ("numpeta", MODE (REAL), genie_num_peta);
  a68_idf ("numtera", MODE (REAL), genie_num_tera);
  a68_idf ("numgiga", MODE (REAL), genie_num_giga);
  a68_idf ("nummega", MODE (REAL), genie_num_mega);
  a68_idf ("numkilo", MODE (REAL), genie_num_kilo);
  a68_idf ("nummilli", MODE (REAL), genie_num_milli);
  a68_idf ("nummicro", MODE (REAL), genie_num_micro);
  a68_idf ("numnano", MODE (REAL), genie_num_nano);
  a68_idf ("numpico", MODE (REAL), genie_num_pico);
  a68_idf ("numfemto", MODE (REAL), genie_num_femto);
  a68_idf ("numatto", MODE (REAL), genie_num_atto);
  a68_idf ("numzepto", MODE (REAL), genie_num_zepto);
  a68_idf ("numyocto", MODE (REAL), genie_num_yocto);
  m = proc_real_real;
  a68_idf ("erf", m, genie_erf_real);
  a68_idf ("erfc", m, genie_erfc_real);
  a68_idf ("gamma", m, genie_gamma_real);
  a68_idf ("lngamma", m, genie_lngamma_real);
  a68_idf ("factorial", m, genie_factorial_real);
  a68_idf ("airyai", m, genie_airy_ai_real);
  a68_idf ("airybi", m, genie_airy_bi_real);
  a68_idf ("airyaiderivative", m, genie_airy_ai_deriv_real);
  a68_idf ("airybiderivative", m, genie_airy_bi_deriv_real);
  a68_idf ("ellipticintegralk", m, genie_elliptic_integral_k_real);
  a68_idf ("ellipticintegrale", m, genie_elliptic_integral_e_real);
  m = proc_real_real_real;
  a68_idf ("beta", m, genie_beta_real);
  a68_idf ("besseljn", m, genie_bessel_jn_real);
  a68_idf ("besselyn", m, genie_bessel_yn_real);
  a68_idf ("besselin", m, genie_bessel_in_real);
  a68_idf ("besselexpin", m, genie_bessel_exp_in_real);
  a68_idf ("besselkn", m, genie_bessel_kn_real);
  a68_idf ("besselexpkn", m, genie_bessel_exp_kn_real);
  a68_idf ("besseljl", m, genie_bessel_jl_real);
  a68_idf ("besselyl", m, genie_bessel_yl_real);
  a68_idf ("besselexpil", m, genie_bessel_exp_il_real);
  a68_idf ("besselexpkl", m, genie_bessel_exp_kl_real);
  a68_idf ("besseljnu", m, genie_bessel_jnu_real);
  a68_idf ("besselynu", m, genie_bessel_ynu_real);
  a68_idf ("besselinu", m, genie_bessel_inu_real);
  a68_idf ("besselexpinu", m, genie_bessel_exp_inu_real);
  a68_idf ("besselknu", m, genie_bessel_knu_real);
  a68_idf ("besselexpknu", m, genie_bessel_exp_knu_real);
  a68_idf ("ellipticintegralrc", m, genie_elliptic_integral_rc_real);
  a68_idf ("incompletegamma", m, genie_gamma_inc_real);
  m = a68_proc (MODE (REAL), MODE (REAL), MODE (REAL), MODE (REAL), NULL);
  a68_idf ("incompletebeta", m, genie_beta_inc_real);
  a68_idf ("ellipticintegralrf", m, genie_elliptic_integral_rf_real);
  a68_idf ("ellipticintegralrd", m, genie_elliptic_integral_rd_real);
  m = a68_proc (MODE (REAL), MODE (REAL), MODE (REAL), MODE (REAL), MODE (REAL), NULL);
  a68_idf ("ellipticintegralrj", m, genie_elliptic_integral_rj_real);
#endif
#ifdef HAVE_UNIX
  m = proc_int;
  a68_idf ("argc", m, genie_argc);
  a68_idf ("errno", m, genie_errno);
  a68_idf ("fork", m, genie_fork);
  m = a68_proc (MODE (STRING), MODE (INT), NULL);
  a68_idf ("argv", m, genie_argv);
  m = proc_void;
  a68_idf ("reseterrno", m, genie_reset_errno);
  m = a68_proc (MODE (STRING), MODE (INT), NULL);
  a68_idf ("strerror", m, genie_strerror);
  m = a68_proc (MODE (INT), MODE (STRING), MODE (ROW_STRING), MODE (ROW_STRING), NULL);
  a68_idf ("execve", m, genie_execve);
  m = a68_proc (MODE (PIPE), NULL);
  a68_idf ("createpipe", m, genie_create_pipe);
  m = a68_proc (MODE (INT), MODE (STRING), MODE (ROW_STRING), MODE (ROW_STRING), NULL);
  a68_idf ("execvechild", m, genie_execve_child);
  m = a68_proc (MODE (PIPE), MODE (STRING), MODE (ROW_STRING), MODE (ROW_STRING), NULL);
  a68_idf ("execvechildpipe", m, genie_execve_child_pipe);
  m = a68_proc (MODE (STRING), MODE (STRING), NULL);
  a68_idf ("getenv", m, genie_getenv);
  m = a68_proc (MODE (VOID), MODE (INT), NULL);
  a68_idf ("waitpid", m, genie_waitpid);
#ifdef HAVE_HTTP
  m = a68_proc (MODE (INT), MODE (REF_STRING), MODE (STRING), MODE (STRING), MODE (INT), NULL);
  a68_idf ("httpcontent", m, genie_http_content);
  a68_idf ("tcprequest", m, genie_tcp_request);
#endif
#ifdef HAVE_REGEX
  m = a68_proc (MODE (INT), MODE (STRING), MODE (STRING), MODE (REF_INT), MODE (REF_INT), NULL);
  a68_idf ("grepinstring", m, genie_grep_in_string);
#endif
#endif
#ifdef HAVE_CURSES
  m = proc_void;
  a68_idf ("cursesstart", m, genie_curses_start);
  a68_idf ("cursesend", m, genie_curses_end);
  a68_idf ("cursesclear", m, genie_curses_clear);
  a68_idf ("cursesrefresh", m, genie_curses_refresh);
  m = proc_char;
  a68_idf ("cursesgetchar", m, genie_curses_getchar);
  m = a68_proc (MODE (VOID), MODE (CHAR), NULL);
  a68_idf ("cursesputchar", m, genie_curses_putchar);
  m = a68_proc (MODE (VOID), MODE (INT), MODE (INT), NULL);
  a68_idf ("cursesmove", m, genie_curses_move);
  m = proc_int;
  a68_idf ("curseslines", m, genie_curses_lines);
  a68_idf ("cursescolumns", m, genie_curses_columns);
#endif
}
