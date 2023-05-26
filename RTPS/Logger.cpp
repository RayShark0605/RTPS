#include "Logger.h"
using namespace std;

string CLogger::LogFilePath = "RTPS.log";
mutex CLogger::LogQueue_Mutex;
queue<string> CLogger::LogQueue;
bool CLogger::StopFlag = false;

void CLogger::Log(const string& Message)
{
	auto now = chrono::system_clock::now();
	time_t time_t_now = chrono::system_clock::to_time_t(now);
	auto ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
	tm time_info;
	localtime_s(&time_info, &time_t_now);
	ostringstream oss;
	oss << put_time(&time_info, "%Y-%m-%d %H:%M:%S:") << setfill('0') << setw(3) << ms.count();

	string re = "[" + oss.str() + "] " + Message;
	lock_guard<mutex> lock(LogQueue_Mutex);
	LogQueue.push(re);
}

void CLogger::Run()
{
	ofstream file(LogFilePath, ofstream::out);
	if (!file.is_open())
	{
		throw "Open log file failed!";
	}
	file.close();
	while (!StopFlag)
	{
		LogQueue_Mutex.lock();
		if (!LogQueue.empty())
		{
			string Message = LogQueue.front();
			LogQueue.pop();

			ofstream ofs(LogFilePath, ofstream::app);
			if (!ofs.is_open())
			{
				throw "Open log file failed!";
			}
			ofs << Message << endl;
			ofs.close();
		}
		LogQueue_Mutex.unlock();
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
}








