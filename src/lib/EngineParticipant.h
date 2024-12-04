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

// change image on GPU
class ImageWriter : public EngineParticipant
{
public:
    virtual ~ImageWriter() = 0;
    virtual void writeToImage(GPUImage* gpui) = 0;
};

// consume image on GPU (copy to CPU or to another GPU image)
class ImageConsumer : public EngineParticipant
{
public:
    //virtual ~ImageConsumer() = 0;
    virtual void consume(GPUImage* gpui) = 0;
};
