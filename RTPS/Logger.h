#pragma once
#include "Base.h"
#include <iomanip>

class CLogger
{
public:
	CLogger() = delete;
	static bool StopFlag;
	static void Log(const std::string& Message);
	static void Run();
private:
	static std::string LogFilePath;
	static std::mutex LogQueue_Mutex;
	static std::queue<std::string> LogQueue;
};

