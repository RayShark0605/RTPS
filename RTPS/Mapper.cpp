#include "Mapper.h"

using namespace std;
using namespace colmap;

void SortAndAppendNextImages(vector<pair<size_t, float>> image_ranks, vector<size_t>* sorted_images_ids)
{
	std::sort(image_ranks.begin(), image_ranks.end(),
		[](const pair<size_t, float>& image1, const pair<size_t, float>& image2)
		{
			return image1.second > image2.second;
		});

	sorted_images_ids->reserve(sorted_images_ids->size() + image_ranks.size());
	for (const auto& image : image_ranks) {
		sorted_images_ids->push_back(image.first);
	}

	image_ranks.clear();
}
float RankNextImageMaxVisiblePointsNum(Image& image) 
{
	return static_cast<float>(image.NumVisiblePoints3D());
}
float RankNextImageMaxVisiblePointsRatio(Image& image) 
{
	return static_cast<float>(image.NumVisiblePoints3D()) / static_cast<float>(image.NumObservations());
}
float RankNextImageMinUncertainty(Image& image) 
{
	return static_cast<float>(image.Point3DVisibilityScore());
}

bool CMapper::Options::Check()
{
	CHECK_OPTION_GT(init_min_num_inliers, 0);
	CHECK_OPTION_GT(init_max_error, 0.0);
	CHECK_OPTION_GE(init_max_forward_motion, 0.0);
	CHECK_OPTION_LE(init_max_forward_motion, 1.0);
	CHECK_OPTION_GE(init_min_tri_angle, 0.0);
	CHECK_OPTION_GE(init_max_reg_trials, 1);
	CHECK_OPTION_GT(abs_pose_max_error, 0.0);
	CHECK_OPTION_GT(abs_pose_min_num_inliers, 0);
	CHECK_OPTION_GE(abs_pose_min_inlier_ratio, 0.0);
	CHECK_OPTION_LE(abs_pose_min_inlier_ratio, 1.0);
	CHECK_OPTION_GE(local_ba_num_images, 2);
	CHECK_OPTION_GE(local_ba_min_tri_angle, 0.0);
	CHECK_OPTION_GE(min_focal_length_ratio, 0.0);
	CHECK_OPTION_GE(max_focal_length_ratio, min_focal_length_ratio);
	CHECK_OPTION_GE(max_extra_param, 0.0);
	CHECK_OPTION_GE(filter_max_reproj_error, 0.0);
	CHECK_OPTION_GE(filter_min_tri_angle, 0.0);
	CHECK_OPTION_GE(max_reg_trials, 1);
	return true;
}

CMapper::CMapper(DatabaseCache* database_cache)
{
	database_cache_ = database_cache;
	Model_ = nullptr;
	triangulator_ = nullptr;
	num_total_reg_images_ = 0;
	num_shared_reg_images_ = 0;
	prev_init_image_pair_id_ = kInvalidImagePairId;
}
void CMapper::BeginReconstruction(CModel* model)
{
	CHECK(Model_ == nullptr);
	Model_ = model;
	Model_->Load(*database_cache_);
	Model_->SetUp(&database_cache_->CorrespondenceGraph());
	triangulator_ = make_unique<CTriangulator>(&database_cache_->CorrespondenceGraph(), model);

	num_shared_reg_images_ = 0;
	num_reg_images_per_camera_.clear();
	vector<size_t> RegImageIDs;
	Model_->GetModelAllRegImageIDs(RegImageIDs);
	for (const image_t image_id : RegImageIDs)
	{
		RegisterImageEvent(image_id);
	}

	existing_image_ids_ = unordered_set<size_t>(RegImageIDs.begin(), RegImageIDs.end());

	prev_init_image_pair_id_ = kInvalidImagePairId;
	prev_init_two_view_geometry_ = TwoViewGeometry();

	filtered_images_.clear();
	num_reg_trials_.clear();
}
void CMapper::EndReconstruction(bool IsDircard) 
{
	CHECK_NOTNULL(Model_);

	if (IsDircard)
	{
		vector<size_t> RegImageIDs;
		Model_->GetModelAllRegImageIDs(RegImageIDs);
		for (const image_t image_id : RegImageIDs)
		{
			DeRegisterImageEvent(image_id);
		}
	}

	Model_->TearDown();
	Model_ = nullptr;
	triangulator_.reset();
}
bool CMapper::FindInitialImagePair(Options options, size_t* image_id1, size_t* image_id2)
{
	CHECK(options.Check());

	vector<size_t> image_ids1;
	if (*image_id1 != kInvalidImageId && *image_id2 == kInvalidImageId)
	{
		// Only *image_id1 provided.
		if (!database_cache_->ExistsImage(*image_id1))
		{
			return false;
		}
		image_ids1.push_back(*image_id1);
	}
	else if (*image_id1 == kInvalidImageId && *image_id2 != kInvalidImageId)
	{
		// Only *image_id2 provided.
		if (!database_cache_->ExistsImage(*image_id2))
		{
			return false;
		}
		image_ids1.push_back(*image_id2);
	}
	else
	{
		// No initial seed image provided.
		image_ids1 = FindFirstInitialImage(options);
	}

	// Try to find good initial pair.
	for (size_t i1 = 0; i1 < image_ids1.size(); ++i1)
	{
		*image_id1 = image_ids1[i1];

		vector<size_t> image_ids2 = FindSecondInitialImage(options, *image_id1);

		for (size_t i2 = 0; i2 < image_ids2.size(); ++i2)
		{
			*image_id2 = image_ids2[i2];

			const image_pair_t pair_id = Database::ImagePairToPairId(*image_id1, *image_id2);

			// Try every pair only once.
			if (init_image_pairs_.count(pair_id) > 0)
			{
				continue;
			}

			init_image_pairs_.insert(pair_id);

			if (EstimateInitialTwoViewGeometry(options, *image_id1, *image_id2))
			{
				return true;
			}
		}
	}

	// No suitable pair found in entire dataset.
	*image_id1 = kInvalidImageId;
	*image_id2 = kInvalidImageId;

	return false;
}
vector<size_t> CMapper::FindNextImages(Options options)
{
	CHECK_NOTNULL(Model_);
	CHECK(options.Check());

	std::function<float(Image&)> rank_image_func;
	switch (options.image_selection_method) 
	{
	case Options::ImageSelectionMethod::MAX_VISIBLE_POINTS_NUM:
		rank_image_func = RankNextImageMaxVisiblePointsNum;
		break;
	case Options::ImageSelectionMethod::MAX_VISIBLE_POINTS_RATIO:
		rank_image_func = RankNextImageMaxVisiblePointsRatio;
		break;
	case Options::ImageSelectionMethod::MIN_UNCERTAINTY:
		rank_image_func = RankNextImageMinUncertainty;
		break;
	}

	vector<pair<size_t, float>> image_ranks;
	vector<pair<size_t, float>> other_image_ranks;

	CModel::CImageMapType AllImages;
	Model_->GetModelAllImages(AllImages);
	// Append images that have not failed to register before.
	for (auto& image : AllImages)
	{
		// Skip images that are already registered.
		if (image.second.IsRegistered()) 
		{
			continue;
		}

		// Only consider images with a sufficient number of visible points.
		if (image.second.NumVisiblePoints3D() < static_cast<size_t>(options.abs_pose_min_num_inliers))
		{
			continue;
		}

		// Only try registration for a certain maximum number of times.
		const size_t num_reg_trials = num_reg_trials_[image.first];
		if (num_reg_trials >= static_cast<size_t>(options.max_reg_trials)) 
		{
			continue;
		}

		// If image has been filtered or failed to register, place it in the
		// second bucket and prefer images that have not been tried before.
		const float rank = rank_image_func(image.second);
		if (filtered_images_.count(image.first) == 0 && num_reg_trials == 0) 
		{
			image_ranks.emplace_back(image.first, rank);
		}
		else 
		{
			other_image_ranks.emplace_back(image.first, rank);
		}
	}

	vector<size_t> ranked_images_ids;
	SortAndAppendNextImages(image_ranks, &ranked_images_ids);
	SortAndAppendNextImages(other_image_ranks, &ranked_images_ids);

	return ranked_images_ids;
}
bool CMapper::RegisterInitialImagePair(Options options, size_t image_id1, size_t image_id2)
{
	CHECK_NOTNULL(Model_);
	CHECK_EQ(Model_->GetModelRegImagesNum(), 0);

	CHECK(options.Check());

	init_num_reg_trials_[image_id1] += 1;
	init_num_reg_trials_[image_id2] += 1;
	num_reg_trials_[image_id1] += 1;
	num_reg_trials_[image_id2] += 1;

	image_pair_t pair_id = Database::ImagePairToPairId(image_id1, image_id2);
	init_image_pairs_.insert(pair_id);

	/*Image& image1 = reconstruction_->Image(image_id1);
	const Camera& camera1 = reconstruction_->Camera(image1.CameraId());

	Image& image2 = reconstruction_->Image(image_id2);
	const Camera& camera2 = reconstruction_->Camera(image2.CameraId());*/

	//////////////////////////////////////////////////////////////////////////////
	// Estimate two-view geometry
	//////////////////////////////////////////////////////////////////////////////

	if (!EstimateInitialTwoViewGeometry(options, image_id1, image_id2)) 
	{
		return false;
	}

	Image image1 = Model_->GetModelImage(image_id1);
	Image image2 = Model_->GetModelImage(image_id2);
	Camera camera1 = Model_->GetModelCamera(image1.CameraId());
	Camera camera2 = Model_->GetModelCamera(image2.CameraId());

	image1.Qvec() = ComposeIdentityQuaternion();
	image1.Tvec() = Eigen::Vector3d(0, 0, 0);
	image2.Qvec() = prev_init_two_view_geometry_.qvec;
	image2.Tvec() = prev_init_two_view_geometry_.tvec;

	Eigen::Matrix3x4d proj_matrix1 = image1.ProjectionMatrix();
	Eigen::Matrix3x4d proj_matrix2 = image2.ProjectionMatrix();
	Eigen::Vector3d proj_center1 = image1.ProjectionCenter();
	Eigen::Vector3d proj_center2 = image2.ProjectionCenter();

	Model_->SetImage(image1, image_id1);
	Model_->SetImage(image2, image_id2);

	//////////////////////////////////////////////////////////////////////////////
	// Update Reconstruction
	//////////////////////////////////////////////////////////////////////////////

	Model_->RegisterImage(image_id1);
	Model_->RegisterImage(image_id2);
	RegisterImageEvent(image_id1);
	RegisterImageEvent(image_id2);

	const CorrespondenceGraph& correspondence_graph = database_cache_->CorrespondenceGraph();
	const FeatureMatches& corrs = correspondence_graph.FindCorrespondencesBetweenImages(image_id1, image_id2);

	const double min_tri_angle_rad = DegToRad(options.init_min_tri_angle);

	// Add 3D point tracks.
	Track track;
	track.Reserve(2);
	track.AddElement(TrackElement());
	track.AddElement(TrackElement());
	track.Element(0).image_id = image_id1;
	track.Element(1).image_id = image_id2;
	for (auto& corr : corrs) 
	{
		Eigen::Vector2d point1_N = camera1.ImageToWorld(image1.Point2D(corr.point2D_idx1).XY());
		Eigen::Vector2d point2_N = camera2.ImageToWorld(image2.Point2D(corr.point2D_idx2).XY());
		Eigen::Vector3d xyz = TriangulatePoint(proj_matrix1, proj_matrix2, point1_N, point2_N);
		const double tri_angle = CalculateTriangulationAngle(proj_center1, proj_center2, xyz);
		if (tri_angle >= min_tri_angle_rad && HasPointPositiveDepth(proj_matrix1, xyz) && HasPointPositiveDepth(proj_matrix2, xyz))
		{
			track.Element(0).point2D_idx = corr.point2D_idx1;
			track.Element(1).point2D_idx = corr.point2D_idx2;
			Model_->ModelAddPoint3D(xyz, track);
		}
	}
	Model_->SetCamera(camera1, image1.CameraId());
	Model_->SetCamera(camera2, image2.CameraId());
	return true;
}
bool CMapper::RegisterNextImage(Options options, size_t image_id)
{
	DebugTimer DebugTimer(__FUNCTION__);
	CHECK_NOTNULL(Model_);
	CHECK_GE(Model_->GetModelRegImagesNum(), 2);

	CHECK(options.Check());

	/*Image& image = reconstruction_->Image(image_id);
	Camera& camera = reconstruction_->Camera(image.CameraId());*/
	
	if (Model_->IsImageRegistered(image_id))
	{
		cout << StringPrintf("Image %d is already registered!", image_id);
	}
	//CHECK(!Model_->IsImageRegistered(image_id)) << "Image cannot be registered multiple times";

	num_reg_trials_[image_id] += 1;

	Image image = Model_->GetModelImage(image_id);
	Camera camera = Model_->GetModelCamera(image.CameraId());

	// Check if enough 2D-3D correspondences.
	if (image.NumVisiblePoints3D() < static_cast<size_t>(options.abs_pose_min_num_inliers))
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Search for 2D-3D correspondences
	//////////////////////////////////////////////////////////////////////////////

	const CorrespondenceGraph& correspondence_graph = database_cache_->CorrespondenceGraph();

	vector<pair<point2D_t, point3D_t>> tri_corrs;
	vector<Eigen::Vector2d> tri_points2D;
	vector<Eigen::Vector3d> tri_points3D;

	unordered_set<point3D_t> corr_point3D_ids;
	for (point2D_t point2D_idx = 0; point2D_idx < image.NumPoints2D(); ++point2D_idx)
	{
		const Point2D& point2D = image.Point2D(point2D_idx);

		corr_point3D_ids.clear();
		for (const auto& corr : correspondence_graph.FindCorrespondences(image_id, point2D_idx))
		{
			Image corr_image = Model_->GetModelImage(corr.image_id);
			//const Image& corr_image = reconstruction_->Image(corr.image_id);
			if (!corr_image.IsRegistered()) 
			{
				continue;
			}

			const Point2D& corr_point2D = corr_image.Point2D(corr.point2D_idx);
			if (!corr_point2D.HasPoint3D()) 
			{
				continue;
			}

			// Avoid duplicate correspondences.
			if (corr_point3D_ids.count(corr_point2D.Point3DId()) > 0) 
			{
				continue;
			}
			Camera corr_camera = Model_->GetModelCamera(corr_image.CameraId());
			//const Camera& corr_camera = reconstruction_->Camera(corr_image.CameraId());

			// Avoid correspondences to images with bogus camera parameters.
			if (corr_camera.HasBogusParams(options.min_focal_length_ratio, options.max_focal_length_ratio, options.max_extra_param))
			{
				continue;
			}

			Point3D point3D = Model_->GetModelPoint3D(corr_point2D.Point3DId());
			//const Point3D& point3D = reconstruction_->Point3D(corr_point2D.Point3DId());

			tri_corrs.emplace_back(point2D_idx, corr_point2D.Point3DId());
			corr_point3D_ids.insert(corr_point2D.Point3DId());
			tri_points2D.push_back(point2D.XY());
			tri_points3D.push_back(point3D.XYZ());
		}
	}

	// The size of `next_image.num_tri_obs` and `tri_corrs_point2D_idxs.size()`
	// can only differ, when there are images with bogus camera parameters, and
	// hence we skip some of the 2D-3D correspondences.
	if (tri_points2D.size() < static_cast<size_t>(options.abs_pose_min_num_inliers))
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////
	// 2D-3D estimation
	//////////////////////////////////////////////////////////////////////////////

	// Only refine / estimate focal length, if no focal length was specified
	// (manually or through EXIF) and if it was not already estimated previously
	// from another image (when multiple images share the same camera
	// parameters)

	AbsolutePoseEstimationOptions abs_pose_options;
	abs_pose_options.num_threads = options.num_threads;
	abs_pose_options.num_focal_length_samples = 30;
	abs_pose_options.min_focal_length_ratio = options.min_focal_length_ratio;
	abs_pose_options.max_focal_length_ratio = options.max_focal_length_ratio;
	abs_pose_options.ransac_options.max_error = options.abs_pose_max_error;
	abs_pose_options.ransac_options.min_inlier_ratio = options.abs_pose_min_inlier_ratio;
	// Use high confidence to avoid preemptive termination of P3P RANSAC
	// - too early termination may lead to bad registration.
	abs_pose_options.ransac_options.min_num_trials = 100;
	abs_pose_options.ransac_options.max_num_trials = 10000;
	abs_pose_options.ransac_options.confidence = 0.99999;

	AbsolutePoseRefinementOptions abs_pose_refinement_options;
	if (num_reg_images_per_camera_[image.CameraId()] > 0) 
	{
		// Camera already refined from another image with the same camera.
		if (camera.HasBogusParams(options.min_focal_length_ratio, options.max_focal_length_ratio, options.max_extra_param))
		{
			// Previously refined camera has bogus parameters,
			// so reset parameters and try to re-estimage.
			camera.SetParams(database_cache_->Camera(image.CameraId()).Params());
			abs_pose_options.estimate_focal_length = !camera.HasPriorFocalLength();
			abs_pose_refinement_options.refine_focal_length = true;
			abs_pose_refinement_options.refine_extra_params = true;
		}
		else 
		{
			abs_pose_options.estimate_focal_length = false;
			abs_pose_refinement_options.refine_focal_length = false;
			abs_pose_refinement_options.refine_extra_params = false;
		}
	}
	else 
	{
		// Camera not refined before. Note that the camera parameters might have
		// been changed before but the image was filtered, so we explicitly reset
		// the camera parameters and try to re-estimate them.
		camera.SetParams(database_cache_->Camera(image.CameraId()).Params());
		abs_pose_options.estimate_focal_length = !camera.HasPriorFocalLength();
		abs_pose_refinement_options.refine_focal_length = true;
		abs_pose_refinement_options.refine_extra_params = true;
	}

	if (!options.abs_pose_refine_focal_length) 
	{
		abs_pose_options.estimate_focal_length = false;
		abs_pose_refinement_options.refine_focal_length = false;
	}

	if (!options.abs_pose_refine_extra_params) 
	{
		abs_pose_refinement_options.refine_extra_params = false;
	}

	size_t num_inliers;
	vector<char> inlier_mask;

	if (!EstimateAbsolutePose(abs_pose_options, tri_points2D, tri_points3D, &image.Qvec(), &image.Tvec(), &camera, &num_inliers, &inlier_mask))
	{
		return false;
	}

	if (num_inliers < static_cast<size_t>(options.abs_pose_min_num_inliers)) 
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Pose refinement
	//////////////////////////////////////////////////////////////////////////////
	abs_pose_refinement_options.print_summary = false;
	if (!RefineAbsolutePose(abs_pose_refinement_options, inlier_mask, tri_points2D, tri_points3D, &image.Qvec(), &image.Tvec(), &camera))
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Continue tracks
	//////////////////////////////////////////////////////////////////////////////

	Model_->RegisterImage(image_id);
	//reconstruction_->RegisterImage(image_id);
	RegisterImageEvent(image_id);

	for (size_t i = 0; i < inlier_mask.size(); i++) 
	{
		if (inlier_mask[i]) 
		{
			const point2D_t point2D_idx = tri_corrs[i].first;
			const Point2D& point2D = image.Point2D(point2D_idx);
			if (!point2D.HasPoint3D()) 
			{
				point3D_t point3D_id = tri_corrs[i].second;
				TrackElement track_el(image_id, point2D_idx);
				if (Model_->GetModelImage(image_id).Point2D(point2D_idx).HasPoint3D())continue;
				Model_->AddObservation(point3D_id, track_el);
				//reconstruction_->AddObservation(point3D_id, track_el);
				triangulator_->AddModifiedPoint3D(point3D_id);
			}
		}
	}

	return true;
}
size_t CMapper::TriangulateImage(CTriangulator::Options tri_options, size_t image_id)
{
	CHECK_NOTNULL(Model_);
	return triangulator_->TriangulateImage(tri_options, image_id);
}
size_t CMapper::Retriangulate(CTriangulator::Options tri_options)
{
	CHECK_NOTNULL(Model_);
	return triangulator_->Retriangulate(tri_options);
}
size_t CMapper::CompleteTracks(CTriangulator::Options tri_options)
{
	DebugTimer timer(__FUNCTION__);
	CHECK_NOTNULL(Model_);
	return triangulator_->CompleteAllTracks(tri_options);
}
size_t CMapper::MergeTracks(CTriangulator::Options tri_options)
{
	DebugTimer timer(__FUNCTION__);
	CHECK_NOTNULL(Model_);
	return triangulator_->MergeAllTracks(tri_options);
}
CMapper::LocalBundleAdjustmentReport CMapper::AdjustLocalBundle(Options options, CBundleAdjustmentOptions ba_options, CTriangulator::Options tri_options, size_t image_id, const unordered_set<size_t>& point3D_ids)
{
	CHECK_NOTNULL(Model_);
	CHECK(options.Check());

	LocalBundleAdjustmentReport report;

	// Find images that have most 3D points with given image in common.
	const std::vector<size_t> local_bundle = FindLocalBundle(options, image_id);

	
	// Do the bundle adjustment only if there is any connected images.
	if (local_bundle.size() > 0) 
	{
		CBundleAdjustmentConfig ba_config;
		ba_config.AddImage(image_id);
		for (const image_t local_image_id : local_bundle) 
		{
			ba_config.AddImage(local_image_id);
		}

		// Fix the existing images, if option specified.
		if (options.fix_existing_images) 
		{
			for (const image_t local_image_id : local_bundle) 
			{
				if (existing_image_ids_.count(local_image_id)) 
				{
					ba_config.SetConstantPose(local_image_id);
				}
			}
		}

		// Determine which cameras to fix, when not all the registered images
		// are within the current local bundle.
		std::unordered_map<camera_t, size_t> num_images_per_camera;
		for (const image_t image_id : ba_config.Images()) 
		{
			const Image& image = Model_->GetModelImage(image_id);
			num_images_per_camera[image.CameraId()] += 1;
		}

		for (const auto& camera_id_and_num_images_pair : num_images_per_camera) 
		{
			const size_t num_reg_images_for_camera = num_reg_images_per_camera_.at(camera_id_and_num_images_pair.first);
			if (camera_id_and_num_images_pair.second < num_reg_images_for_camera) 
			{
				ba_config.SetConstantCamera(camera_id_and_num_images_pair.first);
			}
		}

		// Fix 7 DOF to avoid scale/rotation/translation drift in bundle adjustment.
		if (local_bundle.size() == 1) 
		{
			ba_config.SetConstantPose(local_bundle[0]);
			ba_config.SetConstantTvec(image_id, { 0 });
		}
		else if (local_bundle.size() > 1) 
		{
			const image_t image_id1 = local_bundle[local_bundle.size() - 1];
			const image_t image_id2 = local_bundle[local_bundle.size() - 2];
			ba_config.SetConstantPose(image_id1);
			if (!options.fix_existing_images || !existing_image_ids_.count(image_id2))
			{
				ba_config.SetConstantTvec(image_id2, { 0 });
			}
		}

		// Make sure, we refine all new and short-track 3D points, no matter if
		// they are fully contained in the local image set or not. Do not include
		// long track 3D points as they are usually already very stable and adding
		// to them to bundle adjustment and track merging/completion would slow
		// down the local bundle adjustment significantly.
		unordered_set<point3D_t> variable_point3D_ids;
		for (const point3D_t point3D_id : point3D_ids) 
		{
			const Point3D& point3D = Model_->GetModelPoint3D(point3D_id);
			const size_t kMaxTrackLength = 15;
			if (!point3D.HasError() || point3D.Track().Length() <= kMaxTrackLength) 
			{
				ba_config.AddVariablePoint(point3D_id);
				variable_point3D_ids.insert(point3D_id);
			}
		}
		// Adjust the local bundle.
		CBundleAdjuster bundle_adjuster(ba_options, ba_config);

		QElapsedTimer timer;
		timer.start();
		bundle_adjuster.Solve(Model_);
		qDebug() << "Solve:   " << timer.elapsed();
		timer.restart();

		report.num_adjusted_observations = bundle_adjuster.Summary().num_residuals / 2;

		
		// Merge refined tracks with other existing points.
		report.num_merged_observations = triangulator_->MergeTracks(tri_options, variable_point3D_ids);
		qDebug() << "MergeTracks:   " << timer.elapsed();
		timer.restart();
		// Complete tracks that may have failed to triangulate before refinement
		// of camera pose and calibration in bundle-adjustment. This may avoid
		// that some points are filtered and it helps for subsequent image
		// registrations.
		report.num_completed_observations = triangulator_->CompleteTracks(tri_options, variable_point3D_ids);
		qDebug() << "CompleteTracks:   " << timer.elapsed();
		timer.restart();
		report.num_completed_observations += triangulator_->CompleteImage(tri_options, image_id);
		qDebug() << "CompleteImage:   " << timer.elapsed();
		
	}

	// Filter both the modified images and all changed 3D points to make sure
	// there are no outlier points in the model. This results in duplicate work as
	// many of the provided 3D points may also be contained in the adjusted
	// images, but the filtering is not a bottleneck at this point.
	unordered_set<size_t> filter_image_ids;
	filter_image_ids.insert(image_id);
	filter_image_ids.insert(local_bundle.begin(), local_bundle.end());
	report.num_filtered_observations = Model_->FilterPoints3DInImages(options.filter_max_reproj_error, options.filter_min_tri_angle, filter_image_ids);
	report.num_filtered_observations += Model_->FilterPoints3D(options.filter_max_reproj_error, options.filter_min_tri_angle, point3D_ids);

	return report;
}
bool CMapper::AdjustGlobalBundle(Options& options, CBundleAdjustmentOptions& ba_options)
{
	CHECK_NOTNULL(Model_);

	const std::vector<size_t>& reg_image_ids = Model_->GetAllRegImagesIDs();

	CHECK_GE(reg_image_ids.size(), 2) << "At least two images must be "
		"registered for global "
		"bundle-adjustment";

	// Avoid degeneracies in bundle adjustment.
	Model_->FilterNegativeDepthObservations();

	// Configure bundle adjustment.
	CBundleAdjustmentConfig ba_config;
	for (const image_t image_id : reg_image_ids) 
	{
		ba_config.AddImage(image_id);
	}

	// Fix the existing images, if option specified.
	if (options.fix_existing_images) 
	{
		for (const image_t image_id : reg_image_ids) 
		{
			if (existing_image_ids_.count(image_id)) {
				ba_config.SetConstantPose(image_id);
			}
		}
	}

	// Fix 7-DOFs of the bundle adjustment problem.
	ba_config.SetConstantPose(reg_image_ids[0]);
	if (!options.fix_existing_images || !existing_image_ids_.count(reg_image_ids[1]))
	{
		ba_config.SetConstantTvec(reg_image_ids[1], { 0 });
	}
	// Run bundle adjustment.
	CBundleAdjuster bundle_adjuster(ba_options, ba_config);
	if (!bundle_adjuster.Solve(Model_))
	{
		return false;
	}

	// Normalize scene for numerical stability and
	// to avoid large scale changes in viewer.
	Model_->Normalize();
	return true;
}
size_t CMapper::FilterImages(Options options)
{
	CHECK_NOTNULL(Model_);
	CHECK(options.Check());

	// Do not filter images in the early stage of the reconstruction, since the
	// calibration is often still refining a lot. Hence, the camera parameters
	// are not stable in the beginning.
	const size_t kMinNumImages = 20;
	if (Model_->GetModelRegImagesNum() < kMinNumImages)
	{
		return {};
	}

	vector<size_t> image_ids;
	Model_->FilterImages(options.min_focal_length_ratio, options.max_focal_length_ratio, options.max_extra_param, image_ids);

	for (const image_t image_id : image_ids) 
	{
		DeRegisterImageEvent(image_id);
		filtered_images_.insert(image_id);
	}

	return image_ids.size();
}
size_t CMapper::FilterPoints(Options options)
{
	CHECK_NOTNULL(Model_);
	CHECK(options.Check());
	return Model_->FilterAllPoints3Ds(options.filter_max_reproj_error, options.filter_min_tri_angle);
}
CModel* CMapper::GetModel()
{
	CHECK_NOTNULL(Model_);
	return Model_;
}
size_t CMapper::NumTotalRegImages()
{
	return num_total_reg_images_;
}
size_t CMapper::NumSharedRegImages()
{
	return num_shared_reg_images_;
}
const unordered_set<size_t>& CMapper::GetModifiedPoints3D()
{
	return triangulator_->GetModifiedPoints3D();
}
void CMapper::ClearModifiedPoints3D()
{
	triangulator_->ClearModifiedPoints3D();
}
vector<size_t> CMapper::FindFirstInitialImage(Options& options)
{
	// Struct to hold meta-data for ranking images.
	struct ImageInfo
	{
		image_t image_id;
		bool prior_focal_length;
		image_t num_correspondences;
	};

	const size_t init_max_reg_trials = static_cast<size_t>(options.init_max_reg_trials);

	// Collect information of all not yet registered images with
	// correspondences.
	std::vector<ImageInfo> image_infos;
	image_infos.reserve(Model_->GetModelImagesNum());
	for (const auto& image : Model_->GetModelAllImages())
	{
		if (image.second.IsRegistered())continue;
		// Only images with correspondences can be registered.
		if (image.second.NumCorrespondences() == 0)
		{
			continue;
		}

		// Only use images for initialization a maximum number of times.
		if (init_num_reg_trials_.count(image.first) && init_num_reg_trials_.at(image.first) >= init_max_reg_trials)
		{
			continue;
		}

		// Only use images for initialization that are not registered in any
		// of the other reconstructions.
		if (num_registrations_.count(image.first) > 0 && num_registrations_.at(image.first) > 0)
		{
			continue;
		}

		const class Camera& camera = Model_->GetModelCamera(image.second.CameraId());
		ImageInfo image_info;
		image_info.image_id = image.first;
		image_info.prior_focal_length = camera.HasPriorFocalLength();
		image_info.num_correspondences = image.second.NumCorrespondences();
		image_infos.push_back(image_info);
	}

	// Sort images such that images with a prior focal length and more
	// correspondences are preferred, i.e. they appear in the front of the list.
	std::sort(image_infos.begin(), image_infos.end(), [](const ImageInfo& image_info1, const ImageInfo& image_info2)
		{
			if (image_info1.prior_focal_length && !image_info2.prior_focal_length)
			{
				return true;
			}
			else if (!image_info1.prior_focal_length && image_info2.prior_focal_length)
			{
				return false;
			}
			else
			{
				return image_info1.num_correspondences >
					image_info2.num_correspondences;
			}
		});

	// Extract image identifiers in sorted order.
	vector<size_t> image_ids;
	image_ids.reserve(image_infos.size());
	for (const ImageInfo& image_info : image_infos)
	{
		image_ids.push_back(image_info.image_id);
	}

	return image_ids;
}
vector<size_t> CMapper::FindSecondInitialImage(Options& options, size_t image_id1)
{
	const CorrespondenceGraph& correspondence_graph = database_cache_->CorrespondenceGraph();

	// Collect images that are connected to the first seed image and have
	// not been registered before in other reconstructions.
	const class Image& image1 = Model_->GetModelImage(image_id1);
	unordered_map<image_t, point2D_t> num_correspondences;
	for (point2D_t point2D_idx = 0; point2D_idx < image1.NumPoints2D(); point2D_idx++)
	{
		for (const auto& corr : correspondence_graph.FindCorrespondences(image_id1, point2D_idx))
		{
			if (Model_->IsImageRegistered(corr.image_id))continue;
			if (num_registrations_.count(corr.image_id) == 0 || num_registrations_.at(corr.image_id) == 0)
			{
				num_correspondences[corr.image_id] += 1;
			}
		}
	}

	// Struct to hold meta-data for ranking images.
	struct ImageInfo 
	{
		image_t image_id;
		bool prior_focal_length;
		point2D_t num_correspondences;
	};

	const size_t init_min_num_inliers = static_cast<size_t>(options.init_min_num_inliers);

	// Compose image information in a compact form for sorting.
	std::vector<ImageInfo> image_infos;
	image_infos.reserve(Model_->GetModelImagesNum());
	for (const auto elem : num_correspondences) 
	{
		if (elem.second >= init_min_num_inliers) 
		{
			const class Image& image = Model_->GetModelImage(elem.first);
			const class Camera& camera = Model_->GetModelCamera(image.CameraId());
			ImageInfo image_info;
			image_info.image_id = elem.first;
			image_info.prior_focal_length = camera.HasPriorFocalLength();
			image_info.num_correspondences = elem.second;
			image_infos.push_back(image_info);
		}
	}

	// Sort images such that images with a prior focal length and more
	// correspondences are preferred, i.e. they appear in the front of the list.
	std::sort(
		image_infos.begin(), image_infos.end(),
		[](const ImageInfo& image_info1, const ImageInfo& image_info2) {
			if (image_info1.prior_focal_length && !image_info2.prior_focal_length) {
				return true;
			}
			else if (!image_info1.prior_focal_length &&
				image_info2.prior_focal_length) {
				return false;
			}
			else {
				return image_info1.num_correspondences >
					image_info2.num_correspondences;
			}
		});

	// Extract image identifiers in sorted order.
	vector<size_t> image_ids;
	image_ids.reserve(image_infos.size());
	for (const ImageInfo& image_info : image_infos) 
	{
		image_ids.push_back(image_info.image_id);
	}

	return image_ids;
}
vector<size_t> CMapper::FindLocalBundle(Options& options, size_t image_id)
{
	CHECK(options.Check());

	const Image& image = Model_->GetModelImage(image_id);
	CHECK(image.IsRegistered());

	// Extract all images that have at least one 3D point with the query image
	// in common, and simultaneously count the number of common 3D points.

	unordered_map<image_t, size_t> shared_observations;

	unordered_set<point3D_t> point3D_ids;
	point3D_ids.reserve(image.NumPoints3D());

	for (const Point2D& point2D : image.Points2D()) 
	{
		if (point2D.HasPoint3D()) 
		{
			point3D_ids.insert(point2D.Point3DId());
			const Point3D& point3D = Model_->GetModelPoint3D(point2D.Point3DId());
			for (const TrackElement& track_el : point3D.Track().Elements()) 
			{
				if (track_el.image_id != image_id) 
				{
					shared_observations[track_el.image_id] += 1;
				}
			}
		}
	}

	// Sort overlapping images according to number of shared observations.

	vector<pair<image_t, size_t>> overlapping_images(
		shared_observations.begin(), shared_observations.end());
	sort(overlapping_images.begin(), overlapping_images.end(),
		[](const pair<image_t, size_t>& image1,
			const pair<image_t, size_t>& image2) {
				return image1.second > image2.second;
		});

	// The local bundle is composed of the given image and its most connected
	// neighbor images, hence the subtraction of 1.

	const size_t num_images = static_cast<size_t>(options.local_ba_num_images - 1);
	const size_t num_eff_images = std::min(num_images, overlapping_images.size());

	// Extract most connected images and ensure sufficient triangulation angle.

	vector<size_t> local_bundle_image_ids;
	local_bundle_image_ids.reserve(num_eff_images);

	// If the number of overlapping images equals the number of desired images in
	// the local bundle, then simply copy over the image identifiers.
	if (overlapping_images.size() == num_eff_images) 
	{
		for (const auto& overlapping_image : overlapping_images) 
		{
			local_bundle_image_ids.push_back(overlapping_image.first);
		}
		return local_bundle_image_ids;
	}

	// In the following iteration, we start with the most overlapping images and
	// check whether it has sufficient triangulation angle. If none of the
	// overlapping images has sufficient triangulation angle, we relax the
	// triangulation angle threshold and start from the most overlapping image
	// again. In the end, if we still haven't found enough images, we simply use
	// the most overlapping images.

	const double min_tri_angle_rad = DegToRad(options.local_ba_min_tri_angle);

	// The selection thresholds (minimum triangulation angle, minimum number of
	// shared observations), which are successively relaxed.
	const std::array<std::pair<double, double>, 8> selection_thresholds = { {
		std::make_pair(min_tri_angle_rad / 1.0, 0.6 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 1.5, 0.6 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 2.0, 0.5 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 2.5, 0.4 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 3.0, 0.3 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 4.0, 0.2 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 5.0, 0.1 * image.NumPoints3D()),
		std::make_pair(min_tri_angle_rad / 6.0, 0.1 * image.NumPoints3D()),
	} };

	const Eigen::Vector3d proj_center = image.ProjectionCenter();
	std::vector<Eigen::Vector3d> shared_points3D;
	shared_points3D.reserve(image.NumPoints3D());
	std::vector<double> tri_angles(overlapping_images.size(), -1.0);
	std::vector<char> used_overlapping_images(overlapping_images.size(), false);

	for (const auto& selection_threshold : selection_thresholds) 
	{
		for (size_t overlapping_image_idx = 0; overlapping_image_idx < overlapping_images.size(); ++overlapping_image_idx)
		{
			// Check if the image has sufficient overlap. Since the images are ordered
			// based on the overlap, we can just skip the remaining ones.
			if (overlapping_images[overlapping_image_idx].second < selection_threshold.second)
			{
				break;
			}

			// Check if the image is already in the local bundle.
			if (used_overlapping_images[overlapping_image_idx]) 
			{
				continue;
			}

			const auto& overlapping_image = Model_->GetModelImage(overlapping_images[overlapping_image_idx].first);
			const Eigen::Vector3d overlapping_proj_center = overlapping_image.ProjectionCenter();

			// In the first iteration, compute the triangulation angle. In later
			// iterations, reuse the previously computed value.
			double& tri_angle = tri_angles[overlapping_image_idx];
			if (tri_angle < 0.0) 
			{
				// Collect the commonly observed 3D points.
				shared_points3D.clear();
				for (const Point2D& point2D : image.Points2D()) 
				{
					if (point2D.HasPoint3D() && point3D_ids.count(point2D.Point3DId())) 
					{
						shared_points3D.push_back(Model_->GetModelPoint3D(point2D.Point3DId()).XYZ());
					}
				}

				// Calculate the triangulation angle at a certain percentile.
				const double kTriangulationAnglePercentile = 75;
				tri_angle = Percentile(CalculateTriangulationAngles(proj_center, overlapping_proj_center, shared_points3D), kTriangulationAnglePercentile);
			}

			// Check that the image has sufficient triangulation angle.
			if (tri_angle >= selection_threshold.first) 
			{
				local_bundle_image_ids.push_back(overlapping_image.ImageId());
				used_overlapping_images[overlapping_image_idx] = true;
				// Check if we already collected enough images.
				if (local_bundle_image_ids.size() >= num_eff_images) 
				{
					break;
				}
			}
		}

		// Check if we already collected enough images.
		if (local_bundle_image_ids.size() >= num_eff_images) 
		{
			break;
		}
	}

	// In case there are not enough images with sufficient triangulation angle,
	// simply fill up the rest with the most overlapping images.

	if (local_bundle_image_ids.size() < num_eff_images) 
	{
		for (size_t overlapping_image_idx = 0; overlapping_image_idx < overlapping_images.size(); ++overlapping_image_idx)
		{
			// Collect image if it is not yet in the local bundle.
			if (!used_overlapping_images[overlapping_image_idx]) 
			{
				local_bundle_image_ids.push_back(overlapping_images[overlapping_image_idx].first);
				used_overlapping_images[overlapping_image_idx] = true;

				// Check if we already collected enough images.
				if (local_bundle_image_ids.size() >= num_eff_images) 
				{
					break;
				}
			}
		}
	}
	return local_bundle_image_ids;
}
void CMapper::RegisterImageEvent(size_t image_id) 
{
	const Image& image = Model_->GetModelImage(image_id);
	size_t& num_reg_images_for_camera =num_reg_images_per_camera_[image.CameraId()];
	num_reg_images_for_camera += 1;

	size_t& num_regs_for_image = num_registrations_[image_id];
	num_regs_for_image += 1;
	if (num_regs_for_image == 1) 
	{
		num_total_reg_images_ += 1;
	}
	else if (num_regs_for_image > 1) 
	{
		num_shared_reg_images_ += 1;
	}
}
void CMapper::DeRegisterImageEvent(size_t image_id)
{
	const Image& image = Model_->GetModelImage(image_id);
	size_t& num_reg_images_for_camera = num_reg_images_per_camera_.at(image.CameraId());
	CHECK_GT(num_reg_images_for_camera, 0);
	num_reg_images_for_camera -= 1;

	size_t& num_regs_for_image = num_registrations_[image_id];
	num_regs_for_image -= 1;
	if (num_regs_for_image == 0) 
	{
		num_total_reg_images_ -= 1;
	}
	else if (num_regs_for_image > 0) 
	{
		num_shared_reg_images_ -= 1;
	}
}
bool CMapper::EstimateInitialTwoViewGeometry(Options& options, size_t image_id1, size_t image_id2)
{
	const image_pair_t image_pair_id = Database::ImagePairToPairId(image_id1, image_id2);

	if (prev_init_image_pair_id_ == image_pair_id) 
	{
		return true;
	}

	const Image& image1 = database_cache_->Image(image_id1);
	const Camera& camera1 = database_cache_->Camera(image1.CameraId());

	const Image& image2 = database_cache_->Image(image_id2);
	const Camera& camera2 = database_cache_->Camera(image2.CameraId());

	const CorrespondenceGraph& correspondence_graph = database_cache_->CorrespondenceGraph();
	const FeatureMatches matches = correspondence_graph.FindCorrespondencesBetweenImages(image_id1, image_id2);

	vector<Eigen::Vector2d> points1;
	points1.reserve(image1.NumPoints2D());
	for (const auto& point : image1.Points2D()) 
	{
		points1.push_back(point.XY());
	}

	vector<Eigen::Vector2d> points2;
	points2.reserve(image2.NumPoints2D());
	for (const auto& point : image2.Points2D()) 
	{
		points2.push_back(point.XY());
	}

	TwoViewGeometry two_view_geometry;
	TwoViewGeometry::Options two_view_geometry_options;
	two_view_geometry_options.ransac_options.min_num_trials = 30;
	two_view_geometry_options.ransac_options.max_error = options.init_max_error;
	two_view_geometry.EstimateCalibrated(camera1, points1, camera2, points2, matches, two_view_geometry_options);

	if (!two_view_geometry.EstimateRelativePose(camera1, points1, camera2, points2))
	{
		qDebug() << "Model initialization failed: EstimateRelativePose";
		return false;
	}

	if (static_cast<int>(two_view_geometry.inlier_matches.size()) >= options.init_min_num_inliers && std::abs(two_view_geometry.tvec.z()) < options.init_max_forward_motion && two_view_geometry.tri_angle > DegToRad(options.init_min_tri_angle))
	{
		prev_init_image_pair_id_ = image_pair_id;
		prev_init_two_view_geometry_ = two_view_geometry;
		return true;
	}
	if (static_cast<int>(two_view_geometry.inlier_matches.size()) < options.init_min_num_inliers)
	{
		qDebug() << "Model initialization failed: num_inliers too few";
	}
	if (std::abs(two_view_geometry.tvec.z()) >= options.init_max_forward_motion)
	{
		qDebug() << "Model initialization failed: forward_motion too large";
	}
	if (two_view_geometry.tri_angle <= DegToRad(options.init_min_tri_angle))
	{
		qDebug() << "Model initialization failed: tri_angle too few";
	}
	return false;
}


CMapperOptions::CMapperOptions(IncrementalMapperOptions options)
{
	min_num_matches = options.min_num_matches;
	ignore_watermarks = options.ignore_watermarks;
	multiple_models = options.multiple_models;
	max_num_models = options.max_num_models;
	max_model_overlap = options.max_model_overlap;
	min_model_size = options.min_model_size;
	init_image_id1 = options.init_image_id1;
	init_image_id2 = options.init_image_id2;
	init_num_trials = options.init_num_trials;
	extract_colors = options.extract_colors;
	num_threads = options.num_threads;
	min_focal_length_ratio = options.min_focal_length_ratio;
	max_focal_length_ratio = options.max_focal_length_ratio;
	max_extra_param = options.max_extra_param;
	ba_refine_focal_length = options.ba_refine_focal_length;
	ba_refine_principal_point = options.ba_refine_principal_point;
	ba_refine_extra_params = options.ba_refine_extra_params;
	ba_min_num_residuals_for_multi_threading = options.ba_min_num_residuals_for_multi_threading;
	ba_local_num_images = options.ba_local_num_images;
	ba_local_function_tolerance = options.ba_local_function_tolerance;
	ba_local_max_num_iterations = options.ba_local_max_num_iterations;
	ba_global_images_ratio = options.ba_global_images_ratio;
	ba_global_points_ratio = options.ba_global_points_ratio;
	ba_global_images_freq = options.ba_global_images_freq;
	ba_global_points_freq = options.ba_global_points_freq;
	ba_global_function_tolerance = options.ba_global_function_tolerance;
	ba_global_max_num_iterations = options.ba_global_max_num_iterations;
	ba_local_max_refinements = options.ba_local_max_refinements;
	ba_local_max_refinement_change = options.ba_local_max_refinement_change;
	ba_global_max_refinements = options.ba_global_max_refinements;
	ba_global_max_refinement_change = options.ba_global_max_refinement_change;
	snapshot_path = options.snapshot_path;
	snapshot_images_freq = options.snapshot_images_freq;
	image_names = options.image_names;
	fix_existing_images = options.fix_existing_images;
}
CMapper::Options CMapperOptions::Mapper()
{
	CMapper::Options options = mapper;
	options.abs_pose_refine_focal_length = ba_refine_focal_length;
	options.abs_pose_refine_extra_params = ba_refine_extra_params;
	options.min_focal_length_ratio = min_focal_length_ratio;
	options.max_focal_length_ratio = max_focal_length_ratio;
	options.max_extra_param = max_extra_param;
	options.num_threads = num_threads;
	options.local_ba_num_images = ba_local_num_images;
	options.fix_existing_images = fix_existing_images;
	return options;
}
CTriangulator::Options CMapperOptions::Triangulation()
{
	CTriangulator::Options options = triangulation;
	options.min_focal_length_ratio = min_focal_length_ratio;
	options.max_focal_length_ratio = max_focal_length_ratio;
	options.max_extra_param = max_extra_param;
	return options;
}
CBundleAdjustmentOptions CMapperOptions::LocalBundleAdjustment()
{
	CBundleAdjustmentOptions options;
	options.solver_options.function_tolerance = ba_local_function_tolerance;
	options.solver_options.gradient_tolerance = 10.0;
	options.solver_options.parameter_tolerance = 0.0;
	options.solver_options.max_num_iterations = ba_local_max_num_iterations;
	options.solver_options.max_linear_solver_iterations = 100;
	options.solver_options.minimizer_progress_to_stdout = false;
	options.solver_options.num_threads = num_threads;
#if CERES_VERSION_MAJOR < 2
	options.solver_options.num_linear_solver_threads = num_threads;
#endif  // CERES_VERSION_MAJOR
	options.print_summary = false;
	options.refine_focal_length = ba_refine_focal_length;
	options.refine_principal_point = ba_refine_principal_point;
	options.refine_extra_params = ba_refine_extra_params;
	options.min_num_residuals_for_multi_threading = ba_min_num_residuals_for_multi_threading;
	options.loss_function_scale = 1.0;
	options.loss_function_type = CBundleAdjustmentOptions::LossFunctionType::SOFT_L1;
	return options;
}
CBundleAdjustmentOptions CMapperOptions::GlobalBundleAdjustment()
{
	CBundleAdjustmentOptions options;
	options.solver_options.function_tolerance = ba_global_function_tolerance;
	options.solver_options.gradient_tolerance = 1.0;
	options.solver_options.parameter_tolerance = 0.0;
	options.solver_options.max_num_iterations = ba_global_max_num_iterations;
	options.solver_options.max_linear_solver_iterations = 100;
	options.solver_options.minimizer_progress_to_stdout = true;
	options.solver_options.num_threads = num_threads;
#if CERES_VERSION_MAJOR < 2
	options.solver_options.num_linear_solver_threads = num_threads;
#endif  // CERES_VERSION_MAJOR
	options.print_summary = true;
	options.refine_focal_length = ba_refine_focal_length;
	options.refine_principal_point = ba_refine_principal_point;
	options.refine_extra_params = ba_refine_extra_params;
	options.min_num_residuals_for_multi_threading = ba_min_num_residuals_for_multi_threading;
	options.loss_function_type = CBundleAdjustmentOptions::LossFunctionType::TRIVIAL;
	return options;
}
bool CMapperOptions::Check()
{
	CHECK_OPTION_GT(min_num_matches, 0);
	CHECK_OPTION_GT(max_num_models, 0);
	CHECK_OPTION_GT(max_model_overlap, 0);
	CHECK_OPTION_GE(min_model_size, 0);
	CHECK_OPTION_GT(init_num_trials, 0);
	CHECK_OPTION_GT(min_focal_length_ratio, 0);
	CHECK_OPTION_GT(max_focal_length_ratio, 0);
	CHECK_OPTION_GE(max_extra_param, 0);
	CHECK_OPTION_GE(ba_local_num_images, 2);
	CHECK_OPTION_GE(ba_local_max_num_iterations, 0);
	CHECK_OPTION_GT(ba_global_images_ratio, 1.0);
	CHECK_OPTION_GT(ba_global_points_ratio, 1.0);
	CHECK_OPTION_GT(ba_global_images_freq, 0);
	CHECK_OPTION_GT(ba_global_points_freq, 0);
	CHECK_OPTION_GT(ba_global_max_num_iterations, 0);
	CHECK_OPTION_GT(ba_local_max_refinements, 0);
	CHECK_OPTION_GE(ba_local_max_refinement_change, 0);
	CHECK_OPTION_GT(ba_global_max_refinements, 0);
	CHECK_OPTION_GE(ba_global_max_refinement_change, 0);
	CHECK_OPTION_GE(snapshot_images_freq, 0);
	CHECK_OPTION(Mapper().Check());
	CHECK_OPTION(Triangulation().Check());
	return true;
}
