#pragma once
class Util
{
public:
    // Read all lines of log file for analysis
    static int scanLogFile();
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

