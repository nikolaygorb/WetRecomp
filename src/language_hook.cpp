// sub_83038F78 is the XDK's statically linked XGetLanguage: it reads XCONFIG_USER_LANGUAGE
// through ExGetXConfigSetting(3, 9), which the SDK answers with the user_language cvar
// (settings/hardware.toml). Its only caller, sub_82D24540, maps the result to the game's
// own locale id and the game then loads <name>.<locale>.lu and streams*/<locale>/.
// The retail disc only ships five locales, so anything else is forced back to English
// instead of letting the game look for files that do not exist.
#include <cstdint>

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/flags.h>

REX_EXTERN(__imp__sub_83038F78);

namespace
{
  enum XLanguage : uint32_t
  {
    kEnglish = 1,
    kGerman = 3,
    kFrench = 4,
    kSpanish = 5,
    kItalian = 6,
  };

  bool IsShippedLanguage(uint32_t language)
  {
    switch (language)
    {
    case kEnglish:
    case kGerman:
    case kFrench:
    case kSpanish:
    case kItalian:
      return true;
    default:
      return false;
    }
  }
} // namespace

REX_HOOK_RAW(sub_83038F78)
{
  __imp__sub_83038F78(ctx, base);

  const uint32_t language = ctx.r3.u32;
  if (IsShippedLanguage(language))
    return;

  static bool warned = false;
  if (!warned)
  {
    warned = true;
    REXLOG_WARN("user_language = {} is not on the Wet disc (1, 3, 4, 5, 6); using English",
                REXCVAR_GET(user_language));
  }
  ctx.r3.u64 = kEnglish;
}
