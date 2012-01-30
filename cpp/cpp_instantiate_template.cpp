/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <arith_tools.h>
#include <simplify_expr_class.h>
#include <simplify_expr.h>

#include <ansi-c/c_types.h>

#include "cpp_type2name.h"
#include "cpp_typecheck.h"

/*******************************************************************\

Function: cpp_typecheckt::template_suffix

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

std::string cpp_typecheckt::template_suffix(
  const cpp_template_args_tct &template_args)
{
  // quick hack
  std::string result="<";
  bool first=true;

  const cpp_template_args_tct::argumentst &arguments=
    template_args.arguments();

  for(cpp_template_args_tct::argumentst::const_iterator
      it=arguments.begin();
      it!=arguments.end();
      it++)
  {
    if(first) first=false; else result+=",";

    const exprt expr=*it;

    assert(expr.id()!="ambiguous");

    if(expr.id()=="type")
    {
      const typet &type=expr.type();
      if(type.id()=="symbol")
        result+=type.get_string("identifier");
      else
        result+=cpp_type2name(type);
    }
    else // expression
    {
      exprt e=expr;
      make_constant(e);
      
      // this must be a constant, which includes true/false
      mp_integer i;

      if(e.is_true())
        i=1;
      else if(e.is_false())
        i=0;
      else if(to_integer(e, i))
      {
        err_location(*it);
        str << "template argument expression expected to be "
               "scalar constant, but got `"
            << to_string(e) << "'";
        throw 0;
      }

      result+=integer2string(i);
    }
  }

  result+='>';

  return result;
}

/*******************************************************************\

Function: cpp_typecheckt::show_instantiation_stack

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_typecheckt::show_instantiation_stack(std::ostream &out)
{
  for(instantiation_stackt::const_iterator
      s_it=instantiation_stack.begin();
      s_it!=instantiation_stack.end();
      s_it++)
  {
    const symbolt &symbol=lookup(s_it->identifier);
    out << "instantiating `" << symbol.pretty_name << "' with <";

    forall_expr(a_it, s_it->full_template_args.arguments())
    {
      if(a_it!=s_it->full_template_args.arguments().begin())
        out << ", ";

      if(a_it->id()=="type")
        out << to_string(a_it->type());
      else
        out << to_string(*a_it);
    }

    out << "> at " << s_it->location << std::endl;
  }
}

/*******************************************************************\

Function: cpp_typecheckt::instantiate_template

  Inputs: location of the instantiation,
          the identifier of the template symbol,
          typechecked template arguments,
          an (optional) specialization

 Outputs:

 Purpose:

\*******************************************************************/

const symbolt &cpp_typecheckt::instantiate_template(
  const locationt &location,
  const symbolt &template_symbol,
  const cpp_template_args_tct &specialization_template_args,
  const cpp_template_args_tct &full_template_args,
  const typet &specialization)
{
  if(instantiation_stack.size()==50)
  {
    err_location(location);
    throw "reached maximum template recursion depth";
  }

  instantiation_levelt i_level(instantiation_stack);
  instantiation_stack.back().location=location;
  instantiation_stack.back().identifier=template_symbol.name;
  instantiation_stack.back().full_template_args=full_template_args;

  #if 0
  std::cout << "L: " << location << std::endl;
  std::cout << "I: " << template_symbol.name << std::endl;
  #endif

  cpp_save_scopet cpp_saved_scope(cpp_scopes);
  cpp_saved_template_mapt saved_map(template_map);

  bool specialization_given=specialization.is_not_nil();

  // we should never get 'unassigned' here
  assert(!specialization_template_args.has_unassigned());
  assert(!full_template_args.has_unassigned());

  // do we have args?
  if(full_template_args.arguments().empty())
  {
    err_location(location);
    str << "`" << template_symbol.base_name
        << "' is a template; thus, expected template arguments";
    throw 0;
  }

  // produce new symbol name
  std::string suffix=template_suffix(full_template_args);

  // we need the template scope to see the parameters
  cpp_scopet *template_scope=
    static_cast<cpp_scopet *>(cpp_scopes.id_map[template_symbol.name]);

  if(template_scope==NULL)
  {
    err_location(location);
    str << "identifier: " << template_symbol.name << std::endl;
    throw "template instantiation error: scope not found";
  }

  assert(template_scope!=NULL);

  // produce new declaration
  cpp_declarationt new_decl=to_cpp_declaration(template_symbol.type);

  // the new one is not a template any longer, but we remember the
  // template type
  template_typet template_type=new_decl.template_type();
  new_decl.remove("is_template");
  new_decl.remove("template_type");
  new_decl.set("#template", template_symbol.name);
  new_decl.set("#template_arguments", specialization_template_args);

  // save old scope
  cpp_save_scopet saved_scope(cpp_scopes);

  // mapping from template parameters to values/types
  template_map.build(template_type, specialization_template_args);

  // enter the template scope
  cpp_scopes.go_to(*template_scope);

  // Is it a template method?
  // It's in the scope of a class, and not a class itself.
  bool is_template_method=
    cpp_scopes.current_scope().get_parent().is_class() &&
    new_decl.type().id()!="struct";

  irep_idt class_name;

  if(is_template_method)
    class_name=cpp_scopes.current_scope().get_parent().identifier;

  // sub-scope for fixing the prefix
  std::string subscope_name=id2string(template_scope->identifier)+suffix;

  // let's see if we have the instance already
  cpp_scopest::id_mapt::iterator scope_it=
    cpp_scopes.id_map.find(subscope_name);

  if(scope_it!=cpp_scopes.id_map.end())
  {
    cpp_scopet &scope=cpp_scopes.get_scope(subscope_name);

    cpp_scopet::id_sett id_set;
    cpp_scopes.get_ids(template_symbol.base_name, id_set, true);

    if(id_set.size()==1)
    {
      // It has already been instantianted!
      const cpp_idt &cpp_id = **id_set.begin();

      assert(cpp_id.id_class == cpp_idt::CLASS ||
             cpp_id.id_class == cpp_idt::SYMBOL);

      const symbolt &symb=lookup(cpp_id.identifier);

      // continue if the type is incomplete only
      if(cpp_id.id_class == cpp_idt::CLASS &&
         symb.type.id() == "struct")
        return symb;
      else if(symb.value.is_not_nil())
        return symb;
    }

    cpp_scopes.go_to(scope);
  }
  else
  {
    // set up a scope as subscope of the template scope
    std::string prefix=template_scope->get_parent().prefix+suffix;
    cpp_scopet &sub_scope=
      cpp_scopes.current_scope().new_scope(subscope_name);
    sub_scope.prefix=prefix;
    cpp_scopes.go_to(sub_scope);
    cpp_scopes.id_map.insert(
      cpp_scopest::id_mapt::value_type(subscope_name, &sub_scope));
  }

  // store the information that the template has
  // been instantiated using these arguments
  {
    // need non-const handle on template symbol
    symbolt &s=context.symbols.find(template_symbol.name)->second;
    irept &instantiated_with=s.value.add("instantiated_with");
    instantiated_with.get_sub().push_back(specialization_template_args);
  }

  #if 0
  std::cout << "MAP:" << std::endl;
  template_map.print(std::cout);
  #endif

  // fix the type
  {
    typet declaration_type=new_decl.type();

    // specialization?
    if(specialization_given)
    {
      if(declaration_type.id()=="struct")
      {
        declaration_type=specialization;
        declaration_type.location()=location;
      }
      else
      {
        irept tmp=specialization;
        new_decl.declarators()[0].swap(tmp);
      }
    }

    template_map.apply(declaration_type);
    new_decl.type().swap(declaration_type);
  }

  if(new_decl.type().id()=="struct")
  {
    convert(new_decl);

    // also instantiate all the template methods
    const exprt &template_methods=
      static_cast<const exprt &>(
        template_symbol.value.find("template_methods"));

    for(unsigned i=0; i<template_methods.operands().size(); i++)
    {
      cpp_saved_scope.restore();

      cpp_declarationt method_decl=
        static_cast<const cpp_declarationt &>(
          static_cast<const irept &>(template_methods.operands()[i]));

      // copy the type of the template method
      template_typet method_type=
        method_decl.template_type();

      // do template arguments
      // this also sets up the template scope of the method
      cpp_scopet &method_scope=
        typecheck_template_parameters(method_type);

      cpp_scopes.go_to(method_scope);

      // mapping from template arguments to values/types
      template_map.build(method_type, specialization_template_args);

      method_decl.remove("template_type");
      method_decl.remove("is_template");

      convert(method_decl);
    }

    const symbolt &new_symb=
      lookup(new_decl.type().get("identifier"));

    return new_symb;
  }

  if(is_template_method)
  {
    contextt::symbolst::iterator it =
      context.symbols.find(class_name);

    assert(it!=context.symbols.end());

    symbolt &symb = it->second;

    assert(new_decl.declarators().size() == 1);

    if(new_decl.member_spec().is_virtual())
    {
      err_location(new_decl);
      str <<  "invalid use of `virtual' in template declaration";
      throw 0;
    }

    if(convert_typedef(new_decl.type()))
    {
      err_location(new_decl);
      str << "template declaration for typedef";
      throw 0;
    }

    if(new_decl.storage_spec().is_extern() ||
       new_decl.storage_spec().is_auto() ||
       new_decl.storage_spec().is_register() ||
       new_decl.storage_spec().is_mutable())
    {
      err_location(new_decl);
      str << "invalid storage class specified for template field";
      throw 0;
    }

    bool is_static=new_decl.storage_spec().is_static();
    irep_idt access = new_decl.get("#access");

    assert(access != "");
    assert(symb.type.id()=="struct");

    typecheck_compound_declarator(
      symb,
      new_decl,
      new_decl.declarators()[0],
      to_struct_type(symb.type).components(),
      access,
      is_static,
      false,
      false);

    return lookup(to_struct_type(symb.type).components().back().get("name"));
  }

  // not a template class, not a template method,
  // it must be a template function!

  assert(new_decl.declarators().size()==1);

  convert(new_decl);

  const symbolt &symb=
    lookup(new_decl.declarators()[0].get("identifier"));

  return symb;
}

/*******************************************************************\

Function: cpp_typecheckt::instantiate_template

  Inputs: location of the instantiation,
          the identifier of the template symbol,
          typechecked template arguments,
          an (optional) specialization

 Outputs:

 Purpose:

\*******************************************************************/

const symbolt &cpp_typecheckt::instantiate_template(
  const locationt &location,
  const irep_idt &identifier,
  const irept &template_args,
  const typet& specialization)
{
  if(instantiation_stack.size()==50)
  {
    err_location(location);
    throw "reached maximum template recursion depth";
  }

  instantiation_levelt i_level(instantiation_stack);
  instantiation_stack.back().location=location;
//  instantiation_stack.back().identifier=identifier.;
//  instantiation_stack.back().full_template_args=template_args;

  #if 0
  std::cout << "L: " << location << std::endl;
  std::cout << "I: " << template_symbol.name << std::endl;
  #endif

  cpp_save_scopet cpp_saved_scope(cpp_scopes);
  cpp_saved_template_mapt saved_map(template_map);

  symbolt &old_symbol=context.symbols.find(identifier)->second;
  bool is_specialization = specialization.is_not_nil();

  // do we have args?
  if(template_args.is_nil())
  {
    err_location(location);
    str << "`" << old_symbol.base_name
        << "' is a template; thus, expected template arguments";
    throw 0;
  }

  // typecheck template arguments
  irept tc_template_args(template_args);
  typecheck_template_args(tc_template_args);

  // produce new symbol name
  std::string suffix=template_suffix(to_cpp_template_args_tc(tc_template_args));

  // we need the template scope to see the parameters
  cpp_scopet *template_scope=
    static_cast<cpp_scopet *>(cpp_scopes.id_map[old_symbol.name]);
  if(template_scope==NULL)
  {
    err_location(location);
    str << "identifier: " << identifier << std::endl;
    throw "instantiation error";
  }

  assert(template_scope!=NULL);

  // produce new declaration
  cpp_declarationt new_decl=to_cpp_declaration(old_symbol.type);
  new_decl.remove("is_template");

  // the new one is not a template any longer, but we remember the
  // template type
  template_typet template_type=new_decl.template_type();
  new_decl.remove("is_template");
  new_decl.remove("template_type");
//  new_decl.set("#template", template_symbol.name);
//  new_decl.set("#template_arguments", specialization_template_args);

  // save old scope
  cpp_save_scopet saved_scope(cpp_scopes);

  // mapping from template parameters to values/types
  //template_map.build(template_type, tc_template_args);
  build_template_map(template_type, tc_template_args);

  // enter the template scope
  cpp_scopes.go_to(*template_scope);

  // Is it a template method?
  // It's in the scope of a class, and not a class itself.
  bool is_template_method=
    cpp_scopes.current_scope().get_parent().is_class() &&
    new_decl.type().id()!="struct";

  irep_idt class_name;

  if(is_template_method)
    class_name=cpp_scopes.current_scope().get_parent().identifier;

  // sub-scope for fixing the prefix
  std::string subscope_name=id2string(template_scope->identifier)+suffix;

  // let's see if we have the instance already
  cpp_scopest::id_mapt::iterator scope_it=
    cpp_scopes.id_map.find(subscope_name);

  if(scope_it!=cpp_scopes.id_map.end())
  {
    cpp_scopet &scope=cpp_scopes.get_scope(subscope_name);

    cpp_scopet::id_sett id_set;
    scope.lookup(old_symbol.base_name, id_set);

    if(id_set.size()==1)
    {
      // It has already been instantianted!
      const cpp_idt &cpp_id = **id_set.begin();

      assert(cpp_id.id_class == cpp_idt::CLASS ||
             cpp_id.id_class == cpp_idt::SYMBOL);

      const symbolt &symb=lookup(cpp_id.identifier);

      // continue if the type is incomplete only
      if(cpp_id.id_class == cpp_idt::CLASS &&
         symb.type.id() == "struct")
        return symb;
      else if(symb.value.is_not_nil())
        return symb;
    }

    cpp_scopes.go_to(scope);
  }
  else
  {
    // set up a scope as subscope of the template scope
    std::string prefix=template_scope->get_parent().prefix+suffix;
    cpp_scopet &sub_scope=
      cpp_scopes.current_scope().new_scope(subscope_name);
    sub_scope.prefix=prefix;
    cpp_scopes.go_to(sub_scope);
    cpp_scopes.id_map.insert(
      cpp_scopest::id_mapt::value_type(subscope_name, &sub_scope));
  }

  // store the information that the template has
  // been instantiated using these arguments
  {
    irept &instantiated_with=old_symbol.value.add("instantiated_with");
    irept tc_template_args_tmp = tc_template_args;
    instantiated_with.move_to_sub(tc_template_args_tmp);
  }

  #if 0
  std::cout << "MAP:" << std::endl;
  template_map.print(std::cout);
  #endif

  // fix the type
  {
    typet declaration_type=new_decl.type();

    // specialization?
    if(is_specialization)
    {
      if(declaration_type.id()=="struct")
      {
        declaration_type=specialization;
        declaration_type.location()=location;
      }
      else
      {
        irept tmp=specialization;
        new_decl.declarators()[0].swap(tmp);
      }
    }

    template_map.apply(declaration_type);
    new_decl.type().swap(declaration_type);
  }

  if(new_decl.type().id()=="struct")
  {
    convert(new_decl);
    const symbolt& new_symb = lookup(new_decl.type().get("identifier"));

    // also instantiate all the template methods
    const exprt &template_methods=
      static_cast<const exprt &>(
        old_symbol.value.find("template_methods"));

    for(unsigned i=0; i<template_methods.operands().size(); i++)
    {
      cpp_saved_scope.restore();

      cpp_declarationt method_decl=
        static_cast<const cpp_declarationt &>(
          static_cast<const irept &>(template_methods.operands()[i]));

      // copy the type of the template method
      template_typet method_type=
        method_decl.template_type();

      // do template arguments
      // this also sets up the template scope of the method
      cpp_scopet &method_scope=
        typecheck_template_parameters(method_type);

      cpp_scopes.go_to(method_scope);

      // mapping from template arguments to values/types
      //template_map.build(method_type, tc_template_args);
      build_template_map(method_type, tc_template_args);

      method_decl.remove("template_type");
      method_decl.remove("is_template");

      convert(method_decl);
    }
    return new_symb;
  }

  if(is_template_method)
  {
    contextt::symbolst::iterator it =
      context.symbols.find(class_name);

    assert(it!=context.symbols.end());

    symbolt &symb = it->second;

    assert(new_decl.declarators().size() == 1);

    if(new_decl.member_spec().is_virtual())
    {
      err_location(new_decl);
      str <<  "invalid use of `virtual' in template declaration";
      throw 0;
    }

    if(convert_typedef(new_decl.type()))
    {
      err_location(new_decl);
      str << "template declaration for typedef";
      throw 0;
    }

    if(new_decl.storage_spec().is_extern() ||
       new_decl.storage_spec().is_auto() ||
       new_decl.storage_spec().is_register() ||
       new_decl.storage_spec().is_mutable())
    {
      err_location(new_decl);
      str << "invalid storage class specified for template field";
      throw 0;
    }

    bool is_static=new_decl.storage_spec().is_static();
    irep_idt access = new_decl.get("#access");
    assert(access != "");

    typecheck_compound_declarator(
      symb,
      new_decl,
      new_decl.declarators()[0],
      to_struct_type(symb.type).components(),
      access,
      is_static,
      false,
      false);

    return lookup(to_struct_type(symb.type).components().back().get("name"));
  }
  convert(new_decl);

  const symbolt &symb=
    lookup(new_decl.declarators()[0].get("identifier"));

  return symb;
}
