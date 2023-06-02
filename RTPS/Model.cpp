#include "Model.h"

using namespace std;
using namespace colmap;

CModel::CModel()
{
	CorresGraph = nullptr;
	AddedPoints3DsNum = 0;
}
CModel::CModel(const CModel& Model)
{
	lock(Cameras_Mutex, Images_Mutex, Points3D_Mutex, ImagePairs_Mutex, RegImageIDs_Mutex, AddedPoints3DsNum_Mutex);
	CorresGraph = Model.CorresGraph;
	Model.GetModelAllCameras(Cameras);
	Model.GetModelAllImages(Images);
	Model.GetModelAllPoint3Ds(Points3D);
	Model.GetModelAllImagePairs(ImagePairs);
	Model.GetModelAllRegImageIDs(RegImageIDs);
	AddedPoints3DsNum = Model.AddedPoints3DsNum;

	Cameras_Mutex.unlock();
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
	ImagePairs_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
	AddedPoints3DsNum_Mutex.unlock();
}

size_t CModel::GetModelCamerasNum()
{
	Cameras_Mutex.lock();
	size_t re = Cameras.size();
	Cameras_Mutex.unlock();
	return re;
}
size_t CModel::GetModelImagesNum()
{
	Images_Mutex.lock();
	size_t re = Images.size();
	Images_Mutex.unlock();
	return re;
}
size_t CModel::GetModelRegImagesNum()
{
	RegImageIDs_Mutex.lock();
	size_t re = RegImageIDs.size();
	RegImageIDs_Mutex.unlock();
	return re;
}
size_t CModel::GetModelPoints3DNum()
{
	Points3D_Mutex.lock();
	size_t re = Points3D.size();
	Points3D_Mutex.unlock();
	return re;
}
size_t CModel::GetModelImagePairsNum()
{
	ImagePairs_Mutex.lock();
	size_t re = ImagePairs.size();
	ImagePairs_Mutex.unlock();
	return re;
}
void CModel::ConvertToReconstruction(Reconstruction& Model)
{
	DebugTimer timer(__FUNCTION__);
	lock(Cameras_Mutex, Images_Mutex, Points3D_Mutex, ImagePairs_Mutex, RegImageIDs_Mutex, AddedPoints3DsNum_Mutex);
	Model = Reconstruction();
	Model.cameras_.reserve(Cameras.size());
	for (auto& pair : Cameras)
	{
		Model.cameras_.emplace(pair.first, pair.second);
	}
	Model.images_.reserve(Images.size());
	for (auto& pair : Images)
	{
		Model.images_.emplace(pair.first, pair.second);
	}
	Model.points3D_.reserve(Points3D.size());
	for (auto& pair : Points3D)
	{
		Model.points3D_.emplace(pair.first, pair.second);
	}
	Model.image_pair_stats_.reserve(ImagePairs.size());
	for (auto& pair : ImagePairs)
	{
		size_t ImageID1 = pair.first.first;
		size_t ImageID2 = pair.first.second;
		Reconstruction::ImagePairStat Status;
		Status.num_total_corrs = pair.second.TotalCorrsNum;
		Status.num_tri_corrs = pair.second.TriangulatedCorrsNum;
		Model.image_pair_stats_.emplace(Database::ImagePairToPairId(ImageID1, ImageID2), Status);
	}
	Model.reg_image_ids_.reserve(RegImageIDs.size());
	for (size_t RegImageID : RegImageIDs)
	{
		Model.reg_image_ids_.push_back(RegImageID);
	}
	Model.num_added_points3D_ = AddedPoints3DsNum;

	Cameras_Mutex.unlock();
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
	ImagePairs_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
	AddedPoints3DsNum_Mutex.unlock();
}
bool CModel::IsModelExistCamera(size_t CameraID)
{
	Cameras_Mutex.lock();
	bool re = (Cameras.find(CameraID) != Cameras.end());
	Cameras_Mutex.unlock();
	return re;
}
Camera CModel::GetModelCamera(size_t CameraID)
{
	Cameras_Mutex.lock();
	auto it = Cameras.find(CameraID);
	if (it == Cameras.end())
	{
		Cameras_Mutex.unlock();
		ThrowError("This camera does not exists!");
	}
	Camera camera = it->second;
	Cameras_Mutex.unlock();
	return camera;
}
void CModel::GetModelCamera(Camera& camera, size_t CameraID)
{
	Cameras_Mutex.lock();
	auto it = Cameras.find(CameraID);
	if (it == Cameras.end())
	{
		Cameras_Mutex.unlock();
		ThrowError("This camera does not exists!");
	}
	camera = it->second;
	Cameras_Mutex.unlock();
}
void CModel::GetModelAllCameras(CCameraMapType& cameras) const
{
	Cameras_Mutex.lock();
	cameras = Cameras;
	Cameras_Mutex.unlock();
}
void CModel::SetCamera(Camera& camera, size_t CameraID)
{
	Cameras_Mutex.lock();
	auto it = Cameras.find(CameraID);
	if (it == Cameras.end())
	{
		Cameras_Mutex.unlock();
		ThrowError("This camera does not exists!");
	}
	it->second = camera;
	Cameras_Mutex.unlock();
}

void CModel::ModelAddCamera(Camera& camera)
{
	CHECK(camera.VerifyParams());
	size_t CameraID = camera.CameraId();
	Cameras_Mutex.lock();
	CHECK(Cameras.emplace(CameraID, camera).second);
	Cameras_Mutex.unlock();
}

bool CModel::IsModelExistImage(size_t ImageID)
{
	Images_Mutex.lock();
	bool re = (Images.find(ImageID) != Images.end());
	Images_Mutex.unlock();
	return re;
}
bool CModel::IsModelExistImage(string ImageName)
{
	Images_Mutex.lock();
	for (auto it = Images.begin(); it != Images.end(); it++)
	{
		if (it->second.Name() == ImageName)
		{
			Images_Mutex.unlock();
			return true;
		}
	}
	Images_Mutex.unlock();
	return false;
}
Image CModel::GetModelImage(size_t ImageID)
{
	Images_Mutex.lock();
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	Image image = it->second;
	Images_Mutex.unlock();
	return image;
}
void CModel::GetModelImage(Image& image, size_t ImageID)
{
	Images_Mutex.lock();
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	image = it->second;
	Images_Mutex.unlock();
}
void CModel::GetModelImage(Image& image, string ImageName)
{
	Images_Mutex.lock();
	for (auto it = Images.begin(); it != Images.end(); it++)
	{
		if (it->second.Name() == ImageName)
		{
			image = it->second;
			Images_Mutex.unlock();
			return;
		}
	}
	Images_Mutex.unlock();
	ThrowError("This image does not exists!");
}
void CModel::GetModelAllImages(CImageMapType& images) const
{
	Images_Mutex.lock();
	images = Images;
	Images_Mutex.unlock();
}
CModel::CImageMapType CModel::GetModelAllImages()
{
	CModel::CImageMapType re;
	GetModelAllImages(re);
	return re;
}
void CModel::GetModelAllRegImageIDs(vector<size_t>& RegImageIDs) const
{
	RegImageIDs_Mutex.lock();
	RegImageIDs = this->RegImageIDs;
	RegImageIDs_Mutex.unlock();
}
std::vector<size_t> CModel::GetAllRegImagesIDs()
{
	vector<size_t> re;
	GetModelAllRegImageIDs(re);
	return re;
}
void CModel::SetImage(Image& image, size_t ImageID)
{
	Images_Mutex.lock();
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	it->second = image;
	Images_Mutex.unlock();
}
void CModel::ModelAddImage(Image& image)
{
	size_t ImageID = image.ImageId();
	Images_Mutex.lock();
	CHECK(Images.emplace(ImageID, image).second);
	Images_Mutex.unlock();
}
bool CModel::IsImageRegistered(size_t ImageID)
{
	Images_Mutex.lock();
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	bool re = it->second.IsRegistered();
	Images_Mutex.unlock();
	return re;
}
void CModel::RegisterImage(size_t ImageID)
{
	lock(Images_Mutex, RegImageIDs_Mutex);
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		RegImageIDs_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	else if (!it->second.IsRegistered())
	{
		it->second.SetRegistered(true);
		RegImageIDs.push_back(ImageID);
	}
	Images_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
}
void CModel::DeRegisterImage(size_t ImageID)
{
	Images_Mutex.lock();
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	Image image = it->second;
	Images_Mutex.unlock();
	for (size_t Point2DID = 0; Point2DID < image.NumPoints2D(); Point2DID++)
	{
		if (!image.Point2D(Point2DID).HasPoint3D())continue;
		DeleteObservation(ImageID, Point2DID);
	}
	lock(Images_Mutex, RegImageIDs_Mutex);
	Images[ImageID].SetRegistered(false);
	RegImageIDs.erase(remove(RegImageIDs.begin(), RegImageIDs.end(), ImageID), RegImageIDs.end());
	Images_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
}

bool CModel::IsModelExistPoint3D(size_t Point3DID)
{
	Points3D_Mutex.lock();
	bool re = (Points3D.find(Point3DID) != Points3D.end());
	Points3D_Mutex.unlock();
	return re;
}
Point3D CModel::GetModelPoint3D(size_t Point3DID)
{
	Points3D_Mutex.lock();
	auto it = Points3D.find(Point3DID);
	if (it == Points3D.end())
	{
		Points3D_Mutex.unlock();
		ThrowError("This point3D does not exists!");
	}
	Point3D point = it->second;
	Points3D_Mutex.unlock();
	return point;
}
void CModel::GetModelPoint3D(Point3D& point, size_t Point3DID)
{
	Points3D_Mutex.lock();
	auto it = Points3D.find(Point3DID);
	if (it == Points3D.end())
	{
		Points3D_Mutex.unlock();
		ThrowError("This point3D does not exists!");
	}
	point = it->second;
	Points3D_Mutex.unlock();
}
void CModel::GetModelAllPoint3Ds(CPoint3DMapType& points) const
{
	Points3D_Mutex.lock();
	points = Points3D;
	Points3D_Mutex.unlock();
}
unordered_set<size_t> CModel::GetModelAllPoint3Ds()
{
	unordered_set<size_t> re;
	GetModelAllPoint3DIDs(re);
	return re;
}
void CModel::GetModelAllPoint3DIDs(unordered_set<size_t>& pointIDs)
{
	Points3D_Mutex.lock();
	pointIDs.reserve(Points3D.size());
	for (auto it = Points3D.begin(); it != Points3D.end(); it++)
	{
		pointIDs.insert(it->first);
	}
	Points3D_Mutex.unlock();
}
size_t CModel::ModelAddPoint3D(Eigen::Vector3d& xyz, Track& track, const Eigen::Vector3ub& color)
{
	lock(Points3D_Mutex, AddedPoints3DsNum_Mutex, Images_Mutex);
	if (Points3D.find(AddedPoints3DsNum + 1) != Points3D.end())
	{
		Points3D_Mutex.unlock();
		AddedPoints3DsNum_Mutex.unlock();
		Images_Mutex.unlock();
		ThrowError("The next image ID already exists!");
	}
	AddedPoints3DsNum++;
	size_t Point3DID = AddedPoints3DsNum;
	for (TrackElement& element : track.Elements())
	{
		auto it = Images.find(element.image_id);
		if (it == Images.end())
		{
			Points3D_Mutex.unlock();
			AddedPoints3DsNum_Mutex.unlock();
			Images_Mutex.unlock();
			ThrowError("This image does not exists!");
		}
		if (it->second.Point2D(element.point2D_idx).HasPoint3D())
		{
			Points3D_Mutex.unlock();
			AddedPoints3DsNum_Mutex.unlock();
			Images_Mutex.unlock();
			ThrowError("The 2D point of this element of this track already has a corresponding 3D point!");
		}
		it->second.SetPoint3DForPoint2D(element.point2D_idx, Point3DID);
		CHECK_LE(it->second.NumPoints3D(), it->second.NumPoints2D());
	}
	Points3D_Mutex.unlock();
	AddedPoints3DsNum_Mutex.unlock();
	Images_Mutex.unlock();

	for (TrackElement& element : track.Elements())
	{
		SetObservationAsTriangulated(element.image_id, element.point2D_idx, false);
	}

	Points3D_Mutex.lock();
	Points3D[Point3DID].SetXYZ(xyz);
	Points3D[Point3DID].SetTrack(track);
	Points3D[Point3DID].SetColor(color);
	Points3D_Mutex.unlock();

	return Point3DID;
}
size_t CModel::MergePoint3Ds(size_t PointID1, size_t PointID2)
{
	Points3D_Mutex.lock();
	auto it1 = Points3D.find(PointID1), it2 = Points3D.find(PointID2);
	if (it1 == Points3D.end() || it2 == Points3D.end())
	{
		Points3D_Mutex.unlock();
		ThrowError("PointID1 or PointID2 does not exist!");
	}
	Eigen::Vector3d MergedXYZ = (it1->second.Track().Length() * it1->second.XYZ() + it2->second.Track().Length() * it2->second.XYZ()) / (it1->second.Track().Length() + it2->second.Track().Length());
	Eigen::Vector3d MergedRGB = (it1->second.Track().Length() * it1->second.Color().cast<double>() + it2->second.Track().Length() * it2->second.Color().cast<double>()) / (it1->second.Track().Length() + it2->second.Track().Length());
	Track MergedTrack;
	MergedTrack.Reserve(it1->second.Track().Length() + it2->second.Track().Length());
	MergedTrack.AddElements(it1->second.Track().Elements());
	MergedTrack.AddElements(it2->second.Track().Elements());
	Points3D_Mutex.unlock();

	DeletePoint3D(PointID1);
	DeletePoint3D(PointID2);

	Eigen::Vector3ub MergedRGB_ub = MergedRGB.cast<uint8_t>();
	size_t MergedPointID = ModelAddPoint3D(MergedXYZ, MergedTrack, MergedRGB_ub);
	return MergedPointID;
}
void CModel::DeletePoint3D(size_t PointID)
{
	Points3D_Mutex.lock();
	auto it = Points3D.find(PointID);
	if (it == Points3D.end())
	{
		Points3D_Mutex.unlock();
		ThrowError("This point3D does not exists!");
	}
	Track track = it->second.Track();
	Points3D_Mutex.unlock();
	for (TrackElement& element : track.Elements())
	{
		ResetTriObservations(element.image_id, element.point2D_idx, true);
	}
	lock(Images_Mutex, Points3D_Mutex);
	for (TrackElement& element : track.Elements())
	{
		Images[element.image_id].ResetPoint3DForPoint2D(element.point2D_idx);
	}
	Points3D.erase(PointID);
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}

void CModel::AddObservation(size_t Point3DID, TrackElement& element)
{
	lock(Images_Mutex, Points3D_Mutex);
	auto it = Images.find(element.image_id);
	auto it2 = Points3D.find(Point3DID);
	if (it == Images.end() || it2 == Points3D.end())
	{
		Images_Mutex.unlock();
		Points3D_Mutex.unlock();
		ThrowError("Corresponding image or 3D point does not exist!");
	}
	CHECK(!it->second.Point2D(element.point2D_idx).HasPoint3D());
	it->second.SetPoint3DForPoint2D(element.point2D_idx, Point3DID);
	CHECK_LE(it->second.NumPoints3D(), it->second.NumPoints2D());
	it2->second.Track().AddElement(element);
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
	SetObservationAsTriangulated(element.image_id, element.point2D_idx, true);
}
void CModel::DeleteObservation(size_t ImageID, size_t Point2DID)
{
	lock(Images_Mutex, Points3D_Mutex);
	auto it = Images.find(ImageID);
	if (it == Images.end())
	{
		Images_Mutex.unlock();
		Points3D_Mutex.unlock();
		ThrowError("This image does not exists!");
	}
	size_t Point3DID = it->second.Point2D(Point2DID).Point3DId();
	auto it2 = Points3D.find(Point3DID);
	if (it2 == Points3D.end())
	{
		Images_Mutex.unlock();
		Points3D_Mutex.unlock();
		ThrowError("The corresponding 3D point does not exist!");
	}
	if (it2->second.Track().Length() <= 2)
	{
		Images_Mutex.unlock();
		Points3D_Mutex.unlock();
		DeletePoint3D(Point3DID);
		return;
	}
	it2->second.Track().DeleteElement(ImageID, Point2DID);
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
	ResetTriObservations(ImageID, Point2DID, false);
	Images_Mutex.lock();
	Images[ImageID].ResetPoint3DForPoint2D(Point2DID);
	Images_Mutex.unlock();
}

void CModel::DeleteAllPoint3DPoint2D()
{
	lock(Points3D_Mutex, Images_Mutex);
	Points3D.clear();
	for (auto it = Images.begin(); it != Images.end(); it++)
	{
		Image NewImage;
		NewImage.SetImageId(it->second.ImageId());
		NewImage.SetName(it->second.Name());
		NewImage.SetCameraId(it->second.CameraId());
		NewImage.SetRegistered(it->second.IsRegistered());
		NewImage.SetNumCorrespondences(it->second.NumCorrespondences());
		NewImage.SetQvec(it->second.Qvec());
		NewImage.SetQvecPrior(it->second.QvecPrior());
		NewImage.SetTvec(it->second.Tvec());
		NewImage.SetTvecPrior(it->second.TvecPrior());
		it->second = NewImage;
	}
	Points3D_Mutex.unlock();
	Images_Mutex.unlock();
}

bool CModel::IsModelExistImagePair(size_t ImageID1, size_t ImageID2)
{
	if (ImageID1 > ImageID2)return IsModelExistImagePair(ImageID2, ImageID1);
	ImagePairs_Mutex.lock();
	bool re = (ImagePairs.find(make_pair(ImageID1, ImageID2)) != ImagePairs.end());
	ImagePairs_Mutex.unlock();
	return re;
}
unordered_map<image_pair_t, Reconstruction::ImagePairStat> CModel::GetModelImagePairs()
{
	unordered_map<image_pair_t, Reconstruction::ImagePairStat> re;
	CModel::CImagePairMapType AllImagePairs;
	GetModelAllImagePairs(AllImagePairs);
	re.reserve(AllImagePairs.size());
	for (auto& pair : AllImagePairs)
	{
		size_t ImageID1 = pair.first.first;
		size_t ImageID2 = pair.first.second;
		Reconstruction::ImagePairStat status;
		status.num_total_corrs = pair.second.TotalCorrsNum;
		status.num_tri_corrs = pair.second.TriangulatedCorrsNum;
		re[Database::ImagePairToPairId(ImageID1, ImageID2)] = status;
	}
	return re;
}
CModel::CImagePairStatus CModel::GetModelImagePairStatus(size_t ImageID1, size_t ImageID2)
{
	if (ImageID1 > ImageID2)return GetModelImagePairStatus(ImageID2, ImageID1);
	ImagePairs_Mutex.lock();
	auto it = ImagePairs.find(make_pair(ImageID1, ImageID2));
	if (it == ImagePairs.end())
	{
		ImagePairs_Mutex.unlock();
		ThrowError("This image pair does not exists!");
	}
	CModel::CImagePairStatus re = it->second;
	ImagePairs_Mutex.unlock();
	return re;
}
void CModel::GetModelAllImagePairs(CModel::CImagePairMapType& pairs) const
{
	ImagePairs_Mutex.lock();
	pairs = ImagePairs;
	ImagePairs_Mutex.unlock();
}
void CModel::Load(DatabaseCache& DbCache)
{
	CorresGraph = nullptr;

	Cameras_Mutex.lock();
	Cameras.reserve(DbCache.NumCameras());
	Cameras_Mutex.unlock();
	for (auto& pair : DbCache.Cameras())
	{
		if (!IsModelExistCamera(pair.first))
		{
			Camera camera = pair.second;
			ModelAddCamera(camera);
		}
	}

	Images_Mutex.lock();
	Images.reserve(DbCache.NumImages());
	Images_Mutex.unlock();
	unordered_set<size_t> UnMatchedImages;
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	for (auto& pair : DbCache.Images())
	{
		size_t ImageID = pair.second.ImageId();
		if (CDatabase::MatchedImagesNum(ImageID, db) == 0)
		{
			UnMatchedImages.insert(ImageID);
			continue;
		}
		/*if (GetImageStatus(ImageID) != CImageStatus::Matched)
		{
			UnMatchedImages.insert(ImageID);
			continue;
		}*/
		if (IsModelExistImage(ImageID))
		{
			Images_Mutex.lock();
			CHECK_EQ(Images[ImageID].Name(), pair.second.Name());
			if (Images[ImageID].NumPoints2D() == 0)
			{
				Images[ImageID].SetPoints2D(pair.second.Points2D());
			}
			else
			{
				CHECK_EQ(pair.second.NumPoints2D(), Images[ImageID].NumPoints2D());
			}
			Images[ImageID].SetNumObservations(pair.second.NumObservations());
			Images[ImageID].SetNumCorrespondences(pair.second.NumCorrespondences());
			Images_Mutex.unlock();
		}
		else
		{
			Image image = pair.second;
			ModelAddImage(image);
		}
	}
	ReleaseDatabaseConnect(db);

	ImagePairs_Mutex.lock();
	for (auto& pair : DbCache.CorrespondenceGraph().NumCorrespondencesBetweenImages())
	{
		CImagePairStatus Status;
		Status.TotalCorrsNum = pair.second;
		image_t ImageID1, ImageID2;
		Database::PairIdToImagePair(pair.first, &ImageID1, &ImageID2);
		if (UnMatchedImages.count(ImageID1) || UnMatchedImages.count(ImageID2))
		{
			continue;
		}
		if (ImageID1 < ImageID2)
		{
			ImagePairs.emplace(make_pair((size_t)ImageID1, (size_t)ImageID2), Status);
		}
		else
		{
			ImagePairs.emplace(make_pair((size_t)ImageID2, (size_t)ImageID1), Status);
		}
	}
	ImagePairs_Mutex.unlock();

}
void CModel::SetUp(const CorrespondenceGraph* correspondence_graph)
{
	CHECK_NOTNULL(correspondence_graph);
	lock(Images_Mutex, RegImageIDs_Mutex, Cameras_Mutex);

	for (auto& pair : Images)
	{
		pair.second.SetUp(Cameras[pair.second.CameraId()]);
	}
	Cameras_Mutex.unlock();
	CorresGraph = correspondence_graph;
	// If an existing model was loaded from disk and there were already images registered previously, we need to set observations as triangulated.

	for (size_t ImageID : RegImageIDs)
	{
		RegImageIDs_Mutex.unlock();
		for (size_t i = 0; i < Images[ImageID].NumPoints2D(); i++)
		{
			if (!Images[ImageID].Point2D(i).HasPoint3D())
			{
				continue;
			}

			Images_Mutex.unlock();
			SetObservationAsTriangulated(ImageID, i, false);
			Images_Mutex.lock();
		}
		RegImageIDs_Mutex.lock();
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
}
void CModel::TearDown()
{
	CorresGraph = nullptr;
	lock(ImagePairs_Mutex, Images_Mutex, Cameras_Mutex, Points3D_Mutex);
	ImagePairs.clear();

	unordered_set<size_t> KeepCameraIDs;
	// Remove all not yet registered images.
	for (auto it = Images.begin(); it != Images.end();)
	{
		if (it->second.IsRegistered())
		{
			KeepCameraIDs.insert(it->second.CameraId());
			it->second.TearDown();
			it++;
		}
		else
		{
			it = Images.erase(it);
		}
	}
	// Remove all unused cameras.
	for (auto it = Cameras.begin(); it != Cameras.end();)
	{
		if (!KeepCameraIDs.count(it->first))
		{
			it = Cameras.erase(it);
		}
		else
		{
			it++;
		}
	}
	// Compress tracks.
	for (auto& pair : Points3D)
	{
		pair.second.Track().Compress();
	}

	ImagePairs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	Points3D_Mutex.unlock();
}

void CModel::UpdateImageID()
{
	unordered_map<size_t, size_t> OldID2NewID;
	size_t ImagesNum = GetModelImagesNum();
	OldID2NewID.reserve(ImagesNum);
	CImageMapType NewImage;
	NewImage.reserve(ImagesNum);

	lock(Images_Mutex, RegImageIDs_Mutex, Points3D_Mutex);
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	for (auto& pair : Images)
	{
		string ImageName = pair.second.Name();
		if (!CDatabase::IsExistImage(ImageName,db))
		{
			Images_Mutex.unlock();
			RegImageIDs_Mutex.unlock();
			Points3D_Mutex.unlock();
			LOG(FATAL) << "Image with name " << ImageName << " does not exist in database";
		}
		size_t DbImageID = CDatabase::GetImageID(ImageName,db);
		Image DbImage;
		CDatabase::GetImage(DbImageID, DbImage, db);
		OldID2NewID.emplace(pair.second.ImageId(), DbImageID);
		pair.second.SetImageId(DbImageID);
		NewImage.emplace(DbImageID, pair.second);
	}
	ReleaseDatabaseConnect(db);
	Images = NewImage;

	for (size_t& RegImageID : RegImageIDs)
	{
		RegImageID = OldID2NewID[RegImageID];
	}

	for (auto& pair : Points3D)
	{
		for (auto& element : pair.second.Track().Elements())
		{
			element.image_id = OldID2NewID[element.image_id];
		}
	}

	Images_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
	Points3D_Mutex.unlock();
}

bool CModel::ExtractColors_SingleImage(size_t ImageID, string ImageSaveDir)
{
	Bitmap bitmap;
	lock(Images_Mutex, Points3D_Mutex);
	if (!bitmap.Read(JoinPath(ImageSaveDir, Images[ImageID].Name())))
	{
		Images_Mutex.unlock();
		Points3D_Mutex.unlock();
		return false;
	}
	Eigen::Vector3ub BlackColor(0, 0, 0);
	for (const Point2D& point : Images[ImageID].Points2D())
	{
		if (!point.HasPoint3D())continue;
		if (Points3D[point.Point3DId()].Color() == BlackColor)
		{
			BitmapColor<float> color;
			if (bitmap.InterpolateBilinear(point.X() - 0.5, point.Y() - 0.5, &color))
			{
				BitmapColor<uint8_t> ColorUb = color.Cast<uint8_t>();
				Points3D[point.Point3DId()].SetColor(Eigen::Vector3ub(ColorUb.r, ColorUb.g, ColorUb.b));
			}
		}
	}
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
	return true;
}
void CModel::ExtractColors(string ImageSaveDir)
{
	EIGEN_STL_UMAP(size_t, Eigen::Vector3d) Point3DColors;
	unordered_map<size_t, size_t> Point3DColorCounts;

	lock(RegImageIDs_Mutex, Images_Mutex, Points3D_Mutex);
	for (size_t i = 0; i < RegImageIDs.size(); i++)
	{
		size_t ImageID = RegImageIDs[i];
		string ImageName = Images[ImageID].Name();
		string ImagePath = JoinPath(ImageSaveDir, ImageName);
		Bitmap bitmap;
		if (!bitmap.Read(ImagePath))
		{
			cout << StringPrintf("Could not read image %s at path %s.", ImageName.c_str(), ImagePath.c_str()) << endl;
			continue;
		}
		for (const Point2D& point : Images[ImageID].Points2D())
		{
			if (!point.HasPoint3D())continue;
			BitmapColor<float> color;
			if (bitmap.InterpolateBilinear(point.X() - 0.5, point.Y() - 0.5, &color))
			{
				if (Point3DColors.count(point.Point3DId()))
				{
					Point3DColors[point.Point3DId()](0) += color.r;
					Point3DColors[point.Point3DId()](1) += color.g;
					Point3DColors[point.Point3DId()](2) += color.b;
					Point3DColorCounts[point.Point3DId()]++;
				}
				else
				{
					Point3DColors[point.Point3DId()] = Eigen::Vector3d(color.r, color.g, color.b);
					Point3DColorCounts[point.Point3DId()] = 1;
				}
			}
		}
	}
	Eigen::Vector3ub BlackColor = Eigen::Vector3ub::Zero();
	for (auto& pair : Points3D)
	{
		size_t Point3DID = pair.first;
		if (Point3DColors.count(Point3DID))
		{
			Eigen::Vector3d color = Point3DColors[Point3DID] / Point3DColorCounts[Point3DID];
			color[0] = round(color[0]);
			color[1] = round(color[1]);
			color[2] = round(color[2]);
			pair.second.SetColor(color.cast<uint8_t>());
		}
		else
		{
			pair.second.SetColor(BlackColor);
		}
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}
void CModel::Normalize(double Extent, double p0, double p1, bool IsUseImages)
{
	CHECK_GT(Extent, 0);
	if ((IsUseImages && GetModelRegImagesNum() < 2) || (!IsUseImages && GetModelPoints3DNum() < 2))
	{
		return;
	}
	tuple<Eigen::Vector3d, Eigen::Vector3d, Eigen::Vector3d> BoundsAndCentroid;
	CalcBoundsAndCentroid(p0, p1, IsUseImages, BoundsAndCentroid);
	// Calculate scale and translation, such that translation is applied before scaling.
	double OldExtent = (get<1>(BoundsAndCentroid) - get<0>(BoundsAndCentroid)).norm();
	double Scale = (OldExtent < numeric_limits<double>::epsilon() ? 1 : Extent / OldExtent);
	SimilarityTransform3 tform(Scale, ComposeIdentityQuaternion(), -Scale * get<2>(BoundsAndCentroid));
	Transform(tform);
}
void CModel::CalcPoint3DsCenter(Eigen::Vector3d& Centroid, double p0, double p1)
{
	tuple<Eigen::Vector3d, Eigen::Vector3d, Eigen::Vector3d> BoundsAndCentroid;
	CalcBoundsAndCentroid(p0, p1, false, BoundsAndCentroid);
	Centroid = get<2>(BoundsAndCentroid);
}
void CModel::CalcBoundingBox(pair<Eigen::Vector3d, Eigen::Vector3d>& BoundingBox, double p0, double p1)
{
	tuple<Eigen::Vector3d, Eigen::Vector3d, Eigen::Vector3d> BoundsAndCentroid;
	CalcBoundsAndCentroid(p0, p1, false, BoundsAndCentroid);
	BoundingBox.first = get<0>(BoundsAndCentroid);
	BoundingBox.second = get<1>(BoundsAndCentroid);
}
void CModel::Transform(SimilarityTransform3& Trans)
{
	lock(Images_Mutex, Points3D_Mutex);

	for (auto& pair : Images)
	{
		Trans.TransformPose(&pair.second.Qvec(), &pair.second.Tvec());
	}
	for (auto& pair : Points3D)
	{
		Trans.TransformPoint(&pair.second.XYZ());
	}
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}
void CModel::Crop(pair<Eigen::Vector3d, Eigen::Vector3d>& BoundingBox, CModel& CropedModel)
{
	// add all cameras and images. Only the registered images will be used.
	lock(Cameras_Mutex, Images_Mutex, Points3D_Mutex);

	for (auto& pair : Cameras)
	{
		CropedModel.ModelAddCamera(pair.second);
	}
	for (auto& pair : Images)
	{
		Image NewImage = pair.second;
		NewImage.SetRegistered(false);
		for (size_t pid = 0; pid < NewImage.NumPoints2D(); pid++)
		{
			NewImage.ResetPoint3DForPoint2D(pid);
		}
		CropedModel.ModelAddImage(NewImage);
	}
	for (auto& pair : Points3D)
	{
		if ((pair.second.XYZ().array() >= BoundingBox.first.array()).all() && (pair.second.XYZ().array() <= BoundingBox.second.array()).all())
		{
			for (TrackElement& element : pair.second.Track().Elements())
			{
				CropedModel.RegisterImage(element.image_id);
			}
			CropedModel.ModelAddPoint3D(pair.second.XYZ(), pair.second.Track(), pair.second.Color());
		}
	}
	Cameras_Mutex.unlock();
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}

size_t CModel::FilterPoints3D(double MaxReprojectError, double MinTriAngle, const unordered_set<size_t>& Point3DIDs)
{
	size_t FilteredNum = 0;
	FilteredNum += FilterLargeReprojectErrorPoints3D(MaxReprojectError, Point3DIDs);
	FilteredNum += FilterSmallTriAnglePoints3D(MinTriAngle, Point3DIDs);
	return FilteredNum;
}
size_t CModel::FilterPoints3DInImages(double MaxReprojectError, double MinTriAngle, unordered_set<size_t>& ImageIDs)
{
	unordered_set<size_t> Points3DID;
	Images_Mutex.lock();
	for (size_t ImageID : ImageIDs)
	{
		for (const Point2D& point2D : Images[ImageID].Points2D())
		{
			if (point2D.HasPoint3D())
			{
				Points3DID.insert(point2D.Point3DId());
			}
		}
	}
	Images_Mutex.unlock();
	return FilterPoints3D(MaxReprojectError, MinTriAngle, Points3DID);
}
size_t CModel::FilterAllPoints3Ds(double MaxReprojectError, double MinTriAngle)
{
	// Important: First filter observations and points with large reprojection
	// error, so that observations with large reprojection error do not make
	// a point stable through a large triangulation angle.

	unordered_set<size_t> Points3DIDs;
	GetModelAllPoint3DIDs(Points3DIDs);
	size_t FilteredNum = 0;
	FilteredNum += FilterLargeReprojectErrorPoints3D(MaxReprojectError, Points3DIDs);
	FilteredNum += FilterSmallTriAnglePoints3D(MinTriAngle, Points3DIDs);
	return FilteredNum;
}
size_t CModel::FilterNegativeDepthObservations()
{
	size_t FilteredNum = 0;
	lock(RegImageIDs_Mutex, Images_Mutex);
	for (size_t RegImageID : RegImageIDs)
	{
		for (size_t Point2DID = 0; Point2DID < Images[RegImageID].NumPoints2D(); Point2DID++)
		{
			if (!Images[RegImageID].Point2D(Point2DID).HasPoint3D())continue;
			size_t Point3DID = Images[RegImageID].Point2D(Point2DID).Point3DId();
			if (!HasPointPositiveDepth(Images[RegImageID].ProjectionMatrix(), Points3D[Point3DID].XYZ()))
			{
				DeleteObservation(RegImageID, Point2DID);
				FilteredNum++;
			}
		}
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	return FilteredNum;
}
void CModel::FilterImages(double MinFocalLenRatio, double MaxFocalLenRatio, double MaxExtraParam, vector<size_t>& FilteredImagesID)
{
	FilteredImagesID.resize(0);
	FilteredImagesID.reserve(GetModelRegImagesNum());

	lock(RegImageIDs_Mutex, Images_Mutex, Cameras_Mutex);
	for (size_t RegImageID : RegImageIDs)
	{
		size_t CameraID = Images[RegImageID].CameraId();
		if (Images[RegImageID].NumPoints3D() == 0 || Cameras[CameraID].HasBogusParams(MinFocalLenRatio, MaxFocalLenRatio, MaxExtraParam))
		{
			FilteredImagesID.push_back(RegImageID);
		}
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();

	// Only de-register after iterating over reg_image_ids_ to avoid simultaneous iteration and modification of the vector.
	for (size_t ImageID : FilteredImagesID)
	{
		DeRegisterImage(ImageID);
	}
}

size_t CModel::GetObservationsNum()
{
	lock(RegImageIDs_Mutex, Images_Mutex);
	size_t ObservationsNum = 0;
	for (size_t RegImageID : RegImageIDs)
	{
		ObservationsNum += Images[RegImageID].NumPoints3D();
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	return ObservationsNum;
}
double CModel::GetMeanTrackLength()
{
	size_t Points3DNum = GetModelPoints3DNum();
	if (Points3DNum == 0)
	{
		return 0;
	}
	return GetObservationsNum() * 1.0 / Points3DNum;
}
double CModel::GetMeanObservationsNumEachRegImage()
{
	size_t RegImagesNum = GetModelRegImagesNum();
	if (RegImagesNum == 0)
	{
		return 0;
	}
	return GetObservationsNum() * 1.0 / RegImagesNum;


}
double CModel::GetMeanReprojectError()
{
	double ErrorSum = 0;
	size_t ValidErrorNum = 0;
	Points3D_Mutex.lock();

	for (auto& pair : Points3D)
	{
		if (!pair.second.HasError())continue;
		ErrorSum += pair.second.Error();
		ValidErrorNum++;
	}
	Points3D_Mutex.unlock();

	if (ValidErrorNum == 0)return 0;
	return ErrorSum / ValidErrorNum;
}

bool CModel::Merge(CModel& MergedModel, double MaxReprojectError)
{
	double MinInlierObservations = 0.3;
	Eigen::Matrix3x4d Alignment;
	if (!CalcModelsAlignment(MergedModel, MinInlierObservations, MaxReprojectError, &Alignment))
	{
#ifdef OUTPUTLOG_MODE
		qDebug() << "CalcModelsAlignment return false!";
#endif
		return false;
	}
	SimilarityTransform3 tform(Alignment);
	// Find common and missing images in the two reconstructions.

	unordered_set<size_t> CommonImageIDs, MissingImageIDs;
	CommonImageIDs.reserve(MergedModel.GetModelRegImagesNum());
	MissingImageIDs.reserve(MergedModel.GetModelRegImagesNum());

	vector<size_t> MergedModelRegImages;
	MergedModel.GetModelAllRegImageIDs(MergedModelRegImages);

	for (size_t ImageID : MergedModelRegImages)
	{
		if (IsModelExistImage(ImageID))
		{
			CommonImageIDs.insert(ImageID);
		}
		else
		{
			MissingImageIDs.insert(ImageID);
		}
	}

	// Register the missing images in this reconstruction.
	for (size_t ImageID : MissingImageIDs)
	{
		Image RegImage = MergedModel.GetModelImage(ImageID);
		RegImage.SetRegistered(false);
		ModelAddImage(RegImage);
		RegisterImage(ImageID);
		if (!IsModelExistCamera(RegImage.CameraId()))
		{
			Camera camera = MergedModel.GetModelCamera(RegImage.CameraId());
			ModelAddCamera(camera);
		}
		Images_Mutex.lock();
		tform.TransformPose(&Images[ImageID].Qvec(), &Images[ImageID].Tvec());
		Images_Mutex.unlock();
	}
	// Merge the two point clouds using the following two rules:
	//    - copy points to this reconstruction with non-conflicting tracks,
	//      i.e. points that do not have an already triangulated observation
	//      in this reconstruction.
	//    - merge tracks that are unambiguous, i.e. only merge points in the two
	//      reconstructions if they have a one-to-one mapping.
	// Note that in both cases no cheirality or reprojection test is performed.

	CPoint3DMapType MergedModelPoints3D;
	MergedModel.GetModelAllPoint3Ds(MergedModelPoints3D);
	for (auto& pair : MergedModelPoints3D)
	{
		Track NewTrack, OldTrack;
		unordered_set<size_t> OldPoints3DIDs;
		Images_Mutex.lock();
		for (TrackElement& element : pair.second.Track().Elements())
		{
			if (CommonImageIDs.count(element.image_id))
			{
				if (Images[element.image_id].Point2D(element.point2D_idx).HasPoint3D())
				{
					OldTrack.AddElement(element);
					OldPoints3DIDs.insert(Images[element.image_id].Point2D(element.point2D_idx).Point3DId());
				}
				else
				{
					NewTrack.AddElement(element);
				}
			}
			else if (MissingImageIDs.count(element.image_id))
			{
				Images[element.image_id].ResetPoint3DForPoint2D(element.point2D_idx);
				NewTrack.AddElement(element);
			}
		}
		Images_Mutex.unlock();

		bool IsCreatNewPoint = (NewTrack.Length() >= 2);
		bool IsMergeNewOldPoints = (NewTrack.Length() + OldTrack.Length() >= 2 && OldPoints3DIDs.size() == 1);
		if (IsCreatNewPoint || IsMergeNewOldPoints)
		{
			Eigen::Vector3d xyz = pair.second.XYZ();
			tform.TransformPoint(&xyz);
			size_t Point3DID = ModelAddPoint3D(xyz, NewTrack, pair.second.Color());
			if (OldPoints3DIDs.size() == 1)
			{
				MergePoint3Ds(Point3DID, *OldPoints3DIDs.begin());
			}
		}
	}
	unordered_set<size_t> Point3DsIDs;
	GetModelAllPoint3DIDs(Point3DsIDs);
	FilterLargeReprojectErrorPoints3D(MaxReprojectError, Point3DsIDs);
	return true;
}
void CModel::FindCommonRegImages(vector<size_t>& CommonImagesID, CModel& Model)
{
	lock(RegImageIDs_Mutex, Images_Mutex);
	CommonImagesID.reserve(RegImageIDs.size());
	for (size_t RegImageId : RegImageIDs)
	{
		if (!Model.IsModelExistImage(RegImageId) || !Model.IsImageRegistered(RegImageId))
		{
			continue;
		}
		CHECK_EQ(Images[RegImageId].Name(), Model.GetModelImage(RegImageId).Name());
		CommonImagesID.push_back(RegImageId);
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
}

template <bool kEstimateScale>
bool CModel::Align(vector<string>& ImageNames, vector<Eigen::Vector3d>& Locations, size_t MinCommonImagesNum, SimilarityTransform3* tform)
{
	CHECK_GE(MinCommonImagesNum, 3);
	CHECK_EQ(ImageNames.size(), Locations.size());

	// Find out which images are contained in the reconstruction and get the positions of their camera centers.
	unordered_set<size_t> CommonImageIDs;
	vector<Eigen::Vector3d> src, dst;
	for (size_t i = 0; i < ImageNames.size(); i++)
	{
		if (!IsModelExistImage(ImageNames[i]))continue;

		Image image;
		GetModelImage(image, ImageNames[i]);
		size_t ImageID = image.ImageId();

		if (!IsImageRegistered(ImageID))continue;

		if (CommonImageIDs.count(ImageID))continue;

		CommonImageIDs.insert(ImageID);
		src.push_back(image.ProjectionCenter());
		dst.push_back(Locations[i]);
	}

	// Only compute the alignment if there are enough correspondences.
	if (CommonImageIDs.size() < MinCommonImagesNum)return false;

	SimilarityTransform3 transform;
	if (!transform.Estimate<kEstimateScale>(src, dst))
	{
		return false;
	}

	Transform(transform);

	if (tform)
	{
		*tform = transform;
	}
	return true;
}

template <bool kEstimateScale>
bool CModel::AlignRobust(vector<string>& ImageNames, vector<Eigen::Vector3d>& Locations, size_t MinCommonImagesNum, RANSACOptions& RANSAC_options, SimilarityTransform3* tform)
{
	CHECK_GE(MinCommonImagesNum, 3);
	CHECK_EQ(ImageNames.size(), Locations.size());

	// Find out which images are contained in the reconstruction and get the positions of their camera centers.
	unordered_set<size_t> CommonImageIDs;
	vector<Eigen::Vector3d> src, dst;
	for (size_t i = 0; i < ImageNames.size(); i++)
	{
		if (!IsModelExistImage(ImageNames[i]))continue;
		Image image;
		GetModelImage(image, ImageNames[i]);
		size_t ImageID = image.ImageId();

		if (!IsImageRegistered(ImageID))continue;

		if (CommonImageIDs.count(ImageID))continue;

		CommonImageIDs.insert(ImageID);
		src.push_back(image.ProjectionCenter());
		dst.push_back(Locations[i]);
	}
	// Only compute the alignment if there are enough correspondences.
	if (CommonImageIDs.size() < MinCommonImagesNum)return false;

	LORANSAC<SimilarityTransformEstimator<3, kEstimateScale>, SimilarityTransformEstimator<3, kEstimateScale>> ransac(RANSAC_options);

	auto report = ransac.Estimate(src, dst);
	if (report.support.num_inliers < MinCommonImagesNum)
	{
		return false;
	}

	SimilarityTransform3 transform = SimilarityTransform3(report.model);
	Transform(transform);

	if (tform)
	{
		*tform = transform;
	}
	return true;
}

void CModel::CreateImageDirs(string Path)
{
	unordered_set<string> ImageDirs;
	Images_Mutex.lock();
	for (auto& pair : Images)
	{
		vector<string> NameSplit = StringSplit(pair.second.Name(), "/");
		if (NameSplit.size() > 1)
		{
			string Dir = Path;
			for (size_t i = 0; i < NameSplit.size() - 1; i++)
			{
				Dir = JoinPath(Dir, NameSplit[i]);
				ImageDirs.insert(Dir);
			}
		}
	}
	Images_Mutex.unlock();
	for (const string& Dir : ImageDirs)
	{
		CreateDirIfNotExists(Dir, true);
	}
}

void CModel::Read(string Path)
{
	if (ExistsFile(JoinPath(Path, "cameras.bin")) && ExistsFile(JoinPath(Path, "images.bin")) && ExistsFile(JoinPath(Path, "points3D.bin")))
	{
		ReadBinary(Path);
	}
	else if (ExistsFile(JoinPath(Path, "cameras.txt")) && ExistsFile(JoinPath(Path, "images.txt")) && ExistsFile(JoinPath(Path, "points3D.txt")))
	{
		ReadText(Path);
	}
	else
	{
		LOG(FATAL) << "cameras, images, points3D files do not exist at " << Path;
	}
}
void CModel::ReadText(string Path)
{
	string CamerasPath = JoinPath(Path, "cameras.txt");
	string ImagesPath = JoinPath(Path, "images.txt");
	string Points3DsPath = JoinPath(Path, "points3D.txt");

	ReadCamerasText(CamerasPath);
	ReadImagesText(ImagesPath);
	ReadPoints3DText(Points3DsPath);
}
void CModel::ReadBinary(string Path)
{
	string CamerasPath = JoinPath(Path, "cameras.bin");
	string ImagesPath = JoinPath(Path, "images.bin");
	string Points3DsPath = JoinPath(Path, "points3D.bin");
	ReadCamerasBinary(CamerasPath);
	ReadImagesBinary(ImagesPath);
	ReadPoints3DBinary(Points3DsPath);
}
void CModel::ReadPLY(string Path)
{
	vector<PlyPoint> PlyPoints = ReadPly(Path);
	ReadPLY(PlyPoints);
}
void CModel::ReadPLY(vector<PlyPoint>& PLY)
{
	Points3D_Mutex.lock();
	Points3D.clear();
	Points3D.reserve(PLY.size());
	Points3D_Mutex.unlock();
	for (PlyPoint& Point : PLY)
	{
		Eigen::Vector3d xyz(Point.x, Point.y, Point.z);
		Track track;
		Eigen::Vector3ub rgb(Point.r, Point.g, Point.b);
		ModelAddPoint3D(xyz, track, rgb);
	}
}

void CModel::Write(string Path)
{
	WriteBinary(Path);
}
void CModel::WriteText(string Path)
{
	string CamerasPath = JoinPath(Path, "cameras.txt");
	string ImagesPath = JoinPath(Path, "images.txt");
	string Points3DsPath = JoinPath(Path, "points3D.txt");

	WriteCamerasText(CamerasPath);
	WriteImagesText(ImagesPath);
	WritePoints3DText(Points3DsPath);
}
void CModel::WriteBinary(string Path)
{
	string CamerasPath = JoinPath(Path, "cameras.bin");
	string ImagesPath = JoinPath(Path, "images.bin");
	string Points3DsPath = JoinPath(Path, "points3D.bin");

	WriteCamerasBinary(CamerasPath);
	WriteImagesBinary(ImagesPath);
	WritePoints3DBinary(Points3DsPath);
}
void CModel::WritePLY(string Path)
{
	vector<PlyPoint> PLY;
	WritePLY(PLY);
	WriteBinaryPlyPoints(Path, PLY, false, true);
}
void CModel::WritePLY(vector<PlyPoint>& PLY)
{
	Points3D_Mutex.lock();
	PLY.clear();
	PLY.reserve(Points3D.size());
	for (auto& pair : Points3D)
	{
		PlyPoint point;
		point.x = pair.second.X();
		point.y = pair.second.Y();
		point.z = pair.second.Z();
		point.r = pair.second.Color(0);
		point.g = pair.second.Color(1);
		point.b = pair.second.Color(2);
		PLY.push_back(point);
	}
	Points3D_Mutex.unlock();
}
bool CModel::WriteNVM(string Path, bool IsSkipDistortion)
{
	ofstream file(Path, ios::trunc);
	CHECK(file.is_open()) << Path;

	// Ensure that we don't lose any precision by storing in text.
	file.precision(17);

	lock(RegImageIDs_Mutex, Images_Mutex, Cameras_Mutex, Points3D_Mutex);
	// White space added for compatibility with Meshlab.
	file << "NVM_V3 " << endl << " " << endl;
	file << RegImageIDs.size() << "  " << endl;

	unordered_map<size_t, size_t> ImageID2IDx;
	size_t ImageIDx = 0;
	for (size_t RegImageID : RegImageIDs)
	{
		double k;
		size_t CameraID = Images[RegImageID].CameraId();
		if (IsSkipDistortion || Cameras[CameraID].ModelId() == SimplePinholeCameraModel::model_id || Cameras[CameraID].ModelId() == PinholeCameraModel::model_id)
		{
			k = 0;
		}
		else if (Cameras[CameraID].ModelId() == SimpleRadialCameraModel::model_id)
		{
			k = -1 * Cameras[CameraID].Params(SimpleRadialCameraModel::extra_params_idxs[0]);
		}
		else
		{
			cout << "WARNING: NVM only supports `SIMPLE_RADIAL` and pinhole camera models." << endl;

			RegImageIDs_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();
			Points3D_Mutex.unlock();
			return false;
		}
		Eigen::Vector3d ProjectCenter = Images[RegImageID].ProjectionCenter();
		file << Images[RegImageID].Name() << " ";
		file << Cameras[CameraID].MeanFocalLength() << " ";
		file << Images[RegImageID].Qvec(0) << " ";
		file << Images[RegImageID].Qvec(1) << " ";
		file << Images[RegImageID].Qvec(2) << " ";
		file << Images[RegImageID].Qvec(3) << " ";
		file << ProjectCenter(0) << " ";
		file << ProjectCenter(1) << " ";
		file << ProjectCenter(2) << " ";
		file << k << " ";
		file << 0 << endl;

		ImageID2IDx[RegImageID] = ImageIDx;
		ImageIDx++;
	}
	file << endl << Points3D.size() << endl;
	for (auto& pair : Points3D)
	{
		file << pair.second.XYZ()(0) << " ";
		file << pair.second.XYZ()(1) << " ";
		file << pair.second.XYZ()(2) << " ";
		file << static_cast<int>(pair.second.Color(0)) << " ";
		file << static_cast<int>(pair.second.Color(1)) << " ";
		file << static_cast<int>(pair.second.Color(2)) << " ";

		ostringstream line;
		unordered_set<size_t> ImageIDs;
		for (TrackElement& element : pair.second.Track().Elements())
		{
			if (ImageIDs.count(element.image_id))continue;
			line << ImageID2IDx[element.image_id] << " ";
			line << element.point2D_idx << " ";
			line << Images[element.image_id].Point2D(element.point2D_idx).X() << " ";
			line << Images[element.image_id].Point2D(element.point2D_idx).Y() << " ";
			ImageIDs.insert(element.image_id);
		}
		string line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);
		file << ImageIDs.size() << " ";
		file << line_string << endl;
	}

	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	Points3D_Mutex.unlock();

	return true;
}
bool CModel::WriteCam(string Path, bool IsSkipDistortion)
{
	CreateImageDirs(Path);
	lock(RegImageIDs_Mutex, Images_Mutex, Cameras_Mutex);

	for (size_t RegImageID : RegImageIDs)
	{
		size_t CameraID = Images[RegImageID].CameraId();
		string name, ext;
		SplitFileExtension(Images[RegImageID].Name(), &name, &ext);
		name = JoinPath(Path, name + ".cam");
		ofstream file(name, ios::trunc);

		CHECK(file.is_open()) << name;
		// Ensure that we don't lose any precision by storing in text.
		file.precision(17);

		double k1, k2;
		if (IsSkipDistortion || Cameras[CameraID].ModelId() == SimplePinholeCameraModel::model_id || Cameras[CameraID].ModelId() == PinholeCameraModel::model_id)
		{
			k1 = 0.0;
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == SimpleRadialCameraModel::model_id)
		{
			k1 = Cameras[CameraID].Params(SimpleRadialCameraModel::extra_params_idxs[0]);
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == RadialCameraModel::model_id)
		{
			k1 = Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[0]);
			k2 = Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[1]);
		}
		else
		{
			RegImageIDs_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();

			cout << "WARNING: CAM only supports `SIMPLE_RADIAL`, `RADIAL`, and pinhole camera models." << endl;
			return false;
		}
		if (k1 != 0.0 && k2 == 0.0)
		{
			k2 = 1e-10;
		}
		double fx, fy;
		if (Cameras[CameraID].FocalLengthIdxs().size() == 2)
		{
			fx = Cameras[CameraID].FocalLengthX();
			fy = Cameras[CameraID].FocalLengthY();
		}
		else
		{
			fx = fy = Cameras[CameraID].MeanFocalLength();
		}

		double focal_length;
		if (Cameras[CameraID].Width() * fy < Cameras[CameraID].Height() * fx)
		{
			focal_length = fy / Cameras[CameraID].Height();
		}
		else
		{
			focal_length = fx / Cameras[CameraID].Width();
		}
		Eigen::Matrix3d rot_mat = Images[RegImageID].RotationMatrix();
		file << Images[RegImageID].Tvec(0) << " " << Images[RegImageID].Tvec(1) << " " << Images[RegImageID].Tvec(2) << " "
			<< rot_mat(0, 0) << " " << rot_mat(0, 1) << " " << rot_mat(0, 2) << " "
			<< rot_mat(1, 0) << " " << rot_mat(1, 1) << " " << rot_mat(1, 2) << " "
			<< rot_mat(2, 0) << " " << rot_mat(2, 1) << " " << rot_mat(2, 2)
			<< endl;
		file << focal_length << " " << k1 << " " << k2 << " " << fy / fx << " "
			<< Cameras[CameraID].PrincipalPointX() / Cameras[CameraID].Width() << " "
			<< Cameras[CameraID].PrincipalPointY() / Cameras[CameraID].Height() << endl;
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	return true;
}
bool CModel::WriteRecon3D(string Path, bool IsSkipDistortion)
{
	string BasePath = EnsureTrailingSlash(StringReplace(Path, "\\", "/"));
	CreateDirIfNotExists(BasePath);
	BasePath = BasePath.append("Recon/");
	CreateDirIfNotExists(BasePath);
	string synth_path = BasePath + "synth_0.out";
	string image_list_path = BasePath + "urd-images.txt";
	string image_map_path = BasePath + "imagemap_0.txt";

	ofstream synth_file(synth_path, ios::trunc);
	CHECK(synth_file.is_open()) << synth_path;
	ofstream image_list_file(image_list_path, ios::trunc);
	CHECK(image_list_file.is_open()) << image_list_path;
	ofstream image_map_file(image_map_path, ios::trunc);
	CHECK(image_map_file.is_open()) << image_map_path;

	// Ensure that we don't lose any precision by storing in text.
	synth_file.precision(17);

	lock(RegImageIDs_Mutex, Images_Mutex, Cameras_Mutex, Points3D_Mutex);
	// Write header info
	synth_file << "colmap 1.0" << endl;
	synth_file << RegImageIDs.size() << " " << Points3D.size() << endl;

	unordered_map<size_t, size_t> ImageID2IDx;
	size_t ImageIDx = 0;
	for (size_t RegImageID : RegImageIDs)
	{
		size_t CameraID = Images[RegImageID].CameraId();
		double k1, k2;
		if (IsSkipDistortion || Cameras[CameraID].ModelId() == SimplePinholeCameraModel::model_id || Cameras[CameraID].ModelId() == PinholeCameraModel::model_id)
		{
			k1 = 0.0;
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == SimpleRadialCameraModel::model_id)
		{
			k1 = -1 * Cameras[CameraID].Params(SimpleRadialCameraModel::extra_params_idxs[0]);
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == RadialCameraModel::model_id)
		{
			k1 = -1 * Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[0]);
			k2 = -1 * Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[1]);
		}
		else
		{
			RegImageIDs_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();
			Points3D_Mutex.unlock();

			cout << "WARNING: Recon3D only supports `SIMPLE_RADIAL`, `RADIAL`, and pinhole camera models." << endl;
			return false;
		}
		double scale = 1.0 / (double)max(Cameras[CameraID].Width(), Cameras[CameraID].Height());
		synth_file << scale * Cameras[CameraID].MeanFocalLength() << " " << k1 << " " << k2 << endl;
		synth_file << QuaternionToRotationMatrix(NormalizeQuaternion(Images[RegImageID].Qvec())) << endl;
		synth_file << Images[RegImageID].Tvec(0) << " " << Images[RegImageID].Tvec(1) << " " << Images[RegImageID].Tvec(2) << endl;

		ImageID2IDx[RegImageID] = ImageIDx;
		image_list_file << Images[RegImageID].Name() << endl << Cameras[CameraID].Width() << " " << Cameras[CameraID].Height() << endl;
		image_map_file << ImageIDx << endl;

		ImageIDx++;
	}
	image_list_file.close();
	image_map_file.close();

	// Write point info
	for (auto& pair : Points3D)
	{
		synth_file << pair.second.XYZ()(0) << " " << pair.second.XYZ()(1) << " " << pair.second.XYZ()(2) << endl;
		synth_file << (int)pair.second.Color(0) << " " << (int)pair.second.Color(1) << " " << (int)pair.second.Color(2) << endl;

		ostringstream line;
		unordered_set<size_t> image_ids;
		for (TrackElement& element : pair.second.Track().Elements())
		{
			// Make sure that each point only has a single observation per image,
			// since VisualSfM does not support with multiple observations.
			if (image_ids.count(element.image_id) == 0)
			{
				size_t CameraID = Images[element.image_id].CameraId();
				size_t Point2DID = element.point2D_idx;

				const double scale = 1.0 / (double)max(Cameras[CameraID].Width(), Cameras[CameraID].Height());

				line << ImageID2IDx[element.image_id] << " ";
				line << element.point2D_idx << " ";
				// Use a scale of -1.0 to mark as invalid as it is not needed currently
				line << "-1.0 ";
				line << (Images[element.image_id].Point2D(Point2DID).X() - Cameras[CameraID].PrincipalPointX()) * scale << " ";
				line << (Images[element.image_id].Point2D(Point2DID).Y() - Cameras[CameraID].PrincipalPointY()) * scale << " ";
				image_ids.insert(element.image_id);
			}
		}

		string line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);

		synth_file << image_ids.size() << " ";
		synth_file << line_string << endl;
	}
	synth_file.close();
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	Points3D_Mutex.unlock();
	return true;
}
bool CModel::WriteBundler(string Path, string ListPath, bool IsSkipDistortion)
{
	ofstream file(Path, ios::trunc);
	CHECK(file.is_open()) << Path;

	ofstream list_file(ListPath, ios::trunc);
	CHECK(list_file.is_open()) << ListPath;

	// Ensure that we don't lose any precision by storing in text.
	file.precision(17);

	file << "# Bundle file v0.3" << endl;
	lock(RegImageIDs_Mutex, Images_Mutex, Cameras_Mutex, Points3D_Mutex);
	file << RegImageIDs.size() << " " << Points3D.size() << endl;

	unordered_map<size_t, size_t> ImageID2IDx;
	size_t ImageIDx = 0;
	for (size_t RegImageID : RegImageIDs)
	{
		size_t CameraID = Images[RegImageID].CameraId();
		double k1, k2;
		if (IsSkipDistortion || Cameras[CameraID].ModelId() == SimplePinholeCameraModel::model_id || Cameras[CameraID].ModelId() == PinholeCameraModel::model_id)
		{
			k1 = 0.0;
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == SimpleRadialCameraModel::model_id)
		{
			k1 = Cameras[CameraID].Params(SimpleRadialCameraModel::extra_params_idxs[0]);
			k2 = 0.0;
		}
		else if (Cameras[CameraID].ModelId() == RadialCameraModel::model_id)
		{
			k1 = Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[0]);
			k2 = Cameras[CameraID].Params(RadialCameraModel::extra_params_idxs[1]);
		}
		else
		{
			RegImageIDs_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();
			Points3D_Mutex.unlock();

			cout << "WARNING: Bundler only supports `SIMPLE_RADIAL`, `RADIAL`, and pinhole camera models." << endl;
			return false;
		}
		file << Cameras[CameraID].MeanFocalLength() << " " << k1 << " " << k2 << endl;

		Eigen::Matrix3d R = Images[RegImageID].RotationMatrix();
		file << R(0, 0) << " " << R(0, 1) << " " << R(0, 2) << endl;
		file << -R(1, 0) << " " << -R(1, 1) << " " << -R(1, 2) << endl;
		file << -R(2, 0) << " " << -R(2, 1) << " " << -R(2, 2) << endl;

		file << Images[RegImageID].Tvec(0) << " ";
		file << -Images[RegImageID].Tvec(1) << " ";
		file << -Images[RegImageID].Tvec(2) << endl;

		list_file << Images[RegImageID].Name() << endl;

		ImageID2IDx[RegImageID] = ImageIDx;
		ImageIDx++;
	}
	for (auto& pair : Points3D)
	{
		file << pair.second.XYZ()(0) << " ";
		file << pair.second.XYZ()(1) << " ";
		file << pair.second.XYZ()(2) << endl;

		file << static_cast<int>(pair.second.Color(0)) << " ";
		file << static_cast<int>(pair.second.Color(1)) << " ";
		file << static_cast<int>(pair.second.Color(2)) << endl;

		ostringstream line;

		line << pair.second.Track().Length() << " ";

		for (TrackElement& element : pair.second.Track().Elements())
		{
			size_t CameraID = Images[element.image_id].CameraId();
			size_t Point2DID = element.point2D_idx;

			// Bundler output assumes image coordinate system origin
			// in the lower left corner of the image with the center of
			// the lower left pixel being (0, 0). Our coordinate system
			// starts in the upper left corner with the center of the
			// upper left pixel being (0.5, 0.5).
			line << ImageID2IDx[element.image_id] << " ";
			line << element.point2D_idx << " ";
			line << Images[element.image_id].Point2D(Point2DID).X() - Cameras[CameraID].PrincipalPointX() << " ";
			line << Cameras[CameraID].PrincipalPointY() - Images[element.image_id].Point2D(Point2DID).Y() << " ";
		}

		string line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);

		file << line_string << endl;
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	Points3D_Mutex.unlock();
	return true;
}
void CModel::WriteVRML(string ImagePath, string Points3DPath, double ImageScale, Eigen::Vector3d ImageRGB)
{
	ofstream images_file(ImagePath, ios::trunc);
	CHECK(images_file.is_open()) << ImagePath;

	double six = ImageScale * 0.15;
	double siy = ImageScale * 0.1;

	vector<Eigen::Vector3d> points;
	points.emplace_back(-six, -siy, six * 1.0 * 2.0);
	points.emplace_back(+six, -siy, six * 1.0 * 2.0);
	points.emplace_back(+six, +siy, six * 1.0 * 2.0);
	points.emplace_back(-six, +siy, six * 1.0 * 2.0);
	points.emplace_back(0, 0, 0);
	points.emplace_back(-six / 3.0, -siy / 3.0, six * 1.0 * 2.0);
	points.emplace_back(+six / 3.0, -siy / 3.0, six * 1.0 * 2.0);
	points.emplace_back(+six / 3.0, +siy / 3.0, six * 1.0 * 2.0);
	points.emplace_back(-six / 3.0, +siy / 3.0, six * 1.0 * 2.0);

	lock(Images_Mutex, Points3D_Mutex);
	for (auto& pair : Images)
	{
		if (!pair.second.IsRegistered())
		{
			continue;
		}
		images_file << "Shape{\n";
		images_file << " appearance Appearance {\n";
		images_file << "  material DEF Default-ffRffGffB Material {\n";
		images_file << "  ambientIntensity 0\n";
		images_file << "  diffuseColor " << " " << ImageRGB(0) << " " << ImageRGB(1) << " " << ImageRGB(2) << "\n";
		images_file << "  emissiveColor 0.1 0.1 0.1 } }\n";
		images_file << " geometry IndexedFaceSet {\n";
		images_file << " solid FALSE \n";
		images_file << " colorPerVertex TRUE \n";
		images_file << " ccw TRUE \n";

		images_file << " coord Coordinate {\n";
		images_file << " point [\n";

		Eigen::Transform<double, 3, Eigen::Affine> transform;
		transform.matrix().topLeftCorner<3, 4>() = pair.second.InverseProjectionMatrix();

		// Move camera base model to camera pose.
		for (size_t i = 0; i < points.size(); i++)
		{
			const Eigen::Vector3d point = transform * points[i];
			images_file << point(0) << " " << point(1) << " " << point(2) << "\n";
		}

		images_file << " ] }\n";

		images_file << "color Color {color [\n";
		for (size_t p = 0; p < points.size(); p++)
		{
			images_file << " " << ImageRGB(0) << " " << ImageRGB(1) << " " << ImageRGB(2) << "\n";
		}

		images_file << "\n] }\n";

		images_file << "coordIndex [\n";
		images_file << " 0, 1, 2, 3, -1\n";
		images_file << " 5, 6, 4, -1\n";
		images_file << " 6, 7, 4, -1\n";
		images_file << " 7, 8, 4, -1\n";
		images_file << " 8, 5, 4, -1\n";
		images_file << " \n] \n";

		images_file << " texCoord TextureCoordinate { point [\n";
		images_file << "  1 1,\n";
		images_file << "  0 1,\n";
		images_file << "  0 0,\n";
		images_file << "  1 0,\n";
		images_file << "  0 0,\n";
		images_file << "  0 0,\n";
		images_file << "  0 0,\n";
		images_file << "  0 0,\n";
		images_file << "  0 0,\n";

		images_file << " ] }\n";
		images_file << "} }\n";
	}
	ofstream points3D_file(Points3DPath, ios::trunc);
	CHECK(points3D_file.is_open()) << Points3DPath;
	points3D_file << "#VRML V2.0 utf8\n";
	points3D_file << "Background { skyColor [1.0 1.0 1.0] } \n";
	points3D_file << "Shape{ appearance Appearance {\n";
	points3D_file << " material Material {emissiveColor 1 1 1} }\n";
	points3D_file << " geometry PointSet {\n";
	points3D_file << " coord Coordinate {\n";
	points3D_file << "  point [\n";
	for (auto& pair : Points3D)
	{
		points3D_file << pair.second.XYZ()(0) << ", ";
		points3D_file << pair.second.XYZ()(1) << ", ";
		points3D_file << pair.second.XYZ()(2) << endl;
	}
	points3D_file << " ] }\n";
	points3D_file << " color Color { color [\n";
	for (auto& pair : Points3D)
	{
		points3D_file << pair.second.Color(0) / 255.0 << ", ";
		points3D_file << pair.second.Color(1) / 255.0 << ", ";
		points3D_file << pair.second.Color(2) / 255.0 << endl;
	}
	points3D_file << " ] } } }\n";
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}


void CModel::SetObservationAsTriangulated(size_t ImageID, size_t Point2DID, bool IsContinuedPoint3D)
{
	if (!CorresGraph)return;

	lock(Images_Mutex, ImagePairs_Mutex);

	CHECK(Images[ImageID].IsRegistered());
	CHECK(Images[ImageID].Point2D(Point2DID).HasPoint3D());
	vector<CorrespondenceGraph::Correspondence> corrs = CorresGraph->FindCorrespondences(ImageID, Point2DID);
	for (CorrespondenceGraph::Correspondence& corr : corrs)
	{
		Images[corr.image_id].IncrementCorrespondenceHasPoint3D(corr.point2D_idx);
		// Update number of shared 3D points between image pairs and make sure to only count the correspondences once (not twice forward and backward).
		if (Images[ImageID].Point2D(Point2DID).Point3DId() == Images[corr.image_id].Point2D(corr.point2D_idx).Point3DId() && (IsContinuedPoint3D || ImageID < corr.image_id))
		{
			if (ImageID < corr.image_id)
			{
				ImagePairs[make_pair(ImageID, corr.image_id)].TriangulatedCorrsNum++;
				CHECK_LE(ImagePairs[make_pair(ImageID, corr.image_id)].TriangulatedCorrsNum, ImagePairs[make_pair(ImageID, corr.image_id)].TotalCorrsNum) << "The correspondence graph graph must not contain duplicate matches";
			}
			else
			{
				ImagePairs[make_pair(corr.image_id, ImageID)].TriangulatedCorrsNum++;
				CHECK_LE(ImagePairs[make_pair(corr.image_id, ImageID)].TriangulatedCorrsNum, ImagePairs[make_pair(corr.image_id, ImageID)].TotalCorrsNum) << "The correspondence graph graph must not contain duplicate matches";
			}

		}
	}
	Images_Mutex.unlock();
	ImagePairs_Mutex.unlock();
}
void CModel::ResetTriObservations(size_t ImageID, size_t Point2DID, bool IsDeletedPoint3D)
{
	if (!CorresGraph)return;

	lock(Images_Mutex, ImagePairs_Mutex);
	CHECK(Images[ImageID].IsRegistered());
	CHECK(Images[ImageID].Point2D(Point2DID).HasPoint3D());
	vector<CorrespondenceGraph::Correspondence> corrs = CorresGraph->FindCorrespondences(ImageID, Point2DID);

	for (CorrespondenceGraph::Correspondence& corr : corrs)
	{
		Images[corr.image_id].DecrementCorrespondenceHasPoint3D(corr.point2D_idx);
		// Update number of shared 3D points between image pairs and make sure to only count the correspondences once (not twice forward and backward).
		if (Images[ImageID].Point2D(Point2DID).Point3DId() == Images[corr.image_id].Point2D(corr.point2D_idx).Point3DId() && (!IsDeletedPoint3D|| ImageID < corr.image_id))
		{
			if (ImageID < corr.image_id)
			{
				if (ImagePairs[make_pair(ImageID, corr.image_id)].TriangulatedCorrsNum == 0)continue;
				//CHECK_GT(ImagePairs[make_pair(ImageID, corr.image_id)].TriangulatedCorrsNum, 0) << "The scene graph graph must not contain duplicate matches";
				ImagePairs[make_pair(ImageID, corr.image_id)].TriangulatedCorrsNum--;
			}
			else
			{
				if (ImagePairs[make_pair(corr.image_id, ImageID)].TriangulatedCorrsNum == 0)continue;
				//CHECK_GT(ImagePairs[make_pair(corr.image_id, ImageID)].TriangulatedCorrsNum, 0) << "The scene graph graph must not contain duplicate matches";
				ImagePairs[make_pair(corr.image_id, ImageID)].TriangulatedCorrsNum--;
			}
		}
	}
	Images_Mutex.unlock();
	ImagePairs_Mutex.unlock();
}
void CModel::CalcBoundsAndCentroid(double p0, double p1, bool IsUseImages, tuple<Eigen::Vector3d, Eigen::Vector3d, Eigen::Vector3d>& re)
{
	CHECK_GE(p0, 0);
	CHECK_LE(p0, 1);
	CHECK_GE(p1, 0);
	CHECK_LE(p1, 1);
	CHECK_LE(p0, p1);

	size_t ElementsNum = (IsUseImages ? GetModelRegImagesNum() : GetModelPoints3DNum());
	if (ElementsNum == 0)
	{
		re = make_tuple(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(0, 0, 0));
		return;
	}

	vector<float> CorrdsX, CorrdsY, CorrdsZ; // Coordinates of image centers or point locations.
	CorrdsX.reserve(ElementsNum);
	CorrdsY.reserve(ElementsNum);
	CorrdsZ.reserve(ElementsNum);

	if (IsUseImages)
	{
		lock(Images_Mutex, RegImageIDs_Mutex);
		for (size_t RegImageID : RegImageIDs)
		{
			Eigen::Vector3d ProjectCenter = Images[RegImageID].ProjectionCenter();
			CorrdsX.push_back((float)ProjectCenter(0));
			CorrdsY.push_back((float)ProjectCenter(1));
			CorrdsZ.push_back((float)ProjectCenter(2));
		}
		Images_Mutex.unlock();
		RegImageIDs_Mutex.unlock();
	}
	else
	{
		Points3D_Mutex.lock();
		for (auto& pair : Points3D)
		{
			CorrdsX.push_back((float)pair.second.X());
			CorrdsY.push_back((float)pair.second.Y());
			CorrdsZ.push_back((float)pair.second.Z());
		}
		Points3D_Mutex.unlock();
	}

	// Determine robust bounding box and mean.
	sort(CorrdsX.begin(), CorrdsX.end());
	sort(CorrdsY.begin(), CorrdsY.end());
	sort(CorrdsZ.begin(), CorrdsZ.end());

	size_t P0 = (CorrdsX.size() > 3 ? p0 * (CorrdsX.size() - 1) : 0);
	size_t P1 = (CorrdsX.size() > 3 ? p1 * (CorrdsX.size() - 1) : CorrdsX.size() - 1);

	Eigen::Vector3d bbox_min(CorrdsX[P0], CorrdsY[P0], CorrdsZ[P0]);
	Eigen::Vector3d bbox_max(CorrdsX[P1], CorrdsY[P1], CorrdsZ[P1]);

	Eigen::Vector3d mean_coord(0, 0, 0);
	for (size_t i = P0; i <= P1; i++)
	{
		mean_coord(0) += CorrdsX[i];
		mean_coord(1) += CorrdsY[i];
		mean_coord(2) += CorrdsZ[i];
	}
	mean_coord /= P1 - P0 + 1;

	re = make_tuple(bbox_min, bbox_max, mean_coord);
}
size_t CModel::FilterSmallTriAnglePoints3D(double MinTriAngle, const unordered_set<size_t>& Points3DIDs)
{
	size_t FilteredNum = 0;
	double MinTriAngle_Rad = DegToRad(MinTriAngle);

	EIGEN_STL_UMAP(size_t, Eigen::Vector3d) ProjectCenters;
	for (size_t Point3DID : Points3DIDs)
	{
		if (!IsModelExistPoint3D(Point3DID))continue;

		// Calculate triangulation angle for all pairwise combinations of image
		// poses in the track. Only delete point if none of the combinations
		// has a sufficient triangulation angle.
		bool IsKeepPoint = false;
		lock(Points3D_Mutex, Images_Mutex);
		for (size_t i1 = 0; i1 < Points3D[Point3DID].Track().Length(); i1++)
		{
			size_t ImageID1 = Points3D[Point3DID].Track().Element(i1).image_id;
			Eigen::Vector3d ProjectCenter1;
			if (ProjectCenters.count(ImageID1))
			{
				ProjectCenter1 = ProjectCenters[ImageID1];
			}
			else
			{
				ProjectCenter1 = Images[ImageID1].ProjectionCenter();
				ProjectCenters.emplace(ImageID1, ProjectCenter1);
			}
			for (size_t i2 = 0; i2 < i1; i2++)
			{
				size_t ImageID2 = Points3D[Point3DID].Track().Element(i2).image_id;
				Eigen::Vector3d ProjectCenter2 = ProjectCenters[ImageID2];
				double TriAngle = CalculateTriangulationAngle(ProjectCenter1, ProjectCenter2, Points3D[Point3DID].XYZ());
				if (TriAngle >= MinTriAngle_Rad)
				{
					IsKeepPoint = true;
					break;
				}
			}
			if (IsKeepPoint)
			{
				break;
			}
		}
		Points3D_Mutex.unlock();
		Images_Mutex.unlock();
		if (!IsKeepPoint)
		{
			FilteredNum++;
			DeletePoint3D(Point3DID);
		}
	}
	return FilteredNum;
}
size_t CModel::FilterLargeReprojectErrorPoints3D(double MaxReprojectError, const unordered_set<size_t>& Points3DIDs)
{
	double MaxSquaredReprojectError = MaxReprojectError * MaxReprojectError;
	size_t FilteredNum = 0;
	for (size_t Point3DID : Points3DIDs)
	{
		if (!IsModelExistPoint3D(Point3DID))continue;

		lock(Points3D_Mutex, Images_Mutex, Cameras_Mutex);
		if (Points3D[Point3DID].Track().Length() < 2)
		{
			Points3D_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();

			FilteredNum += Points3D[Point3DID].Track().Length();
			DeletePoint3D(Point3DID);
			continue;
		}
		double ReprojectErrorSum = 0;
		vector<TrackElement> DeleteTrackElement;

		for (TrackElement& element : Points3D[Point3DID].Track().Elements())
		{
			size_t ImageID = element.image_id;
			double SquaredReprojectError = CalculateSquaredReprojectionError(Images[ImageID].Point2D(element.point2D_idx).XY(), Points3D[Point3DID].XYZ(), Images[ImageID].Qvec(), Images[ImageID].Tvec(), Cameras[Images[ImageID].CameraId()]);
			if (SquaredReprojectError > MaxSquaredReprojectError)
			{
				DeleteTrackElement.push_back(element);
			}
			else
			{
				ReprojectErrorSum += sqrt(SquaredReprojectError);
			}
		}

		if (DeleteTrackElement.size() >= Points3D[Point3DID].Track().Length() - 1)
		{
			FilteredNum += Points3D[Point3DID].Track().Length();

			Points3D_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();

			DeletePoint3D(Point3DID);
		}
		else
		{
			Points3D_Mutex.unlock();
			Images_Mutex.unlock();
			Cameras_Mutex.unlock();

			FilteredNum += DeleteTrackElement.size();
			for (TrackElement& element : DeleteTrackElement)
			{
				DeleteObservation(element.image_id, element.point2D_idx);
			}

			Points3D_Mutex.lock();
			Points3D[Point3DID].SetError(ReprojectErrorSum / Points3D[Point3DID].Track().Length());
			Points3D_Mutex.unlock();
		}
	}
	return FilteredNum;
}
bool CModel::CalcModelsAlignment(CModel& SrcModel, double MinInlierObservationsNum, double MaxReprojectError, Eigen::Matrix3x4d* Alignment)
{
	CHECK_GE(MinInlierObservationsNum, 0.0);
	CHECK_LE(MinInlierObservationsNum, 1.0);

	RANSACOptions ransac_options;
	ransac_options.max_error = 1.0 - MinInlierObservationsNum;
	ransac_options.min_inlier_ratio = 0.2;

	LORANSAC<ModelAlignmentEstimator, ModelAlignmentEstimator> ransac(ransac_options);
	ransac.estimator.SetMaxReprojError(MaxReprojectError);
	ransac.estimator.SetModels(&SrcModel, this);
	ransac.local_estimator.SetMaxReprojError(MaxReprojectError);
	ransac.local_estimator.SetModels(&SrcModel, this);

	vector<size_t> CommonImageIDs;
	SrcModel.FindCommonRegImages(CommonImageIDs, *this);
	if (CommonImageIDs.size() < 3)
	{
#ifdef OUTPUTLOG_MODE
		qDebug() << "CommonImageIDs.size()<3";
#endif

		return false;
	}

	vector<const Image*> SrcImages(CommonImageIDs.size());
	vector<const Image*> RefImages(CommonImageIDs.size());
	for (size_t i = 0; i < CommonImageIDs.size(); i++)
	{
		Image* srcimage = new Image();
		SrcModel.GetModelImage(*srcimage, CommonImageIDs[i]);
		SrcImages[i] = srcimage;

		Image* refimage = new Image();
		GetModelImage(*refimage, CommonImageIDs[i]);
		RefImages[i] = refimage;
	}
	auto report = ransac.Estimate(SrcImages, RefImages);
	for (size_t i = 0; i < CommonImageIDs.size(); i++)
	{
		delete SrcImages[i];
		delete RefImages[i];
	}
	if (report.success)
	{
		*Alignment = report.model;
		return true;
	}
#ifdef OUTPUTLOG_MODE
	qDebug() << "report.success==false";
#endif
	return false;
}

void CModel::ReadCamerasText(string& Path)
{
	Cameras_Mutex.lock();
	Cameras.clear();
	ifstream file(Path);
	CHECK(file.is_open()) << Path;

	string line, item;
	while (getline(file, line))
	{
		StringTrim(&line);
		if (line.empty() || line[0] == '#')
		{
			continue;
		}
		stringstream line_stream(line);

		Camera camera;
		// ID
		getline(line_stream, item, ' ');
		camera.SetCameraId(stoul(item));

		// MODEL
		getline(line_stream, item, ' ');
		camera.SetModelIdFromName(item);

		// WIDTH
		getline(line_stream, item, ' ');
		camera.SetWidth(stoll(item));

		// HEIGHT
		getline(line_stream, item, ' ');
		camera.SetHeight(stoll(item));

		// PARAMS
		camera.Params().clear();
		while (!line_stream.eof())
		{
			getline(line_stream, item, ' ');
			camera.Params().push_back(stold(item));
		}

		CHECK(camera.VerifyParams());

		Cameras.emplace(camera.CameraId(), camera);
	}
	Cameras_Mutex.lock();
}
void CModel::ReadImagesText(string& Path)
{
	lock(Images_Mutex, RegImageIDs_Mutex, Cameras_Mutex);
	Images.clear();
	ifstream file(Path);
	CHECK(file.is_open()) << Path;

	string line, item;
	while (getline(file, line))
	{
		StringTrim(&line);

		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		stringstream line_stream1(line);

		// ID
		getline(line_stream1, item, ' ');
		size_t image_id = stoul(item);

		Image image;
		image.SetImageId(image_id);

		image.SetRegistered(true);
		RegImageIDs.push_back(image_id);

		// QVEC (qw, qx, qy, qz)
		getline(line_stream1, item, ' ');
		image.Qvec(0) = stold(item);

		getline(line_stream1, item, ' ');
		image.Qvec(1) = stold(item);

		getline(line_stream1, item, ' ');
		image.Qvec(2) = stold(item);

		getline(line_stream1, item, ' ');
		image.Qvec(3) = stold(item);

		image.NormalizeQvec();

		// TVEC
		getline(line_stream1, item, ' ');
		image.Tvec(0) = stold(item);

		getline(line_stream1, item, ' ');
		image.Tvec(1) = stold(item);

		getline(line_stream1, item, ' ');
		image.Tvec(2) = stold(item);

		// CAMERA_ID
		getline(line_stream1, item, ' ');
		image.SetCameraId(stoul(item));

		// NAME
		getline(line_stream1, item, ' ');
		image.SetName(item);

		// POINTS2D
		if (!getline(file, line))
		{
			break;
		}

		StringTrim(&line);
		stringstream line_stream2(line);

		vector<Eigen::Vector2d> points2D;
		vector<point3D_t> point3D_ids;

		if (!line.empty())
		{
			while (!line_stream2.eof())
			{
				Eigen::Vector2d point;

				getline(line_stream2, item, ' ');
				point.x() = stold(item);

				getline(line_stream2, item, ' ');
				point.y() = stold(item);

				points2D.push_back(point);

				getline(line_stream2, item, ' ');
				if (item == "-1")
				{
					point3D_ids.push_back(kInvalidPoint3DId);
				}
				else
				{
					point3D_ids.push_back(stoll(item));
				}
			}
		}
		image.SetUp(Cameras[image.CameraId()]);
		image.SetPoints2D(points2D);

		for (point2D_t point2D_idx = 0; point2D_idx < image.NumPoints2D(); point2D_idx++)
		{
			if (point3D_ids[point2D_idx] != kInvalidPoint3DId)
			{
				image.SetPoint3DForPoint2D(point2D_idx, point3D_ids[point2D_idx]);
			}
		}
		Images.emplace(image.ImageId(), image);
	}
	Images_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
	Cameras_Mutex.unlock();
}
void CModel::ReadPoints3DText(string& Path)
{
	lock(Points3D_Mutex, AddedPoints3DsNum_Mutex);
	Points3D.clear();

	ifstream file(Path);
	CHECK(file.is_open()) << Path;

	string line, item;
	while (getline(file, line))
	{
		StringTrim(&line);

		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		stringstream line_stream(line);

		// ID
		getline(line_stream, item, ' ');
		size_t point3D_id = stoll(item);

		// Make sure, that we can add new 3D points after reading 3D points without overwriting existing 3D points.
		AddedPoints3DsNum = max(AddedPoints3DsNum, point3D_id);

		Point3D point3D;

		// XYZ
		getline(line_stream, item, ' ');
		point3D.XYZ(0) = stold(item);

		getline(line_stream, item, ' ');
		point3D.XYZ(1) = stold(item);

		getline(line_stream, item, ' ');
		point3D.XYZ(2) = stold(item);

		// Color
		getline(line_stream, item, ' ');
		point3D.Color(0) = static_cast<uint8_t>(stoi(item));

		getline(line_stream, item, ' ');
		point3D.Color(1) = static_cast<uint8_t>(stoi(item));

		getline(line_stream, item, ' ');
		point3D.Color(2) = static_cast<uint8_t>(stoi(item));

		// ERROR
		getline(line_stream, item, ' ');
		point3D.SetError(stold(item));

		// TRACK
		while (!line_stream.eof())
		{
			TrackElement track_el;

			getline(line_stream, item, ' ');
			StringTrim(&item);
			if (item.empty())
			{
				break;
			}
			track_el.image_id = stoul(item);

			getline(line_stream, item, ' ');
			track_el.point2D_idx = stoul(item);

			point3D.Track().AddElement(track_el);
		}

		point3D.Track().Compress();

		Points3D.emplace(point3D_id, point3D);
	}
	Points3D_Mutex.unlock();
	AddedPoints3DsNum_Mutex.unlock();
}
void CModel::ReadCamerasBinary(string& Path)
{
	ifstream file(Path, ios::binary);
	CHECK(file.is_open()) << Path;
	Cameras_Mutex.lock();
	Cameras.clear();
	size_t num_cameras = ReadBinaryLittleEndian<uint64_t>(&file);

	for (size_t i = 0; i < num_cameras; ++i)
	{
		Camera camera;
		camera.SetCameraId(ReadBinaryLittleEndian<camera_t>(&file));
		camera.SetModelId(ReadBinaryLittleEndian<int>(&file));
		camera.SetWidth(ReadBinaryLittleEndian<uint64_t>(&file));
		camera.SetHeight(ReadBinaryLittleEndian<uint64_t>(&file));
		ReadBinaryLittleEndian<double>(&file, &camera.Params());
		CHECK(camera.VerifyParams());
		Cameras.emplace(camera.CameraId(), camera);
	}
	Cameras_Mutex.unlock();
}
void CModel::ReadImagesBinary(string& Path)
{
	ifstream file(Path, ios::binary);
	CHECK(file.is_open()) << Path;

	lock(Images_Mutex, Cameras_Mutex, RegImageIDs_Mutex);
	size_t num_reg_images = ReadBinaryLittleEndian<uint64_t>(&file);
	for (size_t i = 0; i < num_reg_images; ++i)
	{
		Image image;

		image.SetImageId(ReadBinaryLittleEndian<image_t>(&file));

		image.Qvec(0) = ReadBinaryLittleEndian<double>(&file);
		image.Qvec(1) = ReadBinaryLittleEndian<double>(&file);
		image.Qvec(2) = ReadBinaryLittleEndian<double>(&file);
		image.Qvec(3) = ReadBinaryLittleEndian<double>(&file);
		image.NormalizeQvec();

		image.Tvec(0) = ReadBinaryLittleEndian<double>(&file);
		image.Tvec(1) = ReadBinaryLittleEndian<double>(&file);
		image.Tvec(2) = ReadBinaryLittleEndian<double>(&file);

		image.SetCameraId(ReadBinaryLittleEndian<camera_t>(&file));

		char name_char;
		do
		{
			file.read(&name_char, 1);
			if (name_char != '\0')
			{
				image.Name() += name_char;
			}
		} while (name_char != '\0');

		size_t num_points2D = ReadBinaryLittleEndian<uint64_t>(&file);

		vector<Eigen::Vector2d> points2D;
		points2D.reserve(num_points2D);
		vector<point3D_t> point3D_ids;
		point3D_ids.reserve(num_points2D);
		for (size_t j = 0; j < num_points2D; ++j)
		{
			double x = ReadBinaryLittleEndian<double>(&file);
			double y = ReadBinaryLittleEndian<double>(&file);
			points2D.emplace_back(x, y);
			point3D_ids.push_back(ReadBinaryLittleEndian<point3D_t>(&file));
		}
		image.SetUp(Cameras[image.CameraId()]);
		image.SetPoints2D(points2D);

		for (point2D_t point2D_idx = 0; point2D_idx < image.NumPoints2D(); ++point2D_idx)
		{
			if (point3D_ids[point2D_idx] != kInvalidPoint3DId)
			{
				image.SetPoint3DForPoint2D(point2D_idx, point3D_ids[point2D_idx]);
			}
		}

		image.SetRegistered(true);
		RegImageIDs.push_back(image.ImageId());
		Images.emplace(image.ImageId(), image);
	}
	Images_Mutex.unlock();
	Cameras_Mutex.unlock();
	RegImageIDs_Mutex.unlock();
}
void CModel::ReadPoints3DBinary(string& Path)
{
	ifstream file(Path, ios::binary);
	CHECK(file.is_open()) << Path;
	lock(Points3D_Mutex, AddedPoints3DsNum_Mutex);
	size_t num_points3D = ReadBinaryLittleEndian<uint64_t>(&file);
	for (size_t i = 0; i < num_points3D; ++i)
	{
		Point3D point3D;

		point3D_t point3D_id = ReadBinaryLittleEndian<point3D_t>(&file);
		AddedPoints3DsNum = max(AddedPoints3DsNum, point3D_id);

		point3D.XYZ()(0) = ReadBinaryLittleEndian<double>(&file);
		point3D.XYZ()(1) = ReadBinaryLittleEndian<double>(&file);
		point3D.XYZ()(2) = ReadBinaryLittleEndian<double>(&file);
		point3D.Color(0) = ReadBinaryLittleEndian<uint8_t>(&file);
		point3D.Color(1) = ReadBinaryLittleEndian<uint8_t>(&file);
		point3D.Color(2) = ReadBinaryLittleEndian<uint8_t>(&file);
		point3D.SetError(ReadBinaryLittleEndian<double>(&file));

		const size_t track_length = ReadBinaryLittleEndian<uint64_t>(&file);
		for (size_t j = 0; j < track_length; ++j)
		{
			image_t image_id = ReadBinaryLittleEndian<image_t>(&file);
			point2D_t point2D_idx = ReadBinaryLittleEndian<point2D_t>(&file);
			point3D.Track().AddElement(image_id, point2D_idx);
		}
		point3D.Track().Compress();

		Points3D.emplace(point3D_id, point3D);
	}
	Points3D_Mutex.unlock();
	AddedPoints3DsNum_Mutex.unlock();
}

void CModel::WriteCamerasText(string& Path)
{
	ofstream file(Path, ios::trunc);
	CHECK(file.is_open()) << Path;
	Cameras_Mutex.lock();

	// Ensure that we don't loose any precision by storing in text.
	file.precision(17);

	file << "# Camera list with one line of data per camera:" << endl;
	file << "#   CAMERA_ID, MODEL, WIDTH, HEIGHT, PARAMS[]" << endl;
	file << "# Number of cameras: " << Cameras.size() << endl;

	for (const auto& camera : Cameras)
	{
		ostringstream line;
		line.precision(17);

		line << camera.first << " ";
		line << camera.second.ModelName() << " ";
		line << camera.second.Width() << " ";
		line << camera.second.Height() << " ";

		for (const double param : camera.second.Params())
		{
			line << param << " ";
		}

		string line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);

		file << line_string << endl;
	}
	Cameras_Mutex.unlock();
}
void CModel::WriteImagesText(string& Path)
{
	ofstream file(Path, ios::trunc);
	CHECK(file.is_open()) << Path;

	// Ensure that we don't loose any precision by storing in text.
	file.precision(17);

	file << "# Image list with two lines of data per image:" << endl;
	file << "#   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME" << endl;
	file << "#   POINTS2D[] as (X, Y, POINT3D_ID)" << endl;
	file << "# Number of images: " << GetModelRegImagesNum()
		<< ", mean observations per image: "
		<< GetMeanObservationsNumEachRegImage() << endl;

	Images_Mutex.lock();
	for (auto& image : Images)
	{
		if (!image.second.IsRegistered())
		{
			continue;
		}

		ostringstream line;
		line.precision(17);

		string line_string;

		line << image.first << " ";

		// QVEC (qw, qx, qy, qz)
		Eigen::Vector4d normalized_qvec = NormalizeQuaternion(image.second.Qvec());
		line << normalized_qvec(0) << " ";
		line << normalized_qvec(1) << " ";
		line << normalized_qvec(2) << " ";
		line << normalized_qvec(3) << " ";

		// TVEC
		line << image.second.Tvec(0) << " ";
		line << image.second.Tvec(1) << " ";
		line << image.second.Tvec(2) << " ";

		line << image.second.CameraId() << " ";

		line << image.second.Name();

		file << line.str() << endl;

		line.str("");
		line.clear();

		for (const Point2D& point2D : image.second.Points2D())
		{
			line << point2D.X() << " ";
			line << point2D.Y() << " ";
			if (point2D.HasPoint3D())
			{
				line << point2D.Point3DId() << " ";
			}
			else
			{
				line << -1 << " ";
			}
		}
		line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);
		file << line_string << endl;
	}
	Images_Mutex.unlock();
}
void CModel::WritePoints3DText(string& Path)
{
	ofstream file(Path, ios::trunc);
	CHECK(file.is_open()) << Path;

	// Ensure that we don't loose any precision by storing in text.
	file.precision(17);

	file << "# 3D point list with one line of data per point:" << endl;
	file << "#   POINT3D_ID, X, Y, Z, R, G, B, ERROR, "
		"TRACK[] as (IMAGE_ID, POINT2D_IDX)"
		<< endl;
	file << "# Number of points: " << GetModelPoints3DNum()
		<< ", mean track length: " << GetMeanTrackLength() << endl;

	Points3D_Mutex.lock();

	for (auto& point3D : Points3D)
	{
		file << point3D.first << " ";
		file << point3D.second.XYZ()(0) << " ";
		file << point3D.second.XYZ()(1) << " ";
		file << point3D.second.XYZ()(2) << " ";
		file << static_cast<int>(point3D.second.Color(0)) << " ";
		file << static_cast<int>(point3D.second.Color(1)) << " ";
		file << static_cast<int>(point3D.second.Color(2)) << " ";
		file << point3D.second.Error() << " ";

		ostringstream line;
		line.precision(17);

		for (const auto& track_el : point3D.second.Track().Elements())
		{
			line << track_el.image_id << " ";
			line << track_el.point2D_idx << " ";
		}

		string line_string = line.str();
		line_string = line_string.substr(0, line_string.size() - 1);

		file << line_string << endl;
	}
	Points3D_Mutex.unlock();

}
void CModel::WriteCamerasBinary(string& Path)
{
	ofstream file(Path, std::ios::trunc | std::ios::binary);
	CHECK(file.is_open()) << Path;

	Cameras_Mutex.lock();
	WriteBinaryLittleEndian<uint64_t>(&file, Cameras.size());

	for (auto& camera : Cameras)
	{
		WriteBinaryLittleEndian<camera_t>(&file, camera.first);
		WriteBinaryLittleEndian<int>(&file, camera.second.ModelId());
		WriteBinaryLittleEndian<uint64_t>(&file, camera.second.Width());
		WriteBinaryLittleEndian<uint64_t>(&file, camera.second.Height());
		for (double param : camera.second.Params())
		{
			WriteBinaryLittleEndian<double>(&file, param);
		}
	}
	Cameras_Mutex.unlock();
}
void CModel::WriteImagesBinary(string& Path)
{
	ofstream file(Path, ios::trunc | ios::binary);
	CHECK(file.is_open()) << Path;

	lock(RegImageIDs_Mutex, Images_Mutex);

	WriteBinaryLittleEndian<uint64_t>(&file, RegImageIDs.size());

	for (auto& image : Images)
	{
		if (!image.second.IsRegistered())
		{
			continue;
		}

		WriteBinaryLittleEndian<image_t>(&file, image.first);

		Eigen::Vector4d normalized_qvec = NormalizeQuaternion(image.second.Qvec());
		WriteBinaryLittleEndian<double>(&file, normalized_qvec(0));
		WriteBinaryLittleEndian<double>(&file, normalized_qvec(1));
		WriteBinaryLittleEndian<double>(&file, normalized_qvec(2));
		WriteBinaryLittleEndian<double>(&file, normalized_qvec(3));

		WriteBinaryLittleEndian<double>(&file, image.second.Tvec(0));
		WriteBinaryLittleEndian<double>(&file, image.second.Tvec(1));
		WriteBinaryLittleEndian<double>(&file, image.second.Tvec(2));

		WriteBinaryLittleEndian<camera_t>(&file, image.second.CameraId());

		string name = image.second.Name() + '\0';
		file.write(name.c_str(), name.size());

		WriteBinaryLittleEndian<uint64_t>(&file, image.second.NumPoints2D());
		for (const Point2D& point2D : image.second.Points2D())
		{
			WriteBinaryLittleEndian<double>(&file, point2D.X());
			WriteBinaryLittleEndian<double>(&file, point2D.Y());
			WriteBinaryLittleEndian<point3D_t>(&file, point2D.Point3DId());
		}
	}
	RegImageIDs_Mutex.unlock();
	Images_Mutex.unlock();
}
void CModel::WritePoints3DBinary(string& Path)
{
	ofstream file(Path, ios::trunc | ios::binary);
	CHECK(file.is_open()) << Path;

	Points3D_Mutex.lock();
	WriteBinaryLittleEndian<uint64_t>(&file, Points3D.size());

	for (const auto& point3D : Points3D)
	{
		WriteBinaryLittleEndian<point3D_t>(&file, point3D.first);
		WriteBinaryLittleEndian<double>(&file, point3D.second.XYZ()(0));
		WriteBinaryLittleEndian<double>(&file, point3D.second.XYZ()(1));
		WriteBinaryLittleEndian<double>(&file, point3D.second.XYZ()(2));
		WriteBinaryLittleEndian<uint8_t>(&file, point3D.second.Color(0));
		WriteBinaryLittleEndian<uint8_t>(&file, point3D.second.Color(1));
		WriteBinaryLittleEndian<uint8_t>(&file, point3D.second.Color(2));
		WriteBinaryLittleEndian<double>(&file, point3D.second.Error());

		WriteBinaryLittleEndian<uint64_t>(&file, point3D.second.Track().Length());
		for (const auto& track_el : point3D.second.Track().Elements())
		{
			WriteBinaryLittleEndian<image_t>(&file, track_el.image_id);
			WriteBinaryLittleEndian<point2D_t>(&file, track_el.point2D_idx);
		}
	}
	Points3D_Mutex.unlock();
}

void CModel::BundleAdjustLock()
{
	lock(Cameras_Mutex, Images_Mutex, Points3D_Mutex);
}
Image& CModel::GetReferImage(size_t ImageID)
{
	return Images[ImageID];
}
Camera& CModel::GetReferCamera(size_t CameraID)
{
	return Cameras[CameraID];
}
Point3D& CModel::GetReferPoint3D(size_t Point3DID)
{
	return Points3D[Point3DID];
}
void CModel::BundleAdjustUnLock()
{
	Cameras_Mutex.unlock();
	Images_Mutex.unlock();
	Points3D_Mutex.unlock();
}

CModelManager::CModelManager()
{
	Clear();
}
size_t CModelManager::Size()
{
	//DebugTimer timer(__FUNCTION__);
	Models_Mutex.lock();
	size_t re = Models.size();
	Models_Mutex.unlock();
	return re;
}
CModel CModelManager::Get(size_t index)
{
	DebugTimer timer(__FUNCTION__);
	Models_Mutex.lock();
	if (index >= Models.size())
	{
		Models_Mutex.unlock();
		return CModel();
		//ThrowError("The index of the model is out of range!");
	}
	auto it = Models.begin();
	advance(it, index);
	CModel re(*it);
	Models_Mutex.unlock();
	return re;
}
size_t CModelManager::Add()
{
	DebugTimer timer(__FUNCTION__);
	Models_Mutex.lock();
	size_t re = Models.size();
	Models.push_back(CModel());
	Models_Mutex.unlock();
	return re;
}
void CModelManager::Delete(size_t index)
{
	DebugTimer timer(__FUNCTION__);
	Models_Mutex.lock();
	if (index >= Models.size())
	{
		Models_Mutex.unlock();
		ThrowError("The index of the model is out of range!");
	}
	auto it = Models.begin();
	advance(it, index);
	Models.erase(it);
	Models_Mutex.unlock();
}
void CModelManager::Clear()
{
	Models_Mutex.lock();
	Models.clear();
	Models_Mutex.unlock();
}
size_t CModelManager::Read(string Path)
{
	Models_Mutex.lock();
	size_t re = Models.size();
	Models.push_back(CModel());
	Models.back().Read(Path);
	Models_Mutex.unlock();
	return re;
}
void CModelManager::Write(string Path, OptionManager* options)
{
	Models_Mutex.lock();
	vector<pair<size_t, size_t>> ReconSizes(Models.size());
	size_t i = 0;
	for (auto it = Models.begin(); it != Models.end(); it++, i++)
	{
		ReconSizes[i] = make_pair(i, it->GetModelPoints3DNum());
	}
	sort(ReconSizes.begin(), ReconSizes.end(),
		[](const pair<size_t, size_t>& first,
			const pair<size_t, size_t>& second)
		{
			return first.second > second.second;
		});
	Models_Mutex.unlock();
	for (size_t i = 0; i < ReconSizes.size(); i++)
	{
		string ModelPath = JoinPath(Path, to_string(i));
		CreateDirIfNotExists(ModelPath);
		Get(ReconSizes[i].first).Write(ModelPath);
		if (options)
		{
			options->Write(JoinPath(ModelPath, "project.ini"));
		}
	}
}
void CModelManager::Lock()
{
	Models_Mutex.lock();
}
CModel& CModelManager::GetRefer(size_t index)
{
#ifdef OUTPUTLOG_MODE
	qDebug() << "[GetRefer], index=" << index << ", size=" << Models.size();
#endif
	if (index >= Models.size())
	{
		ThrowError("The index of the model is out of range!");
	}
	auto it = Models.begin();
	advance(it, index);
	return *it;
}
void CModelManager::UnLock()
{
	Models_Mutex.unlock();
}



