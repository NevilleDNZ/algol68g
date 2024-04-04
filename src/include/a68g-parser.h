//! @file a68g-parser.h
//! @author J. Marcel van der Veer

//! @section Copyright
//!
//! This file is part of Algol68G - an Algol 68 compiler-interpreter.
//! Copyright 2001-2024 J. Marcel van der Veer [algol68g@xs4all.nl].

//! @section License
//!
//! This program is free software; you can redistribute it and/or modify it 
//! under the terms of the GNU General Public License as published by the 
//! Free Software Foundation; either version 3 of the License, or 
//! (at your option) any later version.
//!
//! This program is distributed in the hope that it will be useful, but 
//! WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//! or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
//! more details. You should have received a copy of the GNU General Public 
//! License along with this program. If not, see [http://www.gnu.org/licenses/].

//! @section Synopsis
//!
//! Definitions related to the parser.

#if !defined (__A68G_PARSER_H__)
#define __A68G_PARSER_H__

#define STOP_CHAR 127

BOOL_T dont_mark_here (NODE_T *);
BOOL_T is_coercion (NODE_T *);
BOOL_T is_firm (MOID_T *, MOID_T *);
BOOL_T is_firm (MOID_T *, MOID_T *);
BOOL_T is_formal_bounds (NODE_T *);
BOOL_T is_loop_keyword (NODE_T *);
BOOL_T are_modes_equivalent (MOID_T *, MOID_T *);
BOOL_T is_new_lexical_level (NODE_T *);
BOOL_T is_one_of (NODE_T * p, ...);
BOOL_T is_ref_refety_flex (MOID_T *);
BOOL_T is_semicolon_less (NODE_T *);
BOOL_T is_subset (MOID_T *, MOID_T *, int);
BOOL_T is_unitable (MOID_T *, MOID_T *, int);
BOOL_T is_unit_terminator (NODE_T *);
BOOL_T lexical_analyser (void);
BOOL_T match_string (char *, char *, char);
BOOL_T prove_moid_equivalence (MOID_T *, MOID_T *);
BOOL_T whether (NODE_T * p, ...);
char *phrase_to_text (NODE_T *, NODE_T **);
GINFO_T *new_genie_info (void);
int count_formal_bounds (NODE_T *);
int count_operands (NODE_T *);
int count_pack_members (PACK_T *);
int first_tag_global (TABLE_T *, char *);
int get_good_attribute (NODE_T *);
int is_identifier_or_label_global (TABLE_T *, char *);
KEYWORD_T *find_keyword_from_attribute (KEYWORD_T *, int);
KEYWORD_T *find_keyword (KEYWORD_T *, char *);
LINE_T *new_source_line (void);
MOID_T *add_mode (MOID_T **, int, int, NODE_T *, MOID_T *, PACK_T *);
MOID_T *depref_completely (MOID_T *);
MOID_T *get_mode_from_declarer (NODE_T *);
MOID_T *new_moid (void);
MOID_T *register_extra_mode (MOID_T **, MOID_T *);
MOID_T *unites_to (MOID_T *, MOID_T *);
NODE_INFO_T *new_node_info (void);
NODE_T *get_next_format_pattern (NODE_T *, A68_REF, BOOL_T);
NODE_T *new_node (void);
NODE_T *reduce_dyadic (NODE_T *, int u);
NODE_T *some_node (char *);
NODE_T *top_down_loop (NODE_T *);
NODE_T *top_down_skip_unit (NODE_T *);
PACK_T *absorb_union_pack (PACK_T *);
PACK_T *new_pack (void);
TABLE_T *find_level (NODE_T *, int);
TABLE_T *new_symbol_table (TABLE_T *);
TAG_T *add_tag (TABLE_T *, int, NODE_T *, MOID_T *, int);
TAG_T *find_tag_global (TABLE_T *, int, char *);
TAG_T *find_tag_local (TABLE_T *, int, const char *);
TAG_T *new_tag (void);
TOKEN_T *add_token (TOKEN_T **, char *);
void a68_parser (void);
void add_mode_to_pack_end (PACK_T **, MOID_T *, char *, NODE_T *);
void add_mode_to_pack (PACK_T **, MOID_T *, char *, NODE_T *);
void append_source_line (char *, LINE_T **, int *, char *);
void assign_offsets (NODE_T *);
void assign_offsets_packs (MOID_T *);
void assign_offsets_table (TABLE_T *);
void bind_format_tags_to_tree (NODE_T *);
void bind_routine_tags_to_tree (NODE_T *);
void bottom_up_error_check (NODE_T *);
void bottom_up_parser (NODE_T *);
void check_parenthesis (NODE_T *);
void coercion_inserter (NODE_T *);
void coercion_inserter (NODE_T *);
void collect_taxes (NODE_T *);
void contract_union (MOID_T *);
void count_pictures (NODE_T *, int *);
void elaborate_bold_tags (NODE_T *);
void extract_declarations (NODE_T *);
void extract_declarations (NODE_T *);
void extract_identities (NODE_T *);
void extract_indicants (NODE_T *);
void extract_labels (NODE_T *, int);
void extract_operators (NODE_T *);
void extract_priorities (NODE_T *);
void extract_proc_identities (NODE_T *);
void extract_proc_variables (NODE_T *);
void extract_variables (NODE_T *);
void fill_symbol_table_outer (NODE_T *, TABLE_T *);
void finalise_symbol_table_setup (NODE_T *, int);
void free_genie_heap (NODE_T *);
void get_max_simplout_size (NODE_T *);
void get_refinements (void);
void ignore_superfluous_semicolons (NODE_T *);
void init_before_tokeniser (void);
void init_parser (void);
void jumps_from_procs (NODE_T * p);
void make_moid_list (MODULE_T *);
void make_special_mode (MOID_T **, int);
void make_standard_environ (void);
void make_sub (NODE_T *, NODE_T *, int);
void mark_auxilliary (NODE_T *);
void mark_jump_in_par (NODE_T *, BOOL_T);
void mark_moids (NODE_T *);
void mode_checker (NODE_T *);
void mode_checker (NODE_T *);
void portcheck (NODE_T *);
void preliminary_symbol_table_setup (NODE_T *);
void prune_echoes (OPTION_LIST_T *);
void put_refinements (void);
void rearrange_goto_less_jumps (NODE_T *);
void recover_from_error (NODE_T *, int, BOOL_T);
void reduce_arguments (NODE_T *);
void reduce_basic_declarations (NODE_T *);
void reduce_bounds (NODE_T *);
void reduce_branch (NODE_T *, int);
void reduce_collateral_clauses (NODE_T *);
void reduce_declaration_lists (NODE_T *);
void reduce_declarers (NODE_T *, int);
void reduce_enclosed_clauses (NODE_T *, int);
void reduce_enquiry_clauses (NODE_T *);
void reduce_erroneous_units (NODE_T *);
void reduce_format_texts (NODE_T *);
void reduce_formulae (NODE_T *);
void reduce_generic_arguments (NODE_T *);
void reduce (NODE_T *, void (*)(NODE_T *), BOOL_T *, ...);
void reduce_primaries (NODE_T *, int);
void reduce_primary_parts (NODE_T *, int);
void reduce_right_to_left_constructs (NODE_T * q);
void reduce_secondaries (NODE_T *);
void reduce_serial_clauses (NODE_T *);
void reduce_tertiaries (NODE_T *);
void reduce_units (NODE_T *);
void register_nodes (NODE_T *);
void renumber_moids (MOID_T *, int);
void renumber_nodes (NODE_T *, int *);
void reset_symbol_table_nest_count (NODE_T *);
void scope_checker (NODE_T *);
void scope_checker (NODE_T *);
void set_moid_sizes (MOID_T *);
void set_nest (NODE_T *, NODE_T *);
void set_proc_level (NODE_T *, int);
void set_up_tables (void);
void substitute_brackets (NODE_T *);
void tie_label_to_serial (NODE_T *);
void tie_label_to_unit (NODE_T *);
void top_down_parser (NODE_T *);
void verbosity (void);
void victal_checker (NODE_T *);
void warn_for_unused_tags (NODE_T *);
void widen_denotation (NODE_T *);

#endif
