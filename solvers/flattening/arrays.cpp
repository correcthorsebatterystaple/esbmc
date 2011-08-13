/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

//#define DEBUG

#include <assert.h>

#include <std_expr.h>
#include <std_types.h>
#include <arith_tools.h>

#include "arrays.h"

/*******************************************************************\

Function: arrayst::arrayst

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

arrayst::arrayst(propt &_prop):equalityt(_prop)
{
}

/*******************************************************************\

Function: arrayst::record_array_index

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::record_array_index(const index_exprt &index)
{
  unsigned number=arrays.number(index.array());
  if(number>=index_map.size()) index_map.resize(number+1);
  index_map[number].insert(index.index());
}

/*******************************************************************\

Function: arrayst::record_array_equality

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

literalt arrayst::record_array_equality(
  const equality_exprt &equality)
{
  const exprt &op0=equality.op0();
  const exprt &op1=equality.op1();

  // check types
  if(op0.type()!=op1.type())
  {
    std::cout << equality.pretty() << std::endl;
    throw "record_array_equality got equality without matching types";
  }

  array_equalities.push_back(array_equalityt());
  
  array_equalities.back().f1=op0;
  array_equalities.back().f2=op1;
  array_equalities.back().l=SUB::equality(op0, op1);

  arrays.make_union(op0, op1);
  collect_arrays(op0);
  collect_arrays(op1);

  return array_equalities.back().l;
}

/*******************************************************************\

Function: arrayst::collect_arrays

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::collect_arrays(const exprt &a)
{
  if(a.id()=="with")
  {
    if(a.operands().size()!=3)
      throw "with expected to have three operands";

    // check types
    if(a.type()!=a.op0().type())
    {
      std::cout << a.pretty() << std::endl;
      throw "collect_arrays got with without matching types";
    }
      
    arrays.make_union(a, a.op0());
    collect_arrays(a.op0());
    
    // make sure this shows as an application
    index_exprt index_expr;
    index_expr.type()=a.type().subtype();
    index_expr.array()=a.op0();
    index_expr.index()=a.op1();
    record_array_index(index_expr);
  }
  else if(a.is_if())
  {
    if(a.operands().size()!=3)
      throw "if expected to have three operands";

    // check types
    if(a.type()!=a.op1().type())
    {
      std::cout << a.pretty() << std::endl;
      throw "collect_arrays got if without matching types";
    }
      
    // check types
    if(a.type()!=a.op2().type())
    {
      std::cout << a.pretty() << std::endl;
      throw "collect_arrays got if without matching types";
    }

    arrays.make_union(a, a.op1());
    arrays.make_union(a, a.op2());
    collect_arrays(a.op1());
    collect_arrays(a.op2());
  }
  else if(a.is_symbol())
  {
  }
  else if(a.is_nondet_symbol())
  {
  }
  else if(a.is_constant() || a.is_array())
  {
  }
  else if(a.is_array_of())
  {
  }
  else
    throw "unexpected array expression: "+a.id_string();
}

/*******************************************************************\

Function: arrayst::add_array_constraints

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints()
{
  // first get index map
  build_index_map();
  
  // add constraints for if, with, array_of
  for(unsigned i=0; i<arrays.size(); i++)
    add_array_constraints(
      index_map[arrays.find_number(i)],
      arrays[i]);

  // add constraints for equalities
  for(array_equalitiest::const_iterator it=
      array_equalities.begin();
      it!=array_equalities.end();
      it++)
    add_array_constraints(
      index_map[arrays.find_number(it->f1)],
      *it);
    
  // add the Ackermann constraints
  add_array_Ackermann_constraints();
}

/*******************************************************************\

Function: arrayst::add_array_Ackermann_constraints

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_Ackermann_constraints()
{
  // this is quadratic!

  // iterate over arrays
  for(unsigned i=0; i<arrays.size(); i++)
  {
    const index_sett &index_set=index_map[arrays.find_number(i)];
    
    // iterate over indices, 2x!
    for(index_sett::const_iterator
        i1=index_set.begin();
        i1!=index_set.end();
        i1++)
      for(index_sett::const_iterator
          i2=index_set.begin();
          i2!=index_set.end();
          i2++)
        if(i1!=i2)
        {
          // skip if both are constants
          if(i1->is_constant() && i2->is_constant())
            continue;
        
          // index equality
          equality_exprt indices_equal(*i1, *i2);

          if(indices_equal.op0().type()!=
             indices_equal.op1().type())
          {
            indices_equal.op1().
              make_typecast(indices_equal.op0().type());
          }
          
          literalt indices_equal_lit=convert(indices_equal);
          
          if(indices_equal_lit!=const_literal(false))
          {
            index_exprt index_expr1;
            index_expr1.type()=arrays[i].type().subtype();
            index_expr1.array()=arrays[i];
            index_expr1.index()=*i1;

            index_exprt index_expr2=index_expr1;
            index_expr2.index()=*i2;
          
            equality_exprt values_equal(index_expr1, index_expr2);

            bvt implication;
            implication.reserve(2);
            implication.push_back(prop.lnot(indices_equal_lit));
            implication.push_back(convert(values_equal));
            prop.lcnf(implication);
          }
        }
  }
}

/*******************************************************************\

Function: arrayst::build_index_map

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::build_index_map()
{
  // merge the indices into the root
  
  if(index_map.size()<arrays.size())
    index_map.resize(arrays.size());

  // iterate over non-roots
  for(unsigned i=0; i<arrays.size(); i++)
  {
    if(arrays.is_root_number(i)) continue;

    unsigned root_number=arrays.find_number(i);
    assert(root_number!=i);

    index_sett &root_index_set=index_map[root_number];
    index_sett &index_set=index_map[i];

    root_index_set.insert(index_set.begin(), index_set.end());
  }
}

/*******************************************************************\

Function: arrayst::add_array_constraints

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints(
  const index_sett &index_set,
  const array_equalityt &array_equality)
{
  // add constraints x=y => x[i]=y[i]

  for(index_sett::const_iterator
      it=index_set.begin();
      it!=index_set.end();
      it++)
  {
    index_exprt index_expr1;
    index_expr1.type()=array_equality.f1.type().subtype();
    index_expr1.array()=array_equality.f1;
    index_expr1.index()=*it;

    index_exprt index_expr2;
    index_expr2.type()=array_equality.f2.type().subtype();
    index_expr2.array()=array_equality.f2;
    index_expr2.index()=*it;
    
    assert(index_expr1.type()==index_expr2.type());

    equality_exprt equality_expr(index_expr1, index_expr2);
    
    // add constraint
    bvt bv;
    bv.push_back(prop.lnot(array_equality.l));
    bv.push_back(convert(equality_expr));
    prop.lcnf(bv);
  }
}  

/*******************************************************************\

Function: arrayst::add_array_constraints

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints(
  const index_sett &index_set,
  const exprt &expr)
{
  if(expr.id()=="with")
    return add_array_constraints_with(index_set, to_with_expr(expr));
  else if(expr.is_if())
    return add_array_constraints_if(index_set, to_if_expr(expr));
  else if(expr.is_array_of())
    return add_array_constraints_array_of(index_set, to_array_of_expr(expr));
  else if(expr.is_symbol() ||
          expr.is_nondet_symbol() ||
          expr.is_constant() ||
          expr.is_array())
  {
  }
  else
    throw "unexpected array expression: "+expr.id_string();
}

/*******************************************************************\

Function: arrayst::add_array_constraints_with

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints_with(
  const index_sett &index_set,
  const with_exprt &expr)
{
  // we got x=(y with [i:=v])
  // add constaint x[i]=v

  const exprt &index=expr.where();
  const exprt &value=expr.new_value();

  {  
    index_exprt index_expr;
    index_expr.type()=expr.type().subtype();
    index_expr.array()=expr;
    index_expr.index()=index;

    assert(index_expr.type()==value.type());

    set_to_true(equality_exprt(index_expr, value));
  }

  // use other array index applications for "else" case
  // add constraint x[I]=y[I] for I!=i

  for(index_sett::const_iterator
      it=index_set.begin();
      it!=index_set.end();
      it++)
  {
    exprt other_index=*it;

    if(other_index!=index)
    {
      // we first build the guard
      
      if(other_index.type()!=index.type())
        other_index.make_typecast(index.type());

      literalt guard_lit=convert(equality_exprt(index, other_index));

      if(guard_lit!=const_literal(true))
      {
        index_exprt index_expr1;
        index_expr1.type()=expr.type().subtype();
        index_expr1.array()=expr;
        index_expr1.index()=other_index;

        index_exprt index_expr2;
        index_expr2.type()=expr.type().subtype();
        index_expr2.array()=expr.op0();
        index_expr2.index()=other_index;

        assert(index_expr1.type()==index_expr2.type());

        equality_exprt equality_expr(index_expr1, index_expr2);
        
        literalt equality_lit=convert(equality_expr);

        // add constraint
        bvt bv;
        bv.reserve(2);
        bv.push_back(equality_lit);
        bv.push_back(guard_lit);
        prop.lcnf(bv);
      }
    }
  }
}

/*******************************************************************\

Function: arrayst::add_array_constraints_array_of

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints_array_of(
  const index_sett &index_set,
  const array_of_exprt &expr)
{
  // we got x=array_of[v]
  // get other array index applications
  // and add constraint x[i]=v

  for(index_sett::const_iterator
      it=index_set.begin();
      it!=index_set.end();
      it++)
  {
    index_exprt index_expr;
    index_expr.type()=expr.type().subtype();
    index_expr.array()=expr;
    index_expr.index()=*it;

    assert(index_expr.type()==expr.op0().type());

    // add constraint
    set_to_true(equality_exprt(index_expr, expr.op0()));
  }
}

/*******************************************************************\

Function: arrayst::add_array_constraints_if

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void arrayst::add_array_constraints_if(
  const index_sett &index_set,
  const if_exprt &expr)
{
  // we got x=(c?a:b)
  literalt cond_lit=convert(expr.cond());

  // get other array index applications
  // and add c => x[i]=a[i]
  //        !c => x[i]=b[i]

  // first do true case

  for(index_sett::const_iterator
      it=index_set.begin();
      it!=index_set.end();
      it++)
  {
    index_exprt index_expr1;
    index_expr1.type()=expr.type().subtype();
    index_expr1.array()=expr;
    index_expr1.index()=*it;

    index_exprt index_expr2;
    index_expr2.type()=expr.type().subtype();
    index_expr2.array()=expr.true_case();
    index_expr2.index()=*it;

    assert(index_expr1.type()==index_expr2.type());

    // add implication
    bvt bv;
    bv.push_back(prop.lnot(cond_lit));
    bv.push_back(convert(equality_exprt(index_expr1, index_expr2)));
    prop.lcnf(bv);
  }

  // now the false case

  for(index_sett::const_iterator
      it=index_set.begin();
      it!=index_set.end();
      it++)
  {
    index_exprt index_expr1;
    index_expr1.type()=expr.type().subtype();
    index_expr1.array()=expr;
    index_expr1.index()=*it;

    index_exprt index_expr2;
    index_expr2.type()=expr.type().subtype();
    index_expr2.array()=expr.false_case();
    index_expr2.index()=*it;

    assert(index_expr1.type()==index_expr2.type());

    // add implication
    bvt bv;
    bv.push_back(cond_lit);
    bv.push_back(convert(equality_exprt(index_expr1, index_expr2)));
    prop.lcnf(bv);
  }
}
