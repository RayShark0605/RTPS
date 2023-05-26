#pragma once
#include "Base.h"

QDataStream& operator<<(QDataStream& out, const colmap::FeatureKeypoint& keypoint);
QDataStream& operator>>(QDataStream& in, colmap::FeatureKeypoint& keypoint);

class CDatabase :public QObject
{
	Q_OBJECT
public:

	static std::string DatabasePath;

	static bool NewDatabase(std::string DatabasePath, QSqlDatabase& db);
	static bool OpenDatabase(std::string DatabasePath, QSqlDatabase& db);

	static bool IsExistCamera(size_t cameraID, QSqlDatabase& db);
	static bool IsExistCamera(colmap::Camera& camera, QSqlDatabase& db);
	static size_t GetCameraID(colmap::Camera& camera, QSqlDatabase& db);
	static size_t GetCamerasNum(QSqlDatabase& db);
	static void GetCamera(size_t cameraID, colmap::Camera& camera, QSqlDatabase& db);
	static void GetAllCameras(std::vector<colmap::Camera>& cameras, QSqlDatabase& db);
	static size_t AddCamera(colmap::Camera& camera, QSqlDatabase& db);

	static size_t GetImageID(std::string ImgName, QSqlDatabase& db);
	static std::string GetImageName(size_t ImageID, QSqlDatabase& db);
	static bool IsExistImage(size_t ImageID, QSqlDatabase& db);
	static bool IsExistImage(std::string ImgName, QSqlDatabase& db);
	static void GetImageCamera(size_t ImageID, colmap::Camera& camera, QSqlDatabase& db);
	static size_t GetImagesNum(QSqlDatabase& db);
	static void GetImage(size_t ImageID, colmap::Image& Image, QSqlDatabase& db);
	static void GetAllImages(std::vector<colmap::Image>& Images_, QSqlDatabase& db);
	static void GetAllImageNames(std::vector<std::string>& ImageNames, QSqlDatabase& db);
	static size_t AddImage(colmap::Image& Image, QSqlDatabase& db);

	static size_t GetImageKeypointsNum(size_t ImageID, QSqlDatabase& db);
	static void GetKeypoints(size_t ImageID, colmap::FeatureKeypoints& Keypoints, QSqlDatabase& db);
	static void AddKeypoints(size_t ImageID, colmap::FeatureKeypoints& Keypoints, QSqlDatabase& db);
	static size_t GetImageDescriptorsNum(size_t ImageID, QSqlDatabase& db);
	static void GetDescriptors(size_t ImageID, colmap::FeatureDescriptors& Descriptors, QSqlDatabase& db);
	static void AddDescriptors(size_t ImageID, colmap::FeatureDescriptors& Descriptors, QSqlDatabase& db);

	static size_t GetMatchesNum(size_t ImageID1, size_t ImageID2, QSqlDatabase& db);
	static void GetMatches(size_t ImageID1, size_t ImageID2, colmap::FeatureMatches& Matches_, QSqlDatabase& db);
	static void GetAllMatches(std::unordered_map<size_t, std::unordered_map<size_t, colmap::FeatureMatches>>& Matches, QSqlDatabase& db);
	static void AddMatches(size_t ImageID1, size_t ImageID2, colmap::FeatureMatches& Matches_, QSqlDatabase& db);
	static size_t MatchedImagesNum(size_t ImageID, QSqlDatabase& db);

	static size_t GetTwoViewGeometriesNum(size_t ImageID1, size_t ImageID2, QSqlDatabase& db);
	static void GetTwoViewGeometries(size_t ImageID1, size_t ImageID2, colmap::TwoViewGeometry& TwoViewGeometry, QSqlDatabase& db);
	static void GetAllTwoViewGeometries(std::unordered_map<size_t, std::unordered_map<size_t, colmap::TwoViewGeometry>>& TwoViewGeometries, QSqlDatabase& db);
	static void AddTwoViewGeometries(size_t ImageID1, size_t ImageID2, colmap::TwoViewGeometry& TwoViewGeometry, QSqlDatabase& db);
	static size_t ExistTwoViewGeometriesImagesNum(size_t ImageID, QSqlDatabase& db);

	static void ClearData(QSqlDatabase& db);
	static void ExportDatabaseCache(colmap::OptionManager* options, colmap::DatabaseCache& DbCache, QSqlDatabase& db);

private:

	static std::mutex ImageID_Mutex;

	/*static inline QSqlDatabase Open()
	{
		DebugTimer timer(__FUNCTION__);
		if (DatabasePath == "")
		{
			ThrowError("No database path has been specified yet!");
		}
		QSqlDatabase db;
		QString TargetConnectName = "Connection" + QString::number((quintptr)QThread::currentThreadId());
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
		return db;
	}*/
	/*static inline void Close(QSqlDatabase* db, QSqlQuery* query)
	{
		QString connectionName = db->connectionName();
		if (query)
		{
			query->finish();
			query->clear();
		}
		db->close();
		QSqlDatabase::removeDatabase(connectionName);
	}*/
	static inline QByteArray WriteCameraParams(std::vector<double> Params)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray byteArray;
		QDataStream stream(&byteArray, QIODevice::WriteOnly);
		for (double param : Params)
		{
			stream << param;
		}
		return byteArray;
	}
	static inline std::vector<double> ReadCameraParams(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		std::vector<double> params;
		double param;
		QDataStream stream(&Data, QIODevice::ReadOnly);
		while (!stream.atEnd()) 
		{
			stream >> param;
			params.push_back(param);
		}
		return params;
	}
	static inline QByteArray WriteDouble(double num)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray byteArray;
		QDataStream ds(&byteArray, QIODevice::WriteOnly);
		ds << num;
		return byteArray;
	}
	static inline double ReadDouble(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		QDataStream ds(&Data, QIODevice::ReadOnly);
		double num;
		ds >> num;
		return num;
	}
	static inline QByteArray WriteKeyPoints(colmap::FeatureKeypoints& Keypoints)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray byteArray;
		QDataStream ds(&byteArray, QIODevice::WriteOnly);
		for (const colmap::FeatureKeypoint& keypoint : Keypoints)
		{
			ds << keypoint;
		}
		return byteArray;
	}
	static inline colmap::FeatureKeypoints ReadKeyPoints(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		QDataStream ds(&Data, QIODevice::ReadOnly);
		colmap::FeatureKeypoints keypoints;
		while (!ds.atEnd()) 
		{
			colmap::FeatureKeypoint keypoint;
			ds >> keypoint;
			keypoints.push_back(keypoint);
		}
		return keypoints;
	}
	static inline QByteArray WriteDescriptors(colmap::FeatureDescriptors& Descriptors)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray data;
		QDataStream ds(&data, QIODevice::WriteOnly);
		ds << static_cast<qint32>(Descriptors.rows());
		ds << static_cast<qint32>(Descriptors.cols());
		ds.writeRawData(reinterpret_cast<const char*>(Descriptors.data()), Descriptors.size() * sizeof(uint8_t));
		return data;
	}
	static inline colmap::FeatureDescriptors ReadDescriptors(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		QDataStream ds(Data);
		qint32 rows, cols;
		ds >> rows;
		ds >> cols;
		colmap::FeatureDescriptors matrix(rows, cols);
		ds.readRawData(reinterpret_cast<char*>(matrix.data()), matrix.size() * sizeof(uint8_t));
		return matrix;
	}
	static inline QByteArray WriteMatches(colmap::FeatureMatches& Matches)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray data;
		QDataStream ds(&data, QIODevice::WriteOnly);
		ds << static_cast<qint32>(Matches.size());
		for (const auto& match : Matches)
		{
			ds << static_cast<quint32>(match.point2D_idx1);
			ds << static_cast<quint32>(match.point2D_idx2);
		}
		return data;
	}
	static inline colmap::FeatureMatches ReadMatches(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		QDataStream ds(Data);
		qint32 size;
		ds >> size;
		colmap::FeatureMatches matches(size);
		for (auto& match : matches) 
		{
			quint32 point2D_idx1, point2D_idx2;
			ds >> point2D_idx1;
			ds >> point2D_idx2;
			match.point2D_idx1 = point2D_idx1;
			match.point2D_idx2 = point2D_idx2;
		}
		return matches;
	}
	static inline QByteArray WriteTwoViewGeometriesData(colmap::TwoViewGeometry& TwoViewGeometry)
	{
		DebugTimer timer(__FUNCTION__);
		QByteArray byteArray;
		QDataStream stream(&byteArray, QIODevice::WriteOnly);
		stream << TwoViewGeometry.tri_angle;

		stream.writeRawData(reinterpret_cast<const char*>(TwoViewGeometry.E.data()), 9 * sizeof(double));
		stream.writeRawData(reinterpret_cast<const char*>(TwoViewGeometry.F.data()), 9 * sizeof(double));
		stream.writeRawData(reinterpret_cast<const char*>(TwoViewGeometry.H.data()), 9 * sizeof(double));

		stream << TwoViewGeometry.qvec(0);
		stream << TwoViewGeometry.qvec(1);
		stream << TwoViewGeometry.qvec(2);
		stream << TwoViewGeometry.qvec(3);

		stream << TwoViewGeometry.tvec(0);
		stream << TwoViewGeometry.tvec(1);
		stream << TwoViewGeometry.tvec(2);

		stream << static_cast<qint32>(TwoViewGeometry.inlier_matches.size());
		for (const auto& match : TwoViewGeometry.inlier_matches)
		{
			stream << static_cast<quint32>(match.point2D_idx1);
			stream << static_cast<quint32>(match.point2D_idx2);
		}
		return byteArray;
	}
	static inline colmap::TwoViewGeometry ReadTwoViewGeometriesData(QByteArray Data)
	{
		DebugTimer timer(__FUNCTION__);
		colmap::TwoViewGeometry TwoViewGeometry;
		QDataStream stream(Data);
		stream >> TwoViewGeometry.tri_angle;

		stream.readRawData(reinterpret_cast<char*>(TwoViewGeometry.E.data()), 9 * sizeof(double));
		stream.readRawData(reinterpret_cast<char*>(TwoViewGeometry.F.data()), 9 * sizeof(double));
		stream.readRawData(reinterpret_cast<char*>(TwoViewGeometry.H.data()), 9 * sizeof(double));

		stream >> TwoViewGeometry.qvec(0);
		stream >> TwoViewGeometry.qvec(1);
		stream >> TwoViewGeometry.qvec(2);
		stream >> TwoViewGeometry.qvec(3);

		stream >> TwoViewGeometry.tvec(0);
		stream >> TwoViewGeometry.tvec(1);
		stream >> TwoViewGeometry.tvec(2);

		qint32 size;
		stream >> size;
		TwoViewGeometry.inlier_matches.resize(size);
		for (auto& match : TwoViewGeometry.inlier_matches)
		{
			quint32 point2D_idx1, point2D_idx2;
			stream >> point2D_idx1;
			stream >> point2D_idx2;
			match.point2D_idx1 = point2D_idx1;
			match.point2D_idx2 = point2D_idx2;
		}
		return TwoViewGeometry;
	}

	static void ExportMatchedDatabase(std::string DatabasePath, QSqlDatabase& db);


};


