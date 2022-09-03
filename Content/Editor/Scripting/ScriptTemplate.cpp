%copyright%#include "%filename%.h"

%class%::%class%(const SpawnParams& params)
    : Script(params)
{
    // Enable ticking OnUpdate function
    _tickUpdate = true;
}

void %class%::OnEnable()
{
    // Here you can add code that needs to be called when script is enabled (eg. register for events)
}

void %class%::OnDisable()
{
    // Here you can add code that needs to be called when script is disabled (eg. unregister from events)
}

void %class%::OnUpdate()
{
    // Here you can add code that needs to be called every frame
}
