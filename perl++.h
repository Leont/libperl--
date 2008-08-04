// External includes

#include <string>
#include <cstring>
#include <map>
#include <exception>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/logical.hpp>

#ifdef DEBUG
#include <cstdio>
#define debug(foo) std::fprintf(stderr, "In file %s on line %u: %s\n", __FILE__, __LINE__, foo)
#define debugf(foo, ...) std::fprintf(stderr, "In file %s on line %u: " foo "\n",  __FILE__, __LINE__,  __VA_ARGS__)
#else
#define debug(foo)
#define debugf(...)
#endif

// I split the header over 5 files, to ease working with them.
#include "helpers.h"
#include "scalar.h"
#include "collections.h"
#include "other.h"
#include "interpreter.h"
