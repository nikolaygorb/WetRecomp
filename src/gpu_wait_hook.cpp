// sub_83066340 is the XDK poll behind BlockUntilRingSpace: it returns nonzero to keep
// waiting for the GPU to free command-ring space and zero once the wait is over. The
// original poll is a few nops, so the render thread spins flat out while the emulated
// GPU is behind. Only the pause between polls changes; the result is untouched.
#include <chrono>
#include <thread>

#include <rex/cvar.h>
#include <rex/hook.h>

REXCVAR_DEFINE_INT32(wet_gpu_wait_mode, 1, "Performance",
                     "Pause between polls while the game waits for free command-buffer space: "
                     "0 = busy spin (original), 1 = yield the core, 2 = sleep 200us")
    .range(0, 2)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REX_EXTERN(__imp__sub_83066340);

namespace
{
  enum GpuWaitMode : int32_t
  {
    kBusySpin = 0,
    kYield = 1,
    kSleep = 2,
  };

  constexpr std::chrono::microseconds kSleepPause{200};
} // namespace

REX_HOOK_RAW(sub_83066340)
{
  __imp__sub_83066340(ctx, base);
  if (ctx.r3.u32 == 0)
    return;

  switch (REXCVAR_GET(wet_gpu_wait_mode))
  {
  case kYield:
    std::this_thread::yield();
    break;
  case kSleep:
    std::this_thread::sleep_for(kSleepPause);
    break;
  default:
    break;
  }
}
