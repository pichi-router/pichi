#ifndef PICHI_SCOPE_GUARD_HPP
#define PICHI_SCOPE_GUARD_HPP

#include <utility>

namespace pichi {

template <typename Destructor> class ScopeGuard {
public:
  ScopeGuard(ScopeGuard&& other)
    : destructor_{std::forward<Destructor>(other.destructor_)}, disabled_{other.disabled_}
  {
    other.disable();
  }

  ScopeGuard(ScopeGuard const&) = delete;
  ScopeGuard& operator=(ScopeGuard const&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

public:
  explicit ScopeGuard(Destructor&& destructor) : destructor_{std::forward<Destructor>(destructor)}
  {
  }

  ~ScopeGuard()
  {
    if (!disabled_) destructor_();
  }

  void disable() { disabled_ = true; }

private:
  Destructor destructor_;
  bool disabled_ = false;
};

template <typename Destructor> ScopeGuard<Destructor> makeScopeGuard(Destructor&& destructor)
{
  return ScopeGuard<Destructor>{std::forward<Destructor>(destructor)};
}

} // namespace pichi

#endif
