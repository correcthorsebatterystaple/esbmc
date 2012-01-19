/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#ifndef CPROVER_CPP_TYPECHECK_RESOLVE_H
#define CPROVER_CPP_TYPECHECK_RESOLVE_H

#include "cpp_typecheck_fargs.h"
#include "cpp_name.h"

class cpp_typecheck_resolvet
{
public:
  cpp_typecheck_resolvet(
    class cpp_typecheckt &_cpp_typecheck);

  typedef enum { VAR, TYPE, BOTH } wantt;

  exprt resolve(
    const cpp_namet &cpp_name,
    const wantt want,
    const cpp_typecheck_fargst &fargs,
    bool fail_with_exception=true);

  // Returns the scope as a side-effect as 'current_scope'.
  // Should really return explicitly.
  void resolve_scope(
    const cpp_namet &cpp_name,
    std::string &base_name,
    irept &template_args);

  cpp_scopet &resolve_namespace(const cpp_namet &cpp_name);

protected:
  cpp_typecheckt &cpp_typecheck;
  exprt this_expr;
  locationt location;
  cpp_scopet *original_scope;

  typedef std::set<exprt> resolve_identifierst;

  void convert_identifiers(
    const cpp_scopest::id_sett &id_set,
    const locationt &location,
    const irept &template_args,
    const cpp_typecheck_fargst &fargs,
    resolve_identifierst &identifiers);

  void convert_template_argument(
    const cpp_idt &id,
    const locationt &location,
    const irept &template_args,
    exprt &e);

  void convert_identifier(
    const cpp_idt &id,
    const locationt &location,
    const irept &template_args,
    const cpp_typecheck_fargst &fargs,
    exprt &e);

  void disambiguate(
    resolve_identifierst &identifiers,
    const cpp_typecheck_fargst &fargs);

  void filter(
    resolve_identifierst &identifiers,
    const wantt want);

  void make_constructors(
    resolve_identifierst &identifiers);

  void apply_template_args(
    resolve_identifierst &identifiers,
    const irept &template_args,
    const cpp_typecheck_fargst &fargs);

  void apply_template_args(
    exprt &expr,
    const irept &template_args,
    const cpp_typecheck_fargst& fargs);

  void remove_templates(
    resolve_identifierst &identifiers);

  void remove_duplicates(
    resolve_identifierst &identifiers);

  bool disambiguate(
    const exprt &expr,
    unsigned &args_distance,
    const cpp_typecheck_fargst &fargs);

  exprt do_builtin(
    const irep_idt &base_name,
    irept &template_args);

  void resolve_with_arguments(
    cpp_scopest::id_sett& id_set,
    const std::string& base_name,
    const cpp_typecheck_fargst &fargs);

  void filter_for_named_scopes(cpp_scopest::id_sett &id_set);
  void filter_for_namespaces(cpp_scopest::id_sett &id_set);

  #ifdef CPP_SYSTEMC_EXTENSION
  exprt do_builtin_sc_uint_extension(
    const cpp_namet &cpp_name,
    exprt& template_args);

  exprt do_builtin_sc_int_extension(
    const cpp_namet &cpp_name,
    exprt& template_args);

  exprt do_builtin_sc_logic_extension(
    const cpp_namet &cpp_name,
    const exprt& template_args);

  exprt do_builtin_sc_lv_extension(
    const cpp_namet &cpp_name,
    exprt& template_args);
  #endif
};

#endif
