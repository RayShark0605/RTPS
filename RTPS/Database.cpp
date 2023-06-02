#include "Database.h"

using namespace std;
using namespace colmap;

string CDatabase::DatabasePath = "";
mutex CDatabase::ImageID_Mutex;

QDataStream& operator<<(QDataStream& out, const FeatureKeypoint& keypoint)
{
	out << keypoint.x << keypoint.y << keypoint.a11 << keypoint.a12 << keypoint.a21 << keypoint.a22;
	return out;
}
QDataStream& operator>>(QDataStream& in, FeatureKeypoint& keypoint)
{
	in >> keypoint.x >> keypoint.y >> keypoint.a11 >> keypoint.a12 >> keypoint.a21 >> keypoint.a22;
	return in;
}

bool CDatabase::NewDatabase(string DatabasePath, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    /*if (ExistsFile(DatabasePath))
    {
        ThrowError("This database already exists!");
    }*/
    /*QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "NewConnection");
    db.setDatabaseName(StdString2QString(DatabasePath));
    if (!db.open())
    {
        ThrowError("Error: Unable to open database!");
        return false;
    }*/
    QSqlQuery query(db);
    // 创建Cameras表
    query.exec("CREATE TABLE IF NOT EXISTS Cameras (camera_id INTEGER PRIMARY KEY, model INTEGER, width INTEGER, height INTEGER, params BLOB, prior_focal_length INTEGER)");

    // 创建Images表
    query.exec("CREATE TABLE IF NOT EXISTS Images (image_id INTEGER PRIMARY KEY, name TEXT, camera_id INTEGER, prior_qw BLOB, prior_qx BLOB, prior_qy BLOB, prior_qz BLOB, prior_tx BLOB, prior_ty BLOB, prior_tz BLOB, FOREIGN KEY(camera_id) REFERENCES Cameras(camera_id))");

    // 创建Keypoints和Descriptors表
    query.exec("CREATE TABLE IF NOT EXISTS Keypoints (image_id INTEGER PRIMARY KEY, nums INTEGER, data BLOB, FOREIGN KEY(image_id) REFERENCES Images(image_id))");
    query.exec("CREATE TABLE IF NOT EXISTS Descriptors (image_id INTEGER PRIMARY KEY, nums INTEGER, data BLOB, FOREIGN KEY(image_id) REFERENCES Images(image_id))");

    // 创建Matches表
    query.exec("CREATE TABLE IF NOT EXISTS Matches (image_id1 INTEGER, image_id2 INTEGER, nums INTEGER, data BLOB, PRIMARY KEY(image_id1, image_id2), FOREIGN KEY(image_id1) REFERENCES Images(image_id), FOREIGN KEY(image_id2) REFERENCES Images(image_id), CHECK(image_id1 > image_id2))");

    // 创建TwoViewGeometries表
    query.exec("CREATE TABLE IF NOT EXISTS TwoViewGeometries (image_id1 INTEGER, image_id2 INTEGER, nums INTEGER, data BLOB, config INTEGER, PRIMARY KEY(image_id1, image_id2), FOREIGN KEY(image_id1) REFERENCES Images(image_id), FOREIGN KEY(image_id2) REFERENCES Images(image_id), CHECK(image_id1 > image_id2))");

    CDatabase::DatabasePath = DatabasePath;
    return true;
}
bool CDatabase::OpenDatabase(string DatabasePath, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!ExistsFile(DatabasePath))
    {
        ThrowError("This database does not exist!");
    }
    /*QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "OpenConnection");
    db.setDatabaseName(StdString2QString(DatabasePath));
    if (!db.open())
    {
        ThrowError("Error: Unable to open database!");
        return false;
    }*/
    QStringList requiredTables = { "Cameras", "Images", "Keypoints", "Descriptors", "Matches", "TwoViewGeometries" };

    QMap<QString, QMap<QString, QVariant::Type>> requiredFields;
    requiredFields["Cameras"] = {
                {"camera_id", QVariant::Int},
                {"model", QVariant::Int},
                {"width", QVariant::Int},
                {"height", QVariant::Int},
                {"params", QVariant::ByteArray},
                {"prior_focal_length", QVariant::Int}
    };
    requiredFields["Images"] = {
                {"image_id", QVariant::Int},
                {"name", QVariant::String},
                {"camera_id", QVariant::Int},
                {"prior_qw", QVariant::ByteArray},
                {"prior_qx", QVariant::ByteArray},
                {"prior_qy", QVariant::ByteArray},
                {"prior_qz", QVariant::ByteArray},
                {"prior_tx", QVariant::ByteArray},
                {"prior_ty", QVariant::ByteArray},
                {"prior_tz", QVariant::ByteArray}
    };
    requiredFields["Keypoints"] = {
                {"image_id", QVariant::Int},
                {"nums", QVariant::Int},
                {"data", QVariant::ByteArray}
    };
    requiredFields["Descriptors"] = {
                {"image_id", QVariant::Int},
                {"nums", QVariant::Int},
                {"data", QVariant::ByteArray}
    };
    requiredFields["Matches"] = {
                {"image_id1", QVariant::Int},
                {"image_id2", QVariant::Int},
                {"nums", QVariant::Int},
                {"data", QVariant::ByteArray}
    };
    requiredFields["TwoViewGeometries"] = {
                {"image_id1", QVariant::Int},
                {"image_id2", QVariant::Int},
                {"nums", QVariant::Int},
                {"data", QVariant::ByteArray},
                {"config", QVariant::Int}
    };

    for (const QString& table : requiredTables)
    {
        if (!db.tables().contains(table))
        {
            QMessageBox::critical(nullptr, tr("Database check error"), tr("Table ") + table + tr(" does not exist!"));
            return false;
        }
        QSqlRecord record = db.record(table);
        for (auto it = requiredFields[table].begin(); it != requiredFields[table].end(); it++)
        {
            if (!record.contains(it.key()) || record.field(it.key()).type() != it.value()) 
            {
                QMessageBox::critical(nullptr, tr("Database check error"), tr("The field name or the data type of the field is incorrect!"));
                return false;
            }
        }
    }
    CDatabase::DatabasePath = DatabasePath;
    return true;
}

bool CDatabase::IsExistCamera(size_t cameraID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Cameras WHERE camera_id = :camera_id");
    query.bindValue(":camera_id", cameraID);
    if (!query.exec())
    {
        qDebug() << "[IsExistCamera] Unable to execute query -" << query.lastError() << ", cameraID=" << cameraID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("IsExistCamera");
    }
    bool re = query.next();
    return re;
}
bool CDatabase::IsExistCamera(Camera& camera, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Cameras WHERE width = :width AND height = :height AND model = :model");
    query.bindValue(":width", camera.Width());
    query.bindValue(":height", camera.Height());
    query.bindValue(":model", camera.ModelId());
    if (!query.exec())
    {
        qDebug() << "[IsExistCamera] Unable to execute query -" << query.lastError() << ", cameraID=" << camera.CameraId();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("IsExistCamera");
    }
    if (!query.next() || ReadCameraParams(query.value("params").toByteArray()) != camera.Params())
    {
        return false;
    }
    return true;
}
size_t CDatabase::GetCameraID(Camera& camera, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Cameras WHERE width = :width AND height = :height AND model = :model");
    query.bindValue(":width", camera.Width());
    query.bindValue(":height", camera.Height());
    query.bindValue(":model", camera.ModelId());
    if (!query.exec())
    {
        qDebug() << "[GetCameraID] Unable to execute query -" << query.lastError() << ", cameraID=" << camera.CameraId();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetCameraID");
    }
    if (!query.next() || ReadCameraParams(query.value("params").toByteArray()) != camera.Params())
    {
        ThrowError("This camera does not exists!");
    }
    size_t re = query.value("camera_id").toInt();
    return re;
}
size_t CDatabase::GetCamerasNum(QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM Cameras");
    if (!query.exec())
    {
        qDebug() << "[GetCamerasNum] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetCamerasNum");
    }
    query.next();
    size_t re = query.value(0).toInt();
    return re;
}
void CDatabase::GetCamera(size_t cameraID, Camera& camera, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Cameras WHERE camera_id = :camera_id");
    query.bindValue(":camera_id", cameraID);
    if (!query.exec())
    {
        qDebug() << "[GetCamera] Unable to execute query -" << query.lastError() << ", cameraID=" << cameraID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetCamera");
    }

    if (!query.next())
    {
        ThrowError("This camera does not exists!");
    }
    camera.SetCameraId(query.value("camera_id").toInt());
    camera.SetModelId(query.value("model").toInt());
    camera.SetWidth(query.value("width").toInt());
    camera.SetHeight(query.value("height").toInt());
    vector<double> params = ReadCameraParams(query.value("params").toByteArray());
    camera.SetParams(params);
    camera.SetPriorFocalLength((query.value("prior_focal_length").toInt() == 1));
}
void CDatabase::GetAllCameras(vector<Camera>& cameras, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Cameras");
    if (!query.exec())
    {
        qDebug() << "[GetAllCameras] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetAllCameras");
    }
    while (query.next())
    {
        Camera camera;
        camera.SetCameraId(query.value("camera_id").toInt());
        camera.SetModelId(query.value("model").toInt());
        camera.SetWidth(query.value("width").toInt());
        camera.SetHeight(query.value("height").toInt());
        vector<double> params = ReadCameraParams(query.value("params").toByteArray());
        camera.SetParams(params);
        camera.SetPriorFocalLength((query.value("prior_focal_length").toInt() == 1));
        cameras.push_back(camera);
    }
}
size_t CDatabase::AddCamera(Camera& camera, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    size_t NewCameraID = GetCamerasNum(db);
    QSqlQuery query(db);
    query.prepare("INSERT INTO Cameras (camera_id, model, width, height, params, prior_focal_length) "
        "VALUES (:camera_id, :model, :width, :height, :params, :prior_focal_length)");
    query.bindValue(":camera_id", NewCameraID);
    query.bindValue(":model", camera.ModelId());
    query.bindValue(":width", camera.Width());
    query.bindValue(":height", camera.Height());
    query.bindValue(":params", WriteCameraParams(camera.Params()));
    query.bindValue(":prior_focal_length", camera.HasPriorFocalLength());
    if (!query.exec())
    {
        qDebug() << "[AddCamera] Unable to execute query -" << query.lastError() << ", CameraID=" << camera.CameraId();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("AddCamera");
    }
    return NewCameraID;
}

size_t CDatabase::GetImageID(string ImgName, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images WHERE name = :name");
    query.bindValue(":name", StdString2QString(ImgName));
    if (!query.exec())
    {
        qDebug() << "[GetImageID] Unable to execute query -" << query.lastError() << ", ImgName=" << StdString2QString(ImgName);
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImageID");
    }
    if (!query.next())
    {
        ThrowError("This image does not exists!");
    }
    size_t re = query.value("image_id").toInt();
    return re;
}
string CDatabase::GetImageName(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[IsExistImage] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("IsExistImage");
    }
    if (!query.next())
    {
        ThrowError("This image does not exists!");
    }
    string re = QString2StdString(query.value("name").toString());
    return re;
}
bool CDatabase::IsExistImage(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[IsExistImage] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("IsExistImage");
    }
    bool re = query.next();
    return re;
}
bool CDatabase::IsExistImage(string ImgName, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images WHERE name = :name");
    query.bindValue(":name", QVariant(StdString2QString(ImgName)));
    if (!query.exec())
    {
        qDebug() << "[IsExistImage] Unable to execute query -" << query.lastError() << ", ImgName=" << StdString2QString(ImgName);
        qDebug() << query.executedQuery();
        qDebug() << query.boundValues();
        ThrowError("IsExistImage");
    }
    bool re = query.next();
    return re;
}
void CDatabase::GetImageCamera(size_t ImageID, Camera& camera, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT Cameras.* FROM Cameras INNER JOIN Images ON Cameras.camera_id = Images.camera_id WHERE Images.image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetImageCamera] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImageCamera");
    }
    if (!query.next())
    {
        ThrowError("This image does not exists!");
    }
    camera.SetCameraId(query.value("camera_id").toInt());
    camera.SetModelId(query.value("model").toInt());
    camera.SetWidth(query.value("width").toInt());
    camera.SetHeight(query.value("height").toInt());
    vector<double> params = ReadCameraParams(query.value("params").toByteArray());
    camera.SetParams(params);
    camera.SetPriorFocalLength((query.value("prior_focal_length").toInt() == 1));
}
size_t CDatabase::GetImagesNum(QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM Images");
    if (!query.exec())
    {
        qDebug() << "[GetImagesNum] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImagesNum");
    }
    query.next();
    size_t re = query.value(0).toInt();
    return re;
}
void CDatabase::GetImage(size_t ImageID, Image& Image, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetImage] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImage");
    }
    if (!query.next())
    {
        ThrowError("This image does not exists!");
    }
    Image.SetCameraId(query.value("camera_id").toInt());
    Image.SetImageId(query.value("image_id").toInt());
    Image.SetName(QString2StdString(query.value("name").toString()));
    Image.QvecPrior(0) = ReadDouble(query.value("prior_qw").toByteArray());
    Image.QvecPrior(1) = ReadDouble(query.value("prior_qx").toByteArray());
    Image.QvecPrior(2) = ReadDouble(query.value("prior_qy").toByteArray());
    Image.QvecPrior(3) = ReadDouble(query.value("prior_qz").toByteArray());
    Image.TvecPrior(0) = ReadDouble(query.value("prior_tx").toByteArray());
    Image.TvecPrior(1) = ReadDouble(query.value("prior_ty").toByteArray());
    Image.TvecPrior(2) = ReadDouble(query.value("prior_tz").toByteArray());
}
void CDatabase::GetAllImages(vector<Image>& Images_, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images");
    if (!query.exec())
    {
        qDebug() << "[GetAllImages] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetAllImages");
    }
    while (query.next())
    {
        Image image;
        image.SetCameraId(query.value("camera_id").toInt());
        image.SetImageId(query.value("image_id").toInt());
        image.SetName(QString2StdString(query.value("name").toString()));
        image.QvecPrior(0) = ReadDouble(query.value("prior_qw").toByteArray());
        image.QvecPrior(1) = ReadDouble(query.value("prior_qx").toByteArray());
        image.QvecPrior(2) = ReadDouble(query.value("prior_qy").toByteArray());
        image.QvecPrior(3) = ReadDouble(query.value("prior_qz").toByteArray());
        image.TvecPrior(0) = ReadDouble(query.value("prior_tx").toByteArray());
        image.TvecPrior(1) = ReadDouble(query.value("prior_ty").toByteArray());
        image.TvecPrior(2) = ReadDouble(query.value("prior_tz").toByteArray());
        Images_.push_back(image);
    }
}
void CDatabase::GetAllImageNames(vector<string>& ImageNames, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Images");
    if (!query.exec())
    {
        qDebug() << "[GetAllImageNames] Unable to execute query -" << query.lastError(); 
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetAllImageNames");
    }
    while (query.next())
    {
        ImageNames.push_back(QString2StdString(query.value("name").toString()));
    }
}
size_t CDatabase::AddImage(Image& Image, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistCamera(Image.CameraId(), db))
    {
        ThrowError("The camera ID for the current image does not exist!!");
    }
    ImageID_Mutex.lock();
    size_t NewImageID = GetImagesNum(db);
    QSqlQuery query(db);
    query.prepare("INSERT INTO Images (image_id, name, camera_id, prior_qw, prior_qx, prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) "
        "VALUES (:image_id, :name, :camera_id, :prior_qw, :prior_qx, :prior_qy, :prior_qz, :prior_tx, :prior_ty, :prior_tz)");
    query.bindValue(":image_id", NewImageID);
    query.bindValue(":name", StdString2QString(Image.Name()));
    query.bindValue(":camera_id", Image.CameraId());
    query.bindValue(":prior_qw", WriteDouble(Image.QvecPrior(0)));
    query.bindValue(":prior_qx", WriteDouble(Image.QvecPrior(1)));
    query.bindValue(":prior_qy", WriteDouble(Image.QvecPrior(2)));
    query.bindValue(":prior_qz", WriteDouble(Image.QvecPrior(3)));
    query.bindValue(":prior_tx", WriteDouble(Image.TvecPrior(0)));
    query.bindValue(":prior_ty", WriteDouble(Image.TvecPrior(1)));
    query.bindValue(":prior_tz", WriteDouble(Image.TvecPrior(2)));
    if (!query.exec())
    {
        qDebug() << "[AddImage] Unable to execute query -" << query.lastError() << ", ImageName=" << StdString2QString(Image.Name());
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("AddImage");
    }
    ImageID_Mutex.unlock();
    return NewImageID;
}

size_t CDatabase::GetImageKeypointsNum(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Keypoints WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetImageKeypointsNum] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImageKeypointsNum");
    }
    if (!query.next())
    {
        return 0;
    }
    size_t re = query.value("nums").toInt();
    return re;
}
void CDatabase::GetKeypoints(size_t ImageID, FeatureKeypoints& Keypoints, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Keypoints WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetKeypoints] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetKeypoints");
    }
    if (!query.next())
    {
        return;
    }
    Keypoints = ReadKeyPoints(query.value("data").toByteArray());
}
void CDatabase::AddKeypoints(size_t ImageID, FeatureKeypoints& Keypoints, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    if (Keypoints.empty())return;
    QSqlQuery query(db);
    query.prepare("INSERT INTO Keypoints (image_id, nums, data) "
        "VALUES (:image_id, :nums, :data)");
    query.bindValue(":image_id", ImageID);
    query.bindValue(":nums", Keypoints.size());
    query.bindValue(":data", WriteKeyPoints(Keypoints));
    if (!query.exec())
    {
        qDebug() << "[AddKeypoints] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("AddKeypoints");
    }
}
size_t CDatabase::GetImageDescriptorsNum(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }

    QSqlQuery query(db);
    query.prepare("SELECT * FROM Descriptors WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetImageDescriptorsNum] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetImageDescriptorsNum");
    }
    if (!query.next())
    {
        return 0;
    }
    size_t re = query.value("nums").toInt();
    return re;
}
void CDatabase::GetDescriptors(size_t ImageID, FeatureDescriptors& Descriptors, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Descriptors WHERE image_id = :image_id");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[GetDescriptors] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetDescriptors");
    }
    if (!query.next())
    {
        return;
    }
    Descriptors = ReadDescriptors(query.value("data").toByteArray());
}
void CDatabase::AddDescriptors(size_t ImageID, FeatureDescriptors& Descriptors, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    if (Descriptors.rows() == 0)return;
    QSqlQuery query(db);
    query.prepare("INSERT INTO Descriptors (image_id, nums, data) "
        "VALUES (:image_id, :nums, :data)");
    query.bindValue(":image_id", ImageID);
    query.bindValue(":nums", Descriptors.rows());
    query.bindValue(":data", WriteDescriptors(Descriptors));
    if (!query.exec())
    {
        qDebug() << "[AddDescriptors] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        ThrowError("AddDescriptors");
    }
}

size_t CDatabase::GetMatchesNum(size_t ImageID1, size_t ImageID2, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1, db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        return GetMatchesNum(ImageID2, ImageID1, db);
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Matches WHERE image_id1 = :image_id1 AND image_id2 = :image_id2");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    if (!query.exec())
    {
        qDebug() << "[GetMatchesNum] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        ThrowError("GetMatchesNum");
    }
    if (!query.next())
    {
        return 0;
    }
    size_t re = query.value("nums").toInt();
    return re;
}
void CDatabase::GetMatches(size_t ImageID1, size_t ImageID2, FeatureMatches& Matches_, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1, db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        GetMatches(ImageID2, ImageID1, Matches_, db);
        return;
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Matches WHERE image_id1 = :image_id1 AND image_id2 = :image_id2");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    if (!query.exec())
    {
        qDebug() << "[GetMatches] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetMatches");
    }
    if (!query.next())
    {
        return;
    }
    Matches_ = ReadMatches(query.value("data").toByteArray());
}
void CDatabase::GetAllMatches(unordered_map<size_t, unordered_map<size_t, FeatureMatches>>& Matches, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM Matches");
    if (!query.exec())
    {
        qDebug() << "[GetAllMatches] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetAllMatches");
    }
    while (query.next())
    {
        size_t ImageID1 = query.value("image_id1").toInt();
        size_t ImageID2 = query.value("image_id2").toInt();
        Matches[ImageID1][ImageID2] = ReadMatches(query.value("data").toByteArray());
    }
}
void CDatabase::AddMatches(size_t ImageID1, size_t ImageID2, FeatureMatches& Matches_, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1, db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        AddMatches(ImageID2, ImageID1, Matches_, db);
        return;
    }
    if (Matches_.empty())return;
    QSqlQuery query(db);
    query.prepare("INSERT INTO Matches (image_id1, image_id2, nums, data) "
        "VALUES (:image_id1, :image_id2, :nums, :data)");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    query.bindValue(":nums", Matches_.size());
    query.bindValue(":data", WriteMatches(Matches_));
    if (!query.exec())
    {
        qDebug() << "[AddMatches] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        //ThrowError("AddMatches");
    }
}
size_t CDatabase::MatchedImagesNum(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM Matches WHERE (image_id1 = :image_id OR image_id2 = :image_id) AND nums != 0");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[MatchedImagesNum] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("MatchedImagesNum");
    }
    query.next();
    size_t re = query.value(0).toInt();
    return re;
}

size_t CDatabase::GetTwoViewGeometriesNum(size_t ImageID1, size_t ImageID2, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1, db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        return GetTwoViewGeometriesNum(ImageID2, ImageID1, db);
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM TwoViewGeometries WHERE image_id1 = :image_id1 AND image_id2 = :image_id2");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    if (!query.exec())
    {
        qDebug() << "[GetTwoViewGeometriesNum] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetTwoViewGeometriesNum");
    }
    if (!query.next())
    {
        return 0;
    }
    size_t re = query.value("nums").toInt();
    return re;
}
void CDatabase::GetTwoViewGeometries(size_t ImageID1, size_t ImageID2, TwoViewGeometry& TwoViewGeometry, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1, db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        GetTwoViewGeometries(ImageID2, ImageID1, TwoViewGeometry, db);
        return;
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM TwoViewGeometries WHERE image_id1 = :image_id1 AND image_id2 = :image_id2");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    if (!query.exec())
    {
        qDebug() << "[GetTwoViewGeometries] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetTwoViewGeometries");
    }
    if (!query.next())
    {
        return;
    }
    TwoViewGeometry = ReadTwoViewGeometriesData(query.value("data").toByteArray());
    TwoViewGeometry.config = query.value("config").toInt();
}
void CDatabase::GetAllTwoViewGeometries(unordered_map<size_t, unordered_map<size_t, TwoViewGeometry>>& TwoViewGeometries, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.prepare("SELECT * FROM TwoViewGeometries");
    if (!query.exec())
    {
        qDebug() << "[GetAllTwoViewGeometries] Unable to execute query -" << query.lastError();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("GetAllTwoViewGeometries");
    }
    while (query.next())
    {
        size_t ImageID1 = query.value("image_id1").toInt();
        size_t ImageID2 = query.value("image_id2").toInt();
        TwoViewGeometries[ImageID1][ImageID2] = ReadTwoViewGeometriesData(query.value("data").toByteArray());
        TwoViewGeometries[ImageID1][ImageID2].config = query.value("config").toInt();
    }
}
void CDatabase::AddTwoViewGeometries(size_t ImageID1, size_t ImageID2, TwoViewGeometry& TwoViewGeometry, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID1,db) || !IsExistImage(ImageID2, db))
    {
        ThrowError("Image 1 or Image 2 does not exist!");
    }
    if (ImageID1 == ImageID2)
    {
        ThrowError("ImageID1 and ImageID2 must not be the same!");
    }
    if (ImageID1 < ImageID2)
    {
        AddTwoViewGeometries(ImageID2, ImageID1, TwoViewGeometry, db);
        return;
    }
    if (TwoViewGeometry.inlier_matches.empty())return;
    QSqlQuery query(db);
    query.prepare("INSERT INTO TwoViewGeometries (image_id1, image_id2, nums, data, config) "
        "VALUES (:image_id1, :image_id2, :nums, :data, :config)");
    query.bindValue(":image_id1", ImageID1);
    query.bindValue(":image_id2", ImageID2);
    query.bindValue(":nums", TwoViewGeometry.inlier_matches.size());
    query.bindValue(":data", WriteTwoViewGeometriesData(TwoViewGeometry));
    query.bindValue(":config", TwoViewGeometry.config);
    if (!query.exec())
    {
        qDebug() << "[AddTwoViewGeometries] Unable to execute query -" << query.lastError() << ", ImageID1=" << ImageID1 << ", ImageID2=" << ImageID2;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        //ThrowError("AddTwoViewGeometries");
    }
}
size_t CDatabase::ExistTwoViewGeometriesImagesNum(size_t ImageID, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (!IsExistImage(ImageID, db))
    {
        ThrowError("This image does not exists!");
    }
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM TwoViewGeometries WHERE (image_id1 = :image_id OR image_id2 = :image_id) AND nums != 0");
    query.bindValue(":image_id", ImageID);
    if (!query.exec())
    {
        qDebug() << "[ExistTwoViewGeometriesImagesNum] Unable to execute query -" << query.lastError() << ", ImageID=" << ImageID;
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        ThrowError("ExistTwoViewGeometriesImagesNum");
    }
    query.next();
    size_t re = query.value(0).toInt();
    return re;
}

void CDatabase::ClearData(QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    QSqlQuery query(db);
    query.exec("DELETE FROM Cameras");
    query.exec("DELETE FROM Images");
    query.exec("DELETE FROM Keypoints");
    query.exec("DELETE FROM Descriptors");
    query.exec("DELETE FROM Matches");
    query.exec("DELETE FROM TwoViewGeometries");
}
void CDatabase::ExportDatabaseCache(OptionManager* options, DatabaseCache& DbCache, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    string TempDatabasePath = QString2StdString(QCoreApplication::applicationDirPath()) + "/temp.db";
    ExportMatchedDatabase(TempDatabasePath, db);
    Database database(TempDatabasePath);
    DatabaseTransaction Transaction(&database);
    unordered_set<string> image_names;
    DbCache.Load(database, options->mapper->min_num_matches, options->mapper->ignore_watermarks, image_names);
    QFile File(StdString2QString(TempDatabasePath));
    File.remove();
}
void CDatabase::GenerateMatchInfo(QSqlDatabase& db)
{
    Base::MatchMatrix_Mutex.lock();
    unordered_map<size_t, unordered_map<size_t, TwoViewGeometry>> TwoViewGeometries;
    GetAllTwoViewGeometries(TwoViewGeometries, db);
    for (auto& pair1 : TwoViewGeometries)
    {
        size_t ImageID = pair1.first;
        for (auto& pair2 : pair1.second)
        {
            size_t MatchedImageID = pair2.first;
            size_t MatchNum = pair2.second.inlier_matches.size();
            if (ImageID > MatchedImageID)
            {
                Base::MatchMatrix[ImageID][MatchedImageID] = MatchNum;
            }
            else
            {
                Base::MatchMatrix[MatchedImageID][ImageID] = MatchNum;
            }
            Base::MatchedImages[ImageID].insert(MatchedImageID);
            Base::MatchedImages[MatchedImageID].insert(ImageID);
            Base::MaxMatches = max(Base::MaxMatches, MatchNum);
        }
    }
    Base::MatchMatrix_Mutex.unlock();
}

void CDatabase::ExportMatchedDatabase(string DatabasePath, QSqlDatabase& db)
{
    //DebugTimer timer(__FUNCTION__);
    if (ExistsFile(DatabasePath))
    {
        QFile File(StdString2QString(DatabasePath));
        File.remove();
    }

    Database database(DatabasePath);
    DatabaseTransaction Transaction(&database);

    vector<Camera> cameras;
    GetAllCameras(cameras, db);
    for (Camera& camera : cameras)
    {
        database.WriteCamera(camera, true);
    }

    vector<Image> Images;
    unordered_set<size_t> MatchedImages;
    GetAllImages(Images, db);
    for (Image& image : Images)
    {
        size_t ImageID = image.ImageId();
        if (MatchedImagesNum(ImageID, db) != 0)
        {
            MatchedImages.insert(ImageID);
            database.WriteImage(image, true);

            FeatureKeypoints keypoints;
            GetKeypoints(ImageID, keypoints, db);
            database.WriteKeypoints(ImageID, keypoints);

            FeatureDescriptors Descriptors;
            GetDescriptors(ImageID, Descriptors, db);
            database.WriteDescriptors(ImageID, Descriptors);
        }
    }

    unordered_map<size_t, unordered_map<size_t, FeatureMatches>> Matches;
    GetAllMatches(Matches, db);

    unordered_map<size_t, unordered_map<size_t, TwoViewGeometry>> TwoViewGeometries;
    GetAllTwoViewGeometries(TwoViewGeometries, db);

    for (auto it1 = Matches.begin(); it1 != Matches.end(); it1++)
    {
        size_t ImageID1 = it1->first;
        if (!MatchedImages.count(ImageID1))continue;
        for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
        {
            size_t ImageID2 = it2->first;
            if (!MatchedImages.count(ImageID2) || it2->second.empty())continue;
            database.WriteMatches(ImageID1, ImageID2, it2->second);
        }
    }

    for (auto it1 = TwoViewGeometries.begin(); it1 != TwoViewGeometries.end(); it1++)
    {
        size_t ImageID1 = it1->first;
        if (!MatchedImages.count(ImageID1))continue;
        for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
        {
            size_t ImageID2 = it2->first;
            if (!MatchedImages.count(ImageID2) || it2->second.inlier_matches.empty())continue;
            database.WriteTwoViewGeometry(ImageID1, ImageID2, it2->second);
        }
    }
}


