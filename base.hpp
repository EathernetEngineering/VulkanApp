#ifndef _BASE_HPP
#define _BASE_HPP

#include "config.h"

#include <memory>
#include <string>

#define CEE_ENABLE_DEBUG_BREAK 1

#if CEE_ENABLE_DEBUG_BREAK == 1
# if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#  define CEE_DEBUG_BREAK() __debugbreak()
# else
#  include <signal.h>
#  if defined(SIGTRAP)
#   define CEE_DEBUG_BREAK() raise(SIGTRAP)
#  else
#   define CEE_DEBUG_BREAK() raise(SIGABRT)
#  endif
# endif
#else
#define CEE_DEBUG_BREAK()
#endif

#define CEE_ENABLE_ASSERTIONS 1

#if CEE_ENABLE_ASSERTIONS == 1
#define CEE_ASSERT_WITH_MESSAGE(expression, message) if (!(expression)) { fprintf(stderr, "Assertion failed:\n\tExpression: %s\n\tMessage: %s\n\tLine:%i\n\tFile: %s\n", #expression, #message, __LINE__, __FILE__); CEE_DEBUG_BREAK(); }
#define CEE_ASSERT(expression) if (!(expression)) { fprintf(stderr, "Assertion failed:\n\tExpression: %s\n\tLine:%i\n\tFile: %s\n", #expression, __LINE__, __FILE__); CEE_DEBUG_BREAK(); }
#else
#define CEE_ASSERT_WITH_MESSAGE(expression)
#define CEE_ASSERT(expression)
#endif

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

#endif
