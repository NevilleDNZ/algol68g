/*!
\file diagnostics.h
\brief
**/

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

#if ! defined A68G_DIAGNOSTICS_H
#define A68G_DIAGNOSTICS_H

#define MOID_ERROR_WIDTH 80

#define ERROR_ACCESSING_NIL "attempt to access N"
#define ERROR_ALIGNMENT "alignment error"
#define ERROR_ARGUMENT_NUMBER "incorrect number of arguments for M"
#define ERROR_CANNOT_END_WITH_DECLARATION "clause cannot end with a declaration"
#define ERROR_CANNOT_OPEN_NAME "cannot open Z"
#define ERROR_CANNOT_WIDEN "cannot widen M to M"
#define ERROR_CANNOT_WRITE_LISTING "cannot write listing file"
#define ERROR_CHANNEL_DOES_NOT_ALLOW "channel does not allow Y"
#define ERROR_CLAUSE_WITHOUT_VALUE "clause does not yield a value"
#define ERROR_CLOSING_DEVICE "error while closing device"
#define ERROR_CLOSING_FILE "error while closing file"
#define ERROR_COMMA_MUST_SEPARATE "A and A must be separated by a comma-symbol"
#define ERROR_COMPONENT_NUMBER "M must have at least two components"
#define ERROR_COMPONENT_RELATED "M has firmly related components"
#define ERROR_CYCLIC_MODE "M specifies no mode (references to self)"
#define ERROR_DEVICE_ALREADY_SET "device parameters already set"
#define ERROR_DEVICE_CANNOT_ALLOCATE "cannot allocate device parameters"
#define ERROR_DEVICE_CANNOT_OPEN "cannot open device"
#define ERROR_DEVICE_NOT_OPEN "device is not open"
#define ERROR_DEVICE_NOT_SET "device parameters not set"
#define ERROR_DIFFERENT_BOUNDS "rows have different bounds"
#define ERROR_DIVISION_BY_ZERO "attempt at M division by zero"
#define ERROR_DYADIC_PRIORITY "dyadic S has no priority declaration"
#define ERROR_EMPTY_ARGUMENT "empty argument"
#define ERROR_EMPTY_VALUE "attempt to use uninitialised M value"
#define ERROR_EMPTY_VALUE_FROM (ERROR_EMPTY_VALUE)
#define ERROR_EXPECTED "Y expected"
#define ERROR_EXPECTED_NEAR "B expected in A, near Z L"
#define ERROR_EXPONENT_DIGIT "invalid exponent digit"
#define ERROR_EXPONENT_INVALID "invalid M exponent"
#define ERROR_FALSE_ASSERTION "false assertion"
#define ERROR_FEATURE_UNSUPPORTED "unsupported feature S"
#define ERROR_FILE_ALREADY_OPEN "file is already open"
#define ERROR_FILE_CANNOT_OPEN_FOR "cannot open Z for Y"
#define ERROR_FILE_CLOSE "error while closing file"
#define ERROR_FILE_ENDED "end of file reached"
#define ERROR_FILE_INCLUDE_CTRL "control characters in include file"
#define ERROR_FILE_LOCK "error while locking file"
#define ERROR_FILE_NOT_OPEN "file is not open"
#define ERROR_FILE_NO_TEMP "cannot create unique temporary file name"
#define ERROR_FILE_READ "error while reading file"
#define ERROR_FILE_RESET "error while resetting file"
#define ERROR_FILE_SCRATCH "error while scratching file"
#define ERROR_FILE_TRANSPUT "error transputting M value"
#define ERROR_FILE_WRONG_MOOD "file is in Y mood"
#define ERROR_FLEX_ROW "flex is a property of rows"
#define ERROR_FORMAT_CANNOT_TRANSPUT "cannot transput M value with A"
#define ERROR_FORMAT_EXHAUSTED "patterns exhausted in format"
#define ERROR_FORMAT_INTS_REQUIRED "1 .. 3 M arguments required"
#define ERROR_FORMAT_PICTURES "number of pictures does not match number of arguments"
#define ERROR_FORMAT_PICTURE_NUMBER "incorrect number of pictures for A"
#define ERROR_INCORRECT_FILENAME "incorrect filename"
#define ERROR_INDEXER_NUMBER "incorrect number of indexers for M"
#define ERROR_INDEX_OUT_OF_BOUNDS "index out of bounds"
#define ERROR_INTERNAL_CONSISTENCY "internal consistency check failure"
#define ERROR_INVALID_ARGUMENT "invalid M argument for S"
#define ERROR_INVALID_DIMENSION "invalid dimension D"
#define ERROR_INVALID_OPERAND "M construct is an invalid operand"
#define ERROR_INVALID_OPERATOR_TAG "invalid operator tag"
#define ERROR_INVALID_PARAMETER "invalid parameter"
#define ERROR_INVALID_PRIORITY "invalid priority declaration"
#define ERROR_INVALID_RADIX "invalid radix D"
#define ERROR_INVALID_SEQUENCE "assumed A, which is not consistent with U"
#define ERROR_INVALID_SIZE "object of invalid size"
#define ERROR_IN_DENOTER "error in M denoter"
#define ERROR_KEYWORD "check for missing or unmatched keyword in clause starting at S"
#define ERROR_LABELED_UNIT_MUST_FOLLOW "S must be followed by a labeled unit"
#define ERROR_LABEL_BEFORE_DECLARATION "declaration cannot follow a labeled unit"
#define ERROR_LABEL_IN_PAR_CLAUSE "target label is in a parallel clause"
#define ERROR_LONG_STRING "string exceeds end of line"
#define ERROR_MATH "M math error"
#define ERROR_MATH_EXCEPTION "math exception E"
#define ERROR_MATH_INFO "M math error (Y)"
#define ERROR_MP_OUT_OF_BOUNDS "multiprecision value out of bounds"
#define ERROR_MULTIPLE_FIELD "multiple declaration of field S"
#define ERROR_MULTIPLE_TAG "multiple declaration of tag S"
#define ERROR_NIL_DESCRIPTOR "descriptor is NIL"
#define ERROR_NOT_WELL_FORMED "S does not specify a well formed mode"
#define ERROR_NO_COMPONENT "M is neither component nor subset of M"
#define ERROR_NO_DYADIC "O S O has not been declared in this range"
#define ERROR_NO_FIELD "M has no field Z"
#define ERROR_NO_INPUT_FILE "no input file specified"
#define ERROR_NO_MONADIC "S O has not been declared in this range"
#define ERROR_NO_NAME "M A does not yield a name"
#define ERROR_NO_NAME_REQUIRED "context does not require a name"
#define ERROR_NO_PRIORITY "S has no priority declaration"
#define ERROR_NO_PROC "M A does not yield a procedure taking arguments"
#define ERROR_NO_ROW_OR_PROC "M A does not yield a row or procedure"
#define ERROR_NO_STRUCT "M A does not yield a structured value"
#define ERROR_NO_UNION "M is not a united mode"
#define ERROR_NO_UNIQUE_MODE "construct has no unique mode"
#define ERROR_OPERAND_NUMBER "incorrect number of operands for S"
#define ERROR_OPERATOR_INVALID "monadic S cannot start with a character from Z"
#define ERROR_OPERATOR_RELATED "M Z is firmly related to M Z"
#define ERROR_OPTION "error in option \"%s\""
#define ERROR_OPTION_INFO "error in option \"%s\" (%s)"
#define ERROR_OUT_OF_BOUNDS "M value out of bounds"
#define ERROR_OUT_OF_CORE "insufficient memory"
#define ERROR_PAGE_SIZE "error in page size"
#define ERROR_PARALLEL_CANNOT_CREATE "cannot create thread"
#define ERROR_PARALLEL_CANNOT_JOIN "cannot join thread"
#define ERROR_PARALLEL_OUTSIDE "invalid outside a parallel clause"
#define ERROR_PARALLEL_OVERFLOW "too many parallel units"
#define ERROR_PARENTHESIS "incorrect parenthesis nesting; check for Y"
#define ERROR_PARENTHESIS_2 "incorrect parenthesis nesting; encountered Z L but expected X; check for Y"
#define ERROR_PRAGMENT "error in pragment"
#define ERROR_QUOTED_BOLD_TAG "error in quoted bold tag"
#define ERROR_REFINEMENT_APPLIED "refinement is applied more than once"
#define ERROR_REFINEMENT_DEFINED "refinement already defined"
#define ERROR_REFINEMENT_EMPTY "empty refinement at end of program"
#define ERROR_REFINEMENT_INVALID "invalid refinement"
#define ERROR_REFINEMENT_NOT_APPLIED "refinement is not applied"
#define ERROR_RELATED_MODES "M is related to M"
#define ERROR_REQUIRE_THREADS "parallel clause requires posix threads"
#define ERROR_SCOPE_DYNAMIC_0 "value is exported out of its scope"
#define ERROR_SCOPE_DYNAMIC_1 "M value is exported out of its scope"
#define ERROR_SCOPE_DYNAMIC_2 "M value from %s is exported out of its scope"
#define ERROR_SOURCE_FILE_OPEN "error while opening source file"
#define ERROR_SPECIFICATION (errno == 0 ? NULL : strerror (errno))
#define ERROR_STACK_OVERFLOW "stack overflow"
#define ERROR_SUBSET_RELATED "M has firmly related subset M"
#define ERROR_SYNTAX "detected in A"
#define ERROR_SYNTAX_EXPECTED "A expected"
#define ERROR_SYNTAX_MISSING_SEPARATOR "probably a missing separator"
#define ERROR_SYNTAX_MISSING_SYMBOL "probably a missing symbol nearby"
#define ERROR_SYNTAX_MIXED_DECLARATION "probably mixed identity and variable declaration"
#define ERROR_SYNTAX_WRONG_SEPARATOR "probably a wrong separator"
#define ERROR_TIME_LIMIT_EXCEEDED "time limit exceeded"
#define ERROR_TOO_MANY_ARGUMENTS "too many arguments"
#define ERROR_TOO_MANY_OPEN_FILES "too many open files"
#define ERROR_TORRIX "linear algebra error (U, U)"
#define ERROR_TRANSIENT_NAME "storing a transient name"
#define ERROR_UNBALANCED_KEYWORD "missing or unbalanced keyword in A, near Z L"
#define ERROR_UNDECLARED_TAG "tag S has not been declared properly in this range"
#define ERROR_UNDECLARED_TAG_1 "tag S has not been declared properly in this range"
#define ERROR_UNDECLARED_TAG_2 "tag Z has not been declared properly in this range"
#define ERROR_UNDETERMINDED_FILE_MOOD "file has undetermined mood"
#define ERROR_UNIMPLEMENTED_PRECISION "M precision is not implemented"
#define ERROR_UNSPECIFIED "unspecified error"
#define ERROR_UNTERMINATED_COMMENT "unterminated comment"
#define ERROR_UNTERMINATED_PRAGMAT "unterminated pragmat"
#define ERROR_UNTERMINATED_PRAGMENT "unterminated pragment"
#define ERROR_UNTERMINATED_STRING "unterminated string"
#define ERROR_UNWORTHY_CHARACTER "unworthy character %s"
#define INFO_APPROPRIATE_DECLARER "appropriate declarer"
#define INFO_MISSING_KEYWORDS "missing or unmatched keyword"
#define WARNING_EXECUTED_AS "A will be executed as A"
#define WARNING_SCOPE_STATIC_1 "value from A could be exported out of its scope"
#define WARNING_SCOPE_STATIC_2 "M value from A could be exported out of its scope"
#define WARNING_SKIPPED_SUPERFLUOUS "skipped superfluous A"
#define WARNING_TAG_NOT_PORTABLE "Tag S is not portable"
#define WARNING_TAG_UNUSED "tag S is not used"
#define WARNING_UNINITIALISED "identifier S might be used before being initialised"
#define WARNING_UNINTENDED "possibly unintended M A in M A"
#define WARNING_VOIDED "value of M @ will be voided"
#define WARNING_WIDENING_NOT_PORTABLE "Implicit widening of M to M is not portable"
#endif
