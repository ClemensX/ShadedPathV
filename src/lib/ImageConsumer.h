// main engine

#pragma once


// most simple image consumer: just discard the image
class ImageConsumerNullify : public ImageConsumer
{
public:
    void consume(FrameInfo* fi) override {
        fi->renderedImage->consumed = true;
        fi->renderedImage->rendered = false;
    }
};

// image consumer to dump generated images to disk
class ImageConsumerDump : public ImageConsumer
{
public:
    void consume(FrameInfo* fi) override;
    void configureFramesToDump(bool dumpAll, std::initializer_list<long> frameNumbers);
    ImageConsumerDump(ShadedPathEngine* s) {
        setEngine(s);
        directImage.setEngine(s);
    }
private:
    bool dumpAll = false;
    std::unordered_set<long> frameNumbersToDump;
    DirectImage directImage;
};

// image consumer to show image in glfw window
class ImageConsumerWindow : public ImageConsumer
{
public:
    void consume(FrameInfo* fi) override;
    void setWindow(WindowInfo* w) {
        window = w;
    }
    ImageConsumerWindow(ShadedPathEngine* s) {
        setEngine(s);
    }
private:
    WindowInfo* window = nullptr;
};

