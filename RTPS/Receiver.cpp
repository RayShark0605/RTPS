#include "Receiver.h"

using namespace std;
using namespace colmap;

CReceiver::CReceiver(string ImageSaveDir)
{
	reg = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\RTPS\\CamFiTransmit", QSettings::NativeFormat);
	if (!reg->contains("Url") || !reg->contains("Port") || !reg->contains("Password")) //注册表中没有相关信息(这台电脑第一次运行这个程序的时候)
	{
		QMessageBox::warning(this, tr("Warning"), tr("No transmit settings found, default transmit parameters will be used!"));
		Url = "http://192.168.9.67";
		Port = "8080";
		Password = "";

		//将默认的"传输参数"写入注册表
		reg->setValue("Url", StdString2QString(Url));
		reg->setValue("Port", StdString2QString(Port));
		reg->setValue("Password", "");
	}
	else
	{
		Url = QString2StdString(reg->value("Url", QVariant()).toString());
		Port = QString2StdString(reg->value("Port", QVariant()).toString());

		QString Password = reg->value("Password", QVariant()).toString();
		if (Password == "")this->Password = "";
		else this->Password = QString2StdString(xorData(Password)); //注册表中的密码非空, 则对其解密
	}
	while (reg->contains("NewImage") && reg->value("NewImage", QVariant()).toString() != "") //将"NewImage"的值设置空
	{
		reg->setValue("NewImage", QVariant(""));
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
	this->ImageSaveDir = ImageSaveDir;
	IsContinue = false;
}
void CReceiver::StartReceive()
{
	//QStringList arguments;
	////arguments << "/c" << "CamFiTransmit.exe" << StdString2QString(Url) << StdString2QString(Port);
	//arguments << StdString2QString(Url) << StdString2QString(Port);
	//if (Password.size() != 0)
	//{
	//	arguments << StdString2QString(Password);
	//}
	//arguments << ("\"" + StdString2QString(ImageSaveDir) + "\"");
	//process.setWorkingDirectory(QCoreApplication::applicationDirPath());
	//process.setProgram("CamFiTransmit.exe");
	//process.setArguments(arguments);
	//process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args)
	//{
	//	args->flags |= CREATE_NEW_CONSOLE; // 设置启动模式为控制台模式
	//	args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES; // 解除标准输出、标准输入、标准错误的句柄重定向
	//});
	//process.start();
	//process.waitForStarted();
	string Command = "CamFiTransmit.exe";
	Command += " "; Command += Url;
	Command += " "; Command += Port;
	if (Password != "")
	{
		Command += " "; Command += Password;
	}
	Command += " \""; Command += ImageSaveDir; Command += "\"";
	WinExec(Command.c_str(), SW_SHOW); //运行外部的CamFiTransmit.exe程序, 并向其传递"传输设置"
	this_thread::sleep_for(chrono::milliseconds(500));
	IsContinue = true;
	std::thread ReceiveThread = std::thread(&CReceiver::Receive, this); //开始侦听注册表
	ReceiveThread.detach();
}
void CReceiver::StopReceive()
{
	/*IsContinue = false;
	if (process.state() == QProcess::Running)
	{
		process.terminate();
		process.waitForFinished();
	}*/
	string Command = string("TASKKILL /F /IM ") + "CamFiTransmit.exe"; //生成"关闭CamFiTransmit.exe"的CMD命令
	system(Command.c_str());
	IsContinue = false; //停止侦听注册表
	RegMutex.lock();
	reg->setValue("NewImage", "");
	RegMutex.unlock();
}
QString CReceiver::xorData(QString MyData)
{
	//一个十分简单但行之有效的密码加密解密算法
	QByteArray data = MyData.toUtf8();
	static QByteArray key1 = "qt";
	static QByteArray key2 = "c++";
	for (int i = 0; i < data.size(); i++)
	{
		int keyIndex1 = i % key1.size();
		int keyIndex2 = i % key2.size();
		data[i] = data[i] ^ key1[keyIndex1];
		data[i] = data[i] ^ key2[keyIndex2];
	}
	return QString(data);
}
void CReceiver::Receive()
{
	ThreadStart();
	while (IsContinue && reg)
	{
		/*if (process.state() == QProcess::NotRunning)
		{
			StopReceive();
			emit SenderQuit_SIGNAL();
			this_thread::sleep_for(chrono::milliseconds(300));
		}*/
		RegMutex.lock();
		if (!reg || (reg && !reg->contains("NewImage")))
		{
			RegMutex.unlock();
			this_thread::sleep_for(chrono::milliseconds(300));
			continue;
		}
		
		QString RegValue = reg->value("NewImage", QVariant()).toString();
		if (RegValue.size() <= 2)
		{
			RegMutex.unlock();
			this_thread::sleep_for(chrono::milliseconds(300));
			continue;
		}
		size_t pos = RegValue.indexOf(";");
		reg->setValue("NewImage", RegValue.mid(pos + 1));
		RegMutex.unlock();

		QString NewImagePath = RegValue.left(pos);
		if (!ExistsFile(QString2StdString(NewImagePath)))continue;
		emit NewImage_SIGNAL(QString2StdString(NewImagePath));
	}
	ThreadEnd();
}
CReceiver::~CReceiver()
{
	StopReceive();
	RegMutex.lock();
	delete reg;
	RegMutex.unlock();
}