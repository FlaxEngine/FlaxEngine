//
//          Tracy profiler
//         ----------------
//
// For fast integration, compile and
// link with this source file (and none
// other) in your executable (or in the
// main DLL / shared object on multi-DLL
// projects).
//

// Define TRACY_ENABLE to enable profiler.

#ifdef TRACY_ENABLE

#include "common/TracySystem.cpp"

#ifdef _MSC_VER
#  pragma warning(push, 0)
#endif

#include <ThirdParty/LZ4/lz4.h>
#include "client/TracyProfiler.cpp"
#include "client/TracyCallstack.cpp"
#include "client/TracySysTime.cpp"
#include "client/TracySysTrace.cpp"
#include "common/TracySocket.cpp"
#include "client/tracy_rpmalloc.cpp"

#if TRACY_HAS_CALLSTACK == 2 || TRACY_HAS_CALLSTACK == 3 || TRACY_HAS_CALLSTACK == 4 || TRACY_HAS_CALLSTACK == 6
#  include "libbacktrace/alloc.cpp"
#  include "libbacktrace/dwarf.cpp"
#  include "libbacktrace/fileline.cpp"
#  include "libbacktrace/mmapio.cpp"
#  include "libbacktrace/posix.cpp"
#  include "libbacktrace/sort.cpp"
#  include "libbacktrace/state.cpp"
#  if TRACY_HAS_CALLSTACK == 4
#    include "libbacktrace/macho.cpp"
#  else
#    include "libbacktrace/elf.cpp"
#  endif
#endif

#ifdef _MSC_VER
#  pragma comment(lib, "ws2_32.lib")
#  pragma comment(lib, "dbghelp.lib")
#  pragma comment(lib, "advapi32.lib")
#  pragma comment(lib, "user32.lib")
#  pragma warning(pop)
#endif

#endif
