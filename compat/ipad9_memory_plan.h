#ifndef XENIOS_IPAD9_MEMORY_PLAN_H_
#define XENIOS_IPAD9_MEMORY_PLAN_H_
#include <algorithm>
#include <cstdint>

namespace ipad9_memory {
// XNU 8792 memorystatus v1 ABI (iOS 16). Keep the existing inactive limit and
// both fatal-limit attributes. The caller only operates on its own getpid().
struct Properties {
  int32_t active_mb;
  uint32_t active_attr;
  int32_t inactive_mb;
  uint32_t inactive_attr;
};
static_assert(sizeof(Properties) == 16, "Unexpected memorystatus v1 ABI");

inline bool PlanRaise(uint64_t physical_bytes, const Properties& current,
                      Properties& requested) {
  requested = current;
  const uint64_t physical_mb = physical_bytes >> 20;
  // Reserve at least 1 GiB for the OS and other processes. Never impose a
  // lower limit, remove an existing limit, or change background behavior.
  if (physical_mb <= 1024 || current.active_mb <= 0) {
    return false;
  }
  const int32_t target_mb =
      static_cast<int32_t>(std::min(uint64_t(2048), physical_mb - 1024));
  if (current.active_mb >= target_mb) {
    return false;
  }
  requested.active_mb = target_mb;
  return true;
}
}  // namespace ipad9_memory
#endif
