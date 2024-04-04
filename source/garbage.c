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
#include "mp.h"

#ifdef HAVE_UNIX_CLOCK
#include <sys/time.h>
#endif

/*-------1---------2---------3---------4---------5---------6---------7---------+
A mark-and-sweep garbage collector defragments the heap. When called, it walks
the stack frames and marks the heap space that is still active. This marking 
process is called "colouring" here since we "pour paint" into the heap.
The active blocks are then joined, the non-active blocks are forgotten.

While colouring the heap, "cookies" are placed in objects as to find circular
references.

Algol68G introduces several anonymous tags in the symbol tables that save 
temporary REF or ROW results, so that they do not get prematurely swept.

The genie is not smart enough to handle every heap clog, e.g. when copying
STOWED objects. This seems not very elegant, but garbage collectors in general 
cannot solve all core management problems. To avoid many of the "unforeseen" 
heap clogs, we try to keep heap occupation low by sweeping the heap 
occasionally, before it fills up completely. If this automatic mechanism does 
not help, one can always invoke the garbage collector by calling "sweep heap" 
from Algol 68 source text.

Mark-and-sweep is simple but since it walks recursive structures, it could
exhaust the C-stack (segment violation).                                      */

void colour_object (BYTE_T *, MOID_T *);
void sweep_heap (NODE_T *, ADDR_T);

int garbage_collects, garbage_bytes_freed, handles_freed;
int free_handle_count, max_handle_count;
static int block_heap_compacter;
A68_HANDLE *free_handles, *unfree_handles;
double garbage_seconds;

/* Total freed is kept in a LONG INT */

mp_digit garbage_total_freed[LONG_MP_DIGITS + 2];
static mp_digit garbage_freed[LONG_MP_DIGITS + 2];

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_sweep_heap: PROC VOID sweep heap                                        */

void
genie_sweep_heap (NODE_T * p)
{
  sweep_heap (p, frame_pointer);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_garbage_collections: INT collections                                    */

void
genie_garbage_collections (NODE_T * p)
{
  PUSH_INT (p, garbage_collects);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_garbage_freed: LONG INT garbage                                         */

void
genie_garbage_freed (NODE_T * p)
{
  PUSH (p, garbage_total_freed, moid_size (MODE (LONG_INT)));
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_garbage_seconds: REAL collect seconds 
Not that this timing is a rough cut.                                          */

void
genie_garbage_seconds (NODE_T * p)
{
  PUSH_REAL (p, garbage_seconds);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
void up_garbage_sema
We can tell the compacter not to sweep the heap at all, to secure temporary 
data while, for instance, handling arrays.                                    */

void
up_garbage_sema (void)
{
  block_heap_compacter++;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
void down_garbage_sema */

void
down_garbage_sema (void)
{
  abend (block_heap_compacter == 0, "invalid state in down_garbage_sema", NULL);
  block_heap_compacter--;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
We can tell the compacter not to sweep certain objects, to secure temporary
things, for instance string denoters.                                         */

void
protect_sweep_handle (A68_REF * z)
{
  (z->handle)->status |= NO_SWEEP_MASK;
}

void
unprotect_sweep_handle (A68_REF * z)
{
  (z->handle)->status &= ~NO_SWEEP_MASK;
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
heap_available: size available for an object in the heap.                     */

int
heap_available ()
{
  return (heap_size - heap_pointer);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
genie_init_heap: initialise heap management.                                  */

void
genie_init_heap (NODE_T * p, MODULE_T * module)
{
  ADDR_T handle_pointer;
  A68_HANDLE *x;
  int counter = 0;
  (void) p;
  if (heap_segment == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, module->top_node, OUT_OF_CORE);
      exit_genie (module->top_node, 1);
    }
  if (handle_segment == NULL)
    {
      diagnostic (A_RUNTIME_ERROR, module->top_node, OUT_OF_CORE);
      exit_genie (module->top_node, 1);
    }
  block_heap_compacter = 0;
  garbage_seconds = 0;
  SET_MP_ZERO (garbage_total_freed, LONG_MP_DIGITS);
  garbage_collects = 0;
  if (fixed_heap_pointer >= heap_size)
    {
      low_core_alert ();
    }
  heap_pointer = fixed_heap_pointer;
  free_handle_count = 0;
  max_handle_count = 0;
  free_handles = NULL;
  unfree_handles = NULL;
  x = NULL;
  for (handle_pointer = 0; handle_pointer < (handle_pool_size - SIZE_OF (A68_HANDLE)); handle_pointer += SIZE_OF (A68_HANDLE))
    {
      A68_HANDLE *z = (A68_HANDLE *) (handle_segment + handle_pointer);
      z->status = NULL_MASK;
      z->offset = 0;
      z->size = 0;
      z->number = counter++;
      NEXT (z) = NULL;
      PREVIOUS (z) = x;
      if (x == NULL)
	{
	  free_handles = z;
	}
      else
	{
	  NEXT (x) = z;
	}
      x = z;
      free_handle_count++;
      max_handle_count++;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
moid_needs_colouring: whether "m" is eligible for colouring.                  */

static BOOL_T
moid_needs_colouring (MOID_T * m)
{
  if (WHETHER (m, REF_SYMBOL))
    {
      return (A_TRUE);
    }
  else if (WHETHER (m, FLEX_SYMBOL) || WHETHER (m, ROW_SYMBOL))
    {
      return (A_TRUE);
    }
  else if (WHETHER (m, STRUCT_SYMBOL) || WHETHER (m, UNION_SYMBOL))
    {
      PACK_T *p = PACK (m);
      BOOL_T k = A_FALSE;
      for (; p != NULL && !k; p = NEXT (p))
	{
	  k |= moid_needs_colouring (MOID (p));
	}
      return (k);
    }
  else
    {
      return (A_FALSE);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
colour_row_elements: colour all elements of a row.                            */

static void
colour_row_elements (A68_REF * z, MOID_T * m)
{
  A68_ARRAY *arr;
  A68_TUPLE *tup;
  GET_DESCRIPTOR (arr, tup, z);
/* Empty rows are trivial since we don't recognise ghost elements */
  if (get_row_size (tup, arr->dimensions) > 0)
    {
/* The multi-dimensional sweeper */
      BYTE_T *elem = ADDRESS (&arr->array);
      BOOL_T done = A_FALSE;
      initialise_internal_index (tup, arr->dimensions);
      while (!done)
	{
	  ADDR_T index = calculate_internal_index (tup, arr->dimensions);
	  ADDR_T addr = ROW_ELEMENT (arr, index);
	  colour_object (&elem[addr], SUB (m));
	  done = increment_internal_index (tup, arr->dimensions);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
colour_object: colour an (active) object.                                     */

void
colour_object (BYTE_T * item, MOID_T * m)
{
  if (item == NULL || m == NULL)
    {
      return;
    }
/* Deeply recursive objects might exhaust the stack. */
  LOW_STACK_ALERT;
/* REF AMODE */
  if (WHETHER (m, REF_SYMBOL))
    {
      A68_REF *z = (A68_REF *) item;
      if ((z != NULL) && (z->status & INITIALISED_MASK) && (z->handle != NULL))
	{
	  if (z->handle->status & COOKIE_MASK)
	    {
	      return;		/* Circular list */
	    }
	  z->handle->status |= COOKIE_MASK;
	  if (z->segment == heap_segment)
	    {
	      z->handle->status |= COLOUR_MASK;
	    }
	  if (!IS_NIL (*z))
	    {
	      colour_object (ADDRESS (z), SUB (m));
	    }
	  z->handle->status &= ~COOKIE_MASK;
	}
/* [] AMODE */
    }
  else if (WHETHER (m, FLEX_SYMBOL) || WHETHER (m, ROW_SYMBOL) || m == MODE (STRING))
    {
      A68_REF *z = (A68_REF *) item;
      /* Claim the descriptor and the row itself */
      if ((z != NULL) && (z->status & INITIALISED_MASK) && (z->handle != NULL))
	{
	  A68_ARRAY *arr;
	  A68_TUPLE *tup;
	  if (z->handle->status & COOKIE_MASK)
	    {
	      return;		/* Circular list */
	    }
	  z->handle->status |= COOKIE_MASK;
	  z->handle->status |= COLOUR_MASK;	/* Array is ALWAYS in the heap */
	  GET_DESCRIPTOR (arr, tup, z);
	  if ((arr->array).handle != NULL)
	    {
	      /* Assume its initialisation */
	      MOID_T *n = DEFLEX (m);
	      (arr->array).handle->status |= COLOUR_MASK;
	      if (moid_needs_colouring (SUB (n)))
		{
		  colour_row_elements (z, n);
		}
	    }
	  z->handle->status &= ~COOKIE_MASK;
	}
/* STRUCT () */
    }
  else if (WHETHER (m, STRUCT_SYMBOL))
    {
      PACK_T *p = PACK (m);
      for (; p != NULL; p = NEXT (p))
	{
	  colour_object (&item[p->offset], MOID (p));
	}
/* UNION () */
    }
  else if (WHETHER (m, UNION_SYMBOL))
    {
      A68_POINTER *z = (A68_POINTER *) item;
      if (z->status & INITIALISED_MASK)
	{
	  MOID_T *united_moid = (MOID_T *) z->value;
	  colour_object (&item[SIZE_OF (A68_POINTER)], united_moid);
	}
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
colour_heap: colour active objects in the heap.                               */

static void
colour_heap (ADDR_T fp)
{
  while (fp != 0)
    {
      NODE_T *p = FRAME_TREE (fp);
      SYMBOL_TABLE_T *q = SYMBOL_TABLE (p);
      if (q != NULL)
	{
	  TAG_T *i;
	  for (i = q->identifiers; i != NULL; i = NEXT (i))
	    {
	      colour_object (FRAME_LOCAL (fp, i->offset), MOID (i));
	    }
	  for (i = q->anonymous; i != NULL; i = NEXT (i))
	    {
	      if (PRIO (i) == PROTECT_FROM_SWEEP)
		{
		  colour_object (FRAME_LOCAL (fp, i->offset), MOID (i));
		}
	    }
	}
      fp = FRAME_DYNAMIC_LINK (fp);
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
defragment_heap: join all active blocks in the heap.                          */

static void
defragment_heap ()
{
  A68_HANDLE *z;
/* Free handles. */
  z = unfree_handles;
  while (z != NULL)
    {
      if (!(z->status & COLOUR_MASK) && !(z->status & NO_SWEEP_MASK))
	{
	  A68_HANDLE *y = NEXT (z);
	  if (PREVIOUS (z) == NULL)
	    {
	      unfree_handles = NEXT (z);
	    }
	  else
	    {
	      NEXT (PREVIOUS (z)) = NEXT (z);
	    }
	  if (NEXT (z) != NULL)
	    {
	      PREVIOUS (NEXT (z)) = PREVIOUS (z);
	    }
	  NEXT (z) = free_handles;
	  PREVIOUS (z) = NULL;
	  if (NEXT (z) != NULL)
	    {
	      PREVIOUS (NEXT (z)) = z;
	    }
	  free_handles = z;
	  z->status &= ~ALLOCATED_MASK;
	  garbage_bytes_freed += z->size;
	  handles_freed++;
	  free_handle_count++;
	  z = y;
	}
      else
	{
	  z = NEXT (z);
	}
    }
/* There can be no uncoloured allocated handle. */
  for (z = unfree_handles; z != NULL; z = NEXT (z))
    {
      abend ((z->status & COLOUR_MASK) == 0 && (z->status & NO_SWEEP_MASK) == 0, "bad GC consistency", NULL);
    }
/* Order in the heap must be preserved. */
  for (z = unfree_handles; z != NULL; z = NEXT (z))
    {
      abend (NEXT (z) != NULL && z->offset < NEXT (z)->offset, "bad GC order", NULL);
    }
/* Defragment the heap. */
  heap_pointer = fixed_heap_pointer;
  for (z = unfree_handles; z != NULL && NEXT (z) != NULL; z = NEXT (z))
    {
      ;
    }
  for (; z != NULL; z = PREVIOUS (z))
    {
      MOVE (&heap_segment[heap_pointer], HEAP_ADDRESS (z->offset), (unsigned) z->size);
      z->status &= ~COLOUR_MASK;
      z->offset = heap_pointer;
      heap_pointer += z->size;
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
sweep_heap: clean up garbage and defragment the heap.                         */

void
sweep_heap (NODE_T * p, ADDR_T fp)
{
/* Must start with fp = current frame_pointer. */
  A68_HANDLE *z;
  double t0 = seconds (), t1;
#ifdef HAVE_UNIX_CLOCK
  double tu;
  struct timeval tim;
  gettimeofday (&tim, NULL);
  tu = tim.tv_sec + tim.tv_usec * 1e-6;
#endif
  if (block_heap_compacter > 0)
    {
      return;
    }
/* Unfree handles are subject to inspection. */
  for (z = unfree_handles; z != NULL; z = NEXT (z))
    {
      z->status &= ~(COLOUR_MASK | COOKIE_MASK);
    }
/* Pour paint into the heap to reveal active objects. */
  colour_heap (fp);
/* Start freeing and compacting. */
  garbage_bytes_freed = 0;
  handles_freed = 0;
  defragment_heap ();
/* Some stats and logging. */
  garbage_collects++;
  int_to_mp (p, garbage_freed, (int) garbage_bytes_freed, LONG_MP_DIGITS);
  add_mp (p, garbage_total_freed, garbage_total_freed, garbage_freed, LONG_MP_DIGITS);
  t1 = seconds ();
  if (t1 > t0)
    {
      garbage_seconds += (t1 - t0);
#ifdef HAVE_UNIX_CLOCK
    }
  else
    {
/* This only works well when A68G is the dominant process. */
      gettimeofday (&tim, NULL);
      tu = tim.tv_sec + tim.tv_usec * 1e-6 - tu;
      if (tu < 1 / CLK_TCK)
	{
	  garbage_seconds += tu;
	}
#endif
    }
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
give_handle: yield a handle that will point to a block in the heap.           */

static A68_HANDLE *
give_handle (NODE_T * p, MOID_T * a68m)
{
  if (free_handles != NULL)
    {
      A68_HANDLE *x = free_handles;
      free_handles = NEXT (x);
      if (free_handles != NULL)
	{
	  PREVIOUS (free_handles) = NULL;
	}
      x->status = ALLOCATED_MASK;
      x->offset = 0;
      x->size = 0;
      MOID (x) = a68m;
      NEXT (x) = unfree_handles;
      PREVIOUS (x) = NULL;
      if (NEXT (x) != NULL)
	{
	  PREVIOUS (NEXT (x)) = x;
	}
      unfree_handles = x;
      free_handle_count--;
      return (x);
    }
  else
    {
      sweep_heap (p, frame_pointer);
      if (free_handles != NULL)
	{
	  return (give_handle (p, a68m));
	}
      else
	{
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_CORE);
	  exit_genie (p, A_RUNTIME_ERROR);
	}
    }
  return (NULL);
}

/*-------1---------2---------3---------4---------5---------6---------7---------+
heap_generator: give a block of heap for an object of indicated mode.         */

A68_REF
heap_generator (NODE_T * p, MOID_T * mode, int size)
{
/* Align to int boundary */
  abend (size < 0, INVALID_SIZE, NULL);
  size = ALIGN (size);
/* Now give it. */
  if (heap_available () >= size)
    {
      A68_HANDLE *x = give_handle (p, mode);
      A68_REF z;
      x->size = size;
      x->offset = heap_pointer;
/* Set all values to uninitialised. */
      memset (&heap_segment[heap_pointer], 0, (unsigned) size);
      heap_pointer += size;
      z.status = INITIALISED_MASK;
      z.segment = heap_segment;
      z.offset = 0;
      z.handle = x;
      return (z);
    }
  else
    {
/* No heap space. First sweep the heap. */
      sweep_heap (p, frame_pointer);
      if (heap_available () > size)
	{
	  return (heap_generator (p, mode, size));
	}
      else
	{
/* Still no heap space. We have to abend. */
	  diagnostic (A_RUNTIME_ERROR, p, OUT_OF_CORE);
	  exit_genie (p, A_RUNTIME_ERROR);
	  return (nil_ref);
	}
    }
}
