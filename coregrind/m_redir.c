
/*--------------------------------------------------------------------*/
/*--- Function replacement and wrapping.                 m_redir.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Valgrind, a dynamic binary instrumentation
   framework.

   Copyright (C) 2000-2012 Julian Seward 
      jseward@acm.org
   Copyright (C) 2003-2012 Jeremy Fitzhardinge
      jeremy@goop.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

#include "pub_core_basics.h"
#include "pub_core_debuglog.h"
#include "pub_core_debuginfo.h"
#include "pub_core_libcbase.h"
#include "pub_core_libcassert.h"
#include "pub_core_libcprint.h"
#include "pub_core_vki.h"
#include "pub_core_libcfile.h"
#include "pub_core_seqmatch.h"
#include "pub_core_mallocfree.h"
#include "pub_core_options.h"
#include "pub_core_oset.h"
#include "pub_core_redir.h"
#include "pub_core_trampoline.h"
#include "pub_core_transtab.h"
#include "pub_core_tooliface.h"    // VG_(needs).malloc_replacement
#include "pub_core_machine.h"      // VG_(fnptr_to_fnentry)
#include "pub_core_aspacemgr.h"    // VG_(am_find_nsegment)
#include "pub_core_xarray.h"
#include "pub_core_clientstate.h"  // VG_(client___libc_freeres_wrapper)
#include "pub_core_demangle.h"     // VG_(maybe_Z_demangle)
#include "pub_core_libcproc.h"     // VG_(libdir)

#include "config.h" /* GLIBC_2_* */


/* This module is a critical part of the redirection/intercept system.
   It keeps track of the current intercept state, cleans up the
   translation caches when that state changes, and finally, answers
   queries about the whether an address is currently redirected or
   not.  It doesn't do any of the control-flow trickery needed to put
   the redirections into practice.  That is the job of m_translate,
   which calls here to find out which translations need to be
   redirected.

   The interface is simple.  VG_(redir_initialise) initialises and
   loads some hardwired redirects which never disappear; this is
   platform-specific.

   The module is notified of redirection state changes by m_debuginfo.
   That calls VG_(redir_notify_new_DebugInfo) when a new DebugInfo
   (shared object symbol table, basically) appears.  Appearance of new
   symbols can cause new (active) redirections to appear for two
   reasons: the symbols in the new table may match existing
   redirection specifications (see comments below), and because the
   symbols in the new table may themselves supply new redirect
   specifications which match existing symbols (or ones in the new
   table).

   Redirect specifications are really symbols with "funny" prefixes
   (_vgrNNNNZU_ and _vgrNNNNZZ_).  These names tell m_redir that the
   associated code should replace the standard entry point for some
   set of functions.  The set of functions is specified by a (soname
   pattern, function name pattern) pair which is encoded in the symbol
   name following the prefix.  The names use a Z-encoding scheme so
   that they may contain punctuation characters and wildcards (*).
   The encoding scheme is described in pub_tool_redir.h and is decoded
   by VG_(maybe_Z_demangle).  The NNNN are behavioural equivalence
   class tags, and are used to by code in this module to resolve
   situations where one address appears to be redirected to more than
   one replacement/wrapper.  This is also described in
   pub_tool_redir.h.

   When a shared object is unloaded, this module learns of it via a
   call to VG_(redir_notify_delete_DebugInfo).  It then removes from
   its tables all active redirections in any way associated with that
   object, and tidies up the translation caches accordingly.

   That takes care of tracking the redirection state.  When a
   translation is actually to be made, m_translate calls to
   VG_(redir_do_lookup) in this module to find out if the
   translation's address should be redirected.
*/

/*------------------------------------------------------------*/
/*--- Semantics                                            ---*/
/*------------------------------------------------------------*/

/* The redirector holds two pieces of state:

     Specs  - a set of   (soname pattern, fnname pattern) -> redir addr
     Active - a set of   orig addr -> (bool, redir addr)

   Active is the currently active set of bindings that the translator
   consults.  Specs is the current set of specifications as harvested
   from reading symbol tables of the currently loaded objects.

   Active is a pure function of Specs and the current symbol table
   state (maintained by m_debuginfo).  Call the latter SyminfoState.

   Therefore whenever either Specs or SyminfoState changes, Active
   must be recomputed.  [Inefficient if done naively, but this is a
   spec].

   Active is computed as follows:

      Active = empty
      for spec in Specs {
         sopatt = spec.soname pattern
         fnpatt = spec.fnname pattern
         redir  = spec.redir addr
         for so matching sopatt in SyminfoState {
            for fn matching fnpatt in fnnames_of(so) {
               &fn -> redir is added to Active
            }
         }
      }

   [as an implementation detail, when a binding (orig -> redir) is
   deleted from Active as a result of recomputing it, then all
   translations intersecting redir must be deleted.  However, this is
   not part of the spec].

   [Active also depends on where the aspacemgr has decided to put all
   the pieces of code -- that affects the "orig addr" and "redir addr"
   values.]

   ---------------------

   That completes the spec, apart from one difficult issue: duplicates.

   Clearly we must impose the requirement that domain(Active) contains
   no duplicates.  The difficulty is how to constrain Specs enough to
   avoid getting into that situation.  It's easy to write specs which
   could cause conflicting bindings in Active, eg:

      (libpthread.so, pthread_mutex_lock) ->    a1
      (libpthread.so, pthread_*)          ->    a2

   for a1 != a2.  Or even hairier:

      (libpthread.so, pthread_mutex_*) ->    a1
      (libpthread.so, pthread_*_lock)  ->    a2

   I can't think of any sane way of detecting when an addition to
   Specs would generate conflicts.  However, considering we don't
   actually want to have a system that allows this, I propose this:
   all changes to Specs are acceptable.  But, when recomputing Active
   following the change, if the same orig is bound to more than one
   redir, then the first binding for orig is retained, and all the
   rest ignored.

   ===========================================================
   ===========================================================
   Incremental implementation:

   When a new DebugInfo appears:
   - it may be the source of new specs
   - it may be the source of new matches for existing specs
   Therefore:

   - (new Specs x existing DebugInfos): scan all symbols in the new
     DebugInfo to find new specs.  Each of these needs to be compared
     against all symbols in all the existing DebugInfos to generate
     new actives.
     
   - (existing Specs x new DebugInfo): scan all symbols in the
     DebugInfo, trying to match them to any existing specs, also
     generating new actives.

   - (new Specs x new DebugInfo): scan all symbols in the new
     DebugInfo, trying to match them against the new specs, to
     generate new actives.

   - Finally, add new new specs to the current set of specs.

   When adding a new active (s,d) to the Actives:
     lookup s in Actives
        if already bound to d, ignore
        if already bound to something other than d, complain loudly and ignore
        else add (s,d) to Actives
             and discard (s,1) and (d,1)  (maybe overly conservative)

   When a DebugInfo disappears:
   - delete all specs acquired from the seginfo
   - delete all actives derived from the just-deleted specs
   - if each active (s,d) deleted, discard (s,1) and (d,1)
*/


/*------------------------------------------------------------*/
/*--- REDIRECTION SPECIFICATIONS                           ---*/
/*------------------------------------------------------------*/

/* A specification of a redirection we want to do.  Note that because
   both the "from" soname and function name may contain wildcards, the
   spec can match an arbitrary number of times. 

   16 Nov 2007: Comments re .mandatory field: The initial motivation
   for this is making Memcheck work sanely on glibc-2.6.X ppc32-linux.
   We really need to intercept 'strlen' in ld.so right from startup.
   If ld.so does not have a visible 'strlen' symbol, Memcheck
   generates an impossible number of errors resulting from highly
   tuned strlen implementation in ld.so, and is completely unusable
   -- the resulting undefinedness eventually seeps everywhere. */
typedef
   struct _Spec {
      struct _Spec* next;  /* linked list */
      /* FIXED PARTS -- set when created and not changed */
      HChar* from_sopatt;  /* from soname pattern  */
      HChar* from_fnpatt;  /* from fnname pattern  */
      Addr   to_addr;      /* where redirecting to */
      Bool   isWrap;       /* wrap or replacement? */
      Int    becTag; /* 0 through 9999.  Behavioural equivalance class tag.
                        If two wrappers have the same (non-zero) tag, they
                        are promising that they behave identically. */
      Int    becPrio; /* 0 through 9.  Behavioural equivalence class prio.
                         Used to choose between competing wrappers with
                         the same (non-zero) tag. */
      const HChar** mandatory; /* non-NULL ==> abort V and print the
                                  strings if from_sopatt is loaded but
                                  from_fnpatt cannot be found */
      /* VARIABLE PARTS -- used transiently whilst processing redirections */
      Bool   mark; /* set if spec requires further processing */
      Bool   done; /* set if spec was successfully matched */
   }
   Spec;

/* Top-level data structure.  It contains a pointer to a DebugInfo and
   also a list of the specs harvested from that DebugInfo.  Note that
   seginfo is allowed to be NULL, meaning that the specs are
   pre-loaded ones at startup and are not associated with any
   particular seginfo. */
typedef
   struct _TopSpec {
      struct _TopSpec* next; /* linked list */
      DebugInfo* seginfo;    /* symbols etc */
      Spec*      specs;      /* specs pulled out of seginfo */
      Bool       mark; /* transient temporary used during deletion */
   }
   TopSpec;

/* This is the top level list of redirections.  m_debuginfo maintains
   a list of DebugInfos, and the idea here is to maintain a list with
   the same number of elements (in fact, with one more element, so as
   to record abovementioned preloaded specifications.) */
static TopSpec* topSpecs = NULL;


/*------------------------------------------------------------*/
/*--- CURRENTLY ACTIVE REDIRECTIONS                        ---*/
/*------------------------------------------------------------*/

/* Represents a currently active binding.  If either parent_spec or
   parent_sym is NULL, then this binding was hardwired at startup and
   should not be deleted.  Same is true if either parent's seginfo
   field is NULL. */
typedef
   struct {
      Addr     from_addr;   /* old addr -- MUST BE THE FIRST WORD! */
      Addr     to_addr;     /* where redirecting to */
      TopSpec* parent_spec; /* the TopSpec which supplied the Spec */
      TopSpec* parent_sym;  /* the TopSpec which supplied the symbol */
      Int      becTag;      /* behavioural eclass tag for ::to_addr */
      Int      becPrio;     /* and its priority */
      Bool     isWrap;      /* wrap or replacement? */
      Bool     isIFunc;     /* indirect function? */
   }
   Active;

/* The active set is a fast lookup table */
static OSet* activeSet = NULL;

/* Wrapper routine for indirect functions */
static Addr iFuncWrapper;

/*------------------------------------------------------------*/
/*--- FWDses                                               ---*/
/*------------------------------------------------------------*/

static void maybe_add_active ( Active /*by value; callee copies*/ );

static void*  dinfo_zalloc(const HChar* ec, SizeT);
static void   dinfo_free(void*);
static HChar* dinfo_strdup(const HChar* ec, const HChar*);
static Bool   is_plausible_guest_addr(Addr);

static void   show_redir_state ( const HChar* who );
static void   show_active ( const HChar* left, Active* act );

static void   handle_maybe_load_notifier( const HChar* soname, 
                                                HChar* symbol, Addr addr );

static void   handle_require_text_symbols ( DebugInfo* );

/*------------------------------------------------------------*/
/*--- NOTIFICATIONS                                        ---*/
/*------------------------------------------------------------*/

static 
void generate_and_add_actives ( 
        /* spec list and the owning TopSpec */
        Spec*    specs, 
        TopSpec* parent_spec,
	/* debuginfo and the owning TopSpec */
        DebugInfo* di,
        TopSpec* parent_sym 
     );


/* Copy all the names from a given symbol into an AR_DINFO allocated,
   NULL terminated array, for easy iteration.  Caller must pass also
   the address of a 2-entry array which can be used in the common case
   to avoid dynamic allocation. */
static HChar** alloc_symname_array ( HChar* pri_name, HChar** sec_names,
                                     HChar** twoslots )
{
   /* Special-case the common case: only one name.  We expect the
      caller to supply a stack-allocated 2-entry array for this. */
   if (sec_names == NULL) {
      twoslots[0] = pri_name;
      twoslots[1] = NULL;
      return twoslots;
   }
   /* Else must use dynamic allocation.  Figure out size .. */
   Word    n_req = 1;
   HChar** pp    = sec_names;
   while (*pp) { n_req++; pp++; }
   /* .. allocate and copy in. */
   HChar** arr = dinfo_zalloc( "redir.asa.1", (n_req+1) * sizeof(HChar*) );
   Word    i   = 0;
   arr[i++] = pri_name;
   pp = sec_names;
   while (*pp) { arr[i++] = *pp; pp++; }
   tl_assert(i == n_req);
   tl_assert(arr[n_req] == NULL);
   return arr;
}


/* Free the array allocated by alloc_symname_array, if any. */
static void free_symname_array ( HChar** names, HChar** twoslots )
{
   if (names != twoslots)
      dinfo_free(names);
}

static HChar const* advance_to_equal ( HChar const* c ) {
   while (*c && *c != '=') {
      ++c;
   }
   return c;
}
static HChar const* advance_to_comma ( HChar const* c ) {
   while (*c && *c != ',') {
      ++c;
   }
   return c;
}

/* Notify m_redir of the arrival of a new DebugInfo.  This is fairly
   complex, but the net effect is to (1) add a new entry to the
   topspecs list, and (2) figure out what new binding are now active,
   and, as a result, add them to the actives mapping. */

#define N_DEMANGLED 256

void VG_(redir_notify_new_DebugInfo)( DebugInfo* newdi )
{
   Bool         ok, isWrap;
   Int          i, nsyms, becTag, becPrio;
   Spec*        specList;
   Spec*        spec;
   TopSpec*     ts;
   TopSpec*     newts;
   HChar*       sym_name_pri;
   HChar**      sym_names_sec;
   Addr         sym_addr, sym_toc;
   HChar        demangled_sopatt[N_DEMANGLED];
   HChar        demangled_fnpatt[N_DEMANGLED];
   Bool         check_ppcTOCs = False;
   Bool         isText;
   const HChar* newdi_soname;

#  if defined(VG_PLAT_USES_PPCTOC)
   check_ppcTOCs = True;
#  endif

   vg_assert(newdi);
   newdi_soname = VG_(DebugInfo_get_soname)(newdi);
   vg_assert(newdi_soname != NULL);

#ifdef ENABLE_INNER
   {
      /* When an outer Valgrind is executing an inner Valgrind, the
         inner "sees" in its address space the mmap-ed vgpreload files
         of the outer.  The inner must avoid interpreting the
         redirections given in the outer vgpreload mmap-ed files.
         Otherwise, some tool combinations badly fail.

         Example: outer memcheck tool executing an inner none tool.

         If inner none interprets the outer malloc redirection, the
         inner will redirect malloc to a memcheck function it does not
         have (as the redirection target is from the outer).  With
         such a failed redirection, a call to malloc inside the inner
         will then result in a "no-operation" (and so no memory will
         be allocated).

         When running as an inner, no redirection will be done
         for a vgpreload file if this file is not located in the
         inner VALGRIND_LIB directory.

         Recognising a vgpreload file based on a filename pattern
         is a kludge. An alternate solution would be to change
         the _vgr prefix according to outer/inner/client.
      */
      const HChar* newdi_filename = VG_(DebugInfo_get_filename)(newdi);
      const HChar* newdi_basename = VG_(basename) (newdi_filename);
      if (VG_(strncmp) (newdi_basename, "vgpreload_", 10) == 0) {
         /* This looks like a vgpreload file => check if this file
            is from the inner VALGRIND_LIB.
            We do this check using VG_(stat) + dev/inode comparison
            as vg-in-place defines a VALGRIND_LIB with symlinks
            pointing to files inside the valgrind build directories. */
         struct vg_stat newdi_stat;
         SysRes newdi_res;
         HChar in_vglib_filename[VKI_PATH_MAX];
         struct vg_stat in_vglib_stat;
         SysRes in_vglib_res;

         newdi_res = VG_(stat)(newdi_filename, &newdi_stat);
         
         VG_(strncpy) (in_vglib_filename, VG_(libdir), VKI_PATH_MAX);
         VG_(strncat) (in_vglib_filename, "/", VKI_PATH_MAX);
         VG_(strncat) (in_vglib_filename, newdi_basename, VKI_PATH_MAX);
         in_vglib_res = VG_(stat)(in_vglib_filename, &in_vglib_stat);

         /* If we find newdi_basename in inner VALGRIND_LIB
            but newdi_filename is not the same file, then we do
            not execute the redirection. */
         if (!sr_isError(in_vglib_res)
             && !sr_isError(newdi_res)
             && (newdi_stat.dev != in_vglib_stat.dev 
                 || newdi_stat.ino != in_vglib_stat.ino)) {
            /* <inner VALGRIND_LIB>/newdi_basename is an existing file
               and is different of newdi_filename.
               So, we do not execute newdi_filename redirection. */
            if ( VG_(clo_verbosity) > 1 ) {
               VG_(message)( Vg_DebugMsg,
                             "Skipping vgpreload redir in %s"
                             " (not from VALGRIND_LIB_INNER)\n",
                             newdi_filename);
            }
            return;
         } else {
            if ( VG_(clo_verbosity) > 1 ) {
               VG_(message)( Vg_DebugMsg,
                             "Executing vgpreload redir in %s"
                             " (from VALGRIND_LIB_INNER)\n",
                             newdi_filename);
            }
         }
      }
   }
#endif


   /* stay sane: we don't already have this. */
   for (ts = topSpecs; ts; ts = ts->next)
      vg_assert(ts->seginfo != newdi);

   /* scan this DebugInfo's symbol table, pulling out and demangling
      any specs found */

   specList = NULL; /* the spec list we're building up */

   nsyms = VG_(DebugInfo_syms_howmany)( newdi );
   for (i = 0; i < nsyms; i++) {
      VG_(DebugInfo_syms_getidx)( newdi, i, &sym_addr, &sym_toc,
                                  NULL, &sym_name_pri, &sym_names_sec,
                                  &isText, NULL );
      /* Set up to conveniently iterate over all names for this symbol. */
      HChar*  twoslots[2];
      HChar** names_init = alloc_symname_array(sym_name_pri, sym_names_sec,
                                               &twoslots[0]);
      HChar** names;
      for (names = names_init; *names; names++) {
         ok = VG_(maybe_Z_demangle)( *names,
                                     demangled_sopatt, N_DEMANGLED,
                                     demangled_fnpatt, N_DEMANGLED,
                                     &isWrap, &becTag, &becPrio );
         /* ignore data symbols */
         if (!isText)
            continue;
         if (!ok) {
            /* It's not a full-scale redirect, but perhaps it is a load-notify
               fn?  Let the load-notify department see it. */
            handle_maybe_load_notifier( newdi_soname, *names, sym_addr );
            continue; 
         }
         if (check_ppcTOCs && sym_toc == 0) {
            /* This platform uses toc pointers, but none could be found
               for this symbol, so we can't safely redirect/wrap to it.
               Just skip it; we'll make a second pass over the symbols in
               the following loop, and complain at that point. */
            continue;
         }

         if (0 == VG_(strncmp) (demangled_sopatt, 
                                VG_SO_SYN_PREFIX, VG_SO_SYN_PREFIX_LEN)) {
            /* This is a redirection for handling lib so synonyms. If we
               have a matching lib synonym, then replace the sopatt.
               Otherwise, just ignore this redirection spec. */

            if (!VG_(clo_soname_synonyms))
               continue; // No synonyms => skip the redir.

            /* Search for a matching synonym=newname*/
            SizeT const sopatt_syn_len 
               = VG_(strlen)(demangled_sopatt+VG_SO_SYN_PREFIX_LEN);
            HChar const* last = VG_(clo_soname_synonyms);
            
            while (*last) {
               HChar const* first = last;
               last = advance_to_equal(first);
               
               if ((last - first) == sopatt_syn_len
                   && 0 == VG_(strncmp)(demangled_sopatt+VG_SO_SYN_PREFIX_LEN,
                                        first,
                                        sopatt_syn_len)) {
                  // Found the demangle_sopatt synonym => replace it
                  first = last + 1;
                  last = advance_to_comma(first);
                  VG_(strncpy)(demangled_sopatt, first, last - first);
                  demangled_sopatt[last - first] = '\0';
                  break;
               }

               last = advance_to_comma(last);
               if (*last == ',')
                  last++;
            }
            
            // If we have not replaced the sopatt, then skip the redir.
            if (0 == VG_(strncmp) (demangled_sopatt, 
                                   VG_SO_SYN_PREFIX, VG_SO_SYN_PREFIX_LEN))
               continue;
         }

         spec = dinfo_zalloc("redir.rnnD.1", sizeof(Spec));
         vg_assert(spec);
         spec->from_sopatt = dinfo_strdup("redir.rnnD.2", demangled_sopatt);
         spec->from_fnpatt = dinfo_strdup("redir.rnnD.3", demangled_fnpatt);
         vg_assert(spec->from_sopatt);
         vg_assert(spec->from_fnpatt);
         spec->to_addr = sym_addr;
         spec->isWrap = isWrap;
         spec->becTag = becTag;
         spec->becPrio = becPrio;
         /* check we're not adding manifestly stupid destinations */
         vg_assert(is_plausible_guest_addr(sym_addr));
         spec->next = specList;
         spec->mark = False; /* not significant */
         spec->done = False; /* not significant */
         specList = spec;
      }
      free_symname_array(names_init, &twoslots[0]);
   }

   if (check_ppcTOCs) {
      for (i = 0; i < nsyms; i++) {
         VG_(DebugInfo_syms_getidx)( newdi, i, &sym_addr, &sym_toc,
                                     NULL, &sym_name_pri, &sym_names_sec,
                                     &isText, NULL );
         HChar*  twoslots[2];
         HChar** names_init = alloc_symname_array(sym_name_pri, sym_names_sec,
                                                  &twoslots[0]);
         HChar** names;
         for (names = names_init; *names; names++) {
            ok = isText
                 && VG_(maybe_Z_demangle)( 
                       *names, demangled_sopatt, N_DEMANGLED,
                       demangled_fnpatt, N_DEMANGLED, &isWrap, NULL, NULL );
            if (!ok)
               /* not a redirect.  Ignore. */
               continue;
            if (sym_toc != 0)
               /* has a valid toc pointer.  Ignore. */
               continue;

            for (spec = specList; spec; spec = spec->next) 
               if (0 == VG_(strcmp)(spec->from_sopatt, demangled_sopatt)
                   && 0 == VG_(strcmp)(spec->from_fnpatt, demangled_fnpatt))
                  break;
            if (spec)
               /* a redirect to some other copy of that symbol, which
                  does have a TOC value, already exists */
               continue;

            /* Complain */
            VG_(message)(Vg_DebugMsg,
                         "WARNING: no TOC ptr for redir/wrap to %s %s\n",
                         demangled_sopatt, demangled_fnpatt);
         }
         free_symname_array(names_init, &twoslots[0]);
      }
   }

   /* Ok.  Now specList holds the list of specs from the DebugInfo.
      Build a new TopSpec, but don't add it to topSpecs yet. */
   newts = dinfo_zalloc("redir.rnnD.4", sizeof(TopSpec));
   vg_assert(newts);
   newts->next    = NULL; /* not significant */
   newts->seginfo = newdi;
   newts->specs   = specList;
   newts->mark    = False; /* not significant */

   /* We now need to augment the active set with the following partial
      cross product:

      (1) actives formed by matching the new specs in specList against
          all symbols currently listed in topSpecs

      (2) actives formed by matching the new symbols in newdi against
          all specs currently listed in topSpecs

      (3) actives formed by matching the new symbols in newdi against
          the new specs in specList

      This is necessary in order to maintain the invariant that
      Actives contains all bindings generated by matching ALL specs in
      topSpecs against ALL symbols in topSpecs (that is, a cross
      product of ALL known specs against ALL known symbols).
   */
   /* Case (1) */
   for (ts = topSpecs; ts; ts = ts->next) {
      if (ts->seginfo)
         generate_and_add_actives( specList,    newts,
                                   ts->seginfo, ts );
   }

   /* Case (2) */
   for (ts = topSpecs; ts; ts = ts->next) {
      generate_and_add_actives( ts->specs, ts, 
                                newdi,     newts );
   }

   /* Case (3) */
   generate_and_add_actives( specList, newts, 
                             newdi,    newts );

   /* Finally, add the new TopSpec. */
   newts->next = topSpecs;
   topSpecs = newts;

   if (VG_(clo_trace_redir))
      show_redir_state("after VG_(redir_notify_new_DebugInfo)");

   /* Really finally (quite unrelated to all the above) check the
      names in the module against any --require-text-symbol=
      specifications we might have. */
   handle_require_text_symbols(newdi);
}

#undef N_DEMANGLED

/* Add a new target for an indirect function. Adds a new redirection
   for the indirection function with address old_from that redirects
   the ordinary function with address new_from to the target address
   of the original redirection. */

void VG_(redir_add_ifunc_target)( Addr old_from, Addr new_from )
{
    Active *old, new;

    old = VG_(OSetGen_Lookup)(activeSet, &old_from);
    vg_assert(old);
    vg_assert(old->isIFunc);

    new = *old;
    new.from_addr = new_from;
    new.isIFunc = False;
    maybe_add_active (new);

    if (VG_(clo_trace_redir)) {
       VG_(message)( Vg_DebugMsg,
                     "Adding redirect for indirect function "
                     "0x%llx from 0x%llx -> 0x%llx\n",
                     (ULong)old_from, (ULong)new_from, (ULong)new.to_addr );
    }
}

/* Do one element of the basic cross product: add to the active set,
   all matches resulting from comparing all the given specs against
   all the symbols in the given seginfo.  If a conflicting binding
   would thereby arise, don't add it, but do complain. */

static 
void generate_and_add_actives ( 
        /* spec list and the owning TopSpec */
        Spec*    specs, 
        TopSpec* parent_spec,
	/* seginfo and the owning TopSpec */
        DebugInfo* di,
        TopSpec* parent_sym 
     )
{
   Spec*   sp;
   Bool    anyMark, isText, isIFunc;
   Active  act;
   Int     nsyms, i;
   Addr    sym_addr;
   HChar*  sym_name_pri;
   HChar** sym_names_sec;

   /* First figure out which of the specs match the seginfo's soname.
      Also clear the 'done' bits, so that after the main loop below
      tell which of the Specs really did get done. */
   anyMark = False;
   for (sp = specs; sp; sp = sp->next) {
      sp->done = False;
      sp->mark = VG_(string_match)( sp->from_sopatt, 
                                    VG_(DebugInfo_get_soname)(di) );
      anyMark = anyMark || sp->mark;
   }

   /* shortcut: if none of the sonames match, there will be no bindings. */
   if (!anyMark)
      return;

   /* Iterate outermost over the symbols in the seginfo, in the hope
      of trashing the caches less. */
   nsyms = VG_(DebugInfo_syms_howmany)( di );
   for (i = 0; i < nsyms; i++) {
      VG_(DebugInfo_syms_getidx)( di, i, &sym_addr, NULL,
                                  NULL, &sym_name_pri, &sym_names_sec,
                                  &isText, &isIFunc );
      HChar*  twoslots[2];
      HChar** names_init = alloc_symname_array(sym_name_pri, sym_names_sec,
                                               &twoslots[0]);
      HChar** names;
      for (names = names_init; *names; names++) {

         /* ignore data symbols */
         if (!isText)
            continue;

         for (sp = specs; sp; sp = sp->next) {
            if (!sp->mark)
               continue; /* soname doesn't match */
            if (VG_(string_match)( sp->from_fnpatt, *names )) {
               /* got a new binding.  Add to collection. */
               act.from_addr   = sym_addr;
               act.to_addr     = sp->to_addr;
               act.parent_spec = parent_spec;
               act.parent_sym  = parent_sym;
               act.becTag      = sp->becTag;
               act.becPrio     = sp->becPrio;
               act.isWrap      = sp->isWrap;
               act.isIFunc     = isIFunc;
               sp->done = True;
               maybe_add_active( act );
            }
         } /* for (sp = specs; sp; sp = sp->next) */

      } /* iterating over names[] */
      free_symname_array(names_init, &twoslots[0]);
   } /* for (i = 0; i < nsyms; i++)  */

   /* Now, finally, look for Specs which were marked to be done, but
      didn't get matched.  If any such are mandatory we must abort the
      system at this point. */
   for (sp = specs; sp; sp = sp->next) {
      if (!sp->mark)
         continue;
      if (sp->mark && (!sp->done) && sp->mandatory)
         break;
   }
   if (sp) {
      const HChar** strp;
      const HChar* v = "valgrind:  ";
      vg_assert(sp->mark);
      vg_assert(!sp->done);
      vg_assert(sp->mandatory);
      VG_(printf)("\n");
      VG_(printf)(
      "%sFatal error at startup: a function redirection\n", v);
      VG_(printf)(
      "%swhich is mandatory for this platform-tool combination\n", v);
      VG_(printf)(
      "%scannot be set up.  Details of the redirection are:\n", v);
      VG_(printf)(
      "%s\n", v);
      VG_(printf)(
      "%sA must-be-redirected function\n", v);
      VG_(printf)(
      "%swhose name matches the pattern:      %s\n", v, sp->from_fnpatt);
      VG_(printf)(
      "%sin an object with soname matching:   %s\n", v, sp->from_sopatt);
      VG_(printf)(
      "%swas not found whilst processing\n", v);
      VG_(printf)(
      "%ssymbols from the object with soname: %s\n",
      v, VG_(DebugInfo_get_soname)(di));
      VG_(printf)(
      "%s\n", v);

      for (strp = sp->mandatory; *strp; strp++)
         VG_(printf)(
         "%s%s\n", v, *strp);

      VG_(printf)(
      "%s\n", v);
      VG_(printf)(
      "%sCannot continue -- exiting now.  Sorry.\n", v);
      VG_(printf)("\n");
      VG_(exit)(1);
   }
}


/* Add an act (passed by value; is copied here) and deal with
   conflicting bindings. */
static void maybe_add_active ( Active act )
{
   const HChar*  what = NULL;
   Active* old     = NULL;
   Bool    add_act = False;

   /* Complain and ignore manifestly bogus 'from' addresses.

      Kludge: because this can get called befor the trampoline area (a
      bunch of magic 'to' addresses) has its ownership changed from V
      to C, we can't check the 'to' address similarly.  Sigh.

      amd64-linux hack: the vsysinfo pages appear to have no
      permissions
         ffffffffff600000-ffffffffffe00000 ---p 00000000 00:00 0
      so skip the check for them.  */
   if (!is_plausible_guest_addr(act.from_addr)
#      if defined(VGP_amd64_linux)
       && act.from_addr != 0xFFFFFFFFFF600000ULL
       && act.from_addr != 0xFFFFFFFFFF600400ULL
       && act.from_addr != 0xFFFFFFFFFF600800ULL
#      endif
      ) {
      what = "redirection from-address is in non-executable area";
      goto bad;
   }

   old = VG_(OSetGen_Lookup)( activeSet, &act.from_addr );
   if (old) {
      /* Dodgy.  Conflicting binding. */
      vg_assert(old->from_addr == act.from_addr);
      if (old->to_addr != act.to_addr) {
         /* We've got a conflicting binding -- that is, from_addr is
            specified to redirect to two different destinations,
            old->to_addr and act.to_addr.  If we can prove that they
            are behaviourally equivalent then that's no problem.  So
            we can look at the behavioural eclass tags for both
            functions to see if that's so.  If they are equal, and
            nonzero, then that's fine.  But if not, we can't show they
            are equivalent, so we have to complain, and ignore the new
            binding. */
         vg_assert(old->becTag  >= 0 && old->becTag  <= 9999);
         vg_assert(old->becPrio >= 0 && old->becPrio <= 9);
         vg_assert(act.becTag   >= 0 && act.becTag   <= 9999);
         vg_assert(act.becPrio  >= 0 && act.becPrio  <= 9);
         if (old->becTag == 0)
            vg_assert(old->becPrio == 0);
         if (act.becTag == 0)
            vg_assert(act.becPrio == 0);

         if (old->becTag == 0 || act.becTag == 0 || old->becTag != act.becTag) {
            /* We can't show that they are equivalent.  Complain and
               ignore. */
            what = "new redirection conflicts with existing -- ignoring it";
            goto bad;
         }
         /* They have the same eclass tag.  Use the priorities to
            resolve the ambiguity. */
         if (act.becPrio <= old->becPrio) {
            /* The new one doesn't have a higher priority, so just
               ignore it. */
            if (VG_(clo_verbosity) > 2) {
               VG_(message)(Vg_UserMsg, "Ignoring %s redirection:\n",
                            act.becPrio < old->becPrio ? "lower priority" 
                                                       : "duplicate");
               show_active(             "    old: ", old);
               show_active(             "    new: ", &act);
            }
         } else {
            /* The tricky case.  The new one has a higher priority, so
               we need to get the old one out of the OSet and install
               this one in its place. */
            if (VG_(clo_verbosity) > 1) {
               VG_(message)(Vg_UserMsg, 
                           "Preferring higher priority redirection:\n");
               show_active(             "    old: ", old);
               show_active(             "    new: ", &act);
            }
            add_act = True;
            void* oldNd = VG_(OSetGen_Remove)( activeSet, &act.from_addr );
            vg_assert(oldNd == old);
            VG_(OSetGen_FreeNode)( activeSet, old );
            old = NULL;
         }
      } else {
         /* This appears to be a duplicate of an existing binding.
            Safe(ish) -- ignore. */
         /* XXXXXXXXXXX COMPLAIN if new and old parents differ */
      }

   } else {
      /* There's no previous binding for this from_addr, so we must
         add 'act' to the active set. */
      add_act = True;
   }

   /* So, finally, actually add it. */
   if (add_act) {
      Active* a = VG_(OSetGen_AllocNode)(activeSet, sizeof(Active));
      vg_assert(a);
      *a = act;
      VG_(OSetGen_Insert)(activeSet, a);
      /* Now that a new from->to redirection is in force, we need to
         get rid of any translations intersecting 'from' in order that
         they get redirected to 'to'.  So discard them.  Just for
         paranoia (but, I believe, unnecessarily), discard 'to' as
         well. */
      VG_(discard_translations)( (Addr64)act.from_addr, 1,
                                 "redir_new_DebugInfo(from_addr)");
      VG_(discard_translations)( (Addr64)act.to_addr, 1,
                                 "redir_new_DebugInfo(to_addr)");
      if (VG_(clo_verbosity) > 2) {
         VG_(message)(Vg_UserMsg, "Adding active redirection:\n");
         show_active(             "    new: ", &act);
      }
   }
   return;

  bad:
   vg_assert(what);
   vg_assert(!add_act);
   if (VG_(clo_verbosity) > 1) {
      VG_(message)(Vg_UserMsg, "WARNING: %s\n", what);
      if (old) {
         show_active(             "    old: ", old);
      }
      show_active(             "    new: ", &act);
   }
}


/* Notify m_redir of the deletion of a DebugInfo.  This is relatively
   simple -- just get rid of all actives derived from it, and free up
   the associated list elements. */

void VG_(redir_notify_delete_DebugInfo)( DebugInfo* delsi )
{
   TopSpec* ts;
   TopSpec* tsPrev;
   Spec*    sp;
   Spec*    sp_next;
   OSet*    tmpSet;
   Active*  act;
   Bool     delMe;
   Addr     addr;

   vg_assert(delsi);

   /* Search for it, and make tsPrev point to the previous entry, if
      any. */
   tsPrev = NULL;
   ts     = topSpecs;
   while (True) {
     if (ts == NULL) break;
     if (ts->seginfo == delsi) break;
     tsPrev = ts;
     ts = ts->next;
   }

   vg_assert(ts); /* else we don't have the deleted DebugInfo */
   vg_assert(ts->seginfo == delsi);

   /* Traverse the actives, copying the addresses of those we intend
      to delete into tmpSet. */
   tmpSet = VG_(OSetWord_Create)(dinfo_zalloc, "redir.rndD.1", dinfo_free);

   ts->mark = True;

   VG_(OSetGen_ResetIter)( activeSet );
   while ( (act = VG_(OSetGen_Next)(activeSet)) ) {
      delMe = act->parent_spec != NULL
              && act->parent_sym != NULL
              && act->parent_spec->seginfo != NULL
              && act->parent_sym->seginfo != NULL
              && (act->parent_spec->mark || act->parent_sym->mark);

      /* While we're at it, a bit of paranoia: delete any actives
         which don't have both feet in valid client executable areas.
         But don't delete hardwired-at-startup ones; these are denoted
         by having parent_spec or parent_sym being NULL.  */
      if ( (!delMe)
           && act->parent_spec != NULL
           && act->parent_sym  != NULL ) {
         if (!is_plausible_guest_addr(act->from_addr))
            delMe = True;
         if (!is_plausible_guest_addr(act->to_addr))
            delMe = True;
      }

      if (delMe) {
         VG_(OSetWord_Insert)( tmpSet, act->from_addr );
         /* While we have our hands on both the 'from' and 'to'
            of this Active, do paranoid stuff with tt/tc. */
         VG_(discard_translations)( (Addr64)act->from_addr, 1,
                                    "redir_del_DebugInfo(from_addr)");
         VG_(discard_translations)( (Addr64)act->to_addr, 1,
                                    "redir_del_DebugInfo(to_addr)");
      }
   }

   /* Now traverse tmpSet, deleting corresponding elements in activeSet. */
   VG_(OSetWord_ResetIter)( tmpSet );
   while ( VG_(OSetWord_Next)(tmpSet, &addr) ) {
      act = VG_(OSetGen_Remove)( activeSet, &addr );
      vg_assert(act);
      VG_(OSetGen_FreeNode)( activeSet, act );
   }

   VG_(OSetWord_Destroy)( tmpSet );

   /* The Actives set is now cleaned up.  Free up this TopSpec and
      everything hanging off it. */
   for (sp = ts->specs; sp; sp = sp_next) {
      if (sp->from_sopatt) dinfo_free(sp->from_sopatt);
      if (sp->from_fnpatt) dinfo_free(sp->from_fnpatt);
      sp_next = sp->next;
      dinfo_free(sp);
   }

   if (tsPrev == NULL) {
      /* first in list */
      topSpecs = ts->next;
   } else {
      tsPrev->next = ts->next;
   }
   dinfo_free(ts);

   if (VG_(clo_trace_redir))
      show_redir_state("after VG_(redir_notify_delete_DebugInfo)");
}


/*------------------------------------------------------------*/
/*--- QUERIES (really the whole point of this module)      ---*/
/*------------------------------------------------------------*/

/* This is the crucial redirection function.  It answers the question:
   should this code address be redirected somewhere else?  It's used
   just before translating a basic block. */
Addr VG_(redir_do_lookup) ( Addr orig, Bool* isWrap )
{
   Active* r = VG_(OSetGen_Lookup)(activeSet, &orig);
   if (r == NULL)
      return orig;

   vg_assert(r->to_addr != 0);
   if (isWrap)
      *isWrap = r->isWrap || r->isIFunc;
   if (r->isIFunc) {
      vg_assert(iFuncWrapper);
      return iFuncWrapper;
   }
   return r->to_addr;
}


/*------------------------------------------------------------*/
/*--- INITIALISATION                                       ---*/
/*------------------------------------------------------------*/

/* Add a never-delete-me Active. */

__attribute__((unused)) /* only used on amd64 */
static void add_hardwired_active ( Addr from, Addr to )
{
   Active act;
   act.from_addr   = from;
   act.to_addr     = to;
   act.parent_spec = NULL;
   act.parent_sym  = NULL;
   act.becTag      = 0; /* "not equivalent to any other fn" */
   act.becPrio     = 0; /* mandatory when becTag == 0 */
   act.isWrap      = False;
   act.isIFunc     = False;
   maybe_add_active( act );
}


/* Add a never-delete-me Spec.  This is a bit of a kludge.  On the
   assumption that this is called only at startup, only handle the
   case where topSpecs is completely empty, or if it isn't, it has
   just one entry and that is the one with NULL seginfo -- that is the
   entry that holds these initial specs. */

__attribute__((unused)) /* not used on all platforms */
static void add_hardwired_spec (const  HChar* sopatt, const HChar* fnpatt, 
                                Addr   to_addr,
                                const HChar** mandatory )
{
   Spec* spec = dinfo_zalloc("redir.ahs.1", sizeof(Spec));
   vg_assert(spec);

   if (topSpecs == NULL) {
      topSpecs = dinfo_zalloc("redir.ahs.2", sizeof(TopSpec));
      vg_assert(topSpecs);
      /* symtab_zalloc sets all fields to zero */
   }

   vg_assert(topSpecs != NULL);
   vg_assert(topSpecs->next == NULL);
   vg_assert(topSpecs->seginfo == NULL);
   /* FIXED PARTS */
   spec->from_sopatt = (HChar *)sopatt;
   spec->from_fnpatt = (HChar *)fnpatt;
   spec->to_addr     = to_addr;
   spec->isWrap      = False;
   spec->mandatory   = mandatory;
   /* VARIABLE PARTS */
   spec->mark        = False; /* not significant */
   spec->done        = False; /* not significant */

   spec->next = topSpecs->specs;
   topSpecs->specs = spec;
}


__attribute__((unused)) /* not used on all platforms */
static const HChar* complain_about_stripped_glibc_ldso[]
= { "Possible fixes: (1, short term): install glibc's debuginfo",
    "package on this machine.  (2, longer term): ask the packagers",
    "for your Linux distribution to please in future ship a non-",
    "stripped ld.so (or whatever the dynamic linker .so is called)",
    "that exports the above-named function using the standard",
    "calling conventions for this platform.  The package you need",
    "to install for fix (1) is called",
    "",
    "  On Debian, Ubuntu:                 libc6-dbg",
    "  On SuSE, openSuSE, Fedora, RHEL:   glibc-debuginfo",
    NULL
  };


/* Initialise the redir system, and create the initial Spec list and
   for amd64-linux a couple of permanent active mappings.  The initial
   Specs are not converted into Actives yet, on the (checked)
   assumption that no DebugInfos have so far been created, and so when
   they are created, that will happen. */

void VG_(redir_initialise) ( void )
{
   // Assert that there are no DebugInfos so far
   vg_assert( VG_(next_DebugInfo)(NULL) == NULL );

   // Initialise active mapping.
   activeSet = VG_(OSetGen_Create)(offsetof(Active, from_addr),
                                   NULL,     // Use fast comparison
                                   dinfo_zalloc,
                                   "redir.ri.1", 
                                   dinfo_free);

   // The rest of this function just adds initial Specs.   

#  if defined(VGP_x86_linux)
   /* If we're using memcheck, use this intercept right from the
      start, otherwise ld.so (glibc-2.3.5) makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {
      const HChar** mandatory;
#     if defined(GLIBC_2_2) || defined(GLIBC_2_3) || defined(GLIBC_2_4) \
         || defined(GLIBC_2_5) || defined(GLIBC_2_6) || defined(GLIBC_2_7) \
         || defined(GLIBC_2_8) || defined(GLIBC_2_9) \
         || defined(GLIBC_2_10) || defined(GLIBC_2_11)
      mandatory = NULL;
#     else
      /* for glibc-2.12 and later, this is mandatory - can't sanely
         continue without it */
      mandatory = complain_about_stripped_glibc_ldso;
#     endif
      add_hardwired_spec(
         "ld-linux.so.2", "index",
         (Addr)&VG_(x86_linux_REDIR_FOR_index), mandatory);
      add_hardwired_spec(
         "ld-linux.so.2", "strlen",
         (Addr)&VG_(x86_linux_REDIR_FOR_strlen), mandatory);
   }

#  elif defined(VGP_amd64_linux)
   /* Redirect vsyscalls to local versions */
   add_hardwired_active(
      0xFFFFFFFFFF600000ULL,
      (Addr)&VG_(amd64_linux_REDIR_FOR_vgettimeofday)
   );
   add_hardwired_active(
      0xFFFFFFFFFF600400ULL,
      (Addr)&VG_(amd64_linux_REDIR_FOR_vtime)
   );
   add_hardwired_active(
      0xFFFFFFFFFF600800ULL,
      (Addr)&VG_(amd64_linux_REDIR_FOR_vgetcpu)
   );

   /* If we're using memcheck, use these intercepts right from
      the start, otherwise ld.so makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {

      add_hardwired_spec(
         "ld-linux-x86-64.so.2", "strlen",
         (Addr)&VG_(amd64_linux_REDIR_FOR_strlen),
#        if defined(GLIBC_2_2) || defined(GLIBC_2_3) || defined(GLIBC_2_4) \
            || defined(GLIBC_2_5) || defined(GLIBC_2_6) || defined(GLIBC_2_7) \
            || defined(GLIBC_2_8) || defined(GLIBC_2_9)
         NULL
#        else
         /* for glibc-2.10 and later, this is mandatory - can't sanely
            continue without it */
         complain_about_stripped_glibc_ldso
#        endif
      );   
   }

#  elif defined(VGP_ppc32_linux)
   /* If we're using memcheck, use these intercepts right from
      the start, otherwise ld.so makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {

      /* this is mandatory - can't sanely continue without it */
      add_hardwired_spec(
         "ld.so.1", "strlen",
         (Addr)&VG_(ppc32_linux_REDIR_FOR_strlen),
         complain_about_stripped_glibc_ldso
      );   
      add_hardwired_spec(
         "ld.so.1", "strcmp",
         (Addr)&VG_(ppc32_linux_REDIR_FOR_strcmp),
         NULL /* not mandatory - so why bother at all? */
         /* glibc-2.6.1 (openSUSE 10.3, ppc32) seems fine without it */
      );
      add_hardwired_spec(
         "ld.so.1", "index",
         (Addr)&VG_(ppc32_linux_REDIR_FOR_strchr),
         NULL /* not mandatory - so why bother at all? */
         /* glibc-2.6.1 (openSUSE 10.3, ppc32) seems fine without it */
      );
   }

#  elif defined(VGP_ppc64_linux)
   /* If we're using memcheck, use these intercepts right from
      the start, otherwise ld.so makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {

      /* this is mandatory - can't sanely continue without it */
      add_hardwired_spec(
         "ld64.so.1", "strlen",
         (Addr)VG_(fnptr_to_fnentry)( &VG_(ppc64_linux_REDIR_FOR_strlen) ),
         complain_about_stripped_glibc_ldso
      );

      add_hardwired_spec(
         "ld64.so.1", "index",
         (Addr)VG_(fnptr_to_fnentry)( &VG_(ppc64_linux_REDIR_FOR_strchr) ),
         NULL /* not mandatory - so why bother at all? */
         /* glibc-2.5 (FC6, ppc64) seems fine without it */
      );
   }

#  elif defined(VGP_arm_linux)
   /* If we're using memcheck, use these intercepts right from
      the start, otherwise ld.so makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {
      add_hardwired_spec(
         "ld-linux.so.3", "strlen",
         (Addr)&VG_(arm_linux_REDIR_FOR_strlen),
         complain_about_stripped_glibc_ldso
      );
      //add_hardwired_spec(
      //   "ld-linux.so.3", "index",
      //   (Addr)&VG_(arm_linux_REDIR_FOR_index),
      //   NULL 
      //);
      add_hardwired_spec(
         "ld-linux.so.3", "memcpy",
         (Addr)&VG_(arm_linux_REDIR_FOR_memcpy),
         complain_about_stripped_glibc_ldso
      );
   }
   /* nothing so far */

#  elif defined(VGP_x86_darwin)
   /* If we're using memcheck, use these intercepts right from
      the start, otherwise dyld makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {
      add_hardwired_spec("dyld", "strcmp",
                         (Addr)&VG_(x86_darwin_REDIR_FOR_strcmp), NULL);
      add_hardwired_spec("dyld", "strlen",
                         (Addr)&VG_(x86_darwin_REDIR_FOR_strlen), NULL);
      add_hardwired_spec("dyld", "strcat",
                         (Addr)&VG_(x86_darwin_REDIR_FOR_strcat), NULL);
      add_hardwired_spec("dyld", "strcpy",
                         (Addr)&VG_(x86_darwin_REDIR_FOR_strcpy), NULL);
      add_hardwired_spec("dyld", "strlcat",
                         (Addr)&VG_(x86_darwin_REDIR_FOR_strlcat), NULL);
   }

#  elif defined(VGP_amd64_darwin)
   /* If we're using memcheck, use these intercepts right from
      the start, otherwise dyld makes a lot of noise. */
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {
      add_hardwired_spec("dyld", "strcmp",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_strcmp), NULL);
      add_hardwired_spec("dyld", "strlen",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_strlen), NULL);
      add_hardwired_spec("dyld", "strcat",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_strcat), NULL);
      add_hardwired_spec("dyld", "strcpy",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_strcpy), NULL);
      add_hardwired_spec("dyld", "strlcat",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_strlcat), NULL);
      // DDD: #warning fixme rdar://6166275
      add_hardwired_spec("dyld", "arc4random",
                         (Addr)&VG_(amd64_darwin_REDIR_FOR_arc4random), NULL);
   }

#  elif defined(VGP_s390x_linux)
   /* nothing so far */

#  elif defined(VGP_mips32_linux)
   if (0==VG_(strcmp)("Memcheck", VG_(details).name)) {

      /* this is mandatory - can't sanely continue without it */
      add_hardwired_spec(
         "ld.so.3", "strlen",
         (Addr)&VG_(mips32_linux_REDIR_FOR_strlen),
         complain_about_stripped_glibc_ldso
      );
   }

#  else
#    error Unknown platform
#  endif

   if (VG_(clo_trace_redir))
      show_redir_state("after VG_(redir_initialise)");
}


/*------------------------------------------------------------*/
/*--- MISC HELPERS                                         ---*/
/*------------------------------------------------------------*/

static void* dinfo_zalloc(const HChar* ec, SizeT n) {
   void* p;
   vg_assert(n > 0);
   p = VG_(arena_malloc)(VG_AR_DINFO, ec, n);
   tl_assert(p);
   VG_(memset)(p, 0, n);
   return p;
}

static void dinfo_free(void* p) {
   tl_assert(p);
   return VG_(arena_free)(VG_AR_DINFO, p);
}

static HChar* dinfo_strdup(const HChar* ec, const HChar* str)
{
   return VG_(arena_strdup)(VG_AR_DINFO, ec, str);
}

/* Really this should be merged with translations_allowable_from_seg
   in m_translate. */
static Bool is_plausible_guest_addr(Addr a)
{
   NSegment const* seg = VG_(am_find_nsegment)(a);
   return seg != NULL
          && (seg->kind == SkAnonC || seg->kind == SkFileC)
          && (seg->hasX || seg->hasR); /* crude x86-specific hack */
}


/*------------------------------------------------------------*/
/*--- NOTIFY-ON-LOAD FUNCTIONS                             ---*/
/*------------------------------------------------------------*/

static 
void handle_maybe_load_notifier( const HChar* soname, 
                                       HChar* symbol, Addr addr )
{
#  if defined(VGP_x86_linux)
   /* x86-linux only: if we see _dl_sysinfo_int80, note its address.
      See comment on declaration of VG_(client__dl_sysinfo_int80) for
      the reason.  As far as I can tell, the relevant symbol is always
      in object with soname "ld-linux.so.2". */
   if (symbol && symbol[0] == '_' 
              && 0 == VG_(strcmp)(symbol, "_dl_sysinfo_int80")
              && 0 == VG_(strcmp)(soname, "ld-linux.so.2")) {
      if (VG_(client__dl_sysinfo_int80) == 0)
         VG_(client__dl_sysinfo_int80) = addr;
   }
#  endif

   /* Normal load-notifier handling after here.  First, ignore all
      symbols lacking the right prefix. */
   vg_assert(symbol); // assert rather than segfault if it is NULL
   if (0 != VG_(strncmp)(symbol, VG_NOTIFY_ON_LOAD_PREFIX, 
                                 VG_NOTIFY_ON_LOAD_PREFIX_LEN))
      /* Doesn't have the right prefix */
      return;

   if (VG_(strcmp)(symbol, VG_STRINGIFY(VG_NOTIFY_ON_LOAD(freeres))) == 0)
      VG_(client___libc_freeres_wrapper) = addr;
   else
   if (VG_(strcmp)(symbol, VG_STRINGIFY(VG_NOTIFY_ON_LOAD(ifunc_wrapper))) == 0)
      iFuncWrapper = addr;
   else
      vg_assert2(0, "unrecognised load notification function: %s", symbol);
}


/*------------------------------------------------------------*/
/*--- REQUIRE-TEXT-SYMBOL HANDLING                         ---*/
/*------------------------------------------------------------*/

/* In short: check that the currently-being-loaded object has text
   symbols that satisfy any --require-text-symbol= specifications that
   apply to it, and abort the run with an error message if not.
*/
static void handle_require_text_symbols ( DebugInfo* di )
{
   /* First thing to do is figure out which, if any,
      --require-text-symbol specification strings apply to this
      object.  Most likely none do, since it is not expected to
      frequently be used.  Work through the list of specs and
      accumulate in fnpatts[] the fn patterns that pertain to this
      object. */
   HChar* fnpatts[VG_CLO_MAX_REQ_TSYMS];
   Int    fnpatts_used = 0;
   Int    i, j;
   const HChar* di_soname = VG_(DebugInfo_get_soname)(di);
   vg_assert(di_soname); // must be present

   VG_(memset)(&fnpatts, 0, sizeof(fnpatts));

   vg_assert(VG_(clo_n_req_tsyms) >= 0);
   vg_assert(VG_(clo_n_req_tsyms) <= VG_CLO_MAX_REQ_TSYMS);
   for (i = 0; i < VG_(clo_n_req_tsyms); i++) {
      const HChar* clo_spec = VG_(clo_req_tsyms)[i];
      vg_assert(clo_spec && VG_(strlen)(clo_spec) >= 4);
      // clone the spec, so we can stick a zero at the end of the sopatt
      HChar *spec = VG_(strdup)("m_redir.hrts.1", clo_spec);
      HChar sep = spec[0];
      HChar* sopatt = &spec[1];
      HChar* fnpatt = VG_(strchr)(sopatt, sep);
      // the initial check at clo processing in time in m_main
      // should ensure this.
      vg_assert(fnpatt && *fnpatt == sep);
      *fnpatt = 0;
      fnpatt++;
      if (VG_(string_match)(sopatt, di_soname))
         fnpatts[fnpatts_used++]
            = VG_(strdup)("m_redir.hrts.2", fnpatt);
      VG_(free)(spec);
   }

   if (fnpatts_used == 0)
      return;  /* no applicable spec strings */

   /* So finally, fnpatts[0 .. fnpatts_used - 1] contains the set of
      (patterns for) text symbol names that must be found in this
      object, in order to continue.  That is, we must find at least
      one text symbol name that matches each pattern, else we must
      abort the run. */

   if (0) VG_(printf)("for %s\n", di_soname);
   for (i = 0; i < fnpatts_used; i++)
      if (0) VG_(printf)("   fnpatt: %s\n", fnpatts[i]);

   /* For each spec, look through the syms to find one that matches.
      This isn't terribly efficient but it happens rarely, so no big
      deal. */
   for (i = 0; i < fnpatts_used; i++) {
      Bool   found  = False;
      HChar* fnpatt = fnpatts[i];
      Int    nsyms  = VG_(DebugInfo_syms_howmany)(di);
      for (j = 0; j < nsyms; j++) {
         Bool    isText        = False;
         HChar*  sym_name_pri  = NULL;
         HChar** sym_names_sec = NULL;
         VG_(DebugInfo_syms_getidx)( di, j, NULL, NULL,
                                     NULL, &sym_name_pri, &sym_names_sec,
                                     &isText, NULL );
         HChar*  twoslots[2];
         HChar** names_init = alloc_symname_array(sym_name_pri, sym_names_sec,
                                                  &twoslots[0]);
         HChar** names;
         for (names = names_init; *names; names++) {
            /* ignore data symbols */
            if (0) VG_(printf)("QQQ %s\n", *names);
            vg_assert(sym_name_pri);
            if (!isText)
               continue;
            if (VG_(string_match)(fnpatt, *names)) {
               found = True;
               break;
            }
         }
         free_symname_array(names_init, &twoslots[0]);
         if (found)
            break;
      }

      if (!found) {
         const HChar* v = "valgrind:  ";
         VG_(printf)("\n");
         VG_(printf)(
         "%sFatal error at when loading library with soname\n", v);
         VG_(printf)(
         "%s   %s\n", v, di_soname);
         VG_(printf)(
         "%sCannot find any text symbol with a name "
         "that matches the pattern\n", v);
         VG_(printf)("%s   %s\n", v, fnpatt);
         VG_(printf)("%sas required by a --require-text-symbol= "
         "specification.\n", v);
         VG_(printf)("\n");
         VG_(printf)(
         "%sCannot continue -- exiting now.\n", v);
         VG_(printf)("\n");
         VG_(exit)(1);
      }
   }

   /* All required specs were found.  Just free memory and return. */
   for (i = 0; i < fnpatts_used; i++)
      VG_(free)(fnpatts[i]);
}


/*------------------------------------------------------------*/
/*--- SANITY/DEBUG                                         ---*/
/*------------------------------------------------------------*/

static void show_spec ( const HChar* left, Spec* spec )
{
   VG_(message)( Vg_DebugMsg, 
                 "%s%25s %30s %s-> (%04d.%d) 0x%08llx\n",
                 left,
                 spec->from_sopatt, spec->from_fnpatt,
                 spec->isWrap ? "W" : "R",
                 spec->becTag, spec->becPrio,
                 (ULong)spec->to_addr );
}

static void show_active ( const HChar* left, Active* act )
{
   Bool ok;
   HChar name1[64] = "";
   HChar name2[64] = "";
   name1[0] = name2[0] = 0;
   ok = VG_(get_fnname_w_offset)(act->from_addr, name1, 64);
   if (!ok) VG_(strcpy)(name1, "???");
   ok = VG_(get_fnname_w_offset)(act->to_addr, name2, 64);
   if (!ok) VG_(strcpy)(name2, "???");

   VG_(message)(Vg_DebugMsg, "%s0x%08llx (%20s) %s-> (%04d.%d) 0x%08llx %s\n", 
                             left, 
                             (ULong)act->from_addr, name1,
                             act->isWrap ? "W" : "R",
                             act->becTag, act->becPrio,
                             (ULong)act->to_addr, name2 );
}

static void show_redir_state ( const HChar* who )
{
   TopSpec* ts;
   Spec*    sp;
   Active*  act;
   VG_(message)(Vg_DebugMsg, "<<\n");
   VG_(message)(Vg_DebugMsg, "   ------ REDIR STATE %s ------\n", who);
   for (ts = topSpecs; ts; ts = ts->next) {
      if (ts->seginfo)
         VG_(message)(Vg_DebugMsg, 
                      "   TOPSPECS of soname %s filename %s\n",
                      VG_(DebugInfo_get_soname)(ts->seginfo),
                      VG_(DebugInfo_get_filename)(ts->seginfo));
      else
         VG_(message)(Vg_DebugMsg, 
                      "   TOPSPECS of soname (hardwired)\n");
         
      for (sp = ts->specs; sp; sp = sp->next)
         show_spec("     ", sp);
   }
   VG_(message)(Vg_DebugMsg, "   ------ ACTIVE ------\n");
   VG_(OSetGen_ResetIter)( activeSet );
   while ( (act = VG_(OSetGen_Next)(activeSet)) ) {
      show_active("    ", act);
   }

   VG_(message)(Vg_DebugMsg, ">>\n");
}

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
