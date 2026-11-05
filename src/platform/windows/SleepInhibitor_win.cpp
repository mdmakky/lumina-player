#include "SleepInhibitor_win.h"

#include <windows.h>

void SleepInhibitor::set(bool active)
{
    // Per-thread state: always call from the GUI thread.
    SetThreadExecutionState(active ? (ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED)
                                   : ES_CONTINUOUS);
}
