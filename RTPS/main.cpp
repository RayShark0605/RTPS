#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include "Base.h"

bool Base::IsDebugMode = false;
std::mutex Base::GPU_Mutex;
std::mutex Base::ThreadCount_Mutex;
std::mutex Base::Reconstruction_Mutex;
size_t Base::ThreadCount = 0;



/*
std::mutex Base::Images_Mutex;
std::mutex Base::Cameras_Mutex;
std::mutex Base::ImageName2ImageID_Mutex;
std::mutex Base::Matches_Mutex;
std::mutex Base::TwoViewGeometries_Mutex;
std::mutex Base::Reconstruction_Mutex;
size_t Base::ThreadCount = 0;
std::vector<CImage> Base::Images;
std::vector<colmap::Camera> Base::Cameras;
std::unordered_map<std::string, size_t> Base::ImageName2ImageID;
std::unordered_map<std::pair<size_t, size_t>, colmap::FeatureMatches, Base::PairHash> Base::Matches;
std::unordered_map<std::pair<size_t, size_t>, colmap::TwoViewGeometry, Base::PairHash> Base::TwoViewGeometries;
*/

using namespace std;
using namespace colmap;

int main(int argc, char* argv[])
{
	QApplication App(argc, argv);
	QTranslator* Translator = new QTranslator();
	Translator->load("Tanslation_zh_CN.qm");
	App.installTranslator(Translator);
	CMainWindow w;
	w.show();
	return App.exec();
}