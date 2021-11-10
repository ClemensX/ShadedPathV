#pragma once

// all resources needed for running ina separate thread.
// it is ok to also READ from GlobalRendering and engine
class ThreadResources
{
public:
	virtual ~ThreadResources() {

	};
};

