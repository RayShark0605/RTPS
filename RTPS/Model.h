#pragma once
#include "Base.h"
#include "BundleAdjuster.h"
#include "Database.h"

class CModel
{
	friend class CBundleAdjuster;
public:
	struct CImagePairStatus
	{
		size_t TriangulatedCorrsNum = 0;
		size_t TotalCorrsNum = 0;
	};
	struct PairHash 
	{
		size_t operator()(const std::pair<size_t, size_t>& p) const 
		{
			return std::hash<size_t>()(p.first) ^ std::hash<size_t>()(p.second);
		}
	};
	typedef std::unordered_map<std::pair<size_t, size_t>, CImagePairStatus, PairHash> CImagePairMapType;
	typedef EIGEN_STL_UMAP(size_t, colmap::Camera) CCameraMapType;
	typedef EIGEN_STL_UMAP(size_t, colmap::Image) CImageMapType;
	typedef EIGEN_STL_UMAP(size_t, colmap::Point3D) CPoint3DMapType;
	
	CModel();
	CModel(const CModel& model);
	void ConvertToReconstruction(colmap::Reconstruction& Model);

	size_t GetModelCamerasNum();
	size_t GetModelImagesNum();
	size_t GetModelRegImagesNum();
	size_t GetModelPoints3DNum();
	size_t GetModelImagePairsNum();

	colmap::Image& GetReferImage(size_t ImageID);
	colmap::Camera& GetReferCamera(size_t CameraID);
	colmap::Point3D& GetReferPoint3D(size_t Point3DID);

	bool IsModelExistCamera(size_t CameraID);
	colmap::Camera GetModelCamera(size_t CameraID);
	void GetModelCamera(colmap::Camera& camera, size_t CameraID);
	void GetModelAllCameras(CCameraMapType& cameras) const;
	void SetCamera(colmap::Camera& camera, size_t CameraID);
	void ModelAddCamera(colmap::Camera& camera);

	bool IsModelExistImage(size_t ImageID);
	bool IsModelExistImage(std::string ImageName);
	colmap::Image GetModelImage(size_t ImageID);
	void GetModelImage(colmap::Image& image, size_t ImageID);
	void GetModelImage(colmap::Image& image, std::string ImageName);
	void GetModelAllImages(CImageMapType& images) const;
	CImageMapType GetModelAllImages();
	void GetModelAllRegImageIDs(std::vector<size_t>& RegImageIDs) const;
	std::vector<size_t> GetAllRegImagesIDs();
	void SetImage(colmap::Image& image, size_t ImageID);
	void ModelAddImage(colmap::Image& image);
	bool IsImageRegistered(size_t ImageID);
	void RegisterImage(size_t ImageID);
	void DeRegisterImage(size_t ImageID);

	bool IsModelExistPoint3D(size_t Point3DID);
	colmap::Point3D GetModelPoint3D(size_t Point3DID);
	void GetModelPoint3D(colmap::Point3D& point, size_t Point3DID);
	void GetModelAllPoint3Ds(CPoint3DMapType& points) const;
	std::unordered_set<size_t> GetModelAllPoint3Ds();
	void GetModelAllPoint3DIDs(std::unordered_set<size_t>& pointIDs);
	size_t ModelAddPoint3D(Eigen::Vector3d& xyz, colmap::Track& track, const Eigen::Vector3ub& color = Eigen::Vector3ub::Zero());
	size_t MergePoint3Ds(size_t PointID1, size_t PointID2);
	void DeletePoint3D(size_t PointID);

	void AddObservation(size_t Point3DID, colmap::TrackElement& element);
	void DeleteObservation(size_t ImageID, size_t Point2DID);

	void DeleteAllPoint3DPoint2D();

	bool IsModelExistImagePair(size_t ImageID1, size_t ImageID2);
	std::unordered_map<colmap::image_pair_t, colmap::Reconstruction::ImagePairStat> GetModelImagePairs();
	CImagePairStatus GetModelImagePairStatus(size_t ImageID1, size_t ImageID2);
	void GetModelAllImagePairs(CImagePairMapType& pairs) const;

	void Load(colmap::DatabaseCache& DbCache);
	void SetUp(const colmap::CorrespondenceGraph* correspondence_graph);
	void TearDown();
	void UpdateImageID();
	bool ExtractColors_SingleImage(size_t ImageID, std::string ImageSaveDir);
	void ExtractColors(std::string ImageSaveDir);
	void Normalize(double Extent = 10.0, double p0 = 0.1, double p1 = 0.9, bool IsUseImages = true);
	void CalcPoint3DsCenter(Eigen::Vector3d& Centroid, double p0 = 0.1, double p1 = 0.9);
	void CalcBoundingBox(std::pair<Eigen::Vector3d, Eigen::Vector3d>& BoundingBox, double p0 = 0.0, double p1 = 1.0);
	void Transform(colmap::SimilarityTransform3& Trans);
	void Crop(std::pair<Eigen::Vector3d, Eigen::Vector3d>& BoundingBox, CModel& CropedModel);

	size_t FilterPoints3D(double MaxReprojectError, double MinTriAngle, const std::unordered_set<size_t>& Point3DIDs);
	size_t FilterPoints3DInImages(double MaxReprojectError, double MinTriAngle, std::unordered_set<size_t>& ImageIDs);
	size_t FilterAllPoints3Ds(double MaxReprojectError, double MinTriAngle);
	size_t FilterNegativeDepthObservations();
	void FilterImages(double MinFocalLenRatio, double MaxFocalLenRatio, double MaxExtraParam, std::vector<size_t>& FilteredImagesID);

	size_t GetObservationsNum();
	double GetMeanTrackLength();
	double GetMeanObservationsNumEachRegImage();
	double GetMeanReprojectError();

	bool Merge(CModel& MergedModel, double MaxReprojectError);
	void FindCommonRegImages(std::vector<size_t>& CommonImagesID, CModel& Model);

	template <bool kEstimateScale = true>
	bool Align(std::vector<std::string>& ImageNames, std::vector<Eigen::Vector3d>& Locations, size_t MinCommonImagesNum, colmap::SimilarityTransform3* tform = nullptr);

	template <bool kEstimateScale = true>
	bool AlignRobust(std::vector<std::string>& ImageNames, std::vector<Eigen::Vector3d>& Locations, size_t MinCommonImagesNum, colmap::RANSACOptions& RANSAC_options, colmap::SimilarityTransform3* tform = nullptr);

	void CreateImageDirs(std::string Path);

	void Read(std::string& Path);
	void ReadText(std::string& Path);
	void ReadBinary(std::string& Path);
	void ReadPLY(std::string& Path);
	void ReadPLY(std::vector<colmap::PlyPoint>& PLY);

	void Write(std::string& Path);
	void WriteText(std::string& Path);
	void WriteBinary(std::string& Path);
	void WritePLY(std::string& Path);
	void WritePLY(std::vector<colmap::PlyPoint>& PLY);
	bool WriteNVM(std::string& Path, bool IsSkipDistortion = false);
	bool WriteCam(std::string& Path, bool IsSkipDistortion = false);
	bool WriteRecon3D(std::string& Path, bool IsSkipDistortion = false);
	bool WriteBundler(std::string& Path, std::string& ListPath, bool IsSkipDistortion = false);
	void WriteVRML(std::string& ImagePath, std::string& Points3DPath, double ImageScale, Eigen::Vector3d& ImageRGB);

	size_t AddedPoints3DsNum;
private:
	const colmap::CorrespondenceGraph* CorresGraph;
	CCameraMapType Cameras;
	CImageMapType Images;
	CPoint3DMapType Points3D;
	CImagePairMapType ImagePairs;
	std::vector<size_t> RegImageIDs;

	mutable std::mutex Cameras_Mutex;
	mutable std::mutex Images_Mutex;
	mutable std::mutex Points3D_Mutex;
	mutable std::mutex ImagePairs_Mutex;
	mutable std::mutex RegImageIDs_Mutex;
	mutable std::mutex AddedPoints3DsNum_Mutex;

	void SetObservationAsTriangulated(size_t ImageID, size_t Point2DID, bool IsContinuedPoint3D);
	void ResetTriObservations(size_t ImageID, size_t Point2DID, bool IsDeletedPoint3D);
	void CalcBoundsAndCentroid(double p0, double p1, bool IsUseImages, std::tuple<Eigen::Vector3d, Eigen::Vector3d, Eigen::Vector3d>& re);
	size_t FilterSmallTriAnglePoints3D(double MinTriAngle, const std::unordered_set<size_t>& Points3DIDs);
	size_t FilterLargeReprojectErrorPoints3D(double MaxReprojectError, const std::unordered_set<size_t>& Points3DIDs);
	bool CalcModelsAlignment(CModel& SrcModel, double MinInlierObservationsNum, double MaxReprojectError, Eigen::Matrix3x4d* Alignment);
	
	void ReadCamerasText(std::string& Path);
	void ReadImagesText(std::string& Path);
	void ReadPoints3DText(std::string& Path);
	void ReadCamerasBinary(std::string& Path);
	void ReadImagesBinary(std::string& Path);
	void ReadPoints3DBinary(std::string& Path);

	void WriteCamerasText(std::string& Path);
	void WriteImagesText(std::string& Path);
	void WritePoints3DText(std::string& Path);
	void WriteCamerasBinary(std::string& Path);
	void WriteImagesBinary(std::string& Path);
	void WritePoints3DBinary(std::string& Path);

	void BundleAdjustLock();
	
	void BundleAdjustUnLock();
};
struct ModelAlignmentEstimator
{
	static const int kMinNumSamples = 3;
	typedef const colmap::Image* X_t;
	typedef const colmap::Image* Y_t;
	typedef Eigen::Matrix3x4d M_t;

	void SetMaxReprojError(double max_reproj_error)
	{
		max_squared_reproj_error_ = max_reproj_error * max_reproj_error;
	}
	void SetModels(CModel* Model1, CModel* Model2)
	{
		CHECK_NOTNULL(Model1);
		CHECK_NOTNULL(Model2);
		Model1_ = Model1;
		Model2_ = Model2;
	}
	std::vector<M_t> Estimate(std::vector<X_t>& images1, std::vector<Y_t>& images2)
	{
		CHECK_GE(images1.size(), 3);
		CHECK_GE(images2.size(), 3);
		CHECK_EQ(images1.size(), images2.size());

		std::vector<Eigen::Vector3d> proj_centers1(images1.size());
		std::vector<Eigen::Vector3d> proj_centers2(images2.size());
		for (size_t i = 0; i < images1.size(); ++i)
		{
			CHECK_EQ(images1[i]->ImageId(), images2[i]->ImageId());
			proj_centers1[i] = images1[i]->ProjectionCenter();
			proj_centers2[i] = images2[i]->ProjectionCenter();
		}
		colmap::SimilarityTransform3 tform12;
		tform12.Estimate(proj_centers1, proj_centers2);
		return { tform12.Matrix().topRows<3>() };
	}
	void Residuals(const std::vector<X_t>& images1, const std::vector<Y_t>& images2, const M_t& alignment12, std::vector<double>* residuals)
	{
		CHECK_EQ(images1.size(), images2.size());
		CHECK_NOTNULL(Model1_);
		CHECK_NOTNULL(Model2_);

		Eigen::Matrix3x4d alignment21 = colmap::SimilarityTransform3(alignment12).Inverse().Matrix().topRows<3>();

		residuals->resize(images1.size());

		for (size_t i = 0; i < images1.size(); ++i)
		{
			CHECK_EQ(images1[i]->ImageId(), images2[i]->ImageId());
			CHECK_EQ(images1[i]->CameraId(), images2[i]->CameraId());
			CHECK_EQ(images1[i]->NumPoints2D(), images2[i]->NumPoints2D());

			Eigen::Matrix3x4d ProjectMatrix1 = images1[i]->ProjectionMatrix();
			Eigen::Matrix3x4d ProjectMatrix2 = images2[i]->ProjectionMatrix();

			size_t InliersNum = 0, CommonPointsNum = 0;
			for (size_t Point2DID = 0; Point2DID < images1[i]->NumPoints2D(); Point2DID++)
			{
				if (!images1[i]->Point2D(Point2DID).HasPoint3D() || !images2[i]->Point2D(Point2DID).HasPoint3D())
				{
					continue;
				}
				CommonPointsNum++;
				// Reproject 3D point in image 1 to image 2.
				colmap::Point3D point = Model1_->GetModelPoint3D(images1[i]->Point2D(Point2DID).Point3DId());
				Eigen::Vector3d xyz12 = alignment12 * point.XYZ().homogeneous();
				if (CalculateSquaredReprojectionError(images2[i]->Point2D(Point2DID).XY(), xyz12, ProjectMatrix2, Model2_->GetModelCamera(images2[i]->CameraId())) > max_squared_reproj_error_)
				{
					continue;
				}

				point = Model2_->GetModelPoint3D(images2[i]->Point2D(Point2DID).Point3DId());
				Eigen::Vector3d xyz21 = alignment21 * point.XYZ().homogeneous();
				if (CalculateSquaredReprojectionError(images1[i]->Point2D(Point2DID).XY(), xyz21, ProjectMatrix1, Model1_->GetModelCamera(images1[i]->CameraId())) > max_squared_reproj_error_)
				{
					continue;
				}

				InliersNum++;
			}
			if (CommonPointsNum == 0)
			{
				(*residuals)[i] = 1.0;
			}
			else
			{
				double NegativeInlierRatio = 1.0 - InliersNum * 1.0 / CommonPointsNum;
				(*residuals)[i] = NegativeInlierRatio * NegativeInlierRatio;
			}
		}
	}
private:
	double max_squared_reproj_error_ = 0.0;
	CModel* Model1_ = nullptr;
	CModel* Model2_ = nullptr;
};
class CModelManager
{
public:
	CModelManager();
	size_t Size();
	CModel Get(size_t index);
	size_t Add();
	void Delete(size_t index);
	void Clear();
	size_t Read(std::string Path);
	void Write(std::string Path, colmap::OptionManager* options);

	void Lock();
	CModel& GetRefer(size_t index);
	void UnLock();
private:
	NON_COPYABLE(CModelManager)
	std::recursive_mutex Models_Mutex;
	std::list<CModel> Models;
};





