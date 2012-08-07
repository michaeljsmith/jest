#include <string>
#include <stdio.h>
#include <boost/variant.hpp>
#include <boost/none.hpp>
#include <tr1/tuple>

using namespace std;
using namespace boost;
using namespace std::tr1;

struct AssertRaiser {
  AssertRaiser(bool condition, char const* file, int line, char const* msg) {
    if (!condition) {
      fprintf(stderr, "%s(%d): Assertion failed: %s\n", file, line, msg);
      exit(1);
    }
  }
};

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define ASSERT_OBJ_NAME_DETAIL(a, b) a##b
#define ASSERT_OBJ_NAME(a, b) ASSERT_OBJ_NAME_DETAIL(a, b)
#define ASSERT(x) AssertRaiser ASSERT_OBJ_NAME(assertObj, __LINE__) \
  ((x), __FILE__, __LINE__, #x);

int main() {
  auto x = [] () {printf("hello\n");};
  x();
  return 0;
}
