#if PLATFORM_WINDOWS
#include "Windows/WindowsScreenUtils.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxScreenUtils.h"
#elif PLATFORM_MAC
#include "Mac/MacScreenUtils.h"
#else
#error No Screen Utils For This Platform! Please ensure you are not targetting an Editor build.
#endif

#include "Types.h"
