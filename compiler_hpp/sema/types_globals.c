/* C globals that must remain in C (H++ can't define mutable globals).
   The H++ types module provides all functions. */
#include "sema/types.h"

HppType HPP_TYPE_BIT  = { .name = "bit",  .bit_width = 1, .is_bit_array = false };
HppType HPP_TYPE_VOID = { .name = NULL,   .bit_width = 0, .is_bit_array = false };

void *get_hpp_type_bit(void) { return &HPP_TYPE_BIT; }
void *get_hpp_type_void(void) { return &HPP_TYPE_VOID; }
