#include "ipad9_memory_plan.h"
#include <TargetConditionals.h>
#if TARGET_OS_IOS || TARGET_OS_IPHONE
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <os/proc.h>
#include <sys/sysctl.h>
#include <unistd.h>

namespace {
constexpr uint32_t kSetMemlimitProperties = 7;
constexpr uint32_t kGetMemlimitProperties = 8;
using MemStatus = int (*)(uint32_t, int32_t, uint32_t, void*, size_t);

void WriteStatus(const char* status, int error,
                 const ipad9_memory::Properties& before,
                 const ipad9_memory::Properties& after,
                 uint64_t physical_bytes, size_t headroom_before,
                 size_t headroom_after) {
  const char* home_dir = std::getenv("HOME");
  if (!home_dir) return;
  char path[4096];
  const int length = std::snprintf(path, sizeof(path),
      "%s/Documents/ipad9-memory-status.txt", home_dir);
  if (length <= 0 || size_t(length) >= sizeof(path)) return;
  FILE* file = std::fopen(path, "w");
  if (!file) return;
  std::fprintf(file,
      "XeniOS iPad9 memory request\n"
      "Status: %s\nError: %d (%s)\n"
      "Physical memory: %llu MiB\n"
      "Foreground process limit before: %d MiB\n"
      "Foreground process limit after: %d MiB\n"
      "Background process limit before/after: %d/%d MiB\n"
      "App headroom before/after: %zu/%zu MiB\n"
      "Only this app PID (%d) is changed. OS memory pressure can still kill "
      "the app; this is not additional physical RAM.\n",
      status, error, error ? std::strerror(error) : "none",
      static_cast<unsigned long long>(physical_bytes >> 20),
      before.active_mb, after.active_mb,
      before.inactive_mb, after.inactive_mb,
      headroom_before >> 20, headroom_after >> 20, int(getpid()));
  std::fclose(file);
}

void ApplyMemoryRequest(void*) {
  char model[64] = {};
  size_t model_length = sizeof(model);
  if (sysctlbyname("hw.machine", model, &model_length, nullptr, 0) != 0 ||
      (std::strcmp(model, "iPad12,1") && std::strcmp(model, "iPad12,2"))) {
    return;
  }
  ipad9_memory::Properties before{}, after{};
  uint64_t physical_bytes = 0;
  size_t physical_length = sizeof(physical_bytes);
  if (sysctlbyname("hw.memsize", &physical_bytes, &physical_length,
                  nullptr, 0) != 0) {
    WriteStatus("physical-memory-query-failed", errno, before, after, 0, 0, 0);
    return;
  }
  auto memstatus = reinterpret_cast<MemStatus>(
      dlsym(RTLD_DEFAULT, "memorystatus_control"));
  if (!memstatus) {
    WriteStatus("memory-API-unavailable", ENOSYS, before, after,
                physical_bytes, 0, 0);
    return;
  }
  // Read and modify only this process. No other PIDs or global jetsam settings.
  const int32_t own_pid = getpid();
  if (memstatus(kGetMemlimitProperties, own_pid, 0, &before,
                sizeof(before)) != 0) {
    WriteStatus("limit-query-failed", errno, before, after,
                physical_bytes, 0, 0);
    return;
  }
  const size_t headroom_before = os_proc_available_memory();
  after = before;
  ipad9_memory::Properties requested{};
  if (!ipad9_memory::PlanRaise(physical_bytes, before, requested)) {
    WriteStatus("existing-limit-retained", 0, before, after,
                physical_bytes, headroom_before, headroom_before);
    return;
  }
  if (memstatus(kSetMemlimitProperties, own_pid, 0, &requested,
                sizeof(requested)) != 0) {
    WriteStatus("raise-request-rejected", errno, before, after,
                physical_bytes, headroom_before, os_proc_available_memory());
    return;
  }
  if (memstatus(kGetMemlimitProperties, own_pid, 0, &after,
                sizeof(after)) != 0) {
    WriteStatus("request-accepted-verification-query-failed", errno,
                before, before, physical_bytes, headroom_before,
                os_proc_available_memory());
    return;
  }
  WriteStatus(after.active_mb >= requested.active_mb ?
              "raise-request-accepted" : "request-not-confirmed",
              0, before, after, physical_bytes, headroom_before,
              os_proc_available_memory());
}

__attribute__((constructor)) void ScheduleIPad9MemoryRequest() {
  // Wait for the app's main queue rather than invoking SPI in early dyld work.
  dispatch_async_f(dispatch_get_main_queue(), nullptr, ApplyMemoryRequest);
}
}  // namespace
#endif
