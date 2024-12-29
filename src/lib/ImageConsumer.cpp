#include "mainheader.h"

using namespace std;

// image consumers

void ImageConsumerDump::consume(FrameInfo* fi)
{
    if (dumpAll || frameNumbersToDump.find(fi->frameNum) != frameNumbersToDump.end()) {
        directImage.dumpToFile(fi->renderedImage);
    }
    fi->renderedImage->consumed = true;
    fi->renderedImage->rendered = false;
}

void ImageConsumerDump::configureFramesToDump(bool dumpAll, std::initializer_list<long> frameNumbers)
{
    this->dumpAll = dumpAll;
    frameNumbersToDump.clear();
    frameNumbersToDump.insert(frameNumbers.begin(), frameNumbers.end());
}

void ImageConsumerWindow::consume(FrameInfo* fi)
{
    Log("copy frame " << fi->frameNum << " to window " << window->title << endl);
    engine->presentation.presentImage(window, fi);
    fi->renderedImage->consumed = true;
    fi->renderedImage->rendered = false;
}
