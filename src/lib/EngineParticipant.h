// almost all classes need a reference to the engine

// forward
class ShadedPathEngine;

class EngineParticipant
{
public:
    void setEngine(ShadedPathEngine* link) {
        engine = link;
    };
    ShadedPathEngine* getEngine() {
        return engine;
    }
    ShadedPathEngine* engine = nullptr;
};
