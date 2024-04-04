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

#include "algol68g.h"
#include "genie.h"
#include "gsl.h"
#include "transput.h"

struct _STANDENV_NAME_
{
  GENIE_PROCEDURE *proc;
  char *text;
};

typedef struct _STANDENV_NAME_ STANDENV_NAME;

struct _FUNCTION_NAME_
{
  PROPAGATOR_PROCEDURE *proc;
  char *text;
};

typedef struct _FUNCTION_NAME_ FUNCTION_NAME;

/*-------1---------2---------3---------4---------5---------6---------7---------+
Table with interpreter routines. These appear in the listing file so you can see
what construct invokes what routine. Use -extensive.                          */

static FUNCTION_NAME fun_names[] = {
  {genie_and_function, "genie_and_function"},
  {genie_assertion, "genie_assertion"},
  {genie_assignation, "genie_assignation"},
  {genie_call, "genie_call"},
  {genie_call_quick, "genie_call_quick"},
  {genie_call_standenv_quick, "genie_call_standenv_quick"},
  {genie_cast, "genie_cast"},
  {genie_closed, "genie_closed"},
  {genie_coercion, "genie_coercion"},
  {genie_collateral, "genie_collateral"},
  {genie_constant, "genie_constant"},
  {genie_denoter, "genie_denoter"},
  {genie_deproceduring, "genie_deproceduring"},
  {genie_dereference_identifier, "genie_dereference_identifier"},
  {genie_dereference_loc_identifier, "genie_dereference_loc_identifier"},
  {genie_dereferencing, "genie_dereferencing"},
  {genie_dereferencing_quick, "genie_dereferencing_quick"},
  {genie_enclosed, "genie_enclosed"},
  {genie_format_text, "genie_format_text"},
  {genie_formula, "genie_formula"},
  {genie_formula_quick, "genie_formula_quick"},
  {genie_formula_quick, "genie_formula_quick"},
  {genie_formula_standenv_quick, "genie_formula_standenv_quick"},
  {genie_generator, "genie_generator"},
  {genie_identifier, "genie_identifier"},
  {genie_identifier_standenv, "genie_identifier_standenv"},
  {genie_identifier_standenv_proc, "genie_identifier_standenv_proc"},
  {genie_identity_relation, "genie_identity_relation"},
  {genie_loc_assignation, "genie_loc_assignation"},
  {genie_loc_constant_assignation, "genie_loc_constant_assignation"},
  {genie_loc_identifier, "genie_loc_identifier"},
  {genie_loop, "genie_loop"},
  {genie_monadic, "genie_monadic"},
  {genie_nihil, "genie_nihil"},
  {genie_or_function, "genie_or_function"},
  {genie_primary, "genie_primary"},
  {genie_routine_text, "genie_routine_text"},
  {genie_rowing, "genie_rowing"},
  {genie_rowing_ref_row_of_row, "genie_rowing_ref_row_of_row"},
  {genie_rowing_ref_row_row, "genie_rowing_ref_row_row"},
  {genie_rowing_row_of_row, "genie_rowing_row_of_row"},
  {genie_rowing_row_row, "genie_rowing_row_row"},
  {genie_secondary, "genie_secondary"},
  {genie_selection, "genie_selection"},
  {genie_selection_name, "genie_selection_name"},
  {genie_selection_value, "genie_selection_value"},
  {genie_skip, "genie_skip"},
  {genie_slice, "genie_slice"},
  {genie_slice_name_quick, "genie_slice_name_quick"},
  {genie_tertiary, "genie_tertiary"},
  {genie_unit, "genie_unit"},
  {genie_uniting, "genie_uniting"},
  {genie_voiding, "genie_voiding"},
  {genie_voiding_loc_assignation, "genie_voiding_loc_assignation"},
  {genie_voiding_loc_constant_assignation, "genie_voiding_loc_constant_assignation"},
  {genie_widening, "genie_widening"},
  {genie_widening_int_to_real, "genie_widening_int_to_real"},
  {NULL, NULL}
};

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_function_name: name for interpreter routine "p".                        */

char *
genie_function_name (PROPAGATOR_PROCEDURE * p)
{
  int i = 0;
  while (fun_names[i].proc != NULL)
    {
      if (p == fun_names[i].proc)
	{
	  return (fun_names[i].text);
	}
      i++;
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
*/

static char *attribute_names[WILDCARD + 1] = {
  NULL, "ACCESS", "ACCO_SYMBOL", "ALT_DO_PART", "ALT_DO_SYMBOL", "ALT_EQUALS_SYMBOL", "ALT_FORMAL_BOUNDS_LIST",
  "ANDF_SYMBOL", "AND_FUNCTION", "ARGUMENT", "ARGUMENT_LIST", "ASSERT_SYMBOL", "ASSERTION", "ASSIGNATION", "ASSIGN_SYMBOL",
  "ASSIGN_TO_SYMBOL", "AT_SYMBOL", "BEGIN_SYMBOL", "BITS_DENOTER", "BITS_PATTERN", "BITS_SYMBOL",
  "BOLD_COMMENT_SYMBOL", "BOLD_PRAGMAT_SYMBOL", "BOLD_TAG", "BOOLEAN_PATTERN", "BOOL_SYMBOL", "BOUND", "BOUNDS",
  "BOUNDS_LIST", "BRIEF_ELIF_IF_PART", "BRIEF_INTEGER_OUSE_PART", "BRIEF_OPERATOR_DECLARATION",
  "BRIEF_UNITED_OUSE_PART", "BUS_SYMBOL", "BY_PART", "BY_SYMBOL", "BYTES_SYMBOL", "CALL", "CASE_PART", "CASE_SYMBOL", "CAST",
  "CHANNEL_SYMBOL", "CHAR_SYMBOL", "CHOICE", "CHOICE_PATTERN", "CLOSED_CLAUSE", "CLOSE_SYMBOL", "CODE_CLAUSE",
  "CODE_SYMBOL", "COLLATERAL_CLAUSE", "COLLECTION", "COLON_SYMBOL", "COMMA_SYMBOL", "COMPLEX_PATTERN",
  "COMPLEX_SYMBOL", "COMPL_SYMBOL", "CONDITIONAL_CLAUSE", "DECLARATION_LIST", "DECLARER", "DEFINING_IDENTIFIER",
  "DEFINING_INDICANT", "DEFINING_OPERATOR", "DEF_SYMBOL", "DENOTER", "DEPROCEDURING", "DEREFERENCING",
  "DOTDOT_SYMBOL", "DO_PART", "DO_SYMBOL", "DYNAMIC_REPLICATOR", "EDOC_SYMBOL", "ELIF_IF_PART", "ELIF_PART",
  "ELIF_SYMBOL", "ELSE_BAR_SYMBOL", "ELSE_OPEN_PART", "ELSE_PART", "ELSE_SYMBOL", "EMPTY_SYMBOL", "ENCLOSED_CLAUSE",
  "END_SYMBOL", "ENQUIRY_CLAUSE", "ENVIRON_NAME", "ENVIRON_SYMBOL", "EQUALS_SYMBOL", "ERROR", "ESAC_SYMBOL",
  "EXIT_SYMBOL", "EXPONENT_FRAME", "EXPORT_CLAUSE", "FALSE_SYMBOL", "FED_SYMBOL", "FIELD_IDENTIFIER", "FILE_SYMBOL",
  "FI_SYMBOL", "FLEX_SYMBOL", "FORMAL_BOUNDS", "FORMAL_BOUNDS_LIST", "FORMAL_DECLARERS", "FORMAL_DECLARERS_LIST",
  "FORMAT_A_FRAME", "FORMAT_DELIMITER_SYMBOL", "FORMAT_D_FRAME", "FORMAT_E_FRAME",
  "FORMAT_ITEM_ESCAPE", "FORMAT_ITEM_A", "FORMAT_ITEM_B",
  "FORMAT_ITEM_C", "FORMAT_ITEM_CLOSE", "FORMAT_ITEM_D", "FORMAT_ITEM_E", "FORMAT_ITEM_F", "FORMAT_ITEM_G",
  "FORMAT_ITEM_H", "FORMAT_ITEM_I", "FORMAT_ITEM_J", "FORMAT_ITEM_K", "FORMAT_ITEM_L", "FORMAT_ITEM_M",
  "FORMAT_ITEM_MINUS", "FORMAT_ITEM_N", "FORMAT_ITEM_O", "FORMAT_ITEM_OPEN", "FORMAT_ITEM_P", "FORMAT_ITEM_PLUS",
  "FORMAT_ITEM_POINT", "FORMAT_ITEM_Q", "FORMAT_ITEM_R", "FORMAT_ITEM_S", "FORMAT_ITEM_T", "FORMAT_ITEM_U",
  "FORMAT_ITEM_V", "FORMAT_ITEM_W", "FORMAT_ITEM_X", "FORMAT_ITEM_Y", "FORMAT_ITEM_Z", "FORMAT_I_FRAME",
  "FORMAT_PATTERN", "FORMAT_POINT_FRAME", "FORMAT_SYMBOL", "FORMAT_TEXT", "FORMAT_Z_FRAME",
  "INTEGRAL_C_PATTERN", "FLOAT_C_PATTERN", "FIXED_C_PATTERN", "STRING_C_PATTERN", "FORMULA", "FOR_PART",
  "FOR_SYMBOL", "FROM_PART", "FROM_SYMBOL", "GENERAL_PATTERN", "GENERATOR", "GOTO_SYMBOL", "GO_SYMBOL", "HEAP_SYMBOL",
  "IDENTIFIER", "IDENTITY_DECLARATION", "IDENTITY_RELATION", "IF_PART", "IF_SYMBOL", "GENERIC_ARGUMENT", "GENERIC_ARGUMENT_LIST",
  "INDICANT", "INITIALISER_SERIES", "INSERTION", "INTEGER_CASE_CLAUSE", "INTEGER_CHOICE_CLAUSE", "INTEGER_IN_PART",
  "INTEGER_OUT_PART", "INTEGRAL_MOULD", "INTEGRAL_PATTERN", "INT_DENOTER", "INT_SYMBOL", "IN_SYMBOL", "IN_TYPE_MODE",
  "ISNT_SYMBOL", "IS_SYMBOL", "JUMP", "LABEL", "LABELED_UNIT", "LABEL_IDENTIFIER", "LABEL_SEQUENCE", "LITERAL",
  "LOC_SYMBOL", "LONGETY", "LONG_SYMBOL", "LOOP_CLAUSE", "MAIN_SYMBOL", "MODE_DECLARATION", "MODE_SYMBOL",
  "MONADIC_FORMULA", "MONAD_SEQUENCE", "NIHIL", "NIL_SYMBOL", "OCCA_SYMBOL", "OD_SYMBOL", "OF_SYMBOL", "OPEN_PART",
  "OPEN_SYMBOL", "OPERATOR", "OPERATOR_DECLARATION", "OPERATOR_PLAN", "OP_SYMBOL", "ORF_SYMBOL", "OR_FUNCTION",
  "OUSE_CASE_PART", "OUSE_SYMBOL", "OUT_PART", "OUT_SYMBOL", "OUT_TYPE_MODE", "PARALLEL_CLAUSE", "PARAMETER",
  "PARAMETER_LIST", "PARAMETER_PACK", "PARTICULAR_PROGRAM", "PAR_SYMBOL", "PATTERN", "PICTURE", "PICTURE_LIST",
  "PIPE_SYMBOL", "POINT_SYMBOL", "POSTLUDE_SYMBOL", "SEMA_SYMBOL", "PRELUDE_SYMBOL", "PRIMARY", "PRIORITY",
  "PRIORITY_DECLARATION", "PRIO_SYMBOL", "PRIVATE_SYMBOL", "PROCEDURE_DECLARATION",
  "PROCEDURE_VARIABLE_DECLARATION", "PROCEDURING", "PROC_SYMBOL", "PUBLIC_SYMBOL", "QUALIFIER", "RADIX_FRAME",
  "REAL_DENOTER", "REAL_PATTERN", "REAL_SYMBOL", "REF_SYMBOL", "REPLICATOR", "ROUTINE_TEXT", "ROUTINE_UNIT",
  "ROWING", "ROWS_SYMBOL", "ROW_CHAR_DENOTER", "ROW_SYMBOL", "SECONDARY", "SELECTION", "SELECTOR", "SEMI_SYMBOL",
  "SERIAL_CLAUSE", "SERIES_MODE", "SHORTETY", "SHORT_SYMBOL", "SIGN_MOULD", "SKIP", "SKIP_SYMBOL", "SLICE", "SOME_CLAUSE",
  "SPECIFIED_UNIT", "SPECIFIED_UNIT_LIST", "SPECIFIED_UNIT_UNIT", "SPECIFIER", "STANDARD", "STATIC_REPLICATOR",
  "STOWED_MODE", "STRING_PATTERN", "STRING_SYMBOL", "STRUCTURED_FIELD", "STRUCTURED_FIELD_LIST", "STRUCTURE_PACK",
  "STRUCT_SYMBOL", "STYLE_II_COMMENT_SYMBOL", "STYLE_I_COMMENT_SYMBOL", "STYLE_I_PRAGMAT_SYMBOL", "SUB_SYMBOL",
  "SUB_UNIT", "TERTIARY", "THEN_BAR_SYMBOL", "THEN_PART", "THEN_SYMBOL", "TO_PART", "TO_SYMBOL", "TRIMMER", "TRUE_SYMBOL",
  "UNION_DECLARER_LIST", "UNION_PACK", "UNION_SYMBOL", "UNIT", "UNITED_CASE_CLAUSE", "UNITED_CHOICE", "UNITED_IN_PART",
  "UNITED_OUSE_PART", "UNITING", "UNIT_LIST", "UNIT_SERIES", "VARIABLE_DECLARATION", "VOIDING", "VOID_SYMBOL",
  "WHILE_PART", "WHILE_SYMBOL", "WIDENING", "WILDCARD"
};

char *
non_terminal_string (char *buf, int att)
{
  if (att > 0 && att < WILDCARD)
    {
      if (attribute_names[att] != NULL)
	{
	  char *q = buf;
	  strcpy (q, attribute_names[att]);
	  while (q[0] != '\0')
	    {
	      if (q[0] == '_')
		{
		  q[0] = '-';
		}
	      else
		{
		  q[0] = TO_LOWER (q[0]);
		}
	      q++;
	    }
	  return (buf);
	}
      else
	{
	  return (NULL);
	}
    }
  else
    {
      return (NULL);
    }
}
