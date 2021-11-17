#include "pch.h"

ShadedPathEngine* ThreadResources::engine = nullptr;

void ThreadResources::initAll(ShadedPathEngine* engine)
{
	ThreadResources::engine = engine;
	for (ThreadResources &res : engine->threadResources) {
		res.init();
	}
}

ThreadResources::ThreadResources()
{
	Log("ThreadResource c'tor: " << this << endl);
}
void ThreadResources::init()
{
	Log("ThreadResource destrcutor: " << this << endl);
}

ThreadResources::~ThreadResources()
{
};

