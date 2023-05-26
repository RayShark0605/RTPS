#include "FeatureMatcher.h"

using namespace std;
using namespace colmap;

void CFeatureMatcher::SiftMatch(size_t ImageID, vector<size_t>& MatchedImageIDs, size_t& TimeConsuming_MS)
{
	QElapsedTimer Timer;
	Timer.start();

	bool UseGPU = true;
	SiftMatchGPU sift_match_gpu;
	if (!CreateSiftGPUMatcher(*options->sift_matching, &sift_match_gpu)) //创建SIFT_GPU匹配器
	{
		cout << "[Feature matcher] SiftGPU not fully supported! Automatically use CPU to match!" << endl;
		UseGPU = false;
	}

	TwoViewGeometry::Options TwoViewGeometryOptions;
	TwoViewGeometryOptions.min_num_inliers = options->sift_matching->min_num_inliers;
	TwoViewGeometryOptions.ransac_options.max_error = options->sift_matching->max_error;
	TwoViewGeometryOptions.ransac_options.confidence = options->sift_matching->confidence;
	TwoViewGeometryOptions.ransac_options.min_num_trials = options->sift_matching->min_num_trials;
	TwoViewGeometryOptions.ransac_options.max_num_trials = options->sift_matching->max_num_trials;
	TwoViewGeometryOptions.ransac_options.min_inlier_ratio = options->sift_matching->min_inlier_ratio;
	TwoViewGeometryOptions.force_H_use = options->sift_matching->planar_scene;

	atomic<size_t> CPU_TaskCount(0);


#pragma omp parallel for
	for (int i = 0; i < MatchedImageIDs.size(); i++)
	{
		size_t MatchedImageID = MatchedImageIDs[i];
		if (MatchedImageID == ImageID)continue;

		QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
		FeatureDescriptors Descriptors1, Descriptors2;
		CDatabase::GetDescriptors(ImageID, Descriptors1, db);

#ifdef OUTPUTLOG_MODE
		if (Descriptors1.cols() != 128 || Descriptors1.rows() == 0)
		{
			ThrowError("");
		}
#else
		while (Descriptors1.cols() != 128 || Descriptors1.rows() == 0)
		{
			CDatabase::GetDescriptors(ImageID, Descriptors1, db);
		}
#endif


		CDatabase::GetDescriptors(MatchedImageID, Descriptors2, db);

#ifdef OUTPUTLOG_MODE
		if (Descriptors2.cols() != 128 || Descriptors2.rows() == 0)
		{
			ThrowError("");
		}
#else
		while (Descriptors2.cols() != 128 || Descriptors2.rows() == 0)
		{
			CDatabase::GetDescriptors(MatchedImageID, Descriptors2, db);
		}
#endif

		FeatureKeypoints Keypoints1, Keypoints2;
		CDatabase::GetKeypoints(ImageID, Keypoints1, db);

#ifdef OUTPUTLOG_MODE
		if (Keypoints1.size() != Descriptors1.rows())
		{
			ThrowError("");
		}
#else
		while (Keypoints1.size() != Descriptors1.rows())
		{
			CDatabase::GetKeypoints(ImageID, Keypoints1, db);
		}
#endif

		CDatabase::GetKeypoints(MatchedImageID, Keypoints2, db);

#ifdef OUTPUTLOG_MODE
		if (Keypoints2.size() != Descriptors2.rows())
		{
			ThrowError("");
		}
#else
		while (Keypoints2.size() != Descriptors2.rows())
		{
			CDatabase::GetKeypoints(MatchedImageID, Keypoints2, db);
		}
#endif

		vector<Eigen::Vector2d> PointsVectors1(Keypoints1.size()), PointsVectors2(Keypoints2.size());
		for (size_t j = 0; j < Keypoints1.size(); j++)
		{
			PointsVectors1[j] = Eigen::Vector2d(Keypoints1[j].x, Keypoints1[j].y);
		}
		for (size_t j = 0; j < Keypoints2.size(); j++)
		{
			PointsVectors2[j] = Eigen::Vector2d(Keypoints2[j].x, Keypoints2[j].y);
		}

		Camera Camera1, Camera2;
		CDatabase::GetImageCamera(ImageID, Camera1, db);
		CDatabase::GetImageCamera(MatchedImageID, Camera2, db);

		FeatureMatches Matches;
		Matches.resize(0);

		TwoViewGeometry Geometries;
		Geometries.inlier_matches.resize(0);

		if (UseGPU && sift_match_gpu.__matcher)
		{
			if (MatchedImageIDs.size() >= 500)
			{
				QElapsedTimer WaitGPU_Timer;
				WaitGPU_Timer.start();
				bool IsFinished = false;
				while (!Base::GPU_Mutex.try_lock())
				{
					if (WaitGPU_Timer.hasExpired(1000) && CPU_TaskCount.load() < 8)
					{
						CPU_TaskCount++;
						MatchSiftFeaturesCPU(*options->sift_matching, Descriptors1, Descriptors2, &Matches);
						MatchGuidedSiftFeaturesCPU(*options->sift_matching, Keypoints1, Keypoints2, Descriptors1, Descriptors2, &Geometries);
						CPU_TaskCount--;
						IsFinished = true;
						break;
					}
					else
					{
						this_thread::sleep_for(chrono::milliseconds(30));
					}
				}
				if (!IsFinished)
				{
					MatchSiftFeaturesGPU(*options->sift_matching, &Descriptors1, &Descriptors2, &sift_match_gpu, &Matches);
					MatchGuidedSiftFeaturesGPU(*options->sift_matching, &Keypoints1, &Keypoints2, &Descriptors1, &Descriptors2, &sift_match_gpu, &Geometries);
					Base::GPU_Mutex.unlock();
				}
			}
			else
			{
				Base::GPU_Mutex.lock();
				MatchSiftFeaturesGPU(*options->sift_matching, &Descriptors1, &Descriptors2, &sift_match_gpu, &Matches);
				MatchGuidedSiftFeaturesGPU(*options->sift_matching, &Keypoints1, &Keypoints2, &Descriptors1, &Descriptors2, &sift_match_gpu, &Geometries);
				Base::GPU_Mutex.unlock();
			}
		}
		else
		{
			CPU_TaskCount++;
			MatchSiftFeaturesCPU(*options->sift_matching, Descriptors1, Descriptors2, &Matches);
			MatchGuidedSiftFeaturesCPU(*options->sift_matching, Keypoints1, Keypoints2, Descriptors1, Descriptors2, &Geometries);
			CPU_TaskCount--;
		}
		if (Matches.empty())continue;

		CPU_TaskCount++;
#ifdef OUTPUTLOG_MODE
		Geometries.Estimate(Camera1, PointsVectors1, Camera2, PointsVectors2, Matches, TwoViewGeometryOptions);
#else
		if (options->sift_matching->multiple_models)
		{
			Geometries.EstimateMultiple(Camera1, PointsVectors1, Camera2, PointsVectors2, Matches, TwoViewGeometryOptions);
		}
		else
		{
			Geometries.Estimate(Camera1, PointsVectors1, Camera2, PointsVectors2, Matches, TwoViewGeometryOptions);
		}
#endif
		
		
		CPU_TaskCount--;

		if (Matches.size() > options->sift_matching->max_num_matches)
		{
			Matches.resize(options->sift_matching->max_num_matches);
			Matches.shrink_to_fit();
		}

		CDatabase::AddMatches(ImageID, MatchedImageID, Matches, db);
		if (!Geometries.inlier_matches.empty())
		{
			if (Geometries.inlier_matches.size() > options->sift_matching->max_num_matches)
			{
				Geometries.inlier_matches.resize(options->sift_matching->max_num_matches);
				Geometries.inlier_matches.shrink_to_fit();
			}
			CDatabase::AddTwoViewGeometries(ImageID, MatchedImageID, Geometries, db);
		}
		//ReleaseDatabaseConnect(db);
	}
	TimeConsuming_MS = Timer.elapsed();
}

CExhaustiveMatcher::CExhaustiveMatcher(OptionManager* options)
{
	this->options = options;
}
bool CExhaustiveMatcher::Match(string ImagePath)
{
	cout << StringPrintf("Start feature matching on image %s", GetFileName(ImagePath)) << endl;
	string ImageName = GetFileName(ImagePath);

	QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
	if (!CDatabase::IsExistImage(ImageName, db))
	{
		cout << StringPrintf("[Feature matcher] Error! Image %s does not exist in the database!", ImageName);
		ReleaseDatabaseConnect(db);
		return false;
	}
	size_t CurrentImageID = CDatabase::GetImageID(ImageName, db);
	if (!CDatabase::GetImageKeypointsNum(CurrentImageID, db))
	{
		cout << StringPrintf("[Feature matcher] Error! Image %s has no feature points!", ImageName);
		ReleaseDatabaseConnect(db);
		return false;
	}
	CHECK(options->exhaustive_matching->Check());
	CHECK(options->sift_matching->Check());

	size_t ImageNum = CDatabase::GetImagesNum(db);
	vector<size_t> MatchImagePairs, WaitImagePairs;
	MatchImagePairs.reserve(ImageNum);
	WaitImagePairs.reserve(ImageNum);

	for (size_t MatchedImageID = 0; MatchedImageID < ImageNum; MatchedImageID++)
	{
		if (MatchedImageID == CurrentImageID)continue;
		if (CDatabase::GetMatchesNum(CurrentImageID, MatchedImageID, db) > 0)
		{
			continue;
		}
		if (CDatabase::GetImageDescriptorsNum(MatchedImageID, db))
		{
			MatchImagePairs.push_back(MatchedImageID);
		}
		else
		{
			WaitImagePairs.push_back(MatchedImageID);
		}
	}

	size_t TimeConsuming1 = 0, TimeConsuming2 = 0;
	SiftMatch(CurrentImageID, MatchImagePairs, TimeConsuming1);

	if (!WaitImagePairs.empty())
	{
		while (true) //等待"需要等待提取特征完成, 才能开始匹配的影像对"中的影像都已被提取特征
		{
			bool IsReady = true;
			for (size_t WaitImageID : WaitImagePairs)
			{
				if (!CDatabase::GetImageKeypointsNum(WaitImageID, db))
				{
					IsReady = false;
					break;
				}
			}
			if (IsReady)
			{
				break;
			}
			this_thread::sleep_for(chrono::milliseconds(200));
		}
		SiftMatch(CurrentImageID, WaitImagePairs, TimeConsuming2);
	}
	

	float TotalTime = (TimeConsuming1 + TimeConsuming2) / 1000.0;

	//SetImageStatus(ImageName, CImageStatus::Matched);
	cout << StringPrintf("[%.2f s] Image %s matching completed, there are %d images matching it!", TotalTime, ImageName, CDatabase::ExistTwoViewGeometriesImagesNum(CurrentImageID, db)) << endl;
	ReleaseDatabaseConnect(db);
	return true;
}

CRetrievalMatcher::CRetrievalMatcher(OptionManager* options)
{
	this->options = options;
}
bool CRetrievalMatcher::Match(string ImagePath)
{
	return true;
}
bool CRetrievalMatcher::Initialize()
{
	return true;
}






