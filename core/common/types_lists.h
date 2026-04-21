#pragma once

#include "type.h"

[[clang::always_inline]]
static bool type_is_correct(t_type_id type) {return type <= tid__error;}

[[clang::always_inline]]
static bool type_is_val(t_type_id type) {return type <= tid__holder;}

[[clang::always_inline]]
static bool type_with_ref_cnt(t_type_id type) {
     return (type >= tid__obj) & (type != tid__box) & (type != tid__breaker) & (type != tid__thread);
}

[[clang::always_inline]]
static bool type_is_common(t_type_id type) {
     return (type != tid__null) & (type != tid__box) & (type != tid__breaker) & (type != tid__error);
}

[[clang::always_inline]]
static bool type_is_common_or_null(t_type_id type) {
     return (type != tid__box) & (type != tid__breaker) & (type != tid__error);
}

[[clang::always_inline]]
static bool type_is_common_or_error(t_type_id type) {
     return (type != tid__null) & (type != tid__box) & (type != tid__breaker);
}

[[clang::always_inline]]
static bool type_is_common_or_null_or_error(t_type_id type) {
     return (type != tid__box) & (type != tid__breaker);
}

[[clang::always_inline]]
static bool type_is_not_copyable_or_error(t_type_id type) {
     return (type == tid__box) | (type == tid__breaker) | (type == tid__thread) | (type == tid__error);
}

[[clang::always_inline]]
static bool type_is_eq_and_common(t_type_id type) {
     return (type != tid__null) & (type != tid__short_fn) & (type != tid__box) & (type != tid__breaker) & (type != tid__error) & (type != tid__fn) & (type != tid__thread) & (type != tid__channel);
}