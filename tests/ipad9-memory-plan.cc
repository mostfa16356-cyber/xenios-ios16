#include <cassert>
#include "../compat/ipad9_memory_plan.h"
int main() {
  using namespace ipad9_memory;
  const uint64_t mib = uint64_t(1) << 20;
  const Properties initial{1536, 1, 1024, 1};
  Properties requested{};
  assert(PlanRaise(3072 * mib, initial, requested));
  assert(requested.active_mb == 2048);
  assert(requested.inactive_mb == initial.inactive_mb);
  assert(requested.active_attr == initial.active_attr);
  assert(requested.inactive_attr == initial.inactive_attr);
  assert(!PlanRaise(0, initial, requested));
  assert(!PlanRaise(2048 * mib, initial, requested));
  assert(!PlanRaise(3072 * mib, Properties{-1, 1, 512, 1}, requested));
  assert(!PlanRaise(3072 * mib, Properties{0, 1, 512, 1}, requested));
  assert(!PlanRaise(3072 * mib, Properties{2600, 1, 512, 1}, requested));
  assert(requested.active_mb == 2600);
  for (uint64_t physical_mb = 1; physical_mb < 8192; ++physical_mb) {
    const bool raise = PlanRaise(physical_mb * mib, initial, requested);
    assert(requested.active_mb >= initial.active_mb);
    assert(requested.inactive_mb == initial.inactive_mb);
    if (raise) {
      assert(requested.active_mb <= 2048);
      assert(uint64_t(requested.active_mb) + 1024 <= physical_mb);
    }
  }
}
