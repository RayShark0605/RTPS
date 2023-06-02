#include "FeatureExtractor.h"

using namespace std;
using namespace colmap;

CFeatureExtractor::CFeatureExtractor(OptionManager* options)
{
	this->options = options;
}
bool CFeatureExtractor::Extract(string ImagePath)
{
	if (Base::IsQuit)
	{
		return false;
	}
	cout << StringPrintf("Start feature extraction on image %s", GetFileName(ImagePath)) << endl;
	QElapsedTimer timer;
	timer.start();

	QSqlDatabase db = CreateDatabaseConnect(*options->database_path);

	bool UseGPU = true;
	SiftGPU sift_gpu;
	if (!CreateSiftGPUExtractor(*options->sift_extraction, &sift_gpu))
	{
		cout << "[Feature extractor] SiftGPU not fully supported! Automatically use CPU to extract!" << endl;
		UseGPU = false;
	}
	Camera camera;
	Image image;
	Bitmap bitmap;
	int ReadResult = ReadImage(ImagePath, camera, image, bitmap, db);
	if (Base::IsQuit)
	{
		return false;
	}
	if (ReadResult == -2)
	{
		cout << StringPrintf("Image %s read error!", ImagePath.c_str()) << endl;
		ReleaseDatabaseConnect(db);
		return false;
	}
	string ImageName = GetFileName(ImagePath);
	if (ReadResult == -1)
	{
		size_t ImageID = CDatabase::GetImageID(ImageName, db);
		size_t KeypointsNum = CDatabase::GetImageKeypointsNum(ImageID, db);
		cout << StringPrintf("Image [%s, id=%d] already exists and contains %d feature points!", ImageName.c_str(), ImageID, KeypointsNum) << endl;
		ReleaseDatabaseConnect(db);
		return true;
	}
	if (ReadResult == -3)
	{
		cout << StringPrintf("Incorrect camera parameters when reading image %s!", ImageName.c_str()) << endl;
		ReleaseDatabaseConnect(db);
		return false;
	}
	FeatureKeypoints keypoints;
	FeatureDescriptors descriptors;

	bool IsExtractSuccess = false;
	if (Base::IsQuit)
	{
		return false;
	}
	if (options->sift_extraction->estimate_affine_shape || options->sift_extraction->domain_size_pooling)
	{
		IsExtractSuccess = ExtractCovariantSiftFeaturesCPU(*options->sift_extraction, bitmap, &keypoints, &descriptors);
	}
	else if (IsGPUAvailable() && UseGPU)
	{
		Base::GPU_Mutex.lock();
		IsExtractSuccess = ExtractSiftFeaturesGPU(*options->sift_extraction, bitmap, &sift_gpu, &keypoints, &descriptors);
		Base::GPU_Mutex.unlock();
	}
	if (Base::IsQuit)
	{
		return false;
	}
	if (!IsExtractSuccess)
	{
		IsExtractSuccess = ExtractSiftFeaturesCPU(*options->sift_extraction, bitmap, &keypoints, &descriptors);
	}
	if (!IsExtractSuccess)
	{
		cout << StringPrintf("Feature extraction for image %s failed!", ImageName.c_str()) << endl;
		ReleaseDatabaseConnect(db);
		return false;
	}
	if (Base::IsQuit)
	{
		return false;
	}
	ScaleKeypoints(bitmap, camera, keypoints);
	if (keypoints.size() > options->sift_extraction->max_num_features)
	{
		keypoints.resize(options->sift_extraction->max_num_features);
		keypoints.shrink_to_fit();

		//image.Descriptors.block(0, 0, options->sift_extraction->max_num_features, image.Descriptors.cols());
		Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> temp(options->sift_extraction->max_num_features, descriptors.cols());
		temp = descriptors.block(0, 0, options->sift_extraction->max_num_features, descriptors.cols());
		descriptors = temp;
	}
	if (Base::IsQuit)
	{
		return false;
	}
	size_t ImageID = CDatabase::AddImage(image, db);
	CDatabase::AddKeypoints(ImageID, keypoints, db);
	CDatabase::AddDescriptors(ImageID, descriptors, db);

	//SetImageStatus(ImageName, CImageStatus::Extracted);
	size_t Elapsed = timer.elapsed();
	cout << StringPrintf("[%d ms] Image %s successfully extracted %d feature points!", Elapsed, ImageName.c_str(), CDatabase::GetImageKeypointsNum(ImageID, db)) << endl;
	ReleaseDatabaseConnect(db);
	return true;
}
int CFeatureExtractor::ReadImage(string ImagePath, Camera& camera, Image& image, Bitmap& bitmap, QSqlDatabase& db)
{
	camera.SetCameraId(kInvalidCameraId);
	camera.SetModelIdFromName(options->image_reader->camera_model); //设置相机的模型ID
	if (!options->image_reader->camera_params.empty()) //给定了相机参数
	{
		CHECK(camera.SetParamsFromString(options->image_reader->camera_params));
		camera.SetPriorFocalLength(true);
	}
	string ImageName = GetFileName(ImagePath);
	size_t ImageID = INT_MAX;
	image.SetName(ImageName); //设置影像名
	if (CDatabase::IsExistImage(ImageName,db))
	{
		ImageID = CDatabase::GetImageID(ImageName, db);
		if (CDatabase::GetImageKeypointsNum(ImageID, db) > 0)
		{
			return -1;
		}
	}
	if (!bitmap.Read(ImagePath, false))
	{
		return -2;
	}
	string CameraModel;
	bool IsCameraModelValid = bitmap.ExifCameraModel(&CameraModel);
	if (options->image_reader->camera_params.empty())
	{
		double focal_length = 0.0;
		if (bitmap.ExifFocalLength(&focal_length))
		{
			camera.SetPriorFocalLength(true);
		}
		else
		{
			focal_length = options->image_reader->default_focal_length_factor * max(bitmap.Width(), bitmap.Height());
			camera.SetPriorFocalLength(false);
		}
		camera.InitializeWithId(camera.ModelId(), focal_length, bitmap.Width(), bitmap.Height());
	}
	camera.SetWidth(bitmap.Width());
	camera.SetHeight(bitmap.Height());
	if (!camera.VerifyParams())
	{
		return -3;
	}
	if (CDatabase::IsExistCamera(camera, db))
	{
		camera.SetCameraId(CDatabase::GetCameraID(camera, db));
	}
	else
	{
		size_t CurrentCameraNum = CDatabase::GetCamerasNum(db);
		camera.SetCameraId(CurrentCameraNum);
		CDatabase::AddCamera(camera, db);
	}
	image.SetCameraId(camera.CameraId());
	if (!bitmap.ExifLatitude(&image.TvecPrior(0)) || !bitmap.ExifLongitude(&image.TvecPrior(1)) || !bitmap.ExifAltitude(&image.TvecPrior(2)))
	{
		image.TvecPrior().setConstant(std::numeric_limits<double>::quiet_NaN());
	}
	return 1;
}
void CFeatureExtractor::ScaleKeypoints(Bitmap& bitmap, Camera& camera, FeatureKeypoints& keypoints)
{
	if (bitmap.Width() != camera.Width() || bitmap.Height() != camera.Height())
	{
		double ScaleX = camera.Width() * 1.0 / bitmap.Width();
		double ScaleY = camera.Height() * 1.0 / bitmap.Height();
		for (auto& keypoint : keypoints)
		{
			keypoint.Rescale(ScaleX, ScaleY);
		}
	}
}




