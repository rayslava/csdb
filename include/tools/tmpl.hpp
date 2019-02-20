#include <functional>

namespace tmpl {

  /* Declarations to use template arguments as std::function */
  template <class T>
  struct AsFunction
    : public AsFunction<decltype(&T::operator())>
  {};

  template <class ReturnType, class ... Args>
  struct AsFunction<ReturnType(Args...)> {
    using type = std::function<ReturnType(Args...)>;
  };

  template <class ReturnType, class ... Args>
  struct AsFunction<ReturnType (*)(Args...)> {
    using type = std::function<ReturnType(Args...)>;
  };

  template <class Class, class ReturnType, class ... Args>
  struct AsFunction<ReturnType (Class::*)(Args...) const> {
    using type = std::function<ReturnType(Args...)>;
  };

  template <class F>
  auto toFunction(F f)->typename AsFunction<F>::type {
    return {f};
  }

  /* Declaration to extract function return type */
  template <typename R, typename ... A>
  R ret(R (*)(A...));

  template <typename C, typename R, typename ... A>
  R ret(R (C::*)(A...));
}
