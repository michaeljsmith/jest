#include <stdio.h>

namespace DETAIL {
  template <typename T> struct Finite {
  };

  template <typename T> void output(T const& val) {
    printf("preprocessed");
  };
}
#define JEST_DEFINE(x) namespace TYPES {struct x {};} DETAIL::Finite<TYPES::x> x;
