#pragma once

#include "../common/include.h"
#include "../common/macro.h"
#include "../common/ref_cnt.h"
#include "../common/type.h"
#include "../struct/error/basic.h"
#include "../struct/obj/fn.h"

[[clang::always_inline]]
core_basic t_any get_obj_field__own(t_thrd_data* const thrd_data, t_any const obj, t_any const field_name) {
     assume(field_name.bytes[15] == tid__token);

     const char* const owner = "function - core.(:)";

     switch (obj.bytes[15]) {
     case tid__obj: {
          call_stack__push(thrd_data, owner);
          t_any const result = obj__get_field__own(thrd_data, obj, field_name, owner);
          call_stack__pop(thrd_data);

          return result;
     }
     case tid__null:
          return obj;

     [[clang::unlikely]]
     case tid__error:
          return obj;
     [[clang::unlikely]]
     default: {
          ref_cnt__dec(thrd_data, obj);

          call_stack__push(thrd_data, owner);
          t_any const result = error__invalid_type(thrd_data, owner);
          call_stack__pop(thrd_data);

          return result;
     }}
}

static t_any get_obj_field__partial_version(t_thrd_data* const thrd_data, const t_any* const args) {return get_obj_field__own(thrd_data, args[0], args[1]);}