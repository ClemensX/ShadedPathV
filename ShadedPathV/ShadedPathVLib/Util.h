#pragma once
class Util
{
public:
    static void printCStringList(vector<const char *> &exts) {
        for (uint32_t i = 0; i < exts.size(); i++) {
            Log("  " << exts[i] << endl);
        }
    };
};

class LogfileScanner
{
public:
    LogfileScanner();
    bool assertLineBefore(string before, string after);
    // search for line in logfile and return line number, -1 means not found
    // matching line beginnings is enough
    int searchForLine(string line, int startline = 0);
private:
    vector<string> lines;
};

