#pragma once
class ShadedPathEngine
{
public:
    ShadedPathEngine() {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine() {
        Log("Engine destructor\n");
    };

    void init();
};

