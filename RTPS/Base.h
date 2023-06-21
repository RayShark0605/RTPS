#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <thread>
#include <windows.h>
#include <stdexcept>

#include <QtCore/QVariant>
#include <QtCore/QSettings>
#include <QtCore/QMetaType>
#include <QtCore/QDir>
#include <QtSql/QtSql>
#include <QtSql/QSqlQuery>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QProgressDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtCore/QDateTime>

#include <base/camera_models.h>
#include <base/image_reader.h>
#include <base/triangulation.h>
#include <controllers/incremental_mapper.h>
#include <estimators/pose.h>
#include <feature/matching.h>
#include <feature/sift.h>
#include <SiftGPU/SiftGPU.h>
#include <sfm/incremental_mapper.h>
#include <ui/options_widget.h>
#include <ui/thread_control_widget.h>
//#include <ui/model_viewer_widget.h>

#include <ui/log_widget.h>
#include <ui/render_options_widget.h>
#include <ui/reconstruction_manager_widget.h>
#include <util/cuda.h>
#include <util/string.h>
#include <util/option_manager.h>
#include <util/misc.h>

#include <cuda_runtime_api.h>
#include <nvml.h>
#include <psapi.h>



//QString转char数组
inline char* QString2Char(QString QString)
{
	return QString.toLocal8Bit().data();
}

//char数组转QString
inline QString Char2QString(char* string)
{
	return QString::fromLocal8Bit(string);
}

//QString转string
inline std::string QString2StdString(QString QString)
{
	return std::string(QString.toLocal8Bit());
}

//string转QString
inline QString StdString2QString(std::string string)
{
	return QString::fromLocal8Bit(string.data());
}

//从文件路径中提取出文件名
inline const char* GetFileName(std::string FilePath)
{
	std::string FileName = colmap::StringReplace(FilePath, "\\", "/");
	size_t pos = FileName.find_last_of('/');
	FileName = FileName.substr(pos + 1, FileName.size() - pos - 1);
	std::string* re = new std::string(FileName.c_str());
	return re->c_str();
}

//从文件路径中提取出其所属文件夹路径
inline std::string GetFileDir(std::string FilePath)
{
	std::string FileDir = colmap::StringReplace(FilePath, "\\", "/");
	size_t pos = FileDir.find_last_of('/');
	FileDir = FileDir.substr(0, pos);
	return FileDir;
}

inline std::string JoinPath(std::string Dir, std::string FileName)
{
	Dir = colmap::StringReplace(Dir, "\\", "/");
	if (Dir.back() != '/')Dir.push_back('/');
	return Dir + FileName;
}

//删除文件夹
inline bool DeleteDir(QString path)
{
	if (path.isEmpty())
	{
		return false;
	}
	QDir dir(path);
	if (!dir.exists())
	{
		return true;
	}
	dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
	QFileInfoList fileList = dir.entryInfoList();
	foreach(QFileInfo file, fileList)
	{
		if (file.isFile())
		{
			file.dir().remove(file.fileName());
		}
		else
		{
			DeleteDir(file.absoluteFilePath());
		}
	}
	return dir.rmpath(dir.absolutePath());
}

//inline int Size(int OriginSize, bool IsWidth)
//{
//	QRect ScreenRect = QApplication::desktop()->screenGeometry();
//	int ThisScreenLen = IsWidth ? ScreenRect.width() : ScreenRect.height();
//	return ThisScreenLen * OriginSize / (IsWidth ? 1920 : 1080);
//}

/*
enum CImageStatus { Error, NotInitialized, Initialized, Extracting, Extracted, Matching, Matched, Reconstructing, Finished };
class CImage
{
public:
	CImage()
	{
		ImageName = "";
		CameraID = 0;
		ImageStatus = NotInitialized;
		Keypoints.resize(0);
	}
	CImage(std::string name, size_t cameraid)
	{
		ImageName = name;
		CameraID = cameraid;
		ImageStatus = NotInitialized;
		Keypoints.resize(0);
	}
	CImage(std::string name, size_t cameraid, colmap::Image& image, CImageStatus status = NotInitialized)
	{
		ImageName = name;
		CameraID = cameraid;
		ImageStatus = status;
		Keypoints.resize(0);
		Image = image;
	}
	colmap::Image Image;
	CImageStatus ImageStatus;
	std::string ImageName;
	size_t ImageID;
	size_t CameraID;
	colmap::FeatureKeypoints Keypoints;
	colmap::FeatureDescriptors Descriptors;
};
*/

class Base
{
public:
	static bool IsDebugMode;
	static std::mutex ThreadCount_Mutex;
	static size_t ThreadCount;
	static std::mutex GPU_Mutex;
	static std::mutex Reconstruction_Mutex;
	static nvmlDevice_t device;
	static bool IsQuit;

	static size_t MaxMatches;
	static std::unordered_map<size_t, std::unordered_map<size_t, size_t>> MatchMatrix;
	static std::unordered_map<size_t, std::unordered_set<size_t>> MatchedImages;
	static std::mutex MatchMatrix_Mutex;



	/*
	static std::mutex Images_Mutex;
	static std::vector<CImage> Images;

	static std::mutex Cameras_Mutex;
	static std::vector<colmap::Camera> Cameras;

	static std::mutex ImageName2ImageID_Mutex;
	static std::unordered_map<std::string, size_t> ImageName2ImageID;

	struct PairHash //unordered_map原本并不支持pair类型, 需要加上这个"针对pair类型的计算哈希值的结构体"
	{
		template <typename T, typename U>
		size_t operator()(const std::pair<T, U>& x) const
		{
			return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
		}
	};
	static std::mutex Matches_Mutex;
	static std::unordered_map<std::pair<size_t, size_t>, colmap::FeatureMatches, PairHash> Matches;

	static std::mutex TwoViewGeometries_Mutex;
	static std::unordered_map<std::pair<size_t, size_t>, colmap::TwoViewGeometry, PairHash> TwoViewGeometries;
	*/
};

class DebugTimer
{
public:
	DebugTimer(const char* function)
		: m_function(function),
		m_startTime(QTime::currentTime())
	{
#ifdef OUTPUTLOG_MODE
		qDebug() << "[" << QThread::currentThreadId() << "] Entering " << m_function;
#endif
	}

	~DebugTimer()
	{
#ifdef OUTPUTLOG_MODE
		int elapsed = m_startTime.msecsTo(QTime::currentTime());
		qDebug() << "[" << QThread::currentThreadId() << "] Exiting " << m_function << " elapsed: " << elapsed << "ms";
#endif
	}

private:
	const char* m_function;
	QTime m_startTime;
};



inline void ThreadStart()
{
	std::lock_guard<std::mutex> lock(Base::ThreadCount_Mutex);
	Base::ThreadCount++;
}
inline void ThreadEnd()
{
	std::lock_guard<std::mutex> lock(Base::ThreadCount_Mutex);
	Base::ThreadCount--;
}
inline size_t GetThreadCount()
{
	size_t re;
	std::lock_guard<std::mutex> lock(Base::ThreadCount_Mutex);
	re = Base::ThreadCount;
	return re;
}

/*
inline bool IsExistCamera(size_t cameraID)
{
	Base::Cameras_Mutex.lock();
	bool re = (Base::Cameras.size() > cameraID);
	Base::Cameras_Mutex.unlock();
	return re;
}
inline bool IsExistCamera(colmap::Camera& camera)
{
	Base::Cameras_Mutex.lock();
	for (int i = 0; i < Base::Cameras.size(); i++)
	{
		if (Base::Cameras[i].Width() == camera.Width() && Base::Cameras[i].Height() == camera.Height() && Base::Cameras[i].Params() == camera.Params())
		{
			Base::Cameras_Mutex.unlock();
			return true;
		}
	}
	Base::Cameras_Mutex.unlock();
	return false;
}
inline size_t GetCameraID(colmap::Camera& camera)
{
	Base::Cameras_Mutex.lock();
	for (int i = 0; i < Base::Cameras.size(); i++)
	{
		if (Base::Cameras[i].Width() == camera.Width() && Base::Cameras[i].Height() == camera.Height() && Base::Cameras[i].Params() == camera.Params())
		{
			Base::Cameras_Mutex.unlock();
			return i;
		}
	}
	Base::Cameras_Mutex.unlock();
	throw "This camera is not exists!";
}
inline size_t GetCamerasNum()
{
	Base::Cameras_Mutex.lock();
	size_t re = Base::Cameras.size();
	Base::Cameras_Mutex.unlock();
	return re;
}
inline void GetCamera(size_t cameraID, colmap::Camera& camera)
{
	if (!IsExistCamera(cameraID))
	{
		throw "This cameraID is not exists!";
	}
	Base::Cameras_Mutex.lock();
	camera = Base::Cameras[cameraID];
	Base::Cameras_Mutex.unlock();
}
inline void GetAllCameras(std::vector<colmap::Camera>& cameras)
{
	Base::Cameras_Mutex.lock();
	cameras = Base::Cameras;
	Base::Cameras_Mutex.unlock();
}
inline size_t AddCamera(colmap::Camera& camera)
{
	if (IsExistCamera(camera))
	{
		throw "This camera is already exists!";
	}
	Base::Cameras_Mutex.lock();
	size_t NewCameraID = Base::Cameras.size();
	Base::Cameras.push_back(camera);
	Base::Cameras[NewCameraID].SetCameraId(NewCameraID);
	Base::Cameras_Mutex.unlock();
	return NewCameraID;
}

inline size_t GetImageID(std::string ImgName)
{
	Base::ImageName2ImageID_Mutex.lock();
	auto it = Base::ImageName2ImageID.find(ImgName);
	if (it == Base::ImageName2ImageID.end())
	{
		Base::ImageName2ImageID_Mutex.unlock();
		throw "This image is not exists!";
	}
	Base::ImageName2ImageID_Mutex.unlock();
	return it->second;
}
inline std::string GetImageName(size_t ImageID)
{
	Base::Images_Mutex.lock();
	if (Base::Images.size() <= ImageID)
	{
		Base::Images_Mutex.unlock();
		throw "This image is not exists!";
	}
	std::string re = Base::Images[ImageID].ImageName;
	Base::Images_Mutex.unlock();
	return re;
}
inline bool IsExistImage(size_t ImageID)
{
	Base::Images_Mutex.lock();
	bool re = (Base::Images.size() > ImageID);
	Base::Images_Mutex.unlock();
	return re;
}
inline bool IsExistImage(std::string ImgName)
{
	Base::ImageName2ImageID_Mutex.lock();
	if (Base::ImageName2ImageID.find(ImgName) == Base::ImageName2ImageID.end())
	{
		Base::ImageName2ImageID_Mutex.unlock();
		return false;
	}
	Base::ImageName2ImageID_Mutex.unlock();
	return true;
}
inline void GetImageCamera(size_t ImageID, colmap::Camera& camera)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image dose not exists!";
	}
	lock(Base::Images_Mutex, Base::Cameras_Mutex);
	size_t CameraID = Base::Images[ImageID].CameraID;
	if (CameraID >= Base::Cameras.size())
	{
		Base::Images_Mutex.unlock();
		Base::Cameras_Mutex.unlock();
		throw "This CameraID dose not exists!";
	}
	camera = Base::Cameras[CameraID];
	Base::Images_Mutex.unlock();
	Base::Cameras_Mutex.unlock();
}
inline CImageStatus GetImageStatus(size_t ImageID)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	CImageStatus re = Base::Images[ImageID].ImageStatus;
	Base::Images_Mutex.unlock();
	return re;
}
inline CImageStatus GetImageStatus(std::string ImgName)
{
	if (!IsExistImage(ImgName))
	{
		throw "This image is not exists!";
	}
	lock(Base::Images_Mutex, Base::ImageName2ImageID_Mutex);
	CImageStatus re = Base::Images[Base::ImageName2ImageID[ImgName]].ImageStatus;
	Base::Images_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
	return re;
}
inline void SetImageStatus(size_t ImageID, CImageStatus Status)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	Base::Images[ImageID].ImageStatus = Status;
	Base::Images_Mutex.unlock();
}
inline void SetImageStatus(std::string ImgName, CImageStatus Status)
{
	if (!IsExistImage(ImgName))
	{
		throw "This image is not exists!";
	}
	lock(Base::Images_Mutex, Base::ImageName2ImageID_Mutex);
	Base::Images[Base::ImageName2ImageID[ImgName]].ImageStatus = Status;
	Base::Images_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
}
inline size_t GetImagesNum()
{
	Base::Images_Mutex.lock();
	size_t re = Base::Images.size();
	Base::Images_Mutex.unlock();
	return re;
}
inline void GetImage(size_t ImageID, CImage& Image)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	Image = Base::Images[ImageID];
	Base::Images_Mutex.unlock();
}
inline void GetAllImages(std::vector<CImage>& Images_)
{
	Base::Images_Mutex.lock();
	Images_ = Base::Images;
	Base::Images_Mutex.unlock();
}
inline void GetAllImageNames(std::vector<std::string>& ImageNames)
{
	Base::Images_Mutex.lock();
	ImageNames.reserve(Base::Images.size());
	for (CImage& Image : Base::Images)
	{
		ImageNames.push_back(Image.Image.Name());
	}
	Base::Images_Mutex.unlock();
}
inline size_t AddImage(CImage& Image)
{
	lock(Base::Images_Mutex, Base::ImageName2ImageID_Mutex);
	if (Base::ImageName2ImageID.find(Image.ImageName) != Base::ImageName2ImageID.end())
	{
		Base::Images_Mutex.unlock();
		Base::ImageName2ImageID_Mutex.unlock();
		throw "This image's name is already exists!";
	}
	size_t NewImageID = Base::Images.size();
	Base::Images.push_back(Image);
	Base::Images[NewImageID].ImageID = NewImageID;
	Base::Images[NewImageID].ImageStatus = NotInitialized;
	Base::Images[NewImageID].Image.SetImageId(NewImageID);
	Base::ImageName2ImageID[Image.ImageName] = NewImageID;
	Base::Images_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
	return NewImageID;
}
inline void SetImageReg(size_t ImageID, bool IsReg)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image dose not exists!";
	}
	Base::Images_Mutex.lock();
	Base::Images[ImageID].Image.SetRegistered(IsReg);
	Base::Images_Mutex.unlock();
}

inline size_t GetImageKeypointsNum(size_t ImageID)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	size_t re = Base::Images[ImageID].Keypoints.size();
	Base::Images_Mutex.unlock();
	return re;
}
inline void GetKeypoints(size_t ImageID, colmap::FeatureKeypoints& Keypoints)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	Keypoints = Base::Images[ImageID].Keypoints;
	Base::Images_Mutex.unlock();
}
inline void AddKeypoints(size_t ImageID, colmap::FeatureKeypoints& Keypoints)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	if (Keypoints.empty())return;
	Base::Images_Mutex.lock();
	Base::Images[ImageID].Keypoints = Keypoints;
	Base::Images[ImageID].ImageStatus = CImageStatus::Extracted;
	Base::Images_Mutex.unlock();
}
inline void GetDescriptors(size_t ImageID, colmap::FeatureDescriptors& Descriptors)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	Descriptors = Base::Images[ImageID].Descriptors;
	Base::Images_Mutex.unlock();
}
inline void AddDescriptors(size_t ImageID, colmap::FeatureDescriptors& Descriptors)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	Base::Images_Mutex.lock();
	Base::Images[ImageID].Descriptors = Descriptors;
	Base::Images[ImageID].ImageStatus = CImageStatus::Extracted;
	Base::Images_Mutex.unlock();
}

inline size_t GetMatchesNum(size_t ImageID1, size_t ImageID2)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	if (ImageID1 == ImageID2)
	{
		throw "ImageID1 and ImageID2 are the same!";
	}
	Base::Matches_Mutex.lock();
	auto it = Base::Matches.find(std::make_pair(ImageID1, ImageID2));
	if (it != Base::Matches.end())
	{
		size_t re = it->second.size();
		Base::Matches_Mutex.unlock();
		return re;
	}
	it = Base::Matches.find(std::make_pair(ImageID2, ImageID1));
	if (it != Base::Matches.end())
	{
		size_t re = it->second.size();
		Base::Matches_Mutex.unlock();
		return re;
	}
	Base::Matches_Mutex.unlock();
	return 0;
}
inline void GetMatches(size_t ImageID1, size_t ImageID2, colmap::FeatureMatches& Matches_)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	if (ImageID1 == ImageID2)
	{
		throw "ImageID1 and ImageID2 are the same!";
	}
	Base::Matches_Mutex.lock();
	auto it = Base::Matches.find(std::make_pair(ImageID1, ImageID2));
	if (it != Base::Matches.end())
	{
		Matches_ = it->second;
		Base::Matches_Mutex.unlock();
		return;
	}
	it = Base::Matches.find(std::make_pair(ImageID2, ImageID1));
	if (it != Base::Matches.end())
	{
		Matches_ = it->second;
		Base::Matches_Mutex.unlock();
#pragma omp parallel for
		for (int i = 0; i < Matches_.size(); i++)
		{
			std::swap(Matches_[i].point2D_idx1, Matches_[i].point2D_idx2);
		}
		return;
	}
	Base::Matches_Mutex.unlock();
	Matches_.resize(0);
}
inline void AddMatches(size_t ImageID1, size_t ImageID2, colmap::FeatureMatches& Matches_)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	if (ImageID1 == ImageID2)
	{
		throw "ImageID1 and ImageID2 are the same!";
	}
	if (Matches_.empty())return;

	Base::Matches_Mutex.lock();
	if (ImageID1 > ImageID2)
	{
		Base::Matches[std::make_pair(ImageID1, ImageID2)] = Matches_;
		Base::Matches_Mutex.unlock();
		return;
	}
	std::pair<size_t, size_t> pair(std::make_pair(ImageID2, ImageID1));
	Base::Matches[pair] = Matches_;
#pragma omp parallel for
	for (int i = 0; i < Matches_.size(); i++)
	{
		std::swap(Base::Matches[pair][i].point2D_idx1, Base::Matches[pair][i].point2D_idx2);
	}
	Base::Matches_Mutex.unlock();
}
inline size_t MatchedImagesNum(size_t ImageID)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	size_t ImagesNum = GetImagesNum(), re = 0;
	for (size_t ImageID2 = 0; ImageID2 < ImagesNum; ImageID2++)
	{
		if (ImageID2 == ImageID)continue;
		if (GetMatchesNum(ImageID, ImageID2) > 0)
		{
			re++;
		}
	}
	return re;
}

inline size_t GetTwoViewGeometriesNum(size_t ImageID1, size_t ImageID2)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	Base::TwoViewGeometries_Mutex.lock();
	auto it = Base::TwoViewGeometries.find(std::make_pair(ImageID1, ImageID2));
	if (it != Base::TwoViewGeometries.end())
	{
		size_t re = it->second.inlier_matches.size();
		Base::TwoViewGeometries_Mutex.unlock();
		return re;
	}
	it = Base::TwoViewGeometries.find(std::make_pair(ImageID2, ImageID1));
	if (it != Base::TwoViewGeometries.end())
	{
		size_t re = it->second.inlier_matches.size();
		Base::TwoViewGeometries_Mutex.unlock();
		return re;
	}
	Base::TwoViewGeometries_Mutex.unlock();
	return 0;
}
inline void GetTwoViewGeometries(size_t ImageID1, size_t ImageID2, colmap::TwoViewGeometry& TwoViewGeometry)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	if (ImageID1 == ImageID2)
	{
		throw "ImageID1 and ImageID2 are the same!";
	}
	Base::TwoViewGeometries_Mutex.lock();
	auto it = Base::TwoViewGeometries.find(std::make_pair(ImageID1, ImageID2));
	if (it != Base::TwoViewGeometries.end())
	{
		TwoViewGeometry = it->second;
		Base::TwoViewGeometries_Mutex.unlock();
		return;
	}
	it = Base::TwoViewGeometries.find(std::make_pair(ImageID2, ImageID1));
	if (it != Base::TwoViewGeometries.end())
	{
		TwoViewGeometry = it->second;
		Base::TwoViewGeometries_Mutex.unlock();
#pragma omp parallel for
		for (int i = 0; i < TwoViewGeometry.inlier_matches.size(); i++)
		{
			std::swap(TwoViewGeometry.inlier_matches[i].point2D_idx1, TwoViewGeometry.inlier_matches[i].point2D_idx2);
		}
		return;
	}
	Base::TwoViewGeometries_Mutex.unlock();
	TwoViewGeometry = colmap::TwoViewGeometry();
}
inline void AddTwoViewGeometries(size_t ImageID1, size_t ImageID2, colmap::TwoViewGeometry& TwoViewGeometry)
{
	if (!IsExistImage(ImageID1) || !IsExistImage(ImageID2))
	{
		throw "One of images is not exists!";
	}
	if (ImageID1 == ImageID2)
	{
		throw "ImageID1 and ImageID2 are the same!";
	}
	if (TwoViewGeometry.inlier_matches.empty())return;
	Base::TwoViewGeometries_Mutex.lock();
	if (ImageID1 > ImageID2)
	{
		Base::TwoViewGeometries[std::make_pair(ImageID1, ImageID2)] = TwoViewGeometry;
		Base::TwoViewGeometries_Mutex.unlock();
		return;
	}
	std::pair<size_t, size_t> pair(std::make_pair(ImageID2, ImageID1));
	Base::TwoViewGeometries[pair] = TwoViewGeometry;
#pragma omp parallel for
	for (int i = 0; i < TwoViewGeometry.inlier_matches.size(); i++)
	{
		std::swap(Base::TwoViewGeometries[pair].inlier_matches[i].point2D_idx1, Base::TwoViewGeometries[pair].inlier_matches[i].point2D_idx2);
	}
	Base::TwoViewGeometries_Mutex.unlock();
}
inline size_t ExistTwoViewGeometriesImagesNum(size_t ImageID)
{
	if (!IsExistImage(ImageID))
	{
		throw "This image is not exists!";
	}
	size_t ImagesNum = GetImagesNum(), re = 0;
	for (size_t ImageID2 = 0; ImageID2 < ImagesNum; ImageID2++)
	{
		if (ImageID2 == ImageID)continue;
		if (GetTwoViewGeometriesNum(ImageID, ImageID2) > 0)
		{
			re++;
		}
	}
	return re;
}

inline void ClearData()
{
	lock(Base::Images_Mutex, Base::Cameras_Mutex, Base::ImageName2ImageID_Mutex, Base::Matches_Mutex, Base::TwoViewGeometries_Mutex);
	std::vector<CImage>().swap(Base::Images);
	std::vector<colmap::Camera>().swap(Base::Cameras);
	std::unordered_map<std::string, size_t>().swap(Base::ImageName2ImageID);
	std::unordered_map<std::pair<size_t, size_t>, colmap::FeatureMatches, Base::PairHash>().swap(Base::Matches);
	std::unordered_map<std::pair<size_t, size_t>, colmap::TwoViewGeometry, Base::PairHash>().swap(Base::TwoViewGeometries);
	Base::Images_Mutex.unlock();
	Base::Cameras_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
	Base::Matches_Mutex.unlock();
	Base::TwoViewGeometries_Mutex.unlock();
}
inline void ImportData(std::string DatabasePath, bool IsClearData = true)
{
	if (!colmap::ExistsFile(DatabasePath))
	{
		throw "This database path is invalid!";
	}
	if (IsClearData)ClearData();
	colmap::Database database(DatabasePath);
	colmap::DatabaseTransaction Transaction(&database);

	std::vector<colmap::Camera> Cameras_ = database.ReadAllCameras();
	std::vector<colmap::Image> Images_ = database.ReadAllImages();
	std::unordered_map<size_t, size_t> OldCameraID2NewCameraID;
	std::unordered_map<size_t, size_t> OldImageID2NewImageID;

	for (colmap::Camera& camera : Cameras_)
	{
		if (!IsExistCamera(camera))
		{
			AddCamera(camera);
		}
		OldCameraID2NewCameraID[camera.CameraId()] = GetCameraID(camera);
	}
	for (colmap::Image& image : Images_)
	{
		image.SetCameraId(OldCameraID2NewCameraID[image.CameraId()]);
		CImage NewImage(image.Name(), image.CameraId(), image, Matched);
		size_t NewImageID = AddImage(NewImage);

		OldImageID2NewImageID[image.ImageId()] = NewImageID;

		colmap::FeatureKeypoints Keypoints_ = database.ReadKeypoints(image.ImageId());
		AddKeypoints(NewImageID, Keypoints_);

		colmap::FeatureDescriptors Descriptors_ = database.ReadDescriptors(image.ImageId());
		AddDescriptors(NewImageID, Descriptors_);
	}

	std::vector<std::pair<colmap::image_pair_t, colmap::FeatureMatches>> Matches_ = database.ReadAllMatches();
	for (std::pair<colmap::image_pair_t, colmap::FeatureMatches>& pair : Matches_)
	{
		colmap::image_t OldImageID1, OldImageID2;
		colmap::Database::PairIdToImagePair(pair.first, &OldImageID1, &OldImageID2);
		AddMatches(OldImageID2NewImageID[OldImageID1], OldImageID2NewImageID[OldImageID2], pair.second);

		colmap::TwoViewGeometry Geometry_ = database.ReadTwoViewGeometry(OldImageID1, OldImageID2);
		AddTwoViewGeometries(OldImageID2NewImageID[OldImageID1], OldImageID2NewImageID[OldImageID2], Geometry_);
	}
}
inline void ExportData(std::string DatabasePath, bool NeedAdd1 = true)
{
	if (colmap::ExistsFile(DatabasePath))
	{
		QFile File(StdString2QString(DatabasePath));
		File.remove();
	}
	colmap::Database database(DatabasePath);
	colmap::DatabaseTransaction Transaction(&database);

	lock(Base::Images_Mutex, Base::Cameras_Mutex, Base::ImageName2ImageID_Mutex, Base::Matches_Mutex, Base::TwoViewGeometries_Mutex);
	std::vector<colmap::Camera> cameras_ = Base::Cameras;
	std::vector<CImage> images_ = Base::Images;
	std::unordered_map<std::pair<size_t, size_t>, colmap::FeatureMatches, Base::PairHash> Matches_ = Base::Matches;
	std::unordered_map<std::pair<size_t, size_t>, colmap::TwoViewGeometry, Base::PairHash> TwoViewGeometries_ = Base::TwoViewGeometries;
	Base::Images_Mutex.unlock();
	Base::Cameras_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
	Base::Matches_Mutex.unlock();
	Base::TwoViewGeometries_Mutex.unlock();

#pragma omp parallel for
	for (int i = 0; i < cameras_.size(); i++)
	{
		cameras_[i].SetCameraId(cameras_[i].CameraId() + NeedAdd1);
	}

#pragma omp parallel for
	for (int i = 0; i < images_.size(); i++)
	{
		images_[i].CameraID = images_[i].CameraID + NeedAdd1;
		images_[i].ImageID = images_[i].ImageID + NeedAdd1;
		images_[i].Image.SetCameraId(images_[i].Image.CameraId() + NeedAdd1);
		images_[i].Image.SetImageId(images_[i].Image.ImageId() + NeedAdd1);
	}

	for (colmap::Camera& camera : cameras_)
	{
		database.WriteCamera(camera, true);
	}
	for (CImage& image : images_)
	{
		database.WriteImage(image.Image, true);
		database.WriteKeypoints(image.ImageID, image.Keypoints);
		database.WriteDescriptors(image.ImageID, image.Descriptors);
	}
	for (auto it = Matches_.begin(); it != Matches_.end(); it++)
	{
		if (it->second.empty())continue;
		size_t ImageID1 = it->first.first + NeedAdd1;
		size_t ImageID2 = it->first.second + NeedAdd1;
		database.WriteMatches(ImageID1, ImageID2, it->second);
	}
	for (auto it = TwoViewGeometries_.begin(); it != TwoViewGeometries_.end(); it++)
	{
		if (it->second.inlier_matches.empty())continue;
		size_t ImageID1 = it->first.first + NeedAdd1;
		size_t ImageID2 = it->first.second + NeedAdd1;
		database.WriteTwoViewGeometry(ImageID1, ImageID2, it->second);
	}
}
inline void ExportData_OnlyMatchedImages(std::string DatabasePath, bool NeedAdd1 = true)
{
	if (colmap::ExistsFile(DatabasePath))
	{
		QFile File(StdString2QString(DatabasePath));
		File.remove();
	}

	std::vector<CImage> images_;
	std::unordered_set<size_t> UnMatchedImages;

	colmap::Database database(DatabasePath);
	colmap::DatabaseTransaction Transaction(&database);

	lock(Base::Images_Mutex, Base::Cameras_Mutex, Base::ImageName2ImageID_Mutex, Base::Matches_Mutex, Base::TwoViewGeometries_Mutex);
	std::vector<colmap::Camera> cameras_ = Base::Cameras;
	std::unordered_map<std::pair<size_t, size_t>, colmap::FeatureMatches, Base::PairHash> Matches_ = Base::Matches;
	std::unordered_map<std::pair<size_t, size_t>, colmap::TwoViewGeometry, Base::PairHash> TwoViewGeometries_ = Base::TwoViewGeometries;
#pragma omp parallel for
	for (int i = 0; i < cameras_.size(); i++)
	{
		cameras_[i].SetCameraId(cameras_[i].CameraId() + NeedAdd1);
	}
	for (colmap::Camera& camera : cameras_)
	{
		database.WriteCamera(camera, true);
	}
	images_.reserve(Base::Images.size());
	for (CImage& image : Base::Images)
	{
		if (image.ImageStatus == Matched)
		{
			images_.push_back(image);
			images_.back().CameraID += NeedAdd1;
			images_.back().ImageID += NeedAdd1;
			images_.back().Image.SetCameraId(images_.back().Image.CameraId() + NeedAdd1);
			images_.back().Image.SetImageId(images_.back().Image.ImageId() + NeedAdd1);
		}
		else
		{
			UnMatchedImages.insert(image.ImageID);
		}
	}
	Base::Images_Mutex.unlock();
	Base::Cameras_Mutex.unlock();
	Base::ImageName2ImageID_Mutex.unlock();
	Base::Matches_Mutex.unlock();
	Base::TwoViewGeometries_Mutex.unlock();
	for (CImage& image : images_)
	{
		database.WriteImage(image.Image, true);
		database.WriteKeypoints(image.ImageID, image.Keypoints);
		database.WriteDescriptors(image.ImageID, image.Descriptors);
	}

	for (auto it = Matches_.begin(); it != Matches_.end(); it++)
	{
		if (it->second.empty() || UnMatchedImages.count(it->first.first) || UnMatchedImages.count(it->first.second))continue;
		size_t ImageID1 = it->first.first + NeedAdd1;
		size_t ImageID2 = it->first.second + NeedAdd1;
		database.WriteMatches(ImageID1, ImageID2, it->second);
	}
	for (auto it = TwoViewGeometries_.begin(); it != TwoViewGeometries_.end(); it++)
	{
		if (it->second.inlier_matches.empty() || UnMatchedImages.count(it->first.first) || UnMatchedImages.count(it->first.second))continue;
		size_t ImageID1 = it->first.first + NeedAdd1;
		size_t ImageID2 = it->first.second + NeedAdd1;
		database.WriteTwoViewGeometry(ImageID1, ImageID2, it->second);
	}
}

inline void ExportDatabaseCache(colmap::OptionManager* options, colmap::DatabaseCache& DbCache)
{
	std::string TempDatabasePath = QString2StdString(QCoreApplication::applicationDirPath()) + "/temp.db";
	ExportData_OnlyMatchedImages(TempDatabasePath, false);
	colmap::Database database(TempDatabasePath);
	colmap::DatabaseTransaction Transaction(&database);
	std::unordered_set<std::string> image_names;
	DbCache.Load(database, options->mapper->min_num_matches, options->mapper->ignore_watermarks, image_names);
	QFile File(StdString2QString(TempDatabasePath));
	File.remove();
}
*/

inline QGridLayout* GenerateMultiOptionsLayout(std::vector<std::string> LabelTexts, std::vector<QWidget*> Controllers)
{
	if (LabelTexts.size() != Controllers.size())
	{
		return nullptr;
	}
	QGridLayout* Layout = new QGridLayout();
	QFont LabelFont;
	LabelFont.setPointSize(10);
	for (size_t i = 0; i < LabelTexts.size(); i++)
	{
		QLabel* Label = new QLabel(StdString2QString(LabelTexts[i]));
		Label->setFont(LabelFont);
		Layout->addWidget(Label, i, 0, Qt::AlignRight);
		if ((dynamic_cast<QSpinBox*>(Controllers[i])) != nullptr || (dynamic_cast<QDoubleSpinBox*>(Controllers[i])) != nullptr || (dynamic_cast<QLineEdit*>(Controllers[i])) != nullptr)
		{
			Controllers[i]->setFixedWidth(150);
		}
		Layout->addWidget(Controllers[i], i, 1, Qt::AlignLeft);
	}
	return Layout;
}

inline void ScaleImage(std::string ImagePath, int MaxSize)
{
	QImage image;
	if (!image.load(StdString2QString(ImagePath)) || image.width() * image.height() == 0)
	{
		throw "Failed to load image";
	}
	int OriginWidth = image.width();
	int OriginHeight = image.height();
	if (std::max(OriginWidth, OriginHeight) <= MaxSize)
	{
		return;
	}
	int TargetWidth, TargetHeight;
	if (OriginWidth > OriginHeight)
	{
		TargetWidth = MaxSize;
		TargetHeight = TargetWidth * (OriginHeight * 1.0 / OriginWidth);
	}
	else
	{
		TargetHeight = MaxSize;
		TargetWidth = TargetHeight * (OriginWidth * 1.0 / OriginHeight);
	}
	QImage ScaledImage = image.scaled(TargetWidth, TargetHeight);
	if (!ScaledImage.save(StdString2QString(ImagePath)))
	{
		throw "Failed to save image";
	}
}

inline void GetGPUMemUsage(double& TotalGB, double& UsedGB, double& FreeGB)
{
	nvmlMemory_t memory;
	nvmlReturn_t result = nvmlDeviceGetMemoryInfo(Base::device, &memory);
	if (result != NVML_SUCCESS)
	{
		std::cout << "Unable to get the GPU memory usage!" << endl;
		TotalGB = UsedGB = FreeGB = 0;
		return;
	}
	TotalGB = memory.total / (1024.0 * 1024.0 * 1024.0);
	UsedGB = memory.used / (1024.0 * 1024.0 * 1024.0);
	FreeGB = memory.free / (1024.0 * 1024.0 * 1024.0);
}
inline bool IsGPUAvailable()
{
	int CUDA_DeviceCount = 0;
	cudaGetDeviceCount(&CUDA_DeviceCount);
	if (CUDA_DeviceCount == 0)return false;
	double TotalGB, UsedGB, FreeGB;
	GetGPUMemUsage(TotalGB, UsedGB, FreeGB);
	if (TotalGB < 4 || FreeGB < 2)
	{
		return false;
	}
	return true;
}
inline void GetMemoryUsage(double& TotalGB, double& UsedGB, double& FreeGB)
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	UsedGB = pmc.WorkingSetSize / (1024.0 * 1024.0 * 1024.0);
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	TotalGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
	FreeGB = TotalGB - UsedGB;
}

inline void ThrowError(std::string ErrorInfo)
{
	try 
	{
		auto ThreadID = QThread::currentThreadId();
		throw std::runtime_error(ErrorInfo);
	}
	catch (const std::exception& ex) 
	{
		QMessageBox::critical(nullptr, "Fatal error", ex.what());
		qApp->exit(0);
	}
}

inline QSqlDatabase CreateDatabaseConnect(std::string DatabasePath)
{
	QSqlDatabase db;
	QString TargetConnectName = "Connection" + QString::number((quintptr)QThread::currentThreadId()) + "_" + QTime::currentTime().toString("hh_mm_ss_zzz");
#ifdef OUTPUTLOG_MODE
	qDebug() << TargetConnectName;
#endif
	if (QSqlDatabase::contains(TargetConnectName))
	{
		db = QSqlDatabase::database(TargetConnectName);
	}
	else
	{
		db = QSqlDatabase::addDatabase("QSQLITE", TargetConnectName);
	}
	db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=10000");
	db.setDatabaseName(StdString2QString(DatabasePath));
	if (!db.open())
	{
		ThrowError("Error: Unable to open database!");
	}
	QSqlQuery query(db);
	query.exec("PRAGMA busy_timeout = 999999");
	return db;
}
inline void ReleaseDatabaseConnect(QSqlDatabase& db)
{
	QString connectionName = db.connectionName();
	db.close();
	QSqlDatabase::removeDatabase(connectionName);
}
