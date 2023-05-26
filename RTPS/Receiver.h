#pragma once
#include "Base.h"


class CReceiver :public QWidget
{
	Q_OBJECT
public:
	CReceiver(std::string ImageSaveDir);
	void StartReceive();
	void StopReceive();
	static QString xorData(QString data);
	~CReceiver();
signals:
	void NewImage_SIGNAL(std::string);
	void SenderQuit_SIGNAL();

private:
	std::string Url = "http://192.168.9.67";
	std::string Port = "8080";
	std::string Password = "";
	std::string ImageSaveDir = "";

	QSettings* reg;
	std::mutex RegMutex;
	QProcess process;
	bool IsContinue = false;
	void Receive();
};
