#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        ShadedPathEngine engine;
        engine.init();
        engine.shutdown();
    }
    Log("SimpleApp ended" << endl);
}
