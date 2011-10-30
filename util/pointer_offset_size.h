/*******************************************************************\

Module: Pointer Logic

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_POINTER_OFFSET_SIZE_H
#define CPROVER_POINTER_OFFSET_SIZE_H

#include <mp_arith.h>
#include <expr.h>
#include <namespace.h>

mp_integer member_offset(
  const namespacet &ns,
  const struct_typet &type,
  const irep_idt &member);

mp_integer pointer_offset_size(const typet &type);

mp_integer compute_pointer_offset(
  const namespacet &ns,
  const exprt &expr);

#endif
