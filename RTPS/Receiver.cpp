#include "Receiver.h"

using namespace std;
using namespace colmap;

CReceiver::CReceiver(string ImageSaveDir)
{
	reg = new QSettings("HKEY_CURRENT_USER\\SOFTWARE\\RTPS\\CamFiTransmit", QSettings::NativeFormat);
	if (!reg->contains("Url") || !reg->contains("Port") || !reg->contains("Password")) //ע�����û�������Ϣ(��̨���Ե�һ��������������ʱ��)
	{
		QMessageBox::warning(this, tr("Warning"), tr("No transmit settings found, default transmit parameters will be used!"));
		Url = "http://192.168.9.67";
		Port = "8080";
		Password = "";

		//��Ĭ�ϵ�"�������"д��ע���
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
		else this->Password = QString2StdString(xorData(Password)); //ע����е�����ǿ�, ��������
	}
	while (reg->contains("NewImage") && reg->value("NewImage", QVariant()).toString() != "") //��"NewImage"��ֵ���ÿ�
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
	//	args->flags |= CREATE_NEW_CONSOLE; // ��������ģʽΪ����̨ģʽ
	//	args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES; // �����׼�������׼���롢��׼����ľ���ض���
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
	WinExec(Command.c_str(), SW_SHOW); //�����ⲿ��CamFiTransmit.exe����, �����䴫��"��������"
	this_thread::sleep_for(chrono::milliseconds(500));
	IsContinue = true;
	std::thread ReceiveThread = std::thread(&CReceiver::Receive, this); //��ʼ����ע���
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
	string Command = string("TASKKILL /F /IM ") + "CamFiTransmit.exe"; //����"�ر�CamFiTransmit.exe"��CMD����
	system(Command.c_str());
	IsContinue = false; //ֹͣ����ע���
	RegMutex.lock();
	reg->setValue("NewImage", "");
	RegMutex.unlock();
}
QString CReceiver::xorData(QString MyData)
{
	//һ��ʮ�ּ򵥵���֮��Ч��������ܽ����㷨
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