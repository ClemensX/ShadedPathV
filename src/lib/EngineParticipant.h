// almost all classes need a reference to the engine

// forward
class ShadedPathEngine;

class ShadedPathEngineParticipant
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
