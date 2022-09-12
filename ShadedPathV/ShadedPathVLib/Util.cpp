#include "pch.h"
#include "Util.h"

using namespace std;

LogfileScanner::LogfileScanner()
{
    string line;
    ifstream myfile("spe_run.log");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            lines.push_back(line);
        }
        myfile.close();
        //Log("lines: " << lines.size() << endl);
    }
    else {
        Error("Could not open log file");
    }
}

bool LogfileScanner::assertLineBefore(string before, string after)
{
    int beforePos = searchForLine(before);
    if (beforePos < 0)
        return false;
    int afterPos = searchForLine(after, beforePos);
    if (beforePos < afterPos)
        return true;
    return false;
}

int LogfileScanner::searchForLine(string line, int startline)
{
    for (int i = startline; i < lines.size(); i++) {
        string lineInFile = lines.at(i);
        if (lineInFile.rfind(line, 0) != string::npos)
            return i;
    }
    return -1;
}

void Util::initializeDebugFunctionPointers() {
    pfnDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(engine->global.device, "vkDebugMarkerSetObjectNameEXT");
}

void Util::debugNameObject(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name) {
    if (pfnDebugMarkerSetObjectNameEXT == nullptr) {
        initializeDebugFunctionPointers();
    }
    // Check for a valid function pointer
    if (pfnDebugMarkerSetObjectNameEXT)
    {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name;
        pfnDebugMarkerSetObjectNameEXT(engine->global.device, &nameInfo);
    }
}
