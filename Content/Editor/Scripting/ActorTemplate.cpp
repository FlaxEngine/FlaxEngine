%copyright%#include "%filename%.h"

%class%::%class%(const SpawnParams& params)
    : Actor(params)
{

}

void %class%::OnEnable()
{
    Actor::OnEnable();
    // Here you can add code that needs to be called when script is enabled (eg. register for events)
}

void %class%::OnDisable()
{
    Actor::OnDisable();
    // Here you can add code that needs to be called when script is disabled (eg. unregister from events)
}
