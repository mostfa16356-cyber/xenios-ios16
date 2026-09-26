#include <cassert>
#include <cstddef>
#include <cstdint>
#include "xenia/base/ipad9_resource_budget.h"

int main() {
  const size_t mib = size_t(1) << 20;
  const auto critical = xe::GetIPad9TextureBudget(0, 384, 768, 30);
  assert(critical.soft_mb == 24 && critical.hard_mb == 32);
  assert(critical.unused_seconds == 0 && critical.pressure_level == 2);
  const auto edge1 = xe::GetIPad9TextureBudget(256 * mib, 384, 768, 30);
  assert(edge1.soft_mb == 64 && edge1.hard_mb == 128);
  const auto edge2 = xe::GetIPad9TextureBudget(512 * mib, 384, 768, 30);
  assert(edge2.soft_mb == 96 && edge2.hard_mb == 192);
  uint32_t previous_hard = 0;
  for (size_t free_mib = 0; free_mib <= 4096; ++free_mib) {
    const auto b = xe::GetIPad9TextureBudget(free_mib * mib, 384, 768, 30);
    assert(b.soft_mb <= b.hard_mb && b.hard_mb <= 192);
    assert(b.hard_mb >= previous_hard);
    previous_hard = b.hard_mb;
    for (uint32_t requested : {0u, 8u, 32u, 64u, 96u, 192u, 768u}) {
      const auto restricted =
          xe::GetIPad9TextureBudget(free_mib * mib, requested, requested / 2, 0);
      assert(restricted.soft_mb <= restricted.hard_mb);
      assert(restricted.soft_mb <= requested);
      assert(restricted.hard_mb <= requested / 2);
      assert(restricted.unused_seconds == 0);
    }
  }
}
