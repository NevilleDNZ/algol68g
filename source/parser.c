/*-------1---------2---------3---------4---------5---------6---------7---------+
This file is part of Algol68G - an Algol 68 interpreter.
Copyright (C) 2001-2004 J. Marcel van der Veer <algol68g@xs4all.nl>.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful,but WITHOUT ANY 
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.                      */

/*-------1---------2---------3---------4---------5---------6---------7---------+
This file contains a hand-coded parser for Algol 68. 

Parsing progresses in various phases to avoid spurious diagnostics from a 
recovering parser. Every phase "tightens" the grammar more.
An error in any phase makes the parser quit when that phase ends.
The parser is forgiving in case of superfluous semicolons.

These are the phases:

 (1) Parenthesis are checked to see whether they match.

 (2) Then, a top-down parser determines the basic-block structure of the program
     so symbol tables can be set up that the bottom-up parser will consult
     as you can define things before they are applied.

 (3) A bottom-up parser tries to resolve the structure of the program.

 (4) After the symbol tables have been finalised, a small rearrangement of the
     tree may be required where JUMPs have no GOTO. This leads to the
     non-standard situation that JUMPs without GOTO can have the syntactic
     position of a PRIMARY, SECONDARY or TERTIARY. The mode checker will reject
     such constructs later on.

 (5) The bottom-up parser does not check VICTAL correctness of declarers. This is 
     done separately. Also structure of a FORMAT_TEXT is checked separately.
                                                                                */

#include "algol68g.h"

static jmp_buf bottom_up_crash_exit, top_down_crash_exit;

static BOOL_T reduce_phrase (NODE_T *, int);
static BOOL_T victal_check_declarer (NODE_T *, int);
static NODE_T *reduce_dyadic (NODE_T *, int u);
static NODE_T *top_down_loop (NODE_T *);
static NODE_T *top_down_skip_unit (NODE_T *);
static void elaborate_bold_tags (NODE_T *);
static void extract_declarations (NODE_T *);
static void extract_identities (NODE_T *);
static void extract_indicants (NODE_T *);
static void extract_labels (NODE_T *, int);
static void extract_operators (NODE_T *);
static void extract_priorities (NODE_T *);
static void extract_proc_identities (NODE_T *);
static void extract_proc_variables (NODE_T *);
static void extract_variables (NODE_T *);
static void f (NODE_T *, void (*)(NODE_T *), BOOL_T *, ...);
static void ignore_superfluous_semicolons (NODE_T *, int);
static void recover_from_error (NODE_T *, int, BOOL_T);
static void reduce_arguments (NODE_T *);
static void reduce_basic_declarations (NODE_T *);
static void reduce_bounds (NODE_T *);
static void reduce_collateral_clauses (NODE_T *);
static void reduce_constructs (NODE_T *, int);
static void reduce_control_structure (NODE_T *, int);
static void reduce_declaration_lists (NODE_T *);
static void reduce_declarer_lists (NODE_T *);
static void reduce_declarers (NODE_T *, int);
static void reduce_deeper_clauses (NODE_T *);
static void reduce_deeper_clauses_driver (NODE_T *);
static void reduce_enclosed_clause_bits (NODE_T *, int);
static void reduce_enclosed_clauses (NODE_T *);
static void reduce_enquiry_clauses (NODE_T *);
static void reduce_erroneous_units (NODE_T *);
static void reduce_formal_declarer_pack (NODE_T *);
static void reduce_format_texts (NODE_T *);
static void reduce_formulae (NODE_T *);
static void reduce_generic_arguments (NODE_T *);
static void reduce_indicants (NODE_T *);
static void reduce_labels (NODE_T *);
static void reduce_lengtheties (NODE_T *);
static void reduce_parameter_pack (NODE_T *);
static void reduce_particular_program (NODE_T *);
static void reduce_primaries (NODE_T *, int);
static void reduce_primary_bits (NODE_T *, int);
static void reduce_qualifiers (NODE_T *);
static void reduce_right_to_left_constructs (NODE_T * q);
static void reduce_row_proc_op_declarers (NODE_T *);
static void reduce_secondaries (NODE_T *);
static void reduce_serial_clauses (NODE_T *);
static void reduce_small_declarers (NODE_T *);
static void reduce_specifiers (NODE_T *);
static void reduce_statements (NODE_T *, int);
static void reduce_struct_pack (NODE_T *);
static void reduce_subordinate (NODE_T *, int);
static void reduce_tertiaries (NODE_T *);
static void reduce_union_pack (NODE_T *);
static void reduce_units (NODE_T *);
static void signal_wrong_exits (NODE_T *, int);

/*-------1---------2---------3---------4---------5---------6---------7---------+
insert_node.                                                                  */

static void
insert_node (NODE_T * p, int att)
{
  NODE_T *q = new_node ();
  *q = *p;
  ATTRIBUTE (q) = att;
  NEXT (q) = NEXT (p);
  NEXT (p) = q;
  PREVIOUS (q) = p;
  if (NEXT (q) != NULL)
    {
      PREVIOUS (NEXT (q)) = q;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
substitute_brackets.                                                          */

void
substitute_brackets (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      substitute_brackets (SUB (p));
      switch (ATTRIBUTE (p))
	{
	case ACCO_SYMBOL:
	  ATTRIBUTE (p) = OPEN_SYMBOL;
	  break;
	case OCCA_SYMBOL:
	  ATTRIBUTE (p) = CLOSE_SYMBOL;
	  break;
	case SUB_SYMBOL:
	  ATTRIBUTE (p) = OPEN_SYMBOL;
	  break;
	case BUS_SYMBOL:
	  ATTRIBUTE (p) = CLOSE_SYMBOL;
	  break;
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_unit_terminator: whether token terminates a unit.                     */

static int
whether_unit_terminator (NODE_T * p)
{
  switch (ATTRIBUTE (p))
    {
    case BUS_SYMBOL:
    case CLOSE_SYMBOL:
    case END_SYMBOL:
    case SEMI_SYMBOL:
    case EXIT_SYMBOL:
    case COMMA_SYMBOL:
    case THEN_BAR_SYMBOL:
    case ELSE_BAR_SYMBOL:
    case THEN_SYMBOL:
    case ELIF_SYMBOL:
    case ELSE_SYMBOL:
    case FI_SYMBOL:
    case IN_SYMBOL:
    case OUT_SYMBOL:
    case OUSE_SYMBOL:
    case ESAC_SYMBOL:
    case FED_SYMBOL:
    case EDOC_SYMBOL:
    case OCCA_SYMBOL:
      {
	return (ATTRIBUTE (p));
      }
    default:
      {
	return (0);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_loop_keyword: whether token is a unit-terminator in a loop clause. */

static BOOL_T
whether_loop_keyword (NODE_T * p)
{
  switch (ATTRIBUTE (p))
    {
    case FOR_SYMBOL:
    case FROM_SYMBOL:
    case BY_SYMBOL:
    case TO_SYMBOL:
    case WHILE_SYMBOL:
    case DO_SYMBOL:
      {
	return (ATTRIBUTE (p));
      }
    default:
      {
	return (0);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_semicolon_less: whether token cannot follow semicolon or EXIT.        */

static int
whether_semicolon_less (NODE_T * p)
{
  switch (ATTRIBUTE (p))
    {
    case BUS_SYMBOL:
    case CLOSE_SYMBOL:
    case END_SYMBOL:
    case SEMI_SYMBOL:
    case EXIT_SYMBOL:
    case THEN_BAR_SYMBOL:
    case ELSE_BAR_SYMBOL:
    case THEN_SYMBOL:
    case ELIF_SYMBOL:
    case ELSE_SYMBOL:
    case FI_SYMBOL:
    case IN_SYMBOL:
    case OUT_SYMBOL:
    case OUSE_SYMBOL:
    case ESAC_SYMBOL:
    case FED_SYMBOL:
    case EDOC_SYMBOL:
    case OCCA_SYMBOL:
    case OD_SYMBOL:
      {
	return (ATTRIBUTE (p));
      }
    default:
      {
	return (0);
      }
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
phrase_to_text: return intelligible diagnostic from syntax tree branch        */

static char *
phrase_to_text (NODE_T * p, NODE_T * q)
{
#define MAX_TERMINALS 8
  int length = 0, count = 0;
  static char buffer[CMS_LINE_SIZE];
  buffer[0] = '\0';
  for (; p != NULL && (q != NULL ? p != NEXT (q) : count < MAX_TERMINALS) && length < CMS_LINE_SIZE / 2; p = NEXT (p))
    {
      char *z = non_terminal_string (edit_line, ATTRIBUTE (p));
      if (strlen (buffer) > 1)
	{
	  strcat (buffer, ", ");
	}
      if (z != NULL)
	{
	  strcat (buffer, z);
	}
      else if (SYMBOL (p) != NULL)
	{
	  sprintf (edit_line, "`%s'", SYMBOL (p));
	  strcat (buffer, edit_line);
	}
      count++;
      length = strlen (buffer);
    }
  if (p != NULL && (q == NULL ? A_FALSE : count == MAX_TERMINALS))
    {
      strcat (buffer, " ..");
    }
  return (buffer);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
This is a parenthesis checker. After this checker, we know that at least
brackets are matched. This stabilises later parser phases.
Top-down parsing is done to place error messages near offending lines.        */

/*-------1---------2---------3---------4---------5---------6---------7---------+
bracket_check_error: synthesise intelligible messages for the bracket checker.*/

static char bracket_check_error_text[BUFFER_SIZE];

static void
bracket_check_error (char *txt, int n, char *bra, char *ket)
{
  if (n != 0)
    {
      char b[BUFFER_SIZE];
      sprintf (b, "`%s' without matching `%s'", (n > 0 ? bra : ket), (n > 0 ? ket : bra));
      if (strlen (txt) > 0)
	{
	  strcat (txt, ", ");
	}
      strcat (txt, b);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bracket_check_diagnose: diagnose brackets in local branch of the tree.        */

static char *
bracket_check_diagnose (NODE_T * p)
{
  int begins = 0, opens = 0, format_opens = 0, subs = 0, ifs = 0, cases = 0, dos = 0, accos = 0;
  for (; p != NULL; p = NEXT (p))
    {
      switch (ATTRIBUTE (p))
	{
	case BEGIN_SYMBOL:
	  begins++;
	  break;
	case END_SYMBOL:
	  begins--;
	  break;
	case OPEN_SYMBOL:
	  opens++;
	  break;
	case CLOSE_SYMBOL:
	  opens--;
	  break;
	case ACCO_SYMBOL:
	  accos++;
	  break;
	case OCCA_SYMBOL:
	  accos--;
	  break;
	case FORMAT_ITEM_OPEN:
	  format_opens++;
	  break;
	case FORMAT_ITEM_CLOSE:
	  format_opens--;
	  break;
	case SUB_SYMBOL:
	  subs++;
	  break;
	case BUS_SYMBOL:
	  subs--;
	  break;
	case IF_SYMBOL:
	  ifs++;
	  break;
	case FI_SYMBOL:
	  ifs--;
	  break;
	case CASE_SYMBOL:
	  cases++;
	  break;
	case ESAC_SYMBOL:
	  cases--;
	  break;
	case DO_SYMBOL:
	  dos++;
	  break;
	case OD_SYMBOL:
	  dos--;
	  break;
	}
    }
  bracket_check_error_text[0] = '\0';
  bracket_check_error (bracket_check_error_text, begins, "BEGIN", "END");
  bracket_check_error (bracket_check_error_text, opens, "(", ")");
  bracket_check_error (bracket_check_error_text, format_opens, "(", ")");
  bracket_check_error (bracket_check_error_text, accos, "{", "}");
  bracket_check_error (bracket_check_error_text, subs, "[", "]");
  bracket_check_error (bracket_check_error_text, ifs, "IF", "FI");
  bracket_check_error (bracket_check_error_text, cases, "CASE", "ESAC");
  bracket_check_error (bracket_check_error_text, dos, "DO", "OD");
  return (bracket_check_error_text);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bracket_check_parse: driver for locally diagnosing non-matching tokens.
returns: token from where to proceed.                                         */

static NODE_T *
bracket_check_parse (NODE_T * top, NODE_T * p)
{
  while (p != NULL)
    {
      int ket;
      NODE_T *q;
      switch (ATTRIBUTE (p))
	{
	case BEGIN_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = END_SYMBOL;
	  break;
	case OPEN_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = CLOSE_SYMBOL;
	  break;
	case ACCO_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = OCCA_SYMBOL;
	  break;
	case FORMAT_ITEM_OPEN:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = FORMAT_ITEM_CLOSE;
	  break;
	case SUB_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = BUS_SYMBOL;
	  break;
	case IF_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = FI_SYMBOL;
	  break;
	case CASE_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = ESAC_SYMBOL;
	  break;
	case DO_SYMBOL:
	  q = bracket_check_parse (top, NEXT (p));
	  ket = OD_SYMBOL;
	  break;
	case END_SYMBOL:
	case OCCA_SYMBOL:
	case CLOSE_SYMBOL:
	case FORMAT_ITEM_CLOSE:
	case BUS_SYMBOL:
	case FI_SYMBOL:
	case ESAC_SYMBOL:
	case OD_SYMBOL:
	  return (p);
	default:
	  goto next;
	}
      if (q == NULL || WHETHER_NOT (q, ket))
	{
	  char *diag = bracket_check_diagnose (top);
	  diagnostic (A_SYNTAX_ERROR, p, PARENTHESIS_ERROR, strlen (diag) > 0 ? diag : "missing or unmatched keywords");
	  longjmp (top_down_crash_exit, 1);
	}
      p = q;
    next:
      if (p != NULL)
	{
	  p = NEXT (p);
	}
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_parenthesis: driver for globally diagnosing non-matching tokens.        */

void
check_parenthesis (NODE_T * top)
{
  if (!setjmp (top_down_crash_exit))
    {
      if (bracket_check_parse (top, top) != NULL)
	{
	  diagnostic (A_SYNTAX_ERROR, top, PARENTHESIS_ERROR, "missing or unmatched keywords", NULL);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Next is a top-down parser that branches out the basic blocks.
After this we can assign symbol tables to basic blocks.                       */

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_diagnose: give diagnose from top-down parser.                    
start: embedding clause starts here.
where: error issued at this point.
clause: type of clause processing.
expected: token expected.                                                     */

static void
top_down_diagnose (NODE_T * start, NODE_T * where, int clause, int expected)
{
  NODE_T *issue = (where != NULL ? where : start);
  if (expected != 0)
    {
      diagnostic (A_SYNTAX_ERROR, issue, "B expected in A near S L", expected, clause, start, start->info->line, NULL);
    }
  else
    {
      diagnostic (A_SYNTAX_ERROR, issue, "missing or unbalanced keyword in A near S L", clause, start, start->info->line, NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
tokens_exhausted: check for premature exhaustion of tokens.                   */

static void
tokens_exhausted (NODE_T * p, NODE_T * q)
{
  if (p == NULL)
    {
      diagnostic (A_SYNTAX_ERROR, q, KEYWORD_ERROR);
      longjmp (top_down_crash_exit, 1);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
This part specifically branches out loop clauses.                             */

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_loop_cast_formula : whether in cast or formula with loop clause.
Accept declarers that can appear in such casts but not much more.
Return number of symbols to skip.                                             */

static int
whether_loop_cast_formula (NODE_T * p)
{
  if (WHETHER (p, VOID_SYMBOL))
    {
      return (1);
    }
  else if (WHETHER (p, INT_SYMBOL))
    {
      return (1);
    }
  else if (WHETHER (p, REF_SYMBOL))
    {
      return (1);
    }
  else if (WHETHER (p, OPERATOR) || WHETHER (p, BOLD_TAG))
    {
      return (1);
    }
  else if (whether (p, UNION_SYMBOL, OPEN_SYMBOL, 0))
    {
      return (2);
    }
  else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, SUB_SYMBOL))
    {
      int k = 0;
      while (p != NULL && (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, SUB_SYMBOL)))
	{
	  p = NEXT (p);
	  k++;
	}
      return (p != NULL && whether (p, UNION_SYMBOL, OPEN_SYMBOL, 0) ? k : 0);
    }
  return (0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_skip_loop_unit: skip a unit in a loop clause (FROM u BY u TO u).
return: token from where to proceed                                           */

static NODE_T *
top_down_skip_loop_unit (NODE_T * p)
{
/* Unit may start with, or consist of, a loop. */
  if (whether_loop_keyword (p))
    {
      p = top_down_loop (p);
    }
/* Skip rest of unit. */
  while (p != NULL)
    {
      int k = whether_loop_cast_formula (p);
      if (k != 0)
	{
/* operator-cast series ... */
	  while (p != NULL && k != 0)
	    {
	      while (k != 0)
		{
		  p = NEXT (p);
		  k--;
		}
	      k = whether_loop_cast_formula (p);
	    }
/* ... may be followed by a loop clause. */
	  if (whether_loop_keyword (p))
	    {
	      p = top_down_loop (p);
	    }
	}
      else if (whether_loop_keyword (p) || WHETHER (p, OD_SYMBOL))
	{
/* new loop or end-of-loop. */
	  return (p);
	}
      else if (WHETHER (p, COLON_SYMBOL))
	{
	  p = NEXT (p);
/* skip routine header: loop clause */
	  if (p != NULL && whether_loop_keyword (p))
	    {
	      p = top_down_loop (p);
	    }
	}
      else if (WHETHER (p, SEMI_SYMBOL) || WHETHER (p, COMMA_SYMBOL) || WHETHER (p, EXIT_SYMBOL))
	{
/* Statement separators. */
	  return (p);
	}
      else
	{
	  p = NEXT (p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_skip_loop_series: skip a loop clause.
return: token from where to proceed.                                          */

static NODE_T *
top_down_skip_loop_series (NODE_T * p)
{
  BOOL_T z;
  do
    {
      p = top_down_skip_loop_unit (p);
      z = (p != NULL && (WHETHER (p, SEMI_SYMBOL) || WHETHER (p, EXIT_SYMBOL) || WHETHER (p, COMMA_SYMBOL) || WHETHER (p, COLON_SYMBOL)));
      if (z)
	{
	  p = NEXT (p);
	}
    }
  while (!(p == NULL || !z));
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_loop: branch out loop parts.
return: token from where to proceed.                                          */

NODE_T *
top_down_loop (NODE_T * p)
{
  NODE_T *start = p, *q = p, *save;
  if (WHETHER (q, FOR_SYMBOL))
    {
      tokens_exhausted (q = NEXT (q), start);
      if (WHETHER (q, IDENTIFIER))
	{
	  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, IDENTIFIER);
	  longjmp (top_down_crash_exit, 1);
	}
      tokens_exhausted (q = NEXT (q), start);
      if (WHETHER (q, FROM_SYMBOL) || WHETHER (q, BY_SYMBOL) || WHETHER (q, TO_SYMBOL) || WHETHER (q, WHILE_SYMBOL))
	{
	  ;
	}
      else if (WHETHER (q, DO_SYMBOL))
	{
	  ATTRIBUTE (q) = ALT_DO_SYMBOL;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, 0);
	  longjmp (top_down_crash_exit, 1);
	}
    }
  if (WHETHER (q, FROM_SYMBOL))
    {
      start = q;
      q = top_down_skip_loop_unit (NEXT (q));
      tokens_exhausted (q, start);
      if (WHETHER (q, BY_SYMBOL) || WHETHER (q, TO_SYMBOL) || WHETHER (q, WHILE_SYMBOL))
	{
	  ;
	}
      else if (WHETHER (q, DO_SYMBOL))
	{
	  ATTRIBUTE (q) = ALT_DO_SYMBOL;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, 0);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (start, PREVIOUS (q), FROM_SYMBOL);
    }
  if (WHETHER (q, BY_SYMBOL))
    {
      start = q;
      q = top_down_skip_loop_series (NEXT (q));
      tokens_exhausted (q, start);
      if (WHETHER (q, TO_SYMBOL) || WHETHER (q, WHILE_SYMBOL))
	{
	  ;
	}
      else if (WHETHER (q, DO_SYMBOL))
	{
	  ATTRIBUTE (q) = ALT_DO_SYMBOL;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, 0);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (start, PREVIOUS (q), BY_SYMBOL);
    }
  if (WHETHER (q, TO_SYMBOL))
    {
      start = q;
      q = top_down_skip_loop_series (NEXT (q));
      tokens_exhausted (q, start);
      if (WHETHER (q, WHILE_SYMBOL))
	{
	  ;
	}
      else if (WHETHER (q, DO_SYMBOL))
	{
	  ATTRIBUTE (q) = ALT_DO_SYMBOL;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, 0);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (start, PREVIOUS (q), TO_SYMBOL);
    }
  if (WHETHER (q, WHILE_SYMBOL))
    {
      start = q;
      q = top_down_skip_loop_series (NEXT (q));
      tokens_exhausted (q, start);
      if (WHETHER (q, DO_SYMBOL))
	{
	  ATTRIBUTE (q) = ALT_DO_SYMBOL;
	}
      else
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, DO_SYMBOL);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (start, PREVIOUS (q), WHILE_SYMBOL);
    }
  if (WHETHER (q, DO_SYMBOL) || WHETHER (q, ALT_DO_SYMBOL))
    {
      int k = ATTRIBUTE (q);
      start = q;
      q = top_down_skip_loop_series (NEXT (q));
      tokens_exhausted (q, start);
      if (WHETHER_NOT (q, OD_SYMBOL))
	{
	  top_down_diagnose (start, q, LOOP_CLAUSE, OD_SYMBOL);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (start, q, k);
    }
  save = NEXT (start);
  make_sub (p, start, LOOP_CLAUSE);
  return (save);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_loops: driver for branching out loop parts.
return: token from where to proceed.                                          */

static void
top_down_loops (NODE_T * p)
{
  NODE_T *q = p;
  for (; q != NULL; q = NEXT (q))
    {
      if (SUB (q) != NULL)
	{
	  top_down_loops (SUB (q));
	}
    }
  q = p;
  while (q != NULL)
    {
      if (whether_loop_keyword (q) != 0)
	{
	  q = top_down_loop (q);
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Branch anything except parts of a loop.                                       */

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_series: skip serial/enquiry clause (unit series)
return: token from where to proceed                                           */

static NODE_T *
top_down_series (NODE_T * p)
{
  BOOL_T z = A_TRUE;
  while (z)
    {
      z = A_FALSE;
      p = top_down_skip_unit (p);
      if (p != NULL)
	{
	  if (WHETHER (p, SEMI_SYMBOL) || WHETHER (p, EXIT_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	    {
	      z = A_TRUE;
	      p = NEXT (p);
	    }
	}
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_begin: branch out BEGIN .. END
return: token from where to proceed                                           */

static NODE_T *
top_down_begin (NODE_T * begin_p)
{
  NODE_T *end_p = top_down_series (NEXT (begin_p));
  if (end_p == NULL || WHETHER_NOT (end_p, END_SYMBOL))
    {
      top_down_diagnose (begin_p, end_p, ENCLOSED_CLAUSE, END_SYMBOL);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
  else
    {
      make_sub (begin_p, end_p, BEGIN_SYMBOL);
      return (NEXT (begin_p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_code: branch out CODE .. EDOC
return: token from where to proceed                                           */

static NODE_T *
top_down_code (NODE_T * code_p)
{
  NODE_T *edoc_p = top_down_series (NEXT (code_p));
  if (edoc_p == NULL || WHETHER_NOT (edoc_p, EDOC_SYMBOL))
    {
      diagnostic (A_SYNTAX_ERROR, code_p, KEYWORD_ERROR);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
  else
    {
      make_sub (code_p, edoc_p, CODE_SYMBOL);
      return (NEXT (code_p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_def: branch out DEF .. FED
return: token from where to proceed                                           */

static NODE_T *
top_down_def (NODE_T * def_p)
{
  NODE_T *fed_p = top_down_series (NEXT (def_p));
  if (fed_p == NULL || WHETHER_NOT (fed_p, FED_SYMBOL))
    {
      diagnostic (A_SYNTAX_ERROR, def_p, KEYWORD_ERROR);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
  else
    {
      make_sub (def_p, fed_p, DEF_SYMBOL);
      return (NEXT (def_p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_open: branch out ( .. )
return: token from where to proceed                                           */

static NODE_T *
top_down_open (NODE_T * open_p)
{
  NODE_T *then_bar_p = top_down_series (NEXT (open_p)), *elif_bar_p;
  if (then_bar_p != NULL && WHETHER (then_bar_p, CLOSE_SYMBOL))
    {
      make_sub (open_p, then_bar_p, OPEN_SYMBOL);
      return (NEXT (open_p));
    }
  if (then_bar_p == NULL || WHETHER_NOT (then_bar_p, THEN_BAR_SYMBOL))
    {
      top_down_diagnose (open_p, then_bar_p, ENCLOSED_CLAUSE, 0);
      longjmp (top_down_crash_exit, 1);
    }
  make_sub (open_p, PREVIOUS (then_bar_p), OPEN_SYMBOL);
  elif_bar_p = top_down_series (NEXT (then_bar_p));
  if (elif_bar_p != NULL && WHETHER (elif_bar_p, CLOSE_SYMBOL))
    {
      make_sub (then_bar_p, PREVIOUS (elif_bar_p), THEN_BAR_SYMBOL);
      make_sub (open_p, elif_bar_p, OPEN_SYMBOL);
      return (NEXT (open_p));
    }
  if (elif_bar_p != NULL && WHETHER (elif_bar_p, THEN_BAR_SYMBOL))
    {
      NODE_T *close_p = top_down_series (NEXT (elif_bar_p));
      if (close_p == NULL || WHETHER_NOT (close_p, CLOSE_SYMBOL))
	{
	  top_down_diagnose (open_p, elif_bar_p, ENCLOSED_CLAUSE, CLOSE_SYMBOL);
	  longjmp (top_down_crash_exit, 1);
	}
      make_sub (then_bar_p, PREVIOUS (elif_bar_p), THEN_BAR_SYMBOL);
      make_sub (elif_bar_p, PREVIOUS (close_p), THEN_BAR_SYMBOL);
      make_sub (open_p, close_p, OPEN_SYMBOL);
      return (NEXT (open_p));
    }
  if (elif_bar_p != NULL && WHETHER (elif_bar_p, ELSE_BAR_SYMBOL))
    {
      NODE_T *close_p = top_down_open (elif_bar_p);
      make_sub (then_bar_p, PREVIOUS (elif_bar_p), THEN_BAR_SYMBOL);
      make_sub (open_p, elif_bar_p, OPEN_SYMBOL);
      return (close_p);
    }
  else
    {
      top_down_diagnose (open_p, elif_bar_p, ENCLOSED_CLAUSE, CLOSE_SYMBOL);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_sub: branch out [ .. ]
return: token from where to proceed                                           */

static NODE_T *
top_down_sub (NODE_T * sub_p)
{
  NODE_T *bus_p = top_down_series (NEXT (sub_p));
  if (bus_p != NULL && WHETHER (bus_p, BUS_SYMBOL))
    {
      make_sub (sub_p, bus_p, SUB_SYMBOL);
      return (NEXT (sub_p));
    }
  else
    {
      top_down_diagnose (sub_p, bus_p, 0, BUS_SYMBOL);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_acco: branch out { .. }
return: token from where to proceed                                           */

static NODE_T *
top_down_acco (NODE_T * acco_p)
{
  NODE_T *occa_p = top_down_series (NEXT (acco_p));
  if (occa_p != NULL && WHETHER (occa_p, OCCA_SYMBOL))
    {
      make_sub (acco_p, occa_p, ACCO_SYMBOL);
      return (NEXT (acco_p));
    }
  else
    {
      diagnostic (A_SYNTAX_ERROR, acco_p, KEYWORD_ERROR);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_if: branch out IF .. THEN .. ELSE .. FI
return: token from where to proceed                                           */

static NODE_T *
top_down_if (NODE_T * if_p)
{
  NODE_T *then_p = top_down_series (NEXT (if_p)), *elif_p;
  if (then_p == NULL || WHETHER_NOT (then_p, THEN_SYMBOL))
    {
      top_down_diagnose (if_p, then_p, CONDITIONAL_CLAUSE, THEN_SYMBOL);
      longjmp (top_down_crash_exit, 1);
    }
  make_sub (if_p, PREVIOUS (then_p), IF_SYMBOL);
  elif_p = top_down_series (NEXT (then_p));
  if (elif_p != NULL && WHETHER (elif_p, FI_SYMBOL))
    {
      make_sub (then_p, PREVIOUS (elif_p), THEN_SYMBOL);
      make_sub (if_p, elif_p, IF_SYMBOL);
      return (NEXT (if_p));
    }
  if (elif_p != NULL && WHETHER (elif_p, ELSE_SYMBOL))
    {
      NODE_T *fi_p = top_down_series (NEXT (elif_p));
      if (fi_p == NULL || WHETHER_NOT (fi_p, FI_SYMBOL))
	{
	  top_down_diagnose (if_p, fi_p, CONDITIONAL_CLAUSE, FI_SYMBOL);
	  longjmp (top_down_crash_exit, 1);
	}
      else
	{
	  make_sub (then_p, PREVIOUS (elif_p), THEN_SYMBOL);
	  make_sub (elif_p, PREVIOUS (fi_p), ELSE_SYMBOL);
	  make_sub (if_p, fi_p, IF_SYMBOL);
	  return (NEXT (if_p));
	}
    }
  if (elif_p != NULL && WHETHER (elif_p, ELIF_SYMBOL))
    {
      NODE_T *fi_p = top_down_if (elif_p);
      make_sub (then_p, PREVIOUS (elif_p), THEN_SYMBOL);
      make_sub (if_p, elif_p, IF_SYMBOL);
      return (fi_p);
    }
  else
    {
      top_down_diagnose (if_p, elif_p, CONDITIONAL_CLAUSE, FI_SYMBOL);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_case: branch out CASE .. IN .. OUT .. ESAC
return: token from where to proceed                                           */

static NODE_T *
top_down_case (NODE_T * case_p)
{
  NODE_T *in_p = top_down_series (NEXT (case_p)), *ouse_p;
  if (in_p == NULL || WHETHER_NOT (in_p, IN_SYMBOL))
    {
      top_down_diagnose (case_p, in_p, ENCLOSED_CLAUSE, IN_SYMBOL);
      diagnostic (A_SYNTAX_ERROR, case_p, KEYWORD_ERROR);
      longjmp (top_down_crash_exit, 1);
    }
  make_sub (case_p, PREVIOUS (in_p), CASE_SYMBOL);
  ouse_p = top_down_series (NEXT (in_p));
  if (ouse_p != NULL && WHETHER (ouse_p, ESAC_SYMBOL))
    {
      make_sub (in_p, PREVIOUS (ouse_p), IN_SYMBOL);
      make_sub (case_p, ouse_p, CASE_SYMBOL);
      return (NEXT (case_p));
    }
  if (ouse_p != NULL && WHETHER (ouse_p, OUT_SYMBOL))
    {
      NODE_T *esac_p = top_down_series (NEXT (ouse_p));
      if (esac_p == NULL || WHETHER_NOT (esac_p, ESAC_SYMBOL))
	{
	  top_down_diagnose (case_p, esac_p, ENCLOSED_CLAUSE, ESAC_SYMBOL);
	  longjmp (top_down_crash_exit, 1);
	}
      else
	{
	  make_sub (in_p, PREVIOUS (ouse_p), IN_SYMBOL);
	  make_sub (ouse_p, PREVIOUS (esac_p), OUT_SYMBOL);
	  make_sub (case_p, esac_p, CASE_SYMBOL);
	  return (NEXT (case_p));
	}
    }
  if (ouse_p != NULL && WHETHER (ouse_p, OUSE_SYMBOL))
    {
      NODE_T *esac_p = top_down_case (ouse_p);
      make_sub (in_p, PREVIOUS (ouse_p), IN_SYMBOL);
      make_sub (case_p, ouse_p, CASE_SYMBOL);
      return (esac_p);
    }
  else
    {
      top_down_diagnose (case_p, ouse_p, ENCLOSED_CLAUSE, ESAC_SYMBOL);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_skip_unit: skip a unit
return: token from where to proceed                                           */

NODE_T *
top_down_skip_unit (NODE_T * p)
{
  while (p != NULL && !whether_unit_terminator (p))
    {
      if (WHETHER (p, BEGIN_SYMBOL))
	{
	  p = top_down_begin (p);
	}
      else if (WHETHER (p, SUB_SYMBOL))
	{
	  p = top_down_sub (p);
	}
      else if (WHETHER (p, OPEN_SYMBOL))
	{
	  p = top_down_open (p);
	}
      else if (WHETHER (p, IF_SYMBOL))
	{
	  p = top_down_if (p);
	}
      else if (WHETHER (p, CASE_SYMBOL))
	{
	  p = top_down_case (p);
	}
      else if (WHETHER (p, DEF_SYMBOL))
	{
	  p = top_down_def (p);
	}
      else if (WHETHER (p, CODE_SYMBOL))
	{
	  p = top_down_code (p);
	}
      else if (WHETHER (p, ACCO_SYMBOL))
	{
	  p = top_down_acco (p);
	}
      else
	{
	  p = NEXT (p);
	}
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_format_open: branch out ( .. ) in a format.
return: token from where to proceed                                           */

static NODE_T *top_down_skip_format (NODE_T *);

static NODE_T *
top_down_format_open (NODE_T * open_p)
{
  NODE_T *close_p = top_down_skip_format (NEXT (open_p));
  if (close_p != NULL && WHETHER (close_p, FORMAT_ITEM_CLOSE))
    {
      make_sub (open_p, close_p, FORMAT_ITEM_OPEN);
      return (NEXT (open_p));
    }
  else
    {
      top_down_diagnose (open_p, close_p, 0, FORMAT_ITEM_CLOSE);
      longjmp (top_down_crash_exit, 1);
      return (NULL);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static NODE_T *
top_down_skip_format (NODE_T * p)
{
  while (p != NULL)
    {
      if (WHETHER (p, FORMAT_ITEM_OPEN))
	{
	  p = top_down_format_open (p);
	}
      else if (WHETHER (p, FORMAT_ITEM_CLOSE) || WHETHER (p, FORMAT_DELIMITER_SYMBOL))
	{
	  return (p);
	}
      else
	{
	  p = NEXT (p);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_formats: branch out $ .. $                                           */

static void
top_down_formats (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (SUB (q) != NULL)
	{
	  top_down_formats (SUB (q));
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, FORMAT_DELIMITER_SYMBOL))
	{
	  NODE_T *f = NEXT (q);
	  while (f != NULL && WHETHER_NOT (f, FORMAT_DELIMITER_SYMBOL))
	    {
	      if (WHETHER (f, FORMAT_ITEM_OPEN))
		{
		  f = top_down_format_open (f);
		}
	      else
		{
		  f = NEXT (f);
		}
	    }
	  if (f == NULL)
	    {
	      top_down_diagnose (p, f, FORMAT_TEXT, FORMAT_DELIMITER_SYMBOL);
	      longjmp (top_down_crash_exit, 1);
	    }
	  else
	    {
	      make_sub (q, f, FORMAT_DELIMITER_SYMBOL);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
top_down_parser: branch out phrases for the bottom-up parser.                 */

void
top_down_parser (NODE_T * p)
{
  if (p != NULL)
    {
      current_module = NULL;
      if (!setjmp (top_down_crash_exit))
	{
	  top_down_series (p);
	  top_down_loops (p);
	  top_down_formats (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Next part is the bottom-up parser, that parses without knowing about modes while
parsing and reducing. It can therefore not exchange "[]" with "()" as was 
blessed by the Revised Report, but this is according to many a trivial deviation
from the original language - arguably it forces using clearer programming style.

This is a Mailloux-type parser, in the sense that it scans a "phrase" for
definitions before it starts parsing, and therefore allows for tags to be used
before they are defined, which gives some freedom in top-down programming.
  
This parser sees the program as a set of "phrases" that needs reducing from
the inside out (bottom up). For instance            
                            
                IF a = b THEN RE a ELSE  pi * (IM a - IM b) FI                   
 Phrase level 3	                              +-----------+                      
 Phrase level 2    +---+      +--+       +----------------+                      
 Phrase level 1 +--------------------------------------------+                   

Roughly speaking, the BU parser will first work out level 3, than level 2, and
finally the level 1 phrase.                                                   */

/*-------1---------2---------3---------4---------5---------6---------7---------+
serial_or_collateral: whether a series is serial or collateral.               */

static int
serial_or_collateral (NODE_T * p)
{
  NODE_T *q;
  int semis = 0, commas = 0, exits = 0;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, COMMA_SYMBOL))
	{
	  commas++;
	}
      else if (WHETHER (q, SEMI_SYMBOL))
	{
	  semis++;
	}
      else if (WHETHER (q, EXIT_SYMBOL))
	{
	  exits++;
	}
    }
  if (semis == 0 && exits == 0 && commas > 0)
    {
      return (COLLATERAL_CLAUSE);
    }
  else if ((semis > 0 || exits > 0) && commas == 0)
    {
      return (SERIAL_CLAUSE);
    }
  else if (semis == 0 && exits == 0 && commas == 0)
    {
      return (SERIAL_CLAUSE);
    }
  else
    {
      return (SOME_CLAUSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
pad_node: insert a node with attribute "a" after "p".

This is used to fill information that Algol 68 does not require to be present.
Filling in gives one format for such construct; this helps later passes.      */

static void
pad_node (NODE_T * p, int a)
{
  NODE_T *z = new_node ();
  *z = *p;
  PREVIOUS (z) = p;
  SUB (z) = NULL;
  ATTRIBUTE (z) = a;
  MOID (z) = NULL;
  if (NEXT (z) != NULL)
    {
      PREVIOUS (NEXT (z)) = z;
    }
  NEXT (p) = z;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
not_supported: diagnose for not-supported features.                           */

static void
not_supported (NODE_T * p)
{
  diagnostic (A_SYNTAX_ERROR, p, "this feature is not supported");
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
not_implemented_yet: future work.                                             */

static void
not_implemented_yet (NODE_T * p)
{
  diagnostic (A_SYNTAX_ERROR, p, "this feature has not been implemented yet");
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
empty clause: diagnose for clauses not yielding a value.                      */

static void
empty_clause (NODE_T * p)
{
  diagnostic (A_SYNTAX_ERROR, p, "clause does not yield a value");
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
par_clause: diagnose for parallel clause.                                     */

static void
par_clause (NODE_T * p)
{
  diagnostic (A_WARNING, p, "A will be executed as A", PARALLEL_CLAUSE, COLLATERAL_CLAUSE);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
missing_separator: diagnose for missing separator.                            */

static void
missing_separator (NODE_T * p)
{
  NODE_T *q = (p != NULL && NEXT (p) != NULL) ? NEXT (p) : p;
  diagnostic (A_SYNTAX_ERROR, q, "probably a missing semicolon, comma or exit nearby");
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
f: whether and reduce a sentence.
p: token where to start matching.
a: if not NULL, procedure to execute upon whether.
z: if not NULL, to be set to TRUE upon whether.
...: series of attributes terminated by zero.                                 */

static void
f (NODE_T * p, void (*a) (NODE_T *), BOOL_T * z, ...)
{
  va_list list;
  int result, arg;
  NODE_T *head = p, *tail = NULL;
  va_start (list, z);
  result = va_arg (list, int);
  while ((arg = va_arg (list, int)) != 0)
    {
/* WILDCARD matches any Algol68G non terminal, but no keyword. */
      if (p != NULL && (arg == WILDCARD ? non_terminal_string (edit_line, ATTRIBUTE (p)) != NULL : arg == ATTRIBUTE (p)))
	{
	  tail = p;
	  p = NEXT (p);
	}
      else
	{
	  va_end (list);
	  return;
	}
    }
  if (head != NULL && head->info->module->options.reductions)
    {
      where (STDOUT_FILENO, head);
      strcpy (output_line, "");
      strcat (output_line, non_terminal_string (edit_line, result));
      strcat (output_line, "<-");
      strcat (output_line, phrase_to_text (head, tail));
      io_write_string (STDOUT_FILENO, output_line);
    }
  make_sub (head, tail, result);
  va_end (list);
/* Execute procedure in case reduction succeeds. */
  if (a != NULL)
    {
      a (tail != NULL ? tail : head);
    }
  if (z != NULL)
    {
      *z = A_TRUE;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bottom_up_parser: driver for the bottom-up parser.                            */

void
bottom_up_parser (NODE_T * p)
{
  if (p != NULL)
    {
      current_module = p->info->module;
      if (!setjmp (bottom_up_crash_exit))
	{
	  reduce_particular_program (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_particular_program: top-level reduction.                               */

static void
reduce_particular_program (NODE_T * p)
{
  NODE_T *q;
  int old_error_count = error_count;
/* A program is "label sequence; particular program" */
  extract_labels (p, SERIAL_CLAUSE /* a fake here, but ok */ );
/* Parse the program itself */
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      if (SUB (q) != NULL)
	{
	  reduce_subordinate (q, SOME_CLAUSE);
	}
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, LABEL, DEFINING_IDENTIFIER, COLON_SYMBOL, 0);
	  f (q, NULL, &z, LABEL, LABEL, DEFINING_IDENTIFIER, COLON_SYMBOL, 0);
	}
    }
/* Determine the encompassing enclosed clause */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, par_clause, NULL, PARALLEL_CLAUSE, PAR_SYMBOL, COLLATERAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, PARALLEL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CLOSED_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, COLLATERAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CONDITIONAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, INTEGER_CASE_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, UNITED_CASE_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, LOOP_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CODE_CLAUSE, 0);
    }
/* Try reducing the particular program */
  q = p;
  f (q, NULL, NULL, PARTICULAR_PROGRAM, LABEL, ENCLOSED_CLAUSE, 0);
  f (q, NULL, NULL, PARTICULAR_PROGRAM, ENCLOSED_CLAUSE, 0);
  f (q, NULL, NULL, PARTICULAR_PROGRAM, EXPORT_CLAUSE, 0);
  if (SUB (p) == NULL || NEXT (p) != NULL)
    {
      recover_from_error (p, PARTICULAR_PROGRAM, (error_count - old_error_count) > MAX_ERRORS);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_subordinate: reduce the sub-phrase that starts one level down.

If this is unsuccessful then it will at least copy the resulting attribute 
as the parser can repair some faults. This gives less spurious messages.      */

static void
reduce_subordinate (NODE_T * p, int expect)
{
  if (p != NULL)
    {
      if (SUB (p) != NULL)
	{
	  BOOL_T no_error = reduce_phrase (SUB (p), expect);
	  ATTRIBUTE (p) = ATTRIBUTE (SUB (p));
	  if (no_error)
	    {
	      SUB (p) = SUB (SUB (p));
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_phrase: driver for reducing a phrase.                                  */

BOOL_T
reduce_phrase (NODE_T * p, int expect)
{
  int old_error_count = error_count;
  BOOL_T declarer_pack = (expect == STRUCTURE_PACK || expect == PARAMETER_PACK || expect == FORMAL_DECLARERS || expect == UNION_PACK || expect == SPECIFIER);
/* Sample all info needed to decide whether a bold tag is operator or indicant */
  extract_indicants (p);
  if (!declarer_pack)
    {
      extract_priorities (p);
      extract_operators (p);
    }
  elaborate_bold_tags (p);
/* Now we can reduce declarers, knowing which bold tags are indicants */
  reduce_declarers (p, expect);
/* Parse the phrase, as appropriate */
  if (!declarer_pack)
    {
      extract_declarations (p);
      extract_labels (p, expect);
      reduce_deeper_clauses_driver (p);
      reduce_statements (p, expect);
      reduce_right_to_left_constructs (p);
      ignore_superfluous_semicolons (p, expect);
      signal_wrong_exits (p, expect);
      reduce_constructs (p, expect);
      reduce_control_structure (p, expect);
    }
/* Do something intelligible if parsing failed. */
  if (SUB (p) == NULL || NEXT (p) != NULL)
    {
      recover_from_error (p, expect, (error_count - old_error_count) > MAX_ERRORS);
      return (A_FALSE);
    }
  else
    {
      return (A_TRUE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_declarers: driver for reducing declarers.                              */

static void
reduce_declarers (NODE_T * p, int expect)
{
  reduce_lengtheties (p);
  reduce_indicants (p);
  reduce_small_declarers (p);
  reduce_declarer_lists (p);
  reduce_row_proc_op_declarers (p);
  if (expect == STRUCTURE_PACK)
    {
      reduce_struct_pack (p);
    }
  else if (expect == PARAMETER_PACK)
    {
      reduce_parameter_pack (p);
    }
  else if (expect == FORMAL_DECLARERS)
    {
      reduce_formal_declarer_pack (p);
    }
  else if (expect == UNION_PACK)
    {
      reduce_union_pack (p);
    }
  else if (expect == SPECIFIER)
    {
      reduce_specifiers (p);
    }
  else
    {
      NODE_T *q;
      for (q = p; q != NULL; q = NEXT (q))
	{
	  if (whether (q, OPEN_SYMBOL, COLON_SYMBOL, 0) && !(expect == GENERIC_ARGUMENT || expect == BOUNDS))
	    {
	      reduce_subordinate (q, SPECIFIER);
	    }
	  if (whether (q, OPEN_SYMBOL, DECLARER, COLON_SYMBOL, 0))
	    {
	      reduce_subordinate (q, PARAMETER_PACK);
	    }
	  if (whether (q, OPEN_SYMBOL, VOID_SYMBOL, COLON_SYMBOL, 0))
	    {
	      reduce_subordinate (q, PARAMETER_PACK);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_deeper_clauses_driver: driver for reducing control structure elements. */

static void
reduce_deeper_clauses_driver (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (SUB (p) != NULL)
	{
	  reduce_deeper_clauses (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_statements: reduces PRIMARY, SECONDARY, TERITARY and FORMAT TEXT.      */

static void
reduce_statements (NODE_T * p, int expect)
{
  reduce_primary_bits (p, expect);
  if (expect != ENCLOSED_CLAUSE)
    {
      reduce_primaries (p, expect);
      if (expect == FORMAT_TEXT)
	{
	  reduce_format_texts (p);
	}
      else
	{
	  reduce_secondaries (p);
	  reduce_formulae (p);
	  reduce_tertiaries (p);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_right_to_left_constructs: ibid.

Here are cases that need reducing from right-to-left whereas many things
can be reduced left-to-right. Assignations are a notable example; one could
discuss whether it would not be more natural to write 1 =: k in stead of
k := 1. The latter is said to be more natural, or it could be just computing
history. Meanwhile we use this routine.                                       */

static void
reduce_right_to_left_constructs (NODE_T * p)
{
  if (p != NULL)
    {
      reduce_right_to_left_constructs (NEXT (p));
/* Assignations */
      if (WHETHER (p, TERTIARY))
	{
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, TERTIARY, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, IDENTITY_RELATION, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, AND_FUNCTION, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, OR_FUNCTION, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, JUMP, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, SKIP, 0);
	  f (p, NULL, NULL, ASSIGNATION, TERTIARY, ASSIGN_SYMBOL, ASSIGNATION, 0);
	}
/* Routine texts with parameter pack */
      else if (WHETHER (p, PARAMETER_PACK))
	{
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, ASSIGNATION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, IDENTITY_RELATION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, AND_FUNCTION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, OR_FUNCTION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, JUMP, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, SKIP, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, TERTIARY, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, DECLARER, COLON_SYMBOL, ROUTINE_TEXT, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, ASSIGNATION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, IDENTITY_RELATION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, AND_FUNCTION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, OR_FUNCTION, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, JUMP, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, SKIP, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, TERTIARY, 0);
	  f (p, NULL, NULL, ROUTINE_TEXT, PARAMETER_PACK, VOID_SYMBOL, COLON_SYMBOL, ROUTINE_TEXT, 0);
	}
/* Routine texts without parameter pack */
      else if (WHETHER (p, DECLARER))
	{
	  if (!(PREVIOUS (p) != NULL && WHETHER (PREVIOUS (p), PARAMETER_PACK)))
	    {
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, ASSIGNATION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, IDENTITY_RELATION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, AND_FUNCTION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, OR_FUNCTION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, JUMP, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, SKIP, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, TERTIARY, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, DECLARER, COLON_SYMBOL, ROUTINE_TEXT, 0);
	    }
	}
      else if (WHETHER (p, VOID_SYMBOL))
	{
	  if (!(PREVIOUS (p) != NULL && WHETHER (PREVIOUS (p), PARAMETER_PACK)))
	    {
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, ASSIGNATION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, IDENTITY_RELATION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, AND_FUNCTION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, OR_FUNCTION, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, JUMP, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, SKIP, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, TERTIARY, 0);
	      f (p, NULL, NULL, ROUTINE_TEXT, VOID_SYMBOL, COLON_SYMBOL, ROUTINE_TEXT, 0);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
ignore_superfluous_semicolons: graciously ignore extra semicolons.                                         
This routine relaxes the parser a bit with respect to superfluous semicolons, 
for instance "FI; OD". These provoke only a warning.                          */

static void
ignore_superfluous_semicolons (NODE_T * p, int expect)
{
  (void) expect;		/* currently no use for this */
  for (; p != NULL && NEXT (p) != NULL; p = NEXT (p))
    {
      if (NEXT (p) != NULL && WHETHER (NEXT (p), SEMI_SYMBOL) && NEXT (NEXT (p)) == NULL)
	{
	  diagnostic (A_WARNING, NEXT (p), "superfluous S skipped");
	  NEXT (p) = NULL;
	}
      else if (WHETHER (p, SEMI_SYMBOL))
	{
	  int z = whether_semicolon_less (NEXT (p));
	  if (z != 0)
	    {
	      diagnostic (A_WARNING, p, "superfluous S skipped");
	      make_sub (p, NEXT (p), z);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
signal_wrong_exits: signal wrongly used exits.                                */

static void
signal_wrong_exits (NODE_T * p, int expect)
{
  (void) expect;		/* currently no use for this */
  for (; p != NULL && NEXT (p) != NULL; p = NEXT (p))
    {
      if (NEXT (p) != NULL && WHETHER (NEXT (p), EXIT_SYMBOL) && NEXT (NEXT (p)) == NULL)
	{
	  diagnostic (A_SYNTAX_ERROR, NEXT (p), "S must be followed by a labeled unit");
	  NEXT (p) = NULL;
	}
      else if (WHETHER (p, EXIT_SYMBOL))
	{
	  int z = whether_semicolon_less (NEXT (p));
	  if (z != 0)
	    {
	      diagnostic (A_SYNTAX_ERROR, NEXT (p), "S must be followed by a labeled unit");
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_constructs: reduce constructs in proper order.                         */

static void
reduce_constructs (NODE_T * p, int expect)
{
  reduce_qualifiers (p);
  reduce_basic_declarations (p);
  reduce_units (p);
  reduce_erroneous_units (p);
  if (expect != UNIT)
    {
      if (expect == GENERIC_ARGUMENT)
	{
	  reduce_generic_arguments (p);
	}
      else if (expect == BOUNDS)
	{
	  reduce_bounds (p);
	}
      else
	{
	  reduce_declaration_lists (p);
	  if (expect != DECLARATION_LIST)
	    {
	      reduce_labels (p);
	      if (expect == SOME_CLAUSE)
		{
		  expect = serial_or_collateral (p);
		  if (expect == SOME_CLAUSE)
		    {
		      diagnostic (A_SYNTAX_ERROR, p, "check for mixed use of semicolons, commas or exits in this clause");
		      return;
		    }
		}
	      if (expect == SERIAL_CLAUSE)
		{
		  reduce_serial_clauses (p);
		}
	      else if (expect == ENQUIRY_CLAUSE)
		{
		  reduce_enquiry_clauses (p);
		}
	      else if (expect == COLLATERAL_CLAUSE)
		{
		  reduce_collateral_clauses (p);
		}
	      else if (expect == ARGUMENT)
		{
		  reduce_arguments (p);
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_control_structure: ibid.                                               */

static void
reduce_control_structure (NODE_T * p, int expect)
{
  reduce_enclosed_clause_bits (p, expect);
  reduce_enclosed_clauses (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_lengtheties: reduce lengths in declarers.                              */

static void
reduce_lengtheties (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      f (q, NULL, NULL, LONGETY, LONG_SYMBOL, 0);
      f (q, NULL, NULL, SHORTETY, SHORT_SYMBOL, 0);
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, LONGETY, LONGETY, LONG_SYMBOL, 0);
	  f (q, NULL, &z, SHORTETY, SHORTETY, SHORT_SYMBOL, 0);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_indicants: ibid.                                                       */

static void
reduce_indicants (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INDICANT, INT_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, REAL_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, BITS_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, BYTES_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, COMPLEX_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, COMPL_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, BOOL_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, CHAR_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, FORMAT_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, STRING_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, FILE_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, CHANNEL_SYMBOL, 0);
      f (q, not_supported, NULL, INDICANT, SEMA_SYMBOL, 0);
      f (q, NULL, NULL, INDICANT, PIPE_SYMBOL, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_small_declarers: reduce basic declarations, like LONG BITS, STRING, .. */

static void
reduce_small_declarers (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (whether (q, LONGETY, INDICANT, 0))
	{
	  int a;
	  if (SUB (NEXT (q)) == NULL)
	    {
	      diagnostic (A_SYNTAX_ERROR, NEXT (q), EXPECTED, "appropriate declarer");
	      f (q, NULL, NULL, DECLARER, LONGETY, INDICANT, 0);
	    }
	  else
	    {
	      a = ATTRIBUTE (SUB (NEXT (q)));
	      if (a == INT_SYMBOL || a == REAL_SYMBOL || a == BITS_SYMBOL || a == BYTES_SYMBOL || a == COMPLEX_SYMBOL || a == COMPL_SYMBOL)
		{
		  f (q, NULL, NULL, DECLARER, LONGETY, INDICANT, 0);
		}
	      else
		{
		  diagnostic (A_SYNTAX_ERROR, NEXT (q), EXPECTED, "appropriate declarer");
		  f (q, NULL, NULL, DECLARER, LONGETY, INDICANT, 0);
		}
	    }
	}
      else if (whether (q, SHORTETY, INDICANT, 0))
	{
	  int a;
	  if (SUB (NEXT (q)) == NULL)
	    {
	      diagnostic (A_SYNTAX_ERROR, NEXT (q), EXPECTED, "appropriate declarer");
	      f (q, NULL, NULL, DECLARER, SHORTETY, INDICANT, 0);
	    }
	  else
	    {
	      a = ATTRIBUTE (SUB (NEXT (q)));
	      if (a == INT_SYMBOL || a == REAL_SYMBOL || a == BITS_SYMBOL || a == BYTES_SYMBOL || a == COMPLEX_SYMBOL || a == COMPL_SYMBOL)
		{
		  f (q, NULL, NULL, DECLARER, SHORTETY, INDICANT, 0);
		}
	      else
		{
		  diagnostic (A_SYNTAX_ERROR, NEXT (q), EXPECTED, "appropriate declarer");
		  f (q, NULL, NULL, DECLARER, LONGETY, INDICANT, 0);
		}
	    }
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, DECLARER, INDICANT, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
whether_formal_bounds:               */

static BOOL_T
whether_formal_bounds (NODE_T * p)
{
  if (p == NULL)
    {
      return (A_TRUE);
    }
  else
    {
      switch (ATTRIBUTE (p))
	{
	case OPEN_SYMBOL:
	case CLOSE_SYMBOL:
	case SUB_SYMBOL:
	case BUS_SYMBOL:
	case COMMA_SYMBOL:
	case COLON_SYMBOL:
	case DOTDOT_SYMBOL:
	case INT_DENOTER:
	case IDENTIFIER:
	case OPERATOR:
	  return (whether_formal_bounds (SUB (p)) && whether_formal_bounds (NEXT (p)));
	default:
	  return (A_FALSE);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_declarer_lists: reduce declarer lists for packs.                       */

static void
reduce_declarer_lists (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (NEXT (q) != NULL && SUB (NEXT (q)) != NULL)
	{
	  if (WHETHER (q, STRUCT_SYMBOL))
	    {
	      reduce_subordinate (NEXT (q), STRUCTURE_PACK);
	      f (q, NULL, NULL, DECLARER, STRUCT_SYMBOL, STRUCTURE_PACK, 0);
	    }
	  else if (WHETHER (q, UNION_SYMBOL))
	    {
	      reduce_subordinate (NEXT (q), UNION_PACK);
	      f (q, NULL, NULL, DECLARER, UNION_SYMBOL, UNION_PACK, 0);
	    }
	  else if (WHETHER (q, PROC_SYMBOL))
	    {
	      if (whether (q, PROC_SYMBOL, OPEN_SYMBOL, 0))
		{
		  if (!whether_formal_bounds (SUB_NEXT (q)))
		    {
		      reduce_subordinate (NEXT (q), FORMAL_DECLARERS);
		    }
		}
	    }
	  else if (WHETHER (q, OP_SYMBOL))
	    {
	      if (whether (q, OP_SYMBOL, OPEN_SYMBOL, 0))
		{
		  if (!whether_formal_bounds (SUB_NEXT (q)))
		    {
		      reduce_subordinate (NEXT (q), FORMAL_DECLARERS);
		    }
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_row_proc_op_declarers: reduce ROW, PROC and OP declarers.              */

static void
reduce_row_proc_op_declarers (NODE_T * p)
{
  BOOL_T z = A_TRUE;
  while (z)
    {
      NODE_T *q;
      z = A_FALSE;
      for (q = p; q != NULL; q = NEXT (q))
	{
/* FLEX DECL */
	  if (whether (q, FLEX_SYMBOL, DECLARER, 0))
	    {
	      f (q, NULL, &z, DECLARER, FLEX_SYMBOL, DECLARER, 0);
	    }
/* FLEX [] DECL */
	  if (whether (q, FLEX_SYMBOL, SUB_SYMBOL, DECLARER, 0) && SUB (NEXT (q)) != NULL)
	    {
	      reduce_subordinate (NEXT (q), BOUNDS);
	      f (q, NULL, &z, DECLARER, FLEX_SYMBOL, BOUNDS, DECLARER, 0);
	      f (q, NULL, &z, DECLARER, FLEX_SYMBOL, FORMAL_BOUNDS, DECLARER, 0);
	    }
/* FLEX () DECL */
	  if (whether (q, FLEX_SYMBOL, OPEN_SYMBOL, DECLARER, 0) && SUB (NEXT (q)) != NULL)
	    {
	      if (!whether (q, FLEX_SYMBOL, OPEN_SYMBOL, DECLARER, COLON_SYMBOL, 0))
		{
		  reduce_subordinate (NEXT (q), BOUNDS);
		  f (q, NULL, &z, DECLARER, FLEX_SYMBOL, BOUNDS, DECLARER, 0);
		  f (q, NULL, &z, DECLARER, FLEX_SYMBOL, FORMAL_BOUNDS, DECLARER, 0);
		}
	    }
/* [] DECL */
	  if (whether (q, SUB_SYMBOL, DECLARER, 0) && SUB (q) != NULL)
	    {
	      reduce_subordinate (q, BOUNDS);
	      f (q, NULL, &z, DECLARER, BOUNDS, DECLARER, 0);
	      f (q, NULL, &z, DECLARER, FORMAL_BOUNDS, DECLARER, 0);
	    }
/* () DECL */
	  if (whether (q, OPEN_SYMBOL, DECLARER, 0) && SUB (q) != NULL)
	    {
	      if (whether (q, OPEN_SYMBOL, DECLARER, COLON_SYMBOL, 0))
		{
/* Catch e.g. (INT i) () INT: */
		  if (whether_formal_bounds (SUB (q)))
		    {
		      reduce_subordinate (q, BOUNDS);
		      f (q, NULL, &z, DECLARER, BOUNDS, DECLARER, 0);
		      f (q, NULL, &z, DECLARER, FORMAL_BOUNDS, DECLARER, 0);
		    }
		}
	      else
		{
		  reduce_subordinate (q, BOUNDS);
		  f (q, NULL, &z, DECLARER, BOUNDS, DECLARER, 0);
		  f (q, NULL, &z, DECLARER, FORMAL_BOUNDS, DECLARER, 0);
		}
	    }
	}
/* PROC DECL, PROC () DECL, OP () DECL */
      for (q = p; q != NULL; q = NEXT (q))
	{
	  int a = ATTRIBUTE (q);
	  if (a == REF_SYMBOL)
	    {
	      f (q, NULL, &z, DECLARER, REF_SYMBOL, DECLARER, 0);
	    }
	  else if (a == PROC_SYMBOL)
	    {
	      f (q, NULL, &z, DECLARER, PROC_SYMBOL, DECLARER, 0);
	      f (q, NULL, &z, DECLARER, PROC_SYMBOL, FORMAL_DECLARERS, DECLARER, 0);
	      f (q, NULL, &z, DECLARER, PROC_SYMBOL, VOID_SYMBOL, 0);
	      f (q, NULL, &z, DECLARER, PROC_SYMBOL, FORMAL_DECLARERS, VOID_SYMBOL, 0);
	    }
	  else if (a == OP_SYMBOL)
	    {
	      f (q, NULL, &z, OPERATOR_PLAN, OP_SYMBOL, FORMAL_DECLARERS, DECLARER, 0);
	      f (q, NULL, &z, OPERATOR_PLAN, OP_SYMBOL, FORMAL_DECLARERS, VOID_SYMBOL, 0);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_struct_pack: reduce structure packs.                                   */

static void
reduce_struct_pack (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, STRUCTURED_FIELD, DECLARER, IDENTIFIER, 0);
	  f (q, NULL, &z, STRUCTURED_FIELD, STRUCTURED_FIELD, COMMA_SYMBOL, IDENTIFIER, 0);
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, STRUCTURED_FIELD_LIST, STRUCTURED_FIELD, 0);
	  f (q, NULL, &z, STRUCTURED_FIELD_LIST, STRUCTURED_FIELD_LIST, COMMA_SYMBOL, STRUCTURED_FIELD, 0);
	  f (q, missing_separator, &z, STRUCTURED_FIELD_LIST, STRUCTURED_FIELD_LIST, STRUCTURED_FIELD, 0);
	}
    }
  q = p;
  f (q, NULL, NULL, STRUCTURE_PACK, OPEN_SYMBOL, STRUCTURED_FIELD_LIST, CLOSE_SYMBOL, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_parameter_pack: reduce parameter packs.                                */

static void
reduce_parameter_pack (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, PARAMETER, DECLARER, IDENTIFIER, 0);
	  f (q, NULL, &z, PARAMETER, PARAMETER, COMMA_SYMBOL, IDENTIFIER, 0);
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, PARAMETER_LIST, PARAMETER, 0);
	  f (q, NULL, &z, PARAMETER_LIST, PARAMETER_LIST, COMMA_SYMBOL, PARAMETER, 0);
	}
    }
  q = p;
  f (q, NULL, NULL, PARAMETER_PACK, OPEN_SYMBOL, PARAMETER_LIST, CLOSE_SYMBOL, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_formal_declarer_pack: reduce formal declarer packs.                    */

static void
reduce_formal_declarer_pack (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, FORMAL_DECLARERS_LIST, DECLARER, 0);
	  f (q, NULL, &z, FORMAL_DECLARERS_LIST, FORMAL_DECLARERS_LIST, COMMA_SYMBOL, DECLARER, 0);
	  f (q, missing_separator, &z, FORMAL_DECLARERS_LIST, FORMAL_DECLARERS_LIST, DECLARER, 0);
	}
    }
  q = p;
  f (q, NULL, NULL, FORMAL_DECLARERS, OPEN_SYMBOL, FORMAL_DECLARERS_LIST, CLOSE_SYMBOL, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_union_pack: reduce union packs (formal declarers and VOID).            */

static void
reduce_union_pack (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, UNION_DECLARER_LIST, DECLARER, 0);
	  f (q, NULL, &z, UNION_DECLARER_LIST, VOID_SYMBOL, 0);
	  f (q, NULL, &z, UNION_DECLARER_LIST, UNION_DECLARER_LIST, COMMA_SYMBOL, DECLARER, 0);
	  f (q, NULL, &z, UNION_DECLARER_LIST, UNION_DECLARER_LIST, COMMA_SYMBOL, VOID_SYMBOL, 0);
	  f (q, missing_separator, &z, UNION_DECLARER_LIST, UNION_DECLARER_LIST, DECLARER, 0);
	  f (q, missing_separator, &z, UNION_DECLARER_LIST, UNION_DECLARER_LIST, VOID_SYMBOL, 0);
	}
    }
  q = p;
  f (q, NULL, NULL, UNION_PACK, OPEN_SYMBOL, UNION_DECLARER_LIST, CLOSE_SYMBOL, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_specifiers: ibid.                                                      */

static void
reduce_specifiers (NODE_T * p)
{
  NODE_T *q = p;
  f (q, NULL, NULL, SPECIFIER, OPEN_SYMBOL, DECLARER, IDENTIFIER, CLOSE_SYMBOL, 0);
  f (q, NULL, NULL, SPECIFIER, OPEN_SYMBOL, DECLARER, CLOSE_SYMBOL, 0);
  f (q, NULL, NULL, SPECIFIER, OPEN_SYMBOL, VOID_SYMBOL, CLOSE_SYMBOL, 0);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_deeper_clauses: reduce control structure elements.                     */

static void
reduce_deeper_clauses (NODE_T * p)
{
  if (WHETHER (p, FORMAT_DELIMITER_SYMBOL))
    {
      reduce_subordinate (p, FORMAT_TEXT);
    }
  else if (WHETHER (p, FORMAT_ITEM_OPEN))
    {
      reduce_subordinate (p, FORMAT_TEXT);
    }
  else if (WHETHER (p, OPEN_SYMBOL))
    {
      if (NEXT (p) != NULL && WHETHER (NEXT (p), THEN_BAR_SYMBOL))
	{
	  reduce_subordinate (p, ENQUIRY_CLAUSE);
	}
      else if (PREVIOUS (p) != NULL && WHETHER (PREVIOUS (p), PAR_SYMBOL))
	{
	  reduce_subordinate (p, COLLATERAL_CLAUSE);
	}
    }
  else if (WHETHER (p, IF_SYMBOL) || WHETHER (p, ELIF_SYMBOL) || WHETHER (p, CASE_SYMBOL) || WHETHER (p, OUSE_SYMBOL) || WHETHER (p, WHILE_SYMBOL) || WHETHER (p, ELSE_BAR_SYMBOL) || WHETHER (p, ACCO_SYMBOL))
    {
      reduce_subordinate (p, ENQUIRY_CLAUSE);
    }
  else if (WHETHER (p, BEGIN_SYMBOL))
    {
      reduce_subordinate (p, SOME_CLAUSE);
    }
  else if (WHETHER (p, THEN_SYMBOL) || WHETHER (p, ELSE_SYMBOL) || WHETHER (p, OUT_SYMBOL) || WHETHER (p, DO_SYMBOL) || WHETHER (p, ALT_DO_SYMBOL) || WHETHER (p, CODE_SYMBOL) || WHETHER (p, DEF_SYMBOL))
    {
      reduce_subordinate (p, SERIAL_CLAUSE);
    }
  else if (WHETHER (p, IN_SYMBOL))
    {
      reduce_subordinate (p, COLLATERAL_CLAUSE);
    }
  else if (WHETHER (p, THEN_BAR_SYMBOL))
    {
      reduce_subordinate (p, SOME_CLAUSE);
    }
  else if (WHETHER (p, LOOP_CLAUSE))
    {
      reduce_subordinate (p, ENCLOSED_CLAUSE);
    }
  else if (WHETHER (p, FOR_SYMBOL) || WHETHER (p, FROM_SYMBOL) || WHETHER (p, BY_SYMBOL) || WHETHER (p, TO_SYMBOL))
    {
      reduce_subordinate (p, UNIT);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_primary_bits: reduce primary elements.                                 */

static void
reduce_primary_bits (NODE_T * p, int expect)
{
  NODE_T *q = p;
  for (; q != NULL; q = NEXT (q))
    {
      if (whether (q, IDENTIFIER, OF_SYMBOL, 0))
	{
	  ATTRIBUTE (q) = FIELD_IDENTIFIER;
	}
      f (q, NULL, NULL, ENVIRON_NAME, ENVIRON_SYMBOL, ROW_CHAR_DENOTER, 0);
      f (q, NULL, NULL, NIHIL, NIL_SYMBOL, 0);
      f (q, NULL, NULL, SKIP, SKIP_SYMBOL, 0);
      f (q, NULL, NULL, SELECTOR, FIELD_IDENTIFIER, OF_SYMBOL, 0);
/* JUMPs without GOTO are resolved later */
      f (q, NULL, NULL, JUMP, GOTO_SYMBOL, IDENTIFIER, 0);
      f (q, NULL, NULL, DENOTER, LONGETY, INT_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, LONGETY, REAL_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, LONGETY, BITS_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, SHORTETY, INT_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, SHORTETY, REAL_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, SHORTETY, BITS_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, INT_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, REAL_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, BITS_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, ROW_CHAR_DENOTER, 0);
      f (q, NULL, NULL, DENOTER, TRUE_SYMBOL, 0);
      f (q, NULL, NULL, DENOTER, FALSE_SYMBOL, 0);
      f (q, NULL, NULL, DENOTER, EMPTY_SYMBOL, 0);
      if (expect == SERIAL_CLAUSE || expect == ENQUIRY_CLAUSE || expect == SOME_CLAUSE)
	{
	  BOOL_T z = A_TRUE;
	  while (z)
	    {
	      z = A_FALSE;
	      f (q, NULL, &z, LABEL, DEFINING_IDENTIFIER, COLON_SYMBOL, 0);
	      f (q, NULL, &z, LABEL, LABEL, DEFINING_IDENTIFIER, COLON_SYMBOL, 0);
	    }
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, par_clause, NULL, PARALLEL_CLAUSE, PAR_SYMBOL, COLLATERAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, PARALLEL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CLOSED_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, COLLATERAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CONDITIONAL_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, INTEGER_CASE_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, UNITED_CASE_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, LOOP_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, CODE_CLAUSE, 0);
      f (q, NULL, NULL, ENCLOSED_CLAUSE, EXPORT_CLAUSE, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_primaries: reduce primaries completely.                                */

static void
reduce_primaries (NODE_T * p, int expect)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      BOOL_T fwd = A_TRUE, z;
/* Primaries excepts call and slice. */
      f (q, NULL, NULL, PRIMARY, IDENTIFIER, 0);
      f (q, NULL, NULL, PRIMARY, DENOTER, 0);
      f (q, NULL, NULL, CAST, DECLARER, ENCLOSED_CLAUSE, 0);
      f (q, NULL, NULL, CAST, VOID_SYMBOL, ENCLOSED_CLAUSE, 0);
      f (q, NULL, NULL, ASSERTION, ASSERT_SYMBOL, ENCLOSED_CLAUSE, 0);
      f (q, NULL, NULL, PRIMARY, CAST, 0);
      f (q, NULL, NULL, PRIMARY, ENCLOSED_CLAUSE, 0);
      f (q, NULL, NULL, PRIMARY, FORMAT_TEXT, 0);
/* Call and slice. */
      z = A_TRUE;
      while (z)
	{
	  NODE_T *x = NEXT (q);
	  z = A_FALSE;
	  if (WHETHER (q, PRIMARY) && x != NULL)
	    {
	      if (WHETHER (x, OPEN_SYMBOL))
		{
		  reduce_subordinate (NEXT (q), GENERIC_ARGUMENT);
		  f (q, NULL, &z, SLICE, PRIMARY, GENERIC_ARGUMENT, 0);
		  f (q, NULL, &z, PRIMARY, SLICE, 0);
/*
	  reduce_subordinate (NEXT (q), ARGUMENT);
	  f (q, NULL, &z, CALL, PRIMARY, ARGUMENT, 0);
	  f (q, NULL, &z, PRIMARY, CALL, 0);
*/
		}
	      else if (WHETHER (x, SUB_SYMBOL))
		{
		  reduce_subordinate (NEXT (q), GENERIC_ARGUMENT);
		  f (q, NULL, &z, SLICE, PRIMARY, GENERIC_ARGUMENT, 0);
		  f (q, NULL, &z, PRIMARY, SLICE, 0);
		}
	    }
	}
/* Now that call and slice are known, reduce remaining ( .. ). */
      if (WHETHER (q, OPEN_SYMBOL) && SUB (q) != NULL)
	{
	  reduce_subordinate (q, SOME_CLAUSE);
	  f (q, NULL, NULL, ENCLOSED_CLAUSE, CLOSED_CLAUSE, 0);
	  f (q, NULL, NULL, ENCLOSED_CLAUSE, COLLATERAL_CLAUSE, 0);
	  f (q, NULL, NULL, ENCLOSED_CLAUSE, CONDITIONAL_CLAUSE, 0);
	  f (q, NULL, NULL, ENCLOSED_CLAUSE, INTEGER_CASE_CLAUSE, 0);
	  f (q, NULL, NULL, ENCLOSED_CLAUSE, UNITED_CASE_CLAUSE, 0);
	  if (PREVIOUS (q) != NULL)
	    {
	      q = PREVIOUS (q);
	      fwd = A_FALSE;
	    }
	}
/* Format text items */
      if (expect == FORMAT_TEXT)
	{
	  NODE_T *r;
	  for (r = p; r != NULL; r = NEXT (r))
	    {
	      f (r, NULL, NULL, DYNAMIC_REPLICATOR, FORMAT_ITEM_N, ENCLOSED_CLAUSE, 0);
	      f (r, NULL, NULL, GENERAL_PATTERN, FORMAT_ITEM_G, ENCLOSED_CLAUSE, 0);
	      f (r, NULL, NULL, FORMAT_PATTERN, FORMAT_ITEM_F, ENCLOSED_CLAUSE, 0);
	    }
	}
      if (fwd)
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
ambiguous_patterns: enforce that ambiguous patterns are separated by commas.

Example: printf (($+d.2d +d.2d$, 1, 2)) can produce either "+1.00 +2.00" or 
"+1+002.00". A comma must be supplied to resolve the ambiguity.

The obvious thing would be to weave this into the syntax, letting the BU parser
sort it out. But the C-style patterns do not suffer from Algol 68 pattern
ambiguity, so by solving it this way we maximise freedom in writing the patterns
as we want without introducing two "kinds" of patterns, and so we have shorter 
routines for implementing formatted transput. This is a pragmatic system.     */

static void
ambiguous_patterns (NODE_T * p)
{
  NODE_T *q, *last_pat = NULL;
  for (q = p; q != NULL; q = NEXT (q))
    {
      switch (ATTRIBUTE (q))
	{
	case INTEGRAL_PATTERN:	/* These are the potentially ambiguous patterns */
	case REAL_PATTERN:
	case COMPLEX_PATTERN:
	case BITS_PATTERN:
	  {
	    if (last_pat != NULL)
	      {
		diagnostic (A_SYNTAX_ERROR, q, "A and A must be separated by a comma", ATTRIBUTE (last_pat), ATTRIBUTE (q));
	      }
	    last_pat = q;
	    break;
	  }
	case COMMA_SYMBOL:
	  {
	    last_pat = NULL;
	    break;
	  }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_format_texts: reduce format texts completely.                          */

static void
reduce_format_texts (NODE_T * p)
{
  NODE_T *q;
/* Replicators */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, REPLICATOR, STATIC_REPLICATOR, 0);
      f (q, NULL, NULL, REPLICATOR, DYNAMIC_REPLICATOR, 0);
    }
/* "OTHER" patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, STRING_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_S, 0);
      f (q, NULL, NULL, STRING_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_S, 0);
      f (q, NULL, NULL, STRING_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_S, 0);
      f (q, NULL, NULL, STRING_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_S, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, INTEGRAL_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_D, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
      f (q, NULL, NULL, FIXED_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_F, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_PLUS, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FLOAT_C_PATTERN, FORMAT_ITEM_ESCAPE, FORMAT_ITEM_MINUS, REPLICATOR, FORMAT_ITEM_POINT, REPLICATOR, FORMAT_ITEM_E, 0);
    }
/* Radix frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, RADIX_FRAME, REPLICATOR, FORMAT_ITEM_R, 0);
    }
/* Insertions */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INSERTION, FORMAT_ITEM_X, 0);
      f (q, not_supported, NULL, INSERTION, FORMAT_ITEM_Y, 0);
      f (q, NULL, NULL, INSERTION, FORMAT_ITEM_L, 0);
      f (q, NULL, NULL, INSERTION, FORMAT_ITEM_P, 0);
      f (q, NULL, NULL, INSERTION, FORMAT_ITEM_Q, 0);
      f (q, NULL, NULL, INSERTION, FORMAT_ITEM_K, 0);
      f (q, NULL, NULL, INSERTION, LITERAL, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INSERTION, REPLICATOR, INSERTION, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, INSERTION, INSERTION, INSERTION, 0);
	}
    }
/* Replicated suppressible frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FORMAT_A_FRAME, REPLICATOR, FORMAT_ITEM_S, FORMAT_ITEM_A, 0);
      f (q, NULL, NULL, FORMAT_Z_FRAME, REPLICATOR, FORMAT_ITEM_S, FORMAT_ITEM_Z, 0);
      f (q, NULL, NULL, FORMAT_D_FRAME, REPLICATOR, FORMAT_ITEM_S, FORMAT_ITEM_D, 0);
    }
/* Suppressible frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FORMAT_A_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_A, 0);
      f (q, NULL, NULL, FORMAT_Z_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_Z, 0);
      f (q, NULL, NULL, FORMAT_D_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, FORMAT_E_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FORMAT_POINT_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_POINT, 0);
      f (q, NULL, NULL, FORMAT_I_FRAME, FORMAT_ITEM_S, FORMAT_ITEM_I, 0);
    }
/* Replicated frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FORMAT_A_FRAME, REPLICATOR, FORMAT_ITEM_A, 0);
      f (q, NULL, NULL, FORMAT_Z_FRAME, REPLICATOR, FORMAT_ITEM_Z, 0);
      f (q, NULL, NULL, FORMAT_D_FRAME, REPLICATOR, FORMAT_ITEM_D, 0);
    }
/* Frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FORMAT_A_FRAME, FORMAT_ITEM_A, 0);
      f (q, NULL, NULL, FORMAT_Z_FRAME, FORMAT_ITEM_Z, 0);
      f (q, NULL, NULL, FORMAT_D_FRAME, FORMAT_ITEM_D, 0);
      f (q, NULL, NULL, FORMAT_E_FRAME, FORMAT_ITEM_E, 0);
      f (q, NULL, NULL, FORMAT_POINT_FRAME, FORMAT_ITEM_POINT, 0);
      f (q, NULL, NULL, FORMAT_I_FRAME, FORMAT_ITEM_I, 0);
    }
/* Frames with an insertion */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, FORMAT_A_FRAME, INSERTION, FORMAT_A_FRAME, 0);
      f (q, NULL, NULL, FORMAT_Z_FRAME, INSERTION, FORMAT_Z_FRAME, 0);
      f (q, NULL, NULL, FORMAT_D_FRAME, INSERTION, FORMAT_D_FRAME, 0);
      f (q, NULL, NULL, FORMAT_E_FRAME, INSERTION, FORMAT_E_FRAME, 0);
      f (q, NULL, NULL, FORMAT_POINT_FRAME, INSERTION, FORMAT_POINT_FRAME, 0);
      f (q, NULL, NULL, FORMAT_I_FRAME, INSERTION, FORMAT_I_FRAME, 0);
    }
/* String patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, STRING_PATTERN, REPLICATOR, FORMAT_A_FRAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, STRING_PATTERN, FORMAT_A_FRAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, STRING_PATTERN, STRING_PATTERN, STRING_PATTERN, 0);
	  f (q, NULL, &z, STRING_PATTERN, STRING_PATTERN, INSERTION, STRING_PATTERN, 0);
	}
    }
/* Integral moulds */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INTEGRAL_MOULD, FORMAT_Z_FRAME, 0);
      f (q, NULL, NULL, INTEGRAL_MOULD, FORMAT_D_FRAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z = A_TRUE;
      while (z)
	{
	  z = A_FALSE;
	  f (q, NULL, &z, INTEGRAL_MOULD, INTEGRAL_MOULD, INTEGRAL_MOULD, 0);
	  f (q, NULL, &z, INTEGRAL_MOULD, INTEGRAL_MOULD, INSERTION, 0);
	}
    }
/* Sign moulds */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_ITEM_PLUS, 0);
      f (q, NULL, NULL, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_ITEM_MINUS, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, SIGN_MOULD, FORMAT_ITEM_PLUS, 0);
      f (q, NULL, NULL, SIGN_MOULD, FORMAT_ITEM_MINUS, 0);
    }
/* Exponent frames */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, EXPONENT_FRAME, FORMAT_E_FRAME, SIGN_MOULD, INTEGRAL_MOULD, 0);
      f (q, NULL, NULL, EXPONENT_FRAME, FORMAT_E_FRAME, INTEGRAL_MOULD, 0);
    }
/* Real patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_POINT_FRAME, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, FORMAT_POINT_FRAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, FORMAT_POINT_FRAME, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, FORMAT_POINT_FRAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, REAL_PATTERN, INTEGRAL_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, INTEGRAL_MOULD, FORMAT_POINT_FRAME, INTEGRAL_MOULD, 0);
      f (q, NULL, NULL, REAL_PATTERN, INTEGRAL_MOULD, FORMAT_POINT_FRAME, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, INTEGRAL_MOULD, FORMAT_POINT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, FORMAT_POINT_FRAME, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, FORMAT_POINT_FRAME, INTEGRAL_MOULD, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, REAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
      f (q, NULL, NULL, REAL_PATTERN, INTEGRAL_MOULD, EXPONENT_FRAME, 0);
    }
/* Complex patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, COMPLEX_PATTERN, REAL_PATTERN, FORMAT_I_FRAME, REAL_PATTERN, 0);
    }
/* Bits patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, BITS_PATTERN, RADIX_FRAME, INTEGRAL_MOULD, 0);
    }
/* Integral patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, INTEGRAL_PATTERN, SIGN_MOULD, INTEGRAL_MOULD, 0);
      f (q, NULL, NULL, INTEGRAL_PATTERN, INTEGRAL_MOULD, 0);
    }
/* Patterns */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, BOOLEAN_PATTERN, FORMAT_ITEM_B, COLLECTION, 0);
      f (q, NULL, NULL, CHOICE_PATTERN, FORMAT_ITEM_C, COLLECTION, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, BOOLEAN_PATTERN, FORMAT_ITEM_B, 0);
      f (q, NULL, NULL, GENERAL_PATTERN, FORMAT_ITEM_G, 0);
    }
  ambiguous_patterns (p);
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, PATTERN, GENERAL_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, INTEGRAL_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, REAL_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, COMPLEX_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, BITS_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, STRING_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, BOOLEAN_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, CHOICE_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, FORMAT_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, STRING_C_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, INTEGRAL_C_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, FIXED_C_PATTERN, 0);
      f (q, NULL, NULL, PATTERN, FLOAT_C_PATTERN, 0);
    }
/* Pictures */
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, PICTURE, INSERTION, 0);
      f (q, NULL, NULL, PICTURE, PATTERN, 0);
      f (q, NULL, NULL, PICTURE, COLLECTION, 0);
      f (q, NULL, NULL, PICTURE, REPLICATOR, COLLECTION, 0);
    }
/* Picture lists */
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, PICTURE))
	{
	  BOOL_T z = A_TRUE;
	  f (q, NULL, NULL, PICTURE_LIST, PICTURE, 0);
	  while (z)
	    {
	      z = A_FALSE;
	      f (q, NULL, &z, PICTURE_LIST, PICTURE_LIST, COMMA_SYMBOL, PICTURE, 0);
	      /* We filtered ambiguous patterns, so commas may be omitted */
	      f (q, NULL, &z, PICTURE_LIST, PICTURE_LIST, PICTURE, 0);
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_secondaries: reduce secondaries completely.                            */

static void
reduce_secondaries (NODE_T * p)
{
  NODE_T *q;
  BOOL_T z;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, SECONDARY, PRIMARY, 0);
      f (q, NULL, NULL, GENERATOR, LOC_SYMBOL, DECLARER, 0);
      f (q, NULL, NULL, GENERATOR, HEAP_SYMBOL, DECLARER, 0);
      f (q, NULL, NULL, SECONDARY, GENERATOR, 0);
    }
  z = A_TRUE;
  while (z)
    {
      z = A_FALSE;
      q = p;
      while (NEXT (q) != NULL)
	{
	  q = NEXT (q);
	}
      for (; q != NULL; q = PREVIOUS (q))
	{
	  f (q, NULL, &z, SELECTION, SELECTOR, SECONDARY, 0);
	  f (q, NULL, &z, SECONDARY, SELECTION, 0);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
operator_with_priority: whether "q" is an operator with priority "k".         */

static int
operator_with_priority (NODE_T * q, int k)
{
  return (NEXT (q) != NULL && ATTRIBUTE (NEXT (q)) == OPERATOR && PRIO (NEXT (q)->info) == k);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_formulae: ibid.                                                        */

static void
reduce_formulae (NODE_T * p)
{
  NODE_T *q = p;
  int priority;
  while (q != NULL)
    {
      if (WHETHER (q, OPERATOR) || WHETHER (q, SECONDARY))
	{
	  q = reduce_dyadic (q, 0);
	}
      else
	{
	  q = NEXT (q);
	}
    }
/* Reduce the expression */
  for (priority = MAX_PRIORITY; priority >= 0; priority--)
    {
      for (q = p; q != NULL; q = NEXT (q))
	{
	  if (operator_with_priority (q, priority))
	    {
	      BOOL_T z = A_FALSE;
	      NODE_T *op = NEXT (q);
	      if (WHETHER (q, SECONDARY))
		{
		  f (q, NULL, &z, FORMULA, SECONDARY, OPERATOR, SECONDARY, 0);
		  f (q, NULL, &z, FORMULA, SECONDARY, OPERATOR, MONADIC_FORMULA, 0);
		  f (q, NULL, &z, FORMULA, SECONDARY, OPERATOR, FORMULA, 0);
		}
	      else if (WHETHER (q, MONADIC_FORMULA))
		{
		  f (q, NULL, &z, FORMULA, MONADIC_FORMULA, OPERATOR, SECONDARY, 0);
		  f (q, NULL, &z, FORMULA, MONADIC_FORMULA, OPERATOR, MONADIC_FORMULA, 0);
		  f (q, NULL, &z, FORMULA, MONADIC_FORMULA, OPERATOR, FORMULA, 0);
		}
	      if (priority == 0 && z)
		{
		  diagnostic (A_SYNTAX_ERROR, op, "no priority declaration for operator S", NULL);
		}
	      z = A_TRUE;
	      while (z)
		{
		  NODE_T *op = NEXT (q);
		  z = A_FALSE;
		  if (operator_with_priority (q, priority))
		    {
		      f (q, NULL, &z, FORMULA, FORMULA, OPERATOR, SECONDARY, 0);
		    }
		  if (operator_with_priority (q, priority))
		    {
		      f (q, NULL, &z, FORMULA, FORMULA, OPERATOR, MONADIC_FORMULA, 0);
		    }
		  if (operator_with_priority (q, priority))
		    {
		      f (q, NULL, &z, FORMULA, FORMULA, OPERATOR, FORMULA, 0);
		    }
		  if (priority == 0 && z)
		    {
		      diagnostic (A_SYNTAX_ERROR, op, "no priority declaration for operator S", NULL);
		    }
		}
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_dyadic: reduce dyadic expressions.

We work inside out - higher priority expressions get reduced first.           */

static NODE_T *
reduce_dyadic (NODE_T * p, int u)
{
  if (u > MAX_PRIORITY)
    {
      if (p == NULL)
	{
	  return (NULL);
	}
      else if (WHETHER (p, OPERATOR))
	{
/* Reduce monadic formulas */
	  NODE_T *q;
	  BOOL_T z;
	  q = p;
	  do
	    {
	      PRIO (q->info) = 10;
	      z = (NEXT (q) != NULL) && (WHETHER (NEXT (q), OPERATOR));
	      if (z)
		{
		  q = NEXT (q);
		}
	    }
	  while (z);
	  f (q, NULL, NULL, MONADIC_FORMULA, OPERATOR, SECONDARY, 0);
	  while (q != p)
	    {
	      q = PREVIOUS (q);
	      f (q, NULL, NULL, MONADIC_FORMULA, OPERATOR, MONADIC_FORMULA, 0);
	    }
	}
      p = NEXT (p);
    }
  else
    {
      p = reduce_dyadic (p, u + 1);
      while (p != NULL && WHETHER (p, OPERATOR) && PRIO (p->info) == u)
	{
	  p = NEXT (p);
	  p = reduce_dyadic (p, u + 1);
	}
    }
  return (p);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_tertiaries: reduce tertiaries completely.                              */

static void
reduce_tertiaries (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, TERTIARY, NIHIL, 0);
      f (q, NULL, NULL, FORMULA, MONADIC_FORMULA, 0);
      f (q, NULL, NULL, TERTIARY, FORMULA, 0);
      f (q, NULL, NULL, TERTIARY, SECONDARY, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, IDENTITY_RELATION, TERTIARY, IS_SYMBOL, TERTIARY, 0);
      f (q, NULL, NULL, IDENTITY_RELATION, TERTIARY, ISNT_SYMBOL, TERTIARY, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, AND_FUNCTION, TERTIARY, ANDF_SYMBOL, TERTIARY, 0);
      f (q, NULL, NULL, OR_FUNCTION, TERTIARY, ORF_SYMBOL, TERTIARY, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_qualifiers: reduce qualifiers in declarations.                         */

static void
reduce_qualifiers (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, not_implemented_yet, NULL, ACCESS, PUBLIC_SYMBOL, 0);
      f (q, not_implemented_yet, NULL, ACCESS, PRELUDE_SYMBOL, 0);
      f (q, not_implemented_yet, NULL, ACCESS, POSTLUDE_SYMBOL, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_basic_declarations: reduce declarations.                               */

static void
reduce_basic_declarations (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, PRIORITY_DECLARATION, ACCESS, PRIO_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, PRIORITY, 0);
      f (q, NULL, NULL, MODE_DECLARATION, ACCESS, MODE_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, DECLARER, 0);
      f (q, NULL, NULL, MODE_DECLARATION, ACCESS, MODE_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, VOID_SYMBOL, 0);
      f (q, NULL, NULL, PROCEDURE_DECLARATION, ACCESS, PROC_SYMBOL, DEFINING_IDENTIFIER, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, PROCEDURE_VARIABLE_DECLARATION, ACCESS, PROC_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, PROCEDURE_VARIABLE_DECLARATION, ACCESS, QUALIFIER, PROC_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, BRIEF_OPERATOR_DECLARATION, ACCESS, OP_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, ENVIRON_NAME, ENVIRON_SYMBOL, ROW_CHAR_DENOTER, 0);
      f (q, NULL, NULL, PRIORITY_DECLARATION, PRIO_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, PRIORITY, 0);
      f (q, NULL, NULL, MODE_DECLARATION, MODE_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, DECLARER, 0);
      f (q, NULL, NULL, MODE_DECLARATION, MODE_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, VOID_SYMBOL, 0);
      f (q, NULL, NULL, PROCEDURE_DECLARATION, PROC_SYMBOL, DEFINING_IDENTIFIER, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, PROCEDURE_VARIABLE_DECLARATION, PROC_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, PROCEDURE_VARIABLE_DECLARATION, QUALIFIER, PROC_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, BRIEF_OPERATOR_DECLARATION, OP_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z;
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, ENVIRON_NAME, ENVIRON_NAME, COMMA_SYMBOL, ROW_CHAR_DENOTER, 0);
	  f (q, NULL, &z, PRIORITY_DECLARATION, PRIORITY_DECLARATION, COMMA_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, PRIORITY, 0);
	  f (q, NULL, &z, MODE_DECLARATION, MODE_DECLARATION, COMMA_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, DECLARER, 0);
	  f (q, NULL, &z, MODE_DECLARATION, MODE_DECLARATION, COMMA_SYMBOL, DEFINING_INDICANT, EQUALS_SYMBOL, VOID_SYMBOL, 0);
	  f (q, NULL, &z, PROCEDURE_DECLARATION, PROCEDURE_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
	  f (q, NULL, &z, PROCEDURE_VARIABLE_DECLARATION, PROCEDURE_VARIABLE_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, ROUTINE_TEXT, 0);
	  f (q, NULL, &z, BRIEF_OPERATOR_DECLARATION, BRIEF_OPERATOR_DECLARATION, COMMA_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, ROUTINE_TEXT, 0);
	}
      while (z);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_units: ibid.                                                           */

static void
reduce_units (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, UNIT, ASSIGNATION, 0);
      f (q, NULL, NULL, UNIT, IDENTITY_RELATION, 0);
      f (q, NULL, NULL, UNIT, AND_FUNCTION, 0);
      f (q, NULL, NULL, UNIT, OR_FUNCTION, 0);
      f (q, NULL, NULL, UNIT, ROUTINE_TEXT, 0);
      f (q, NULL, NULL, UNIT, JUMP, 0);
      f (q, NULL, NULL, UNIT, SKIP, 0);
      f (q, NULL, NULL, UNIT, TERTIARY, 0);
      f (q, NULL, NULL, UNIT, ASSERTION, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_generic_arguments: ibid.                                                        */

static void
reduce_generic_arguments (NODE_T * p)
{
  NODE_T *q;
  BOOL_T z;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, UNIT))
	{
	  f (q, NULL, NULL, TRIMMER, UNIT, COLON_SYMBOL, UNIT, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, COLON_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, COLON_SYMBOL, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, COLON_SYMBOL, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, DOTDOT_SYMBOL, UNIT, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, DOTDOT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, DOTDOT_SYMBOL, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, UNIT, DOTDOT_SYMBOL, 0);
	}
      else if (WHETHER (q, COLON_SYMBOL))
	{
	  f (q, NULL, NULL, TRIMMER, COLON_SYMBOL, UNIT, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, COLON_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, COLON_SYMBOL, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, COLON_SYMBOL, 0);
	}
      else if (WHETHER (q, DOTDOT_SYMBOL))
	{
	  f (q, NULL, NULL, TRIMMER, DOTDOT_SYMBOL, UNIT, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, DOTDOT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, DOTDOT_SYMBOL, AT_SYMBOL, UNIT, 0);
	  f (q, NULL, NULL, TRIMMER, DOTDOT_SYMBOL, 0);
	}
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, TRIMMER, AT_SYMBOL, UNIT, 0);
    }
  for (q = p; q && NEXT (q); q = NEXT (q))
    {
      if (WHETHER (q, COMMA_SYMBOL))
	{
	  if (!(ATTRIBUTE (NEXT (q)) == UNIT || ATTRIBUTE (NEXT (q)) == TRIMMER))
	    {
	      pad_node (q, TRIMMER);
	    }
	}
      else
	{
	  if (WHETHER (NEXT (q), COMMA_SYMBOL))
	    {
	      if (WHETHER_NOT (q, UNIT) && WHETHER_NOT (q, TRIMMER))
		{
		  pad_node (q, TRIMMER);
		}
	    }
	}
    }
  q = NEXT (p);
  abend (q == NULL, "erroneous parser state", NULL);
  f (q, NULL, NULL, GENERIC_ARGUMENT_LIST, UNIT, 0);
  f (q, NULL, NULL, GENERIC_ARGUMENT_LIST, TRIMMER, 0);
  do
    {
      z = A_FALSE;
      f (q, NULL, &z, GENERIC_ARGUMENT_LIST, GENERIC_ARGUMENT_LIST, COMMA_SYMBOL, UNIT, 0);
      f (q, NULL, &z, GENERIC_ARGUMENT_LIST, GENERIC_ARGUMENT_LIST, COMMA_SYMBOL, TRIMMER, 0);
      f (q, missing_separator, &z, GENERIC_ARGUMENT_LIST, GENERIC_ARGUMENT_LIST, UNIT, 0);
      f (q, missing_separator, &z, GENERIC_ARGUMENT_LIST, GENERIC_ARGUMENT_LIST, TRIMMER, 0);
    }
  while (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_bounds: ibid.                                                          */

static void
reduce_bounds (NODE_T * p)
{
  NODE_T *q;
  BOOL_T z;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, BOUND, UNIT, COLON_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, BOUND, UNIT, DOTDOT_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, BOUND, UNIT, 0);
    }
  q = NEXT (p);
  f (q, NULL, NULL, BOUNDS_LIST, BOUND, 0);
  f (q, NULL, NULL, FORMAL_BOUNDS_LIST, COMMA_SYMBOL, 0);
  f (q, NULL, NULL, ALT_FORMAL_BOUNDS_LIST, COLON_SYMBOL, 0);
  f (q, NULL, NULL, ALT_FORMAL_BOUNDS_LIST, DOTDOT_SYMBOL, 0);
  do
    {
      z = A_FALSE;
      f (q, NULL, &z, BOUNDS_LIST, BOUNDS_LIST, COMMA_SYMBOL, BOUND, 0);
      f (q, NULL, &z, FORMAL_BOUNDS_LIST, FORMAL_BOUNDS_LIST, COMMA_SYMBOL, 0);
      f (q, NULL, &z, ALT_FORMAL_BOUNDS_LIST, FORMAL_BOUNDS_LIST, COLON_SYMBOL, 0);
      f (q, NULL, &z, ALT_FORMAL_BOUNDS_LIST, FORMAL_BOUNDS_LIST, DOTDOT_SYMBOL, 0);
      f (q, NULL, &z, FORMAL_BOUNDS_LIST, ALT_FORMAL_BOUNDS_LIST, COMMA_SYMBOL, 0);
      f (q, missing_separator, &z, BOUNDS_LIST, BOUNDS_LIST, BOUND, 0);
    }
  while (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_arguments: reduce argument packs.                                      */

static void
reduce_arguments (NODE_T * p)
{
  NODE_T *q = NEXT (p);
  BOOL_T z;
  f (q, NULL, NULL, ARGUMENT_LIST, UNIT, 0);
  do
    {
      z = A_FALSE;
      f (q, NULL, &z, ARGUMENT_LIST, ARGUMENT_LIST, COMMA_SYMBOL, UNIT, 0);
      f (q, missing_separator, &z, ARGUMENT_LIST, ARGUMENT_LIST, UNIT, 0);
    }
  while (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_declaration_lists: ibid.                                               */

static void
reduce_declaration_lists (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, IDENTITY_DECLARATION, ACCESS, DECLARER, DEFINING_IDENTIFIER, EQUALS_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, ACCESS, QUALIFIER, DECLARER, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, ACCESS, QUALIFIER, DECLARER, DEFINING_IDENTIFIER, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, ACCESS, DECLARER, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, ACCESS, DECLARER, DEFINING_IDENTIFIER, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, IDENTITY_DECLARATION, DECLARER, DEFINING_IDENTIFIER, EQUALS_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, QUALIFIER, DECLARER, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, QUALIFIER, DECLARER, DEFINING_IDENTIFIER, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, DECLARER, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, VARIABLE_DECLARATION, DECLARER, DEFINING_IDENTIFIER, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z;
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, IDENTITY_DECLARATION, IDENTITY_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, EQUALS_SYMBOL, UNIT, 0);
	  f (q, NULL, &z, VARIABLE_DECLARATION, VARIABLE_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0);
	  if (!whether (q, VARIABLE_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, ASSIGN_SYMBOL, UNIT, 0))
	    {
	      f (q, NULL, &z, VARIABLE_DECLARATION, VARIABLE_DECLARATION, COMMA_SYMBOL, DEFINING_IDENTIFIER, 0);
	    }
	}
      while (z);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, OPERATOR_DECLARATION, ACCESS, OPERATOR_PLAN, DEFINING_OPERATOR, EQUALS_SYMBOL, UNIT, 0);
      f (q, NULL, NULL, OPERATOR_DECLARATION, OPERATOR_PLAN, DEFINING_OPERATOR, EQUALS_SYMBOL, UNIT, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z;
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, OPERATOR_DECLARATION, OPERATOR_DECLARATION, COMMA_SYMBOL, DEFINING_OPERATOR, EQUALS_SYMBOL, UNIT, 0);
	}
      while (z);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, DECLARATION_LIST, MODE_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, PRIORITY_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, BRIEF_OPERATOR_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, OPERATOR_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, IDENTITY_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, PROCEDURE_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, PROCEDURE_VARIABLE_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, VARIABLE_DECLARATION, 0);
      f (q, NULL, NULL, DECLARATION_LIST, ENVIRON_NAME, 0);
    }
  for (q = p; q != NULL; q = NEXT (q))
    {
      BOOL_T z;
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, DECLARATION_LIST, DECLARATION_LIST, COMMA_SYMBOL, DECLARATION_LIST, 0);
	}
      while (z);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_labels: reduce labels and specifiers.                                  */

static void
reduce_labels (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      f (q, NULL, NULL, LABELED_UNIT, LABEL, UNIT, 0);
      f (q, NULL, NULL, SPECIFIED_UNIT, SPECIFIER, COLON_SYMBOL, UNIT, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_serial_clauses: ibid.                                                  */

static void
reduce_serial_clauses (NODE_T * p)
{
  NODE_T *q = NEXT (p);
  BOOL_T z;
  if (q == NULL)
    {
      return;
    }
  f (q, NULL, NULL, SERIAL_CLAUSE, LABELED_UNIT, 0);
  f (q, NULL, NULL, SERIAL_CLAUSE, UNIT, 0);
  f (q, NULL, NULL, INITIALISER_SERIES, DECLARATION_LIST, 0);
  do
    {
      z = A_FALSE;
      if (WHETHER (q, SERIAL_CLAUSE))
	{
	  f (q, NULL, &z, SERIAL_CLAUSE, SERIAL_CLAUSE, SEMI_SYMBOL, UNIT, 0);
	  f (q, NULL, &z, SERIAL_CLAUSE, SERIAL_CLAUSE, EXIT_SYMBOL, LABELED_UNIT, 0);
	  f (q, NULL, &z, SERIAL_CLAUSE, SERIAL_CLAUSE, SEMI_SYMBOL, LABELED_UNIT, 0);
	  f (q, NULL, &z, INITIALISER_SERIES, SERIAL_CLAUSE, SEMI_SYMBOL, DECLARATION_LIST, 0);
	  /* Errors */
	  f (q, missing_separator, &z, SERIAL_CLAUSE, SERIAL_CLAUSE, UNIT, 0);
	  f (q, missing_separator, &z, SERIAL_CLAUSE, SERIAL_CLAUSE, LABELED_UNIT, 0);
	  f (q, missing_separator, &z, INITIALISER_SERIES, SERIAL_CLAUSE, DECLARATION_LIST, 0);
	}
      else if (WHETHER (q, INITIALISER_SERIES))
	{
	  f (q, NULL, &z, SERIAL_CLAUSE, INITIALISER_SERIES, SEMI_SYMBOL, UNIT, 0);
	  f (q, NULL, &z, SERIAL_CLAUSE, INITIALISER_SERIES, SEMI_SYMBOL, LABELED_UNIT, 0);
	  f (q, NULL, &z, INITIALISER_SERIES, INITIALISER_SERIES, SEMI_SYMBOL, DECLARATION_LIST, 0);
	  /* Errors */
	  f (q, missing_separator, &z, SERIAL_CLAUSE, INITIALISER_SERIES, UNIT, 0);
	  f (q, missing_separator, &z, SERIAL_CLAUSE, INITIALISER_SERIES, LABELED_UNIT, 0);
	  f (q, missing_separator, &z, INITIALISER_SERIES, INITIALISER_SERIES, DECLARATION_LIST, 0);
	}
    }
  while (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_enquiry_clauses: ibid.                                                 */

static void
reduce_enquiry_clauses (NODE_T * p)
{
  NODE_T *q = NEXT (p);
  BOOL_T z;
  f (q, NULL, NULL, ENQUIRY_CLAUSE, UNIT, 0);
  f (q, NULL, NULL, INITIALISER_SERIES, DECLARATION_LIST, 0);
  do
    {
      z = A_FALSE;
      if (WHETHER (q, ENQUIRY_CLAUSE))
	{
	  f (q, NULL, &z, ENQUIRY_CLAUSE, ENQUIRY_CLAUSE, SEMI_SYMBOL, UNIT, 0);
	  f (q, NULL, &z, INITIALISER_SERIES, ENQUIRY_CLAUSE, SEMI_SYMBOL, DECLARATION_LIST, 0);
	  f (q, missing_separator, &z, ENQUIRY_CLAUSE, ENQUIRY_CLAUSE, UNIT, 0);
	  f (q, missing_separator, &z, INITIALISER_SERIES, ENQUIRY_CLAUSE, DECLARATION_LIST, 0);
	}
      else if (WHETHER (q, INITIALISER_SERIES))
	{
	  f (q, NULL, &z, ENQUIRY_CLAUSE, INITIALISER_SERIES, SEMI_SYMBOL, UNIT, 0);
	  f (q, NULL, &z, INITIALISER_SERIES, INITIALISER_SERIES, SEMI_SYMBOL, DECLARATION_LIST, 0);
	  f (q, missing_separator, &z, ENQUIRY_CLAUSE, INITIALISER_SERIES, UNIT, 0);
	  f (q, missing_separator, &z, INITIALISER_SERIES, INITIALISER_SERIES, DECLARATION_LIST, 0);
	}
    }
  while (z);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_collateral_clauses: ibid.                                              */

static void
reduce_collateral_clauses (NODE_T * p)
{
  NODE_T *q = NEXT (p);
  if (WHETHER (q, UNIT))
    {
      BOOL_T z;
      f (q, NULL, NULL, UNIT_LIST, UNIT, 0);
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, UNIT_LIST, UNIT_LIST, COMMA_SYMBOL, UNIT, 0);
	  f (q, missing_separator, &z, UNIT_LIST, UNIT_LIST, UNIT, 0);
	}
      while (z);
    }
  else if (WHETHER (q, SPECIFIED_UNIT))
    {
      BOOL_T z;
      f (q, NULL, NULL, SPECIFIED_UNIT_LIST, SPECIFIED_UNIT, 0);
      do
	{
	  z = A_FALSE;
	  f (q, NULL, &z, SPECIFIED_UNIT_LIST, SPECIFIED_UNIT_LIST, COMMA_SYMBOL, SPECIFIED_UNIT, 0);
	  f (q, missing_separator, &z, SPECIFIED_UNIT_LIST, SPECIFIED_UNIT_LIST, SPECIFIED_UNIT, 0);
	}
      while (z);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_enclosed_clause_bits: reduces clause parts, before the clause itself.  */

static void
reduce_enclosed_clause_bits (NODE_T * p, int expect)
{
  if (SUB (p) != NULL)
    {
      return;
    }
  if (WHETHER (p, FOR_SYMBOL))
    {
      f (p, NULL, NULL, FOR_PART, FOR_SYMBOL, DEFINING_IDENTIFIER, 0);
    }
  else if (WHETHER (p, OPEN_SYMBOL))
    {
      if (expect == ENQUIRY_CLAUSE)
	{
	  f (p, NULL, NULL, OPEN_PART, OPEN_SYMBOL, ENQUIRY_CLAUSE, 0);
	}
      else if (expect == ARGUMENT)
	{
	  f (p, NULL, NULL, ARGUMENT, OPEN_SYMBOL, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, ARGUMENT, OPEN_SYMBOL, ARGUMENT_LIST, CLOSE_SYMBOL, 0);
	  f (p, empty_clause, NULL, ARGUMENT, OPEN_SYMBOL, INITIALISER_SERIES, CLOSE_SYMBOL, 0);
	}
      else if (expect == GENERIC_ARGUMENT)
	{
	  if (whether (p, OPEN_SYMBOL, CLOSE_SYMBOL, 0))
	    {
	      pad_node (p, TRIMMER);
	      f (p, NULL, NULL, GENERIC_ARGUMENT, OPEN_SYMBOL, TRIMMER, CLOSE_SYMBOL, 0);
	    }
	  f (p, NULL, NULL, GENERIC_ARGUMENT, OPEN_SYMBOL, GENERIC_ARGUMENT_LIST, CLOSE_SYMBOL, 0);
	}
      else if (expect == BOUNDS)
	{
	  f (p, NULL, NULL, FORMAL_BOUNDS, OPEN_SYMBOL, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, BOUNDS, OPEN_SYMBOL, BOUNDS_LIST, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, FORMAL_BOUNDS, OPEN_SYMBOL, FORMAL_BOUNDS_LIST, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, FORMAL_BOUNDS, OPEN_SYMBOL, ALT_FORMAL_BOUNDS_LIST, CLOSE_SYMBOL, 0);
	}
      else
	{
	  f (p, NULL, NULL, CLOSED_CLAUSE, OPEN_SYMBOL, SERIAL_CLAUSE, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, COLLATERAL_CLAUSE, OPEN_SYMBOL, UNIT_LIST, CLOSE_SYMBOL, 0);
	  f (p, NULL, NULL, COLLATERAL_CLAUSE, OPEN_SYMBOL, CLOSE_SYMBOL, 0);
	  f (p, empty_clause, NULL, CLOSED_CLAUSE, OPEN_SYMBOL, INITIALISER_SERIES, CLOSE_SYMBOL, 0);
	}
    }
  else if (WHETHER (p, SUB_SYMBOL))
    {
      if (expect == GENERIC_ARGUMENT)
	{
	  if (whether (p, SUB_SYMBOL, BUS_SYMBOL, 0))
	    {
	      pad_node (p, TRIMMER);
	      f (p, NULL, NULL, GENERIC_ARGUMENT, SUB_SYMBOL, TRIMMER, BUS_SYMBOL, 0);
	    }
	  f (p, NULL, NULL, GENERIC_ARGUMENT, SUB_SYMBOL, GENERIC_ARGUMENT_LIST, BUS_SYMBOL, 0);
	}
      else if (expect == BOUNDS)
	{
	  f (p, NULL, NULL, FORMAL_BOUNDS, SUB_SYMBOL, BUS_SYMBOL, 0);
	  f (p, NULL, NULL, BOUNDS, SUB_SYMBOL, BOUNDS_LIST, BUS_SYMBOL, 0);
	  f (p, NULL, NULL, FORMAL_BOUNDS, SUB_SYMBOL, FORMAL_BOUNDS_LIST, BUS_SYMBOL, 0);
	  f (p, NULL, NULL, FORMAL_BOUNDS, SUB_SYMBOL, ALT_FORMAL_BOUNDS_LIST, BUS_SYMBOL, 0);
	}
    }
  else if (WHETHER (p, BEGIN_SYMBOL))
    {
      f (p, NULL, NULL, COLLATERAL_CLAUSE, BEGIN_SYMBOL, UNIT_LIST, END_SYMBOL, 0);
      f (p, NULL, NULL, COLLATERAL_CLAUSE, BEGIN_SYMBOL, END_SYMBOL, 0);
      f (p, NULL, NULL, CLOSED_CLAUSE, BEGIN_SYMBOL, SERIAL_CLAUSE, END_SYMBOL, 0);
      f (p, empty_clause, NULL, CLOSED_CLAUSE, BEGIN_SYMBOL, INITIALISER_SERIES, END_SYMBOL, 0);
    }
  else if (WHETHER (p, FORMAT_DELIMITER_SYMBOL))
    {
      f (p, NULL, NULL, FORMAT_TEXT, FORMAT_DELIMITER_SYMBOL, PICTURE_LIST, FORMAT_DELIMITER_SYMBOL, 0);
      f (p, NULL, NULL, FORMAT_TEXT, FORMAT_DELIMITER_SYMBOL, FORMAT_DELIMITER_SYMBOL, 0);
    }
  else if (WHETHER (p, FORMAT_ITEM_OPEN))
    {
      f (p, NULL, NULL, COLLECTION, FORMAT_ITEM_OPEN, PICTURE_LIST, FORMAT_ITEM_CLOSE, 0);
    }
  else if (WHETHER (p, DEF_SYMBOL))
    {
/* Export-clauses are a bit of a future extension, but fragments are already here. */
      f (p, not_implemented_yet, NULL, EXPORT_CLAUSE, DEF_SYMBOL, INITIALISER_SERIES, FED_SYMBOL, 0);
    }
  else if (WHETHER (p, CODE_SYMBOL))
    {
      f (p, NULL, NULL, CODE_CLAUSE, CODE_SYMBOL, SERIAL_CLAUSE, EDOC_SYMBOL, 0);
    }
  else if (WHETHER (p, IF_SYMBOL))
    {
      f (p, NULL, NULL, IF_PART, IF_SYMBOL, ENQUIRY_CLAUSE, 0);
      f (p, empty_clause, NULL, IF_PART, IF_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, THEN_SYMBOL))
    {
      f (p, NULL, NULL, THEN_PART, THEN_SYMBOL, SERIAL_CLAUSE, 0);
      f (p, empty_clause, NULL, THEN_PART, THEN_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, ELSE_SYMBOL))
    {
      f (p, NULL, NULL, ELSE_PART, ELSE_SYMBOL, SERIAL_CLAUSE, 0);
      f (p, empty_clause, NULL, ELSE_PART, ELSE_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, ELIF_SYMBOL))
    {
      f (p, NULL, NULL, ELIF_IF_PART, ELIF_SYMBOL, ENQUIRY_CLAUSE, 0);
    }
  else if (WHETHER (p, CASE_SYMBOL))
    {
      f (p, NULL, NULL, CASE_PART, CASE_SYMBOL, ENQUIRY_CLAUSE, 0);
      f (p, empty_clause, NULL, CASE_PART, CASE_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, IN_SYMBOL))
    {
      f (p, NULL, NULL, INTEGER_IN_PART, IN_SYMBOL, UNIT_LIST, 0);
      f (p, NULL, NULL, UNITED_IN_PART, IN_SYMBOL, SPECIFIED_UNIT_LIST, 0);
    }
  else if (WHETHER (p, OUT_SYMBOL))
    {
      f (p, NULL, NULL, OUT_PART, OUT_SYMBOL, SERIAL_CLAUSE, 0);
      f (p, empty_clause, NULL, OUT_PART, OUT_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, OUSE_SYMBOL))
    {
      f (p, NULL, NULL, OUSE_CASE_PART, OUSE_SYMBOL, ENQUIRY_CLAUSE, 0);
    }
  else if (WHETHER (p, THEN_BAR_SYMBOL))
    {
      f (p, NULL, NULL, CHOICE, THEN_BAR_SYMBOL, SERIAL_CLAUSE, 0);
      f (p, NULL, NULL, INTEGER_CHOICE_CLAUSE, THEN_BAR_SYMBOL, UNIT_LIST, 0);
      f (p, NULL, NULL, UNITED_CHOICE, THEN_BAR_SYMBOL, SPECIFIED_UNIT_LIST, 0);
      f (p, NULL, NULL, UNITED_CHOICE, THEN_BAR_SYMBOL, SPECIFIED_UNIT, 0);
      f (p, empty_clause, NULL, CHOICE, THEN_BAR_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, ELSE_BAR_SYMBOL))
    {
      f (p, NULL, NULL, ELSE_OPEN_PART, ELSE_BAR_SYMBOL, ENQUIRY_CLAUSE, 0);
      f (p, empty_clause, NULL, ELSE_OPEN_PART, ELSE_BAR_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, FROM_SYMBOL))
    {
      f (p, NULL, NULL, FROM_PART, FROM_SYMBOL, UNIT, 0);
    }
  else if (WHETHER (p, BY_SYMBOL))
    {
      f (p, NULL, NULL, BY_PART, BY_SYMBOL, UNIT, 0);
    }
  else if (WHETHER (p, TO_SYMBOL))
    {
      f (p, NULL, NULL, TO_PART, TO_SYMBOL, UNIT, 0);
    }
  else if (WHETHER (p, WHILE_SYMBOL))
    {
      f (p, NULL, NULL, WHILE_PART, WHILE_SYMBOL, ENQUIRY_CLAUSE, 0);
      f (p, empty_clause, NULL, WHILE_PART, WHILE_SYMBOL, INITIALISER_SERIES, 0);
    }
  else if (WHETHER (p, DO_SYMBOL))
    {
      f (p, NULL, NULL, DO_PART, DO_SYMBOL, SERIAL_CLAUSE, OD_SYMBOL, 0);
    }
  else if (WHETHER (p, ALT_DO_SYMBOL))
    {
      f (p, NULL, NULL, ALT_DO_PART, ALT_DO_SYMBOL, SERIAL_CLAUSE, OD_SYMBOL, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_enclosed_clauses: ibid.                                                */

static void
reduce_enclosed_clauses (NODE_T * p)
{
  if (SUB (p) == NULL)
    {
      return;
    }
  if (WHETHER (p, OPEN_PART))
    {
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, OPEN_PART, CHOICE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, OPEN_PART, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, OPEN_PART, CHOICE, BRIEF_ELIF_IF_PART, 0);
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, OPEN_PART, INTEGER_CHOICE_CLAUSE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, OPEN_PART, INTEGER_CHOICE_CLAUSE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, OPEN_PART, INTEGER_CHOICE_CLAUSE, BRIEF_INTEGER_OUSE_PART, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, OPEN_PART, UNITED_CHOICE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, OPEN_PART, UNITED_CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, OPEN_PART, UNITED_CHOICE, BRIEF_UNITED_OUSE_PART, 0);
    }
  else if (WHETHER (p, ELSE_OPEN_PART))
    {
      f (p, NULL, NULL, BRIEF_ELIF_IF_PART, ELSE_OPEN_PART, CHOICE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_ELIF_IF_PART, ELSE_OPEN_PART, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_ELIF_IF_PART, ELSE_OPEN_PART, CHOICE, BRIEF_ELIF_IF_PART, 0);
      f (p, NULL, NULL, BRIEF_INTEGER_OUSE_PART, ELSE_OPEN_PART, INTEGER_CHOICE_CLAUSE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_INTEGER_OUSE_PART, ELSE_OPEN_PART, INTEGER_CHOICE_CLAUSE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_INTEGER_OUSE_PART, ELSE_OPEN_PART, INTEGER_CHOICE_CLAUSE, BRIEF_INTEGER_OUSE_PART, 0);
      f (p, NULL, NULL, BRIEF_UNITED_OUSE_PART, ELSE_OPEN_PART, UNITED_CHOICE, CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_UNITED_OUSE_PART, ELSE_OPEN_PART, UNITED_CHOICE, CLOSE_SYMBOL, 0);
      f (p, NULL, NULL, BRIEF_UNITED_OUSE_PART, ELSE_OPEN_PART, UNITED_CHOICE, BRIEF_UNITED_OUSE_PART, 0);
    }
  else if (WHETHER (p, IF_PART))
    {
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, IF_PART, THEN_PART, ELSE_PART, FI_SYMBOL, 0);
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, IF_PART, THEN_PART, ELIF_PART, 0);
      f (p, NULL, NULL, CONDITIONAL_CLAUSE, IF_PART, THEN_PART, FI_SYMBOL, 0);
    }
  else if (WHETHER (p, ELIF_IF_PART))
    {
      f (p, NULL, NULL, ELIF_PART, ELIF_IF_PART, THEN_PART, ELSE_PART, FI_SYMBOL, 0);
      f (p, NULL, NULL, ELIF_PART, ELIF_IF_PART, THEN_PART, FI_SYMBOL, 0);
      f (p, NULL, NULL, ELIF_PART, ELIF_IF_PART, THEN_PART, ELIF_PART, 0);
    }
  else if (WHETHER (p, CASE_PART))
    {
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, CASE_PART, INTEGER_IN_PART, OUT_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, CASE_PART, INTEGER_IN_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_CASE_CLAUSE, CASE_PART, INTEGER_IN_PART, INTEGER_OUT_PART, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, CASE_PART, UNITED_IN_PART, OUT_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, CASE_PART, UNITED_IN_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_CASE_CLAUSE, CASE_PART, UNITED_IN_PART, UNITED_OUSE_PART, 0);
    }
  else if (WHETHER (p, OUSE_CASE_PART))
    {
      f (p, NULL, NULL, INTEGER_OUT_PART, OUSE_CASE_PART, INTEGER_IN_PART, OUT_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_OUT_PART, OUSE_CASE_PART, INTEGER_IN_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, INTEGER_OUT_PART, OUSE_CASE_PART, INTEGER_IN_PART, INTEGER_OUT_PART, 0);
      f (p, NULL, NULL, UNITED_OUSE_PART, OUSE_CASE_PART, UNITED_IN_PART, OUT_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_OUSE_PART, OUSE_CASE_PART, UNITED_IN_PART, ESAC_SYMBOL, 0);
      f (p, NULL, NULL, UNITED_OUSE_PART, OUSE_CASE_PART, UNITED_IN_PART, UNITED_OUSE_PART, 0);
    }
  else if (WHETHER (p, FOR_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, BY_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, BY_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, BY_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, BY_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, BY_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, BY_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, FROM_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, BY_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, BY_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FOR_PART, ALT_DO_PART, 0);
    }
  else if (WHETHER (p, FROM_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, BY_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, BY_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, BY_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, BY_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, FROM_PART, ALT_DO_PART, 0);
    }
  else if (WHETHER (p, BY_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, BY_PART, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, BY_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, BY_PART, TO_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, BY_PART, ALT_DO_PART, 0);
    }
  else if (WHETHER (p, TO_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, TO_PART, WHILE_PART, ALT_DO_PART, 0);
      f (p, NULL, NULL, LOOP_CLAUSE, TO_PART, ALT_DO_PART, 0);
    }
  else if (WHETHER (p, WHILE_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, WHILE_PART, ALT_DO_PART, 0);
    }
  else if (WHETHER (p, DO_PART))
    {
      f (p, NULL, NULL, LOOP_CLAUSE, DO_PART, 0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
recover_from_error: substitute reduction when a phrase could not be parsed.
expect: any info on what was actually expected
suppress: suppresses a diagnostic message (nested c.q. related messages).

This routine does not do fancy things as that might introduce more errors.    */

static void
recover_from_error (NODE_T * p, int expect, BOOL_T suppress)
{
  NODE_T *q = p;
  if (p == NULL)
    {
      return;
    }
  if (!suppress)
    {
/* Give a general error message */
      if (expect == SOME_CLAUSE)
	{
	  expect = serial_or_collateral (p);
	}
      diagnostic (A_SYNTAX_ERROR, p, "in A, cannot recognise Y", expect, phrase_to_text (p, NULL));
      if (error_count >= MAX_ERRORS)
	{
	  longjmp (bottom_up_crash_exit, 1);
	}
    }
/* Try to prevent spurious messages by guessing what was expected */
  while (NEXT (q) != NULL)
    {
      q = NEXT (q);
    }
  if (WHETHER (p, BEGIN_SYMBOL) || WHETHER (p, OPEN_SYMBOL))
    {
      if (expect == ARGUMENT || expect == COLLATERAL_CLAUSE || expect == PARAMETER_PACK || expect == STRUCTURE_PACK || expect == UNION_PACK)
	{
	  make_sub (p, q, expect);
	}
      else if (expect == ENQUIRY_CLAUSE)
	{
	  make_sub (p, q, OPEN_PART);
	}
      else if (expect == FORMAL_DECLARERS)
	{
	  make_sub (p, q, FORMAL_DECLARERS);
	}
      else
	{
	  make_sub (p, q, CLOSED_CLAUSE);
	}
    }
  else if (WHETHER (p, FORMAT_DELIMITER_SYMBOL) && expect == FORMAT_TEXT)
    {
      make_sub (p, q, FORMAT_TEXT);
    }
  else if (WHETHER (p, DEF_SYMBOL))
    {
      make_sub (p, q, EXPORT_CLAUSE);
    }
  else if (WHETHER (p, CODE_SYMBOL))
    {
      make_sub (p, q, CODE_CLAUSE);
    }
  else if (WHETHER (p, THEN_BAR_SYMBOL) || WHETHER (p, CHOICE))
    {
      make_sub (p, q, CHOICE);
    }
  else if (WHETHER (p, IF_SYMBOL) || WHETHER (p, IF_PART))
    {
      make_sub (p, q, IF_PART);
    }
  else if (WHETHER (p, THEN_SYMBOL) || WHETHER (p, THEN_PART))
    {
      make_sub (p, q, THEN_PART);
    }
  else if (WHETHER (p, ELSE_SYMBOL) || WHETHER (p, ELSE_PART))
    {
      make_sub (p, q, ELSE_PART);
    }
  else if (WHETHER (p, ELIF_SYMBOL) || WHETHER (p, ELIF_IF_PART))
    {
      make_sub (p, q, ELIF_IF_PART);
    }
  else if (WHETHER (p, CASE_SYMBOL) || WHETHER (p, CASE_PART))
    {
      make_sub (p, q, CASE_PART);
    }
  else if (WHETHER (p, OUT_SYMBOL) || WHETHER (p, OUT_PART))
    {
      make_sub (p, q, OUT_PART);
    }
  else if (WHETHER (p, OUSE_SYMBOL) || WHETHER (p, OUSE_CASE_PART))
    {
      make_sub (p, q, OUSE_CASE_PART);
    }
  else if (WHETHER (p, FOR_SYMBOL) || WHETHER (p, FOR_PART))
    {
      make_sub (p, q, FOR_PART);
    }
  else if (WHETHER (p, FROM_SYMBOL) || WHETHER (p, FROM_PART))
    {
      make_sub (p, q, FROM_PART);
    }
  else if (WHETHER (p, BY_SYMBOL) || WHETHER (p, BY_PART))
    {
      make_sub (p, q, BY_PART);
    }
  else if (WHETHER (p, TO_SYMBOL) || WHETHER (p, TO_PART))
    {
      make_sub (p, q, TO_PART);
    }
  else if (WHETHER (p, WHILE_SYMBOL) || WHETHER (p, WHILE_PART))
    {
      make_sub (p, q, WHILE_PART);
    }
  else if (WHETHER (p, DO_SYMBOL) || WHETHER (p, DO_PART))
    {
      make_sub (p, q, DO_PART);
    }
  else if (WHETHER (p, ALT_DO_SYMBOL) || WHETHER (p, ALT_DO_PART))
    {
      make_sub (p, q, ALT_DO_PART);
    }
  else if (non_terminal_string (edit_line, expect) != NULL)
    {
      make_sub (p, q, expect);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
reduce_erroneous_units: heuristic aid in pinpointing errors.
Constructs are reduced to units in an attempt to limit spurious messages.     */

static void
reduce_erroneous_units (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
/* Some implementations allow selection from a tertiary, when there is no risk
of ambiguity. Algol68G follows RR, so some extra attention here to guide an 
unsuspecting user. */
      if (whether (q, SELECTOR, -SECONDARY, 0))
	{
	  diagnostic (A_SYNTAX_ERROR, NEXT (q), SYNTAX_ERROR_EXPECTED, SECONDARY);
	  f (q, NULL, NULL, UNIT, SELECTOR, WILDCARD, 0);
	}
/* Attention for identity relations that require tertiaries */
      if (whether (q, -TERTIARY, IS_SYMBOL, TERTIARY, 0) || whether (q, TERTIARY, IS_SYMBOL, -TERTIARY, 0) || whether (q, -TERTIARY, IS_SYMBOL, -TERTIARY, 0))
	{
	  diagnostic (A_SYNTAX_ERROR, NEXT (q), SYNTAX_ERROR_EXPECTED, TERTIARY);
	  f (q, NULL, NULL, UNIT, WILDCARD, IS_SYMBOL, WILDCARD, 0);
	}
      else if (whether (q, -TERTIARY, ISNT_SYMBOL, TERTIARY, 0) || whether (q, TERTIARY, ISNT_SYMBOL, -TERTIARY, 0) || whether (q, -TERTIARY, ISNT_SYMBOL, -TERTIARY, 0))
	{
	  diagnostic (A_SYNTAX_ERROR, NEXT (q), SYNTAX_ERROR_EXPECTED, TERTIARY);
	  f (q, NULL, NULL, UNIT, WILDCARD, ISNT_SYMBOL, WILDCARD, 0);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Here is a set of routines that gather definitions from phrases.
This way we can apply tags before defining them.
These routines do not look very elegant as they have to scan through all
kind of symbols to find a pattern that they recognise.                        */

/*-------1---------2---------3---------4---------5---------6---------7---------+
skip_unit: skip anything until a comma, semicolon or EXIT is found.
return: node from where to proceed.                                           */

static NODE_T *
skip_unit (NODE_T * q)
{
  for (; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, COMMA_SYMBOL))
	{
	  return (q);
	}
      else if (WHETHER (q, SEMI_SYMBOL))
	{
	  return (q);
	}
      else if (WHETHER (q, EXIT_SYMBOL))
	{
	  return (q);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
find_tag_definition: attribute of entry in symbol table, or 0 if not found    */

static int
find_tag_definition (SYMBOL_TABLE_T * table, char *name)
{
  if (table != NULL)
    {
      int ret = 0;
      TAG_T *s;
      BOOL_T found;
      found = A_FALSE;
      for (s = table->indicants; s != NULL && !found; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      ret += INDICANT;
	      found = A_TRUE;
	    }
	}
      found = A_FALSE;
      for (s = table->operators; s != NULL && !found; s = NEXT (s))
	{
	  if (SYMBOL (NODE (s)) == name)
	    {
	      ret += OPERATOR;
	      found = A_TRUE;
	    }
	}
      if (ret == 0)
	{
	  return (find_tag_definition (PREVIOUS (table), name));
	}
      else
	{
	  return (ret);
	}
    }
  else
    {
      return (0);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
elaborate_bold_tags: fill in whether bold tag is operator or indicant.        */

static void
elaborate_bold_tags (NODE_T * p)
{
  NODE_T *q;
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, BOLD_TAG))
	{
	  switch (find_tag_definition (SYMBOL_TABLE (q), SYMBOL (q)))
	    {
	    case 0:
	      {
		diagnostic (A_SYNTAX_ERROR, q, UNDECLARED_TAG);
		break;
	      }
	    case INDICANT:
	      {
		ATTRIBUTE (q) = INDICANT;
		break;
	      }
	    case OPERATOR:
	      {
		ATTRIBUTE (q) = OPERATOR;
		break;
	      }
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_indicants: search MODE A = .., B = .. and store indicants.            */

static void
extract_indicants (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (WHETHER (q, MODE_SYMBOL))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, BOLD_TAG, EQUALS_SYMBOL, 0))
		{
		  add_tag (SYMBOL_TABLE (p), INDICANT, q, NULL, 0);
		  ATTRIBUTE (q) = DEFINING_INDICANT;
		  q = NEXT (q);
		  q->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_priorities: search PRIO X = .., Y = .. and store priorities.          */

static void
extract_priorities (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (WHETHER (q, PRIO_SYMBOL))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, BOLD_TAG, EQUALS_SYMBOL, INT_DENOTER, 0) || whether (q, OPERATOR, EQUALS_SYMBOL, INT_DENOTER, 0) || whether (q, EQUALS_SYMBOL, EQUALS_SYMBOL, INT_DENOTER, 0))
		{
		  int k;
		  NODE_T *y = q;
		  ATTRIBUTE (q) = DEFINING_OPERATOR;
		  q = NEXT (q);
		  q->attribute = ALT_EQUALS_SYMBOL;
/* Check value, as the parser only handles up to MAX_PRIORITY */
		  q = NEXT (q);
		  k = atoi (SYMBOL (q));
		  if (k < 1 || k > MAX_PRIORITY)
		    {
		      diagnostic (A_SYNTAX_ERROR, q, "priority must be from 1 to D", MAX_PRIORITY);
		      k = MAX_PRIORITY;
		    }
		  ATTRIBUTE (q) = PRIORITY;
		  add_tag (SYMBOL_TABLE (p), PRIO_SYMBOL, y, NULL, k);
		  q = NEXT (q);
		}
	      else if (whether (q, BOLD_TAG, INT_DENOTER, 0) || whether (q, OPERATOR, INT_DENOTER, 0) || whether (q, EQUALS_SYMBOL, INT_DENOTER, 0))
		{
/* The scanner cannot separate operator and "=" sign so we do this here. */
		  int len = (int) strlen (SYMBOL (q));
		  if (len > 1 && SYMBOL (q)[len - 1] == '=')
		    {
		      int k;
		      NODE_T *y = q;
		      char *sym = (char *) get_temp_heap_space (len);
		      strncpy (sym, SYMBOL (q), len - 1);
		      sym[len] = '\0';
		      SYMBOL (q) = TEXT (add_token (&top_token, sym));
		      ATTRIBUTE (q) = DEFINING_OPERATOR;
		      insert_node (q, ALT_EQUALS_SYMBOL);
		      q = NEXT (q);
		      q = NEXT (q);
		      k = atoi (SYMBOL (q));
		      if (k < 1 || k > MAX_PRIORITY)
			{
			  diagnostic (A_SYNTAX_ERROR, q, "priority must be from 1 to D", MAX_PRIORITY);
			  k = MAX_PRIORITY;
			}
		      ATTRIBUTE (q) = PRIORITY;
		      add_tag (SYMBOL_TABLE (p), PRIO_SYMBOL, y, NULL, k);
		      q = NEXT (q);
		    }
		  else
		    {
		      diagnostic (A_SYNTAX_ERROR, p, SYNTAX_ERROR_EXPECTED, EQUALS_SYMBOL, NULL);
		    }
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_operators: search OP [( .. ) ..] X = .., Y = .. and store operators.  */

static void
extract_operators (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (WHETHER (q, OP_SYMBOL))
	{
	  BOOL_T z = A_TRUE;
/* Skip operator plan until next item is a candidate operator */
	  while (NEXT (q) != NULL && !(WHETHER (NEXT (q), OPERATOR) || WHETHER (NEXT (q), BOLD_TAG) || WHETHER (NEXT (q), EQUALS_SYMBOL)))
	    {
	      q = NEXT (q);
	    }
/* Sample operators */
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, OPERATOR, EQUALS_SYMBOL, 0) || whether (q, BOLD_TAG, EQUALS_SYMBOL, 0) || whether (q, EQUALS_SYMBOL, EQUALS_SYMBOL, 0))
		{
		  ATTRIBUTE (q) = DEFINING_OPERATOR;
		  add_tag (SYMBOL_TABLE (p), OP_SYMBOL, q, NULL, 0);
		  q = NEXT (q);
		  q->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else if (q != NULL && (WHETHER (q, OPERATOR) || WHETHER (q, BOLD_TAG) || WHETHER (q, EQUALS_SYMBOL)))
		{
/* The scanner cannot separate operator and "=" sign so we do this here. */
		  int len = (int) strlen (SYMBOL (q));
		  if (len > 1 && SYMBOL (q)[len - 1] == '=')
		    {
		      char *sym = (char *) get_temp_heap_space (len);
		      strncpy (sym, SYMBOL (q), len - 1);
		      sym[len] = '\0';
		      SYMBOL (q) = TEXT (add_token (&top_token, sym));
		      ATTRIBUTE (q) = DEFINING_OPERATOR;
		      insert_node (q, ALT_EQUALS_SYMBOL);
		      add_tag (SYMBOL_TABLE (p), OP_SYMBOL, q, NULL, 0);
		      q = NEXT (q);
		      q = skip_unit (q);
		    }
		  else
		    {
		      diagnostic (A_SYNTAX_ERROR, p, SYNTAX_ERROR_EXPECTED, EQUALS_SYMBOL, NULL);
		    }
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_labels: search and store labels.                                      */

static void
extract_labels (NODE_T * p, int expect)
{
  NODE_T *q;
/* Only handle candidate phrases as not to search indexers! */
  if (expect == SERIAL_CLAUSE || expect == ENQUIRY_CLAUSE || expect == SOME_CLAUSE)
    {
      for (q = p; q != NULL; q = NEXT (q))
	{
	  if (whether (q, IDENTIFIER, COLON_SYMBOL, 0))
	    {
	      TAG_T *z = add_tag (SYMBOL_TABLE (p), LABEL, q, NULL, LOCAL_LABEL);
	      ATTRIBUTE (q) = DEFINING_IDENTIFIER;
	      z->unit = NULL;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_identities: search MOID x = .., y = .. and store identifiers.         */

static void
extract_identities (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (whether (q, DECLARER, IDENTIFIER, EQUALS_SYMBOL, 0))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      if (whether ((q = NEXT (q)), IDENTIFIER, EQUALS_SYMBOL, 0))
		{
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  q = NEXT (q);
		  q->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else if (whether (q, IDENTIFIER, ASSIGN_SYMBOL, 0))
		{
/* Handle common error in ALGOL 68 programs */
		  diagnostic (A_SYNTAX_ERROR, q, SYNTAX_ERROR_MIXED);
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  (q = NEXT (q))->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_variables: search MOID x [:= ..], y [:= ..] and store identifiers.    */

static void
extract_variables (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (whether (q, DECLARER, IDENTIFIER, 0))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, IDENTIFIER, 0))
		{
		  if (whether (q, IDENTIFIER, EQUALS_SYMBOL, 0))
		    {
/* Handle common error in ALGOL 68 programs */
		      diagnostic (A_SYNTAX_ERROR, q, SYNTAX_ERROR_MIXED);
		      NEXT (q)->attribute = ASSIGN_SYMBOL;
		    }
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  q = skip_unit (q);
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_proc_identities: search PROC x = .., y = .. and stores identifiers.   */

static void
extract_proc_identities (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (whether (q, PROC_SYMBOL, IDENTIFIER, EQUALS_SYMBOL, 0))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, IDENTIFIER, EQUALS_SYMBOL, 0))
		{
		  TAG_T *t = add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  t->in_proc = A_TRUE;
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  (q = NEXT (q))->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else if (whether (q, IDENTIFIER, ASSIGN_SYMBOL, 0))
		{
/* Handle common error in ALGOL 68 programs */
		  diagnostic (A_SYNTAX_ERROR, q, SYNTAX_ERROR_MIXED);
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  (q = NEXT (q))->attribute = ALT_EQUALS_SYMBOL;
		  q = skip_unit (q);
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_proc_variables: search PROC x [:= ..], y [:= ..]; store identifiers.  */

static void
extract_proc_variables (NODE_T * p)
{
  NODE_T *q = p;
  while (q != NULL)
    {
      if (whether (q, PROC_SYMBOL, IDENTIFIER, 0))
	{
	  BOOL_T z = A_TRUE;
	  do
	    {
	      q = NEXT (q);
	      if (whether (q, IDENTIFIER, ASSIGN_SYMBOL, 0))
		{
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  q = skip_unit (q = NEXT (q));
		}
	      else if (whether (q, IDENTIFIER, EQUALS_SYMBOL, 0))
		{
/* Handle common error in ALGOL 68 programs */
		  diagnostic (A_SYNTAX_ERROR, q, SYNTAX_ERROR_MIXED);
		  add_tag (SYMBOL_TABLE (p), IDENTIFIER, q, NULL, NORMAL_IDENTIFIER);
		  ATTRIBUTE (q) = DEFINING_IDENTIFIER;
		  (q = NEXT (q))->attribute = ASSIGN_SYMBOL;
		  q = skip_unit (q);
		}
	      else
		{
		  z = A_FALSE;
		}
	    }
	  while (z && q != NULL && WHETHER (q, COMMA_SYMBOL));
	}
      else
	{
	  q = NEXT (q);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
extract_declarations: schedule gathering of definitions in a phrase.          */

static void
extract_declarations (NODE_T * p)
{
  NODE_T *q;
/* Get definitions so we know what is defined in this range */
  extract_identities (p);
  extract_variables (p);
  extract_proc_identities (p);
  extract_proc_variables (p);
/* By now we know whether "=" is an operator or not */
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, EQUALS_SYMBOL))
	{
	  ATTRIBUTE (q) = OPERATOR;
	}
      else if (WHETHER (q, ALT_EQUALS_SYMBOL))
	{
	  ATTRIBUTE (q) = EQUALS_SYMBOL;
	}
    }
/* Get qualifiers */
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (whether (q, LOC_SYMBOL, DECLARER, DEFINING_IDENTIFIER, 0))
	{
	  make_sub (q, q, QUALIFIER);
	}
      if (whether (q, HEAP_SYMBOL, DECLARER, DEFINING_IDENTIFIER, 0))
	{
	  make_sub (q, q, QUALIFIER);
	}
      if (whether (q, LOC_SYMBOL, PROC_SYMBOL, DEFINING_IDENTIFIER, 0))
	{
	  make_sub (q, q, QUALIFIER);
	}
      if (whether (q, HEAP_SYMBOL, PROC_SYMBOL, DEFINING_IDENTIFIER, 0))
	{
	  make_sub (q, q, QUALIFIER);
	}
    }
/* Give priorities to operators */
  for (q = p; q != NULL; q = NEXT (q))
    {
      if (WHETHER (q, OPERATOR))
	{
	  if (find_tag_global (SYMBOL_TABLE (q), OP_SYMBOL, SYMBOL (q)))
	    {
	      TAG_T *s = find_tag_global (SYMBOL_TABLE (q), PRIO_SYMBOL, SYMBOL (q));
	      if (s != NULL)
		{
		  PRIO (q->info) = PRIO (s);
		}
	      else
		{
		  PRIO (q->info) = 0;
		}
	    }
	  else
	    {
	      diagnostic (A_SYNTAX_ERROR, q, UNDECLARED_TAG);
	      PRIO (q->info) = 1;
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
A posteriori check of the syntax tree built by the BU parser.                 */

/*-------1---------2---------3---------4---------5---------6---------7---------+
check_export_clause: check import-export clause

Export-clauses are a bit of a future extension, but parts are already here.   */

static void
check_export_clause (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, UNIT))
	{
	  diagnostic (A_SYNTAX_ERROR, p, "export clause must be a proper declaration list");
	}
      else
	{
	  check_export_clause (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
count_pictures: ibid.                                                         */

static void
count_pictures (NODE_T * p, int *k)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, PICTURE))
	{
	  (*k)++;
	}
      count_pictures (SUB (p), k);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
bottom_up_error_check: driver for a posteriori error checking.                */

void
bottom_up_error_check (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, EXPORT_CLAUSE))
	{
	  check_export_clause (SUB (p));
	}
      else if (WHETHER (p, BOOLEAN_PATTERN))
	{
	  int k = 0;
	  count_pictures (SUB (p), &k);
	  if (k != 2)
	    {
	      diagnostic (A_SYNTAX_ERROR, p, "A should have two pictures", ATTRIBUTE (p), NULL);
	    }
	}
      else
	{
	  bottom_up_error_check (SUB (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
Next part rearranges the tree after the symbol tables are finished.           */

/*-------1---------2---------3---------4---------5---------6---------7---------+
rearrange_goto_less_jumps: transfer IDENTIFIER to JUMP where appropriate.     */

void
rearrange_goto_less_jumps (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, UNIT))
	{
	  NODE_T *q = SUB (p);
	  if (WHETHER (q, TERTIARY))
	    {
	      NODE_T *tertiary = q;
	      q = SUB (q);
	      if (q != NULL && WHETHER (q, SECONDARY))
		{
		  q = SUB (q);
		  if (q != NULL && WHETHER (q, PRIMARY))
		    {
		      q = SUB (q);
		      if (q != NULL && WHETHER (q, IDENTIFIER))
			{
			  if (whether_identifier_or_label_global (SYMBOL_TABLE (q), SYMBOL (q)) == LABEL)
			    {
			      ATTRIBUTE (tertiary) = JUMP;
			      SUB (tertiary) = q;
			    }
			}
		    }
		}
	    }
	}
      else if (WHETHER (p, TERTIARY))
	{
	  NODE_T *q = SUB (p);
	  if (q != NULL && WHETHER (q, SECONDARY))
	    {
	      NODE_T *secondary = q;
	      q = SUB (q);
	      if (q != NULL && WHETHER (q, PRIMARY))
		{
		  q = SUB (q);
		  if (q != NULL && WHETHER (q, IDENTIFIER))
		    {
		      if (whether_identifier_or_label_global (SYMBOL_TABLE (q), SYMBOL (q)) == LABEL)
			{
			  ATTRIBUTE (secondary) = JUMP;
			  SUB (secondary) = q;
			}
		    }
		}
	    }
	}
      else if (WHETHER (p, SECONDARY))
	{
	  NODE_T *q = SUB (p);
	  if (q != NULL && WHETHER (q, PRIMARY))
	    {
	      NODE_T *primary = q;
	      q = SUB (q);
	      if (q != NULL && WHETHER (q, IDENTIFIER))
		{
		  if (whether_identifier_or_label_global (SYMBOL_TABLE (q), SYMBOL (q)) == LABEL)
		    {
		      ATTRIBUTE (primary) = JUMP;
		      SUB (primary) = q;
		    }
		}
	    }
	}
      else if (WHETHER (p, PRIMARY))
	{
	  NODE_T *q = SUB (p);
	  if (q != NULL && WHETHER (q, IDENTIFIER))
	    {
	      if (whether_identifier_or_label_global (SYMBOL_TABLE (q), SYMBOL (q)) == LABEL)
		{
		  make_sub (q, q, JUMP);
		}
	    }
	}
      rearrange_goto_less_jumps (SUB (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
VICTAL CHECKER. Checks correct use of formal, actual and virtual declarers.   */

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_generator : ibid.                                                */

static void
victal_check_generator (NODE_T * p)
{
  if (!victal_check_declarer (NEXT (p), ACTUAL_DECLARER_MARK))
    {
      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "actual declarer");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_formal_pack : ibid.                                              */

static void
victal_check_formal_pack (NODE_T * p, int x, BOOL_T * z)
{
  if (p != NULL)
    {
      if (WHETHER (p, FORMAL_DECLARERS))
	{
	  victal_check_formal_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_formal_pack (NEXT (p), x, z);
	}
      else if (WHETHER (p, FORMAL_DECLARERS_LIST))
	{
	  victal_check_formal_pack (NEXT (p), x, z);
	  victal_check_formal_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, DECLARER))
	{
	  victal_check_formal_pack (NEXT (p), x, z);
	  *z &= victal_check_declarer (SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_operator_dec: ibid.                                              */

static void
victal_check_operator_dec (NODE_T * p)
{
  if (WHETHER (NEXT (p), FORMAL_DECLARERS))
    {
      BOOL_T z = A_TRUE;
      victal_check_formal_pack (NEXT (p), FORMAL_DECLARER_MARK, &z);
      if (!z)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarers");
	}
      p = NEXT (p);
    }
  if (!victal_check_declarer (NEXT (p), FORMAL_DECLARER_MARK))
    {
      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_mode_dec: ibid.                                                  */

static void
victal_check_mode_dec (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, MODE_DECLARATION))
	{
	  victal_check_mode_dec (SUB (p));
	  victal_check_mode_dec (NEXT (p));
	}
      else if (WHETHER (p, MODE_SYMBOL) || WHETHER (p, DEFINING_INDICANT) || WHETHER (p, EQUALS_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_mode_dec (NEXT (p));
	}
      else if (WHETHER (p, DECLARER))
	{
	  if (!victal_check_declarer (p, ACTUAL_DECLARER_MARK))
	    {
	      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "actual declarer");
	    }
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_variable_dec: ibid.                                              */

static void
victal_check_variable_dec (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  victal_check_variable_dec (SUB (p));
	  victal_check_variable_dec (NEXT (p));
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER) || WHETHER (p, ASSIGN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_variable_dec (NEXT (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  victal_checker (SUB (p));
	}
      else if (WHETHER (p, DECLARER))
	{
	  if (!victal_check_declarer (p, ACTUAL_DECLARER_MARK))
	    {
	      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "actual declarer");
	    }
	  victal_check_variable_dec (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_identity_dec: ibid.                                              */

static void
victal_check_identity_dec (NODE_T * p)
{
  if (p != NULL)
    {
      if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  victal_check_identity_dec (SUB (p));
	  victal_check_identity_dec (NEXT (p));
	}
      else if (WHETHER (p, DEFINING_IDENTIFIER) || WHETHER (p, EQUALS_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_identity_dec (NEXT (p));
	}
      else if (WHETHER (p, UNIT))
	{
	  victal_checker (SUB (p));
	}
      else if (WHETHER (p, DECLARER))
	{
	  if (!victal_check_declarer (p, FORMAL_DECLARER_MARK))
	    {
	      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
	    }
	  victal_check_identity_dec (NEXT (p));
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_routine_pack: ibid.                                              */

static void
victal_check_routine_pack (NODE_T * p, int x, BOOL_T * z)
{
  if (p != NULL)
    {
      if (WHETHER (p, PARAMETER_PACK))
	{
	  victal_check_routine_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_routine_pack (NEXT (p), x, z);
	}
      else if (WHETHER (p, PARAMETER_LIST) || WHETHER (p, PARAMETER))
	{
	  victal_check_routine_pack (NEXT (p), x, z);
	  victal_check_routine_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, DECLARER))
	{
	  *z &= victal_check_declarer (SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_routine_text: ibid.                                              */

static void
victal_check_routine_text (NODE_T * p)
{
  if (WHETHER (p, PARAMETER_PACK))
    {
      BOOL_T z = A_TRUE;
      victal_check_routine_pack (p, FORMAL_DECLARER_MARK, &z);
      if (!z)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarers");
	}
      p = NEXT (p);
    }
  if (!victal_check_declarer (p, FORMAL_DECLARER_MARK))
    {
      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
    }
  victal_checker (NEXT (p));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_structure_pack: ibid.                                            */

static void
victal_check_structure_pack (NODE_T * p, int x, BOOL_T * z)
{
  if (p != NULL)
    {
      if (WHETHER (p, STRUCTURE_PACK))
	{
	  victal_check_structure_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL))
	{
	  victal_check_structure_pack (NEXT (p), x, z);
	}
      else if (WHETHER (p, STRUCTURED_FIELD_LIST) || WHETHER (p, STRUCTURED_FIELD))
	{
	  victal_check_structure_pack (NEXT (p), x, z);
	  victal_check_structure_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, DECLARER))
	{
	  *z &= victal_check_declarer (SUB (p), x);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_union_pack: ibid.                                                */

static void
victal_check_union_pack (NODE_T * p, int x, BOOL_T * z)
{
  if (p != NULL)
    {
      if (WHETHER (p, UNION_PACK))
	{
	  victal_check_union_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, OPEN_SYMBOL) || WHETHER (p, COMMA_SYMBOL) || WHETHER (p, VOID_SYMBOL))
	{
	  victal_check_union_pack (NEXT (p), x, z);
	}
      else if (WHETHER (p, UNION_DECLARER_LIST))
	{
	  victal_check_union_pack (NEXT (p), x, z);
	  victal_check_union_pack (SUB (p), x, z);
	}
      else if (WHETHER (p, DECLARER))
	{
	  victal_check_union_pack (NEXT (p), x, z);
	  *z &= victal_check_declarer (SUB (p), FORMAL_DECLARER_MARK);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_declarer: ibid.                                                  */

static BOOL_T
victal_check_declarer (NODE_T * p, int x)
{
  if (p == NULL)
    {
      return (A_FALSE);
    }
  else if (WHETHER (p, DECLARER))
    {
      return (victal_check_declarer (SUB (p), x));
    }
  else if (WHETHER (p, LONGETY) || WHETHER (p, SHORTETY))
    {
      return (A_TRUE);
    }
  else if (WHETHER (p, VOID_SYMBOL) || WHETHER (p, INDICANT) || WHETHER (p, STANDARD))
    {
      return (A_TRUE);
    }
  else if (WHETHER (p, REF_SYMBOL))
    {
      return (victal_check_declarer (NEXT (p), VIRTUAL_DECLARER_MARK));
    }
  else if (WHETHER (p, FLEX_SYMBOL))
    {
      return (victal_check_declarer (NEXT (p), x));
    }
  else if (WHETHER (p, BOUNDS))
    {
      victal_checker (SUB (p));
      if (x == FORMAL_DECLARER_MARK)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal bounds");
	  victal_check_declarer (NEXT (p), x);
	  return (A_TRUE);
	}
      else if (x == VIRTUAL_DECLARER_MARK)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "virtual bounds");
	  victal_check_declarer (NEXT (p), x);
	  return (A_TRUE);
	}
      else
	{
	  return (victal_check_declarer (NEXT (p), x));
	}
    }
  else if (WHETHER (p, FORMAL_BOUNDS))
    {
      victal_checker (SUB (p));
      if (x == ACTUAL_DECLARER_MARK)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "actual bounds");
	  victal_check_declarer (NEXT (p), x);
	  return (A_TRUE);
	}
      else
	{
	  return (victal_check_declarer (NEXT (p), x));
	}
    }
  else if (WHETHER (p, STRUCT_SYMBOL))
    {
      BOOL_T z = A_TRUE;
      victal_check_structure_pack (NEXT (p), x, &z);
      return (z);
    }
  else if (WHETHER (p, UNION_SYMBOL))
    {
      BOOL_T z = A_TRUE;
      victal_check_union_pack (NEXT (p), FORMAL_DECLARER_MARK, &z);
      if (!z)
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer pack");
	}
      return (A_TRUE);
    }
  else if (WHETHER (p, PROC_SYMBOL))
    {
      if (WHETHER (NEXT (p), FORMAL_DECLARERS))
	{
	  BOOL_T z = A_TRUE;
	  victal_check_formal_pack (NEXT (p), FORMAL_DECLARER_MARK, &z);
	  if (!z)
	    {
	      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
	    }
	  p = NEXT (p);
	}
      if (!victal_check_declarer (NEXT (p), FORMAL_DECLARER_MARK))
	{
	  diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
	}
      return (A_TRUE);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_check_cast: ibid.                                                      */

static void
victal_check_cast (NODE_T * p)
{
  if (!victal_check_declarer (p, FORMAL_DECLARER_MARK))
    {
      diagnostic (A_SYNTAX_ERROR, p, EXPECTED, "formal declarer");
      victal_checker (NEXT (p));
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
victal_checker: driver for checking VICTALITY of declarers.                   */

void
victal_checker (NODE_T * p)
{
  for (; p != NULL; p = NEXT (p))
    {
      if (WHETHER (p, MODE_DECLARATION))
	{
	  victal_check_mode_dec (SUB (p));
	}
      else if (WHETHER (p, VARIABLE_DECLARATION))
	{
	  victal_check_variable_dec (SUB (p));
	}
      else if (WHETHER (p, IDENTITY_DECLARATION))
	{
	  victal_check_identity_dec (SUB (p));
	}
      else if (WHETHER (p, GENERATOR))
	{
	  victal_check_generator (SUB (p));
	}
      else if (WHETHER (p, ROUTINE_TEXT))
	{
	  victal_check_routine_text (SUB (p));
	}
      else if (WHETHER (p, OPERATOR_PLAN))
	{
	  victal_check_operator_dec (SUB (p));
	}
      else if (WHETHER (p, CAST))
	{
	  victal_check_cast (SUB (p));
	}
      else
	{
	  victal_checker (SUB (p));
	}
    }
}
