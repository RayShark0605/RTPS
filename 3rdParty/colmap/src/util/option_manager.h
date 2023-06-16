// Copyright (c) 2023, ETH Zurich and UNC Chapel Hill.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of ETH Zurich and UNC Chapel Hill nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: Johannes L. Schoenberger (jsch-at-demuc-dot-de)

#ifndef COLMAP_SRC_UTIL_OPTION_MANAGER_H_
#define COLMAP_SRC_UTIL_OPTION_MANAGER_H_

#include <memory>

#include <boost/program_options.hpp>
#include <QtCore/QSettings>

#include "util/logging.h"
#include <feature/sift.h>
#include <feature/matching.h>
#include <base/image_reader.h>
#include <optim/bundle_adjustment.h>
#include <controllers/incremental_mapper.h>
#include <mvs/patch_match.h>
#include <mvs/fusion.h>
#include <mvs/meshing.h>
#include <ui/render_options.h>


namespace colmap 
{

	struct ImageReaderOptions;
	struct SiftExtractionOptions;
	struct SiftMatchingOptions;
	struct ExhaustiveMatchingOptions;
	struct SequentialMatchingOptions;
	struct VocabTreeMatchingOptions;
	struct SpatialMatchingOptions;
	struct TransitiveMatchingOptions;
	struct ImagePairsMatchingOptions;
	struct BundleAdjustmentOptions;
	struct IncrementalMapperOptions;
	struct RenderOptions;

	namespace mvs 
	{
		struct PatchMatchOptions;
		struct StereoFusionOptions;
		struct PoissonMeshingOptions;
		struct DelaunayMeshingOptions;
	}  // namespace mvs

	enum CFeatureMatchMethod 
	{
		Exhaustive,
		Sequential,
		VocabTree,
		Spatial,
		Transitive,
		Custom,
		Retrieval
	};
	class OptionManager 
	{
	public:
		OptionManager(bool add_project_options = true);

		// Create "optimal" set of options for different reconstruction scenarios.
		// Note that the existing options are modified, so if your parameters are
		// already low quality, they will be further modified.
		void ModifyForIndividualData();
		void ModifyForVideoData();
		void ModifyForInternetData();

		// Create "optimal" set of options for different quality settings.
		// Note that the existing options are modified, so if your parameters are
		// already low quality, they will be further degraded.
		void ModifyForLowQuality();
		void ModifyForMediumQuality();
		void ModifyForHighQuality();
		void ModifyForExtremeQuality();

		void AddAllOptions();
		void AddLogOptions();
		void AddRandomOptions();
		void AddDatabaseOptions();
		void AddImageOptions();
		void AddExtractionOptions();
		void AddMatchingOptions();
		void AddExhaustiveMatchingOptions();
		void AddSequentialMatchingOptions();
		void AddVocabTreeMatchingOptions();
		void AddSpatialMatchingOptions();
		void AddTransitiveMatchingOptions();
		void AddImagePairsMatchingOptions();
		void AddBundleAdjustmentOptions();
		void AddMapperOptions();
		void AddPatchMatchStereoOptions();
		void AddStereoFusionOptions();
		void AddPoissonMeshingOptions();
		void AddDelaunayMeshingOptions();
		void AddRenderOptions();

		template <typename T>
		void AddRequiredOption(const std::string& name, T* option, const std::string& help_text = "");
		template <typename T>
		void AddDefaultOption(const std::string& name, T* option, const std::string& help_text = "");

		void Reset();
		void ResetOptions(const bool reset_paths);

		bool Check();

		void Parse(const int argc, char** argv);
		bool Read(const std::string& path);
		bool ReRead(const std::string& path);
		void Write(const std::string& path) const;

		std::shared_ptr<std::string> project_path;
		std::shared_ptr<std::string> database_path;
		std::shared_ptr<std::string> image_path;
		std::shared_ptr<CFeatureMatchMethod> FeatureMatchMethod;
		std::shared_ptr<std::string> PthPath;
		std::shared_ptr<std::string> HDF5Path;
		std::shared_ptr<std::string> CheckPointPath;
		std::shared_ptr<std::string> PCA_ModelPath;
		std::shared_ptr<std::string> TreeModelPath;
		std::shared_ptr<int> RetrievalTopN;

		std::shared_ptr<ImageReaderOptions> image_reader;
		std::shared_ptr<SiftExtractionOptions> sift_extraction;

		std::shared_ptr<SiftMatchingOptions> sift_matching;
		std::shared_ptr<ExhaustiveMatchingOptions> exhaustive_matching;
		std::shared_ptr<SequentialMatchingOptions> sequential_matching;
		std::shared_ptr<VocabTreeMatchingOptions> vocab_tree_matching;
		std::shared_ptr<SpatialMatchingOptions> spatial_matching;
		std::shared_ptr<TransitiveMatchingOptions> transitive_matching;
		std::shared_ptr<ImagePairsMatchingOptions> image_pairs_matching;

		std::shared_ptr<BundleAdjustmentOptions> bundle_adjustment;
		std::shared_ptr<IncrementalMapperOptions> mapper;

		std::shared_ptr<mvs::PatchMatchOptions> patch_match_stereo;
		std::shared_ptr<mvs::StereoFusionOptions> stereo_fusion;
		std::shared_ptr<mvs::PoissonMeshingOptions> poisson_meshing;
		std::shared_ptr<mvs::DelaunayMeshingOptions> delaunay_meshing;

		std::shared_ptr<RenderOptions> render;

		inline void RegReadOptions()
		{
			QSettings ExtractorOptionsKey("RTPS", "ExtractorOptions");
			sift_extraction->max_image_size = ExtractorOptionsKey.value("max_image_size", 2000).toInt();
			sift_extraction->max_num_features = ExtractorOptionsKey.value("max_num_features", 16384).toInt();
			sift_extraction->first_octave = ExtractorOptionsKey.value("first_octave", -1).toInt();
			sift_extraction->num_octaves = ExtractorOptionsKey.value("num_octaves", 4).toInt();
			sift_extraction->octave_resolution = ExtractorOptionsKey.value("octave_resolution", 3).toInt();
			sift_extraction->peak_threshold = ExtractorOptionsKey.value("peak_threshold", 0.0066666).toDouble();
			sift_extraction->edge_threshold = ExtractorOptionsKey.value("edge_threshold", 10.00).toDouble();
			sift_extraction->estimate_affine_shape = ExtractorOptionsKey.value("estimate_affine_shape", false).toBool();
			sift_extraction->max_num_orientations = ExtractorOptionsKey.value("max_num_orientations", 2).toInt();
			sift_extraction->upright = ExtractorOptionsKey.value("upright", false).toBool();
			sift_extraction->domain_size_pooling = ExtractorOptionsKey.value("domain_size_pooling", false).toBool();
			sift_extraction->dsp_min_scale = ExtractorOptionsKey.value("dsp_min_scale", 0.166666666).toDouble();
			sift_extraction->dsp_max_scale = ExtractorOptionsKey.value("dsp_max_scale", 3.0).toDouble();
			sift_extraction->dsp_num_scales = ExtractorOptionsKey.value("dsp_num_scales", 10).toInt();
			sift_extraction->use_gpu = ExtractorOptionsKey.value("use_gpu", true).toBool();
			sift_extraction->gpu_index = QString2StdString(ExtractorOptionsKey.value("gpu_index", -1).toString());
			image_reader->camera_model = QString2StdString(ExtractorOptionsKey.value("camera_model", "SIMPLE_RADIAL").toString());
			image_reader->single_camera = ExtractorOptionsKey.value("single_camera", false).toBool();
			image_reader->single_camera_per_folder = ExtractorOptionsKey.value("single_camera_per_folder", false).toBool();
			image_reader->camera_params = QString2StdString(ExtractorOptionsKey.value("camera_params", "").toString());

			QSettings MatcherOptionsKey("RTPS", "MatcherOptions");
			std::string FeatureMatchMethod_String = QString2StdString(MatcherOptionsKey.value("feature_match_method", "Exhaustive").toString());
			if (FeatureMatchMethod_String == "Exhaustive")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Exhaustive;
			}
			else if (FeatureMatchMethod_String == "Retrieval")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Retrieval;
			}
			else if (FeatureMatchMethod_String == "Sequential")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Sequential;
			}
			else if (FeatureMatchMethod_String == "VocabTree")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::VocabTree;
			}
			else if (FeatureMatchMethod_String == "Spatial")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Spatial;
			}
			else if (FeatureMatchMethod_String == "Transitive")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Transitive;
			}
			else if (FeatureMatchMethod_String == "Custom")
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Custom;
			}
			else
			{
				*FeatureMatchMethod = colmap::CFeatureMatchMethod::Exhaustive;
			}

			sift_matching->use_gpu = MatcherOptionsKey.value("use_gpu", true).toBool();
			sift_matching->gpu_index = QString2StdString(MatcherOptionsKey.value("gpu_index", -1).toString());
			sift_matching->max_ratio = MatcherOptionsKey.value("max_ratio", 0.8).toDouble();
			sift_matching->max_distance = MatcherOptionsKey.value("max_distance", 0.7).toDouble();
			sift_matching->cross_check = MatcherOptionsKey.value("cross_check", true).toBool();
			sift_matching->max_num_matches = MatcherOptionsKey.value("max_num_matches", 16384).toInt();
			sift_matching->max_error = MatcherOptionsKey.value("max_error", 4.0).toDouble();
			sift_matching->confidence = MatcherOptionsKey.value("confidence", 0.99).toDouble();
			sift_matching->max_num_trials = MatcherOptionsKey.value("max_num_trials", 10000).toInt();
			sift_matching->min_inlier_ratio = MatcherOptionsKey.value("min_inlier_ratio", 0.25).toDouble();
			sift_matching->min_num_inliers = MatcherOptionsKey.value("min_num_inliers", 15).toInt();
			sift_matching->multiple_models = MatcherOptionsKey.value("multiple_models", true).toBool();
			sift_matching->guided_matching = MatcherOptionsKey.value("guided_matching", true).toBool();
			sift_matching->planar_scene = MatcherOptionsKey.value("planar_scene", false).toBool();
			exhaustive_matching->block_size = MatcherOptionsKey.value("Exhaustive/block_size", 50).toInt();
			sequential_matching->overlap = MatcherOptionsKey.value("Sequential/overlap", 10).toInt();
			sequential_matching->quadratic_overlap = MatcherOptionsKey.value("Sequential/quadratic_overlap", true).toBool();
			sequential_matching->loop_detection = MatcherOptionsKey.value("Sequential/loop_detection", false).toBool();
			sequential_matching->loop_detection_period = MatcherOptionsKey.value("Sequential/loop_detection_period", 10).toInt();
			sequential_matching->loop_detection_num_images = MatcherOptionsKey.value("Sequential/loop_detection_num_images", 50).toInt();
			sequential_matching->loop_detection_num_nearest_neighbors = MatcherOptionsKey.value("Sequential/loop_detection_num_nearest_neighbors", 1).toInt();
			sequential_matching->loop_detection_num_checks = MatcherOptionsKey.value("Sequential/loop_detection_num_checks", 256).toInt();
			sequential_matching->loop_detection_num_images_after_verification = MatcherOptionsKey.value("Sequential/loop_detection_num_images_after_verification", 0).toInt();
			sequential_matching->loop_detection_max_num_features = MatcherOptionsKey.value("Sequential/loop_detection_max_num_features", -1).toInt();
			sequential_matching->vocab_tree_path = QString2StdString(MatcherOptionsKey.value("Sequential/vocab_tree_path", "").toString());
			vocab_tree_matching->num_images = MatcherOptionsKey.value("VocabTree/num_images", 100).toInt();
			vocab_tree_matching->num_nearest_neighbors = MatcherOptionsKey.value("VocabTree/num_nearest_neighbors", 5).toInt();
			vocab_tree_matching->num_checks = MatcherOptionsKey.value("VocabTree/num_checks", 256).toInt();
			vocab_tree_matching->num_images_after_verification = MatcherOptionsKey.value("VocabTree/num_images_after_verification", 0).toInt();
			vocab_tree_matching->max_num_features = MatcherOptionsKey.value("VocabTree/max_num_features", -1).toInt();
			vocab_tree_matching->vocab_tree_path = QString2StdString(MatcherOptionsKey.value("VocabTree/vocab_tree_path", "").toString());
			spatial_matching->is_gps = MatcherOptionsKey.value("Spatial/is_gps", true).toBool();
			spatial_matching->ignore_z = MatcherOptionsKey.value("Spatial/ignore_z", true).toBool();
			spatial_matching->max_num_neighbors = MatcherOptionsKey.value("Spatial/max_num_neighbors", 50).toInt();
			spatial_matching->max_distance = MatcherOptionsKey.value("Spatial/max_distance", 100.0).toDouble();
			transitive_matching->batch_size = MatcherOptionsKey.value("Transitive/batch_size", 1000).toInt();
			transitive_matching->num_iterations = MatcherOptionsKey.value("Transitive/num_iterations", 3).toInt();
			image_pairs_matching->block_size = MatcherOptionsKey.value("Custom/block_size", 1225).toInt();
			*PthPath = QString2StdString(MatcherOptionsKey.value("Retrieval/VGG_pth_file_path", "").toString());
			*HDF5Path = QString2StdString(MatcherOptionsKey.value("Retrieval/HDF5_file_path", "").toString());
			*CheckPointPath = QString2StdString(MatcherOptionsKey.value("Retrieval/checkpoint_file_path", "").toString());
			*PCA_ModelPath = QString2StdString(MatcherOptionsKey.value("Retrieval/PCA_model_path", "").toString());
			*TreeModelPath = QString2StdString(MatcherOptionsKey.value("Retrieval/tree_path", "").toString());
			*RetrievalTopN = MatcherOptionsKey.value("Retrieval/TopN", 30).toInt();

			QSettings ReconstructionOptionsKey("RTPS", "ReconstructionOptions");
			mapper->multiple_models = ReconstructionOptionsKey.value("multiple_models", true).toBool();
			mapper->max_num_models = ReconstructionOptionsKey.value("max_num_models", 10000000).toInt();
			mapper->max_model_overlap = ReconstructionOptionsKey.value("max_model_overlap", 4).toInt();
			mapper->min_model_size = ReconstructionOptionsKey.value("min_model_size", 3).toInt();
			mapper->extract_colors = ReconstructionOptionsKey.value("extract_colors", true).toBool();
			mapper->num_threads = ReconstructionOptionsKey.value("num_threads", -1).toInt();
			mapper->min_num_matches = ReconstructionOptionsKey.value("min_num_matches", 100).toInt();
			mapper->ignore_watermarks = ReconstructionOptionsKey.value("ignore_watermarks", false).toBool();
			mapper->snapshot_path = QString2StdString(ReconstructionOptionsKey.value("snapshot_path", "").toString());
			mapper->snapshot_images_freq = ReconstructionOptionsKey.value("snapshot_images_freq", 0).toInt();
			mapper->init_image_id1 = ReconstructionOptionsKey.value("init_image_id1", -1).toInt();
			mapper->init_image_id2 = ReconstructionOptionsKey.value("init_image_id2", -1).toInt();
			mapper->init_num_trials = ReconstructionOptionsKey.value("init_num_trials", 200).toInt();
			mapper->mapper.init_min_num_inliers = ReconstructionOptionsKey.value("init_min_num_inliers", 100).toInt();
			mapper->mapper.init_max_error = ReconstructionOptionsKey.value("init_max_error", 4.0).toDouble();
			mapper->mapper.init_max_forward_motion = ReconstructionOptionsKey.value("init_max_forward_motion", 0.95).toDouble();
			mapper->mapper.init_min_tri_angle = ReconstructionOptionsKey.value("init_min_tri_angle", 16.0).toDouble();
			mapper->mapper.init_max_reg_trials = ReconstructionOptionsKey.value("init_max_reg_trials", 2).toInt();
			mapper->mapper.abs_pose_max_error = ReconstructionOptionsKey.value("abs_pose_max_error", 12.0).toDouble();
			mapper->mapper.abs_pose_min_num_inliers = ReconstructionOptionsKey.value("abs_pose_min_num_inliers", 30).toInt();
			mapper->mapper.abs_pose_min_inlier_ratio = ReconstructionOptionsKey.value("abs_pose_min_inlier_ratio", 0.25).toDouble();
			mapper->mapper.max_reg_trials = ReconstructionOptionsKey.value("max_reg_trials", 3).toInt();
			mapper->triangulation.max_transitivity = ReconstructionOptionsKey.value("max_transitivity", 1).toInt();
			mapper->triangulation.create_max_angle_error = ReconstructionOptionsKey.value("create_max_angle_error", 2.0).toDouble();
			mapper->triangulation.continue_max_angle_error = ReconstructionOptionsKey.value("continue_max_angle_error", 2.00).toDouble();
			mapper->triangulation.merge_max_reproj_error = ReconstructionOptionsKey.value("merge_max_reproj_error", 4.00).toDouble();
			mapper->triangulation.re_max_angle_error = ReconstructionOptionsKey.value("re_max_angle_error", 5.00).toDouble();
			mapper->triangulation.re_min_ratio = ReconstructionOptionsKey.value("re_min_ratio", 0.20).toDouble();
			mapper->triangulation.re_max_trials = ReconstructionOptionsKey.value("re_max_trials", 1).toInt();
			mapper->triangulation.complete_max_reproj_error = ReconstructionOptionsKey.value("complete_max_reproj_error", 4.00).toDouble();
			mapper->triangulation.complete_max_transitivity = ReconstructionOptionsKey.value("complete_max_transitivity", 5).toInt();
			mapper->triangulation.min_angle = ReconstructionOptionsKey.value("min_angle", 1.50).toDouble();
			mapper->triangulation.ignore_two_view_tracks = ReconstructionOptionsKey.value("ignore_two_view_tracks", true).toBool();
			mapper->ba_refine_focal_length = ReconstructionOptionsKey.value("refine_focal_length", true).toBool();
			mapper->ba_refine_principal_point = ReconstructionOptionsKey.value("refine_principal_point", false).toBool();
			mapper->ba_refine_extra_params = ReconstructionOptionsKey.value("refine_extra_params", true).toBool();
			mapper->ba_local_num_images = ReconstructionOptionsKey.value("num_images", 8).toInt();
			mapper->ba_local_max_num_iterations = ReconstructionOptionsKey.value("max_num_iterations", 25).toInt();
			mapper->ba_local_max_refinements = ReconstructionOptionsKey.value("max_refinements", 2).toInt();
			mapper->ba_local_max_refinement_change = ReconstructionOptionsKey.value("max_refinement_change", 0.001).toDouble();
			mapper->ba_global_images_ratio = ReconstructionOptionsKey.value("images_ratio", 1.10).toDouble();
			mapper->ba_global_images_freq = ReconstructionOptionsKey.value("images_freq", 500).toInt();
			mapper->ba_global_points_ratio = ReconstructionOptionsKey.value("points_ratio", 1.10).toDouble();
			mapper->ba_global_points_freq = ReconstructionOptionsKey.value("points_freq", 250000).toInt();
			mapper->ba_global_max_num_iterations = ReconstructionOptionsKey.value("max_num_iterations", 30).toInt();
			mapper->ba_global_max_refinements = ReconstructionOptionsKey.value("max_refinements", 2).toInt();
			mapper->ba_global_max_refinement_change = ReconstructionOptionsKey.value("max_refinement_change", 0.0005).toDouble();
			mapper->min_focal_length_ratio = ReconstructionOptionsKey.value("min_focal_length_ratio", 0.1).toDouble();
			mapper->max_focal_length_ratio = ReconstructionOptionsKey.value("max_focal_length_ratio", 10.0).toDouble();
			mapper->max_extra_param = ReconstructionOptionsKey.value("max_extra_param", 1.00).toDouble();
			mapper->mapper.filter_max_reproj_error = ReconstructionOptionsKey.value("filter_max_reproj_error", 4.00).toDouble();
			mapper->mapper.filter_min_tri_angle = ReconstructionOptionsKey.value("filter_min_tri_angle", 1.50).toDouble();
		}
		inline void RegWriteOptions()
		{
			QSettings ExtractorOptionsKey("RTPS", "ExtractorOptions");
			ExtractorOptionsKey.setValue("max_image_size", sift_extraction->max_image_size);
			ExtractorOptionsKey.setValue("max_num_features", sift_extraction->max_num_features);
			ExtractorOptionsKey.setValue("first_octave", sift_extraction->first_octave);
			ExtractorOptionsKey.setValue("num_octaves", sift_extraction->num_octaves);
			ExtractorOptionsKey.setValue("octave_resolution", sift_extraction->octave_resolution);
			ExtractorOptionsKey.setValue("peak_threshold", sift_extraction->peak_threshold);
			ExtractorOptionsKey.setValue("edge_threshold", sift_extraction->edge_threshold);
			ExtractorOptionsKey.setValue("estimate_affine_shape", sift_extraction->estimate_affine_shape);
			ExtractorOptionsKey.setValue("max_num_orientations", sift_extraction->max_num_orientations);
			ExtractorOptionsKey.setValue("upright", sift_extraction->upright);
			ExtractorOptionsKey.setValue("domain_size_pooling", sift_extraction->domain_size_pooling);
			ExtractorOptionsKey.setValue("dsp_min_scale", sift_extraction->dsp_min_scale);
			ExtractorOptionsKey.setValue("dsp_max_scale", sift_extraction->dsp_max_scale);
			ExtractorOptionsKey.setValue("dsp_num_scales", sift_extraction->dsp_num_scales);
			ExtractorOptionsKey.setValue("use_gpu", sift_extraction->use_gpu);
			ExtractorOptionsKey.setValue("gpu_index", StdString2QString(sift_extraction->gpu_index));
			ExtractorOptionsKey.setValue("camera_model", StdString2QString(image_reader->camera_model));
			ExtractorOptionsKey.setValue("single_camera", image_reader->single_camera);
			ExtractorOptionsKey.setValue("single_camera_per_folder", image_reader->single_camera_per_folder);
			ExtractorOptionsKey.setValue("camera_params", StdString2QString(image_reader->camera_params));


			QSettings MatcherOptionsKey("RTPS", "MatcherOptions");
			switch (*FeatureMatchMethod)
			{
			case colmap::CFeatureMatchMethod::Exhaustive:
				MatcherOptionsKey.setValue("feature_match_method", "Exhaustive");
				break;
			case colmap::CFeatureMatchMethod::Retrieval:
				MatcherOptionsKey.setValue("feature_match_method", "Retrieval");
				break;
			case colmap::CFeatureMatchMethod::Sequential:
				MatcherOptionsKey.setValue("feature_match_method", "Sequential");
				break;
			case colmap::CFeatureMatchMethod::VocabTree:
				MatcherOptionsKey.setValue("feature_match_method", "VocabTree");
				break;
			case colmap::CFeatureMatchMethod::Spatial:
				MatcherOptionsKey.setValue("feature_match_method", "Spatial");
				break;
			case colmap::CFeatureMatchMethod::Transitive:
				MatcherOptionsKey.setValue("feature_match_method", "Transitive");
				break;
			case colmap::CFeatureMatchMethod::Custom:
				MatcherOptionsKey.setValue("feature_match_method", "Custom");
				break;
			default:
				MatcherOptionsKey.setValue("feature_match_method", "Exhaustive");
				break;
			}
			MatcherOptionsKey.setValue("use_gpu", sift_matching->use_gpu);
			MatcherOptionsKey.setValue("gpu_index", StdString2QString(sift_matching->gpu_index));
			MatcherOptionsKey.setValue("max_ratio", sift_matching->max_ratio);
			MatcherOptionsKey.setValue("max_distance", sift_matching->max_distance);
			MatcherOptionsKey.setValue("cross_check", sift_matching->cross_check);
			MatcherOptionsKey.setValue("max_num_matches", sift_matching->max_num_matches);
			MatcherOptionsKey.setValue("max_error", sift_matching->max_error);
			MatcherOptionsKey.setValue("confidence", sift_matching->confidence);
			MatcherOptionsKey.setValue("max_num_trials", sift_matching->max_num_trials);
			MatcherOptionsKey.setValue("min_inlier_ratio", sift_matching->min_inlier_ratio);
			MatcherOptionsKey.setValue("min_num_inliers", sift_matching->min_num_inliers);
			MatcherOptionsKey.setValue("multiple_models", sift_matching->multiple_models);
			MatcherOptionsKey.setValue("guided_matching", sift_matching->guided_matching);
			MatcherOptionsKey.setValue("planar_scene", sift_matching->planar_scene);
			MatcherOptionsKey.setValue("Exhaustive/block_size", exhaustive_matching->block_size);
			MatcherOptionsKey.setValue("Sequential/overlap", sequential_matching->overlap);
			MatcherOptionsKey.setValue("Sequential/quadratic_overlap", sequential_matching->quadratic_overlap);
			MatcherOptionsKey.setValue("Sequential/loop_detection", sequential_matching->loop_detection);
			MatcherOptionsKey.setValue("Sequential/loop_detection_period", sequential_matching->loop_detection_period);
			MatcherOptionsKey.setValue("Sequential/loop_detection_num_images", sequential_matching->loop_detection_num_images);
			MatcherOptionsKey.setValue("Sequential/loop_detection_num_nearest_neighbors", sequential_matching->loop_detection_num_nearest_neighbors);
			MatcherOptionsKey.setValue("Sequential/loop_detection_num_checks", sequential_matching->loop_detection_num_checks);
			MatcherOptionsKey.setValue("Sequential/loop_detection_num_images_after_verification", sequential_matching->loop_detection_num_images_after_verification);
			MatcherOptionsKey.setValue("Sequential/loop_detection_max_num_features", sequential_matching->loop_detection_max_num_features);
			MatcherOptionsKey.setValue("Sequential/vocab_tree_path", StdString2QString(sequential_matching->vocab_tree_path));
			MatcherOptionsKey.setValue("VocabTree/num_images", vocab_tree_matching->num_images);
			MatcherOptionsKey.setValue("VocabTree/num_nearest_neighbors", vocab_tree_matching->num_nearest_neighbors);
			MatcherOptionsKey.setValue("VocabTree/num_checks", vocab_tree_matching->num_checks);
			MatcherOptionsKey.setValue("VocabTree/num_images_after_verification", vocab_tree_matching->num_images_after_verification);
			MatcherOptionsKey.setValue("VocabTree/max_num_features", vocab_tree_matching->max_num_features);
			MatcherOptionsKey.setValue("VocabTree/vocab_tree_path", StdString2QString(vocab_tree_matching->vocab_tree_path));
			MatcherOptionsKey.setValue("Spatial/is_gps", spatial_matching->is_gps);
			MatcherOptionsKey.setValue("Spatial/ignore_z", spatial_matching->ignore_z);
			MatcherOptionsKey.setValue("Spatial/max_num_neighbors", spatial_matching->max_num_neighbors);
			MatcherOptionsKey.setValue("Spatial/max_distance", spatial_matching->max_distance);
			MatcherOptionsKey.setValue("Transitive/batch_size", transitive_matching->batch_size);
			MatcherOptionsKey.setValue("Transitive/num_iterations", transitive_matching->num_iterations);
			MatcherOptionsKey.setValue("Custom/block_size", image_pairs_matching->block_size);
			MatcherOptionsKey.setValue("Retrieval/VGG_pth_file_path", StdString2QString(*PthPath));
			MatcherOptionsKey.setValue("Retrieval/HDF5_file_path", StdString2QString(*HDF5Path));
			MatcherOptionsKey.setValue("Retrieval/checkpoint_file_path", StdString2QString(*CheckPointPath));
			MatcherOptionsKey.setValue("Retrieval/PCA_model_path", StdString2QString(*PCA_ModelPath));
			MatcherOptionsKey.setValue("Retrieval/tree_path", StdString2QString(*TreeModelPath));
			MatcherOptionsKey.setValue("Retrieval/TopN", *RetrievalTopN);

			QSettings ReconstructionOptionsKey("RTPS", "ReconstructionOptions");
			ReconstructionOptionsKey.setValue("multiple_models", mapper->multiple_models);
			ReconstructionOptionsKey.setValue("max_num_models", mapper->max_num_models);
			ReconstructionOptionsKey.setValue("max_model_overlap", mapper->max_model_overlap);
			ReconstructionOptionsKey.setValue("min_model_size", mapper->min_model_size);
			ReconstructionOptionsKey.setValue("extract_colors", mapper->extract_colors);
			ReconstructionOptionsKey.setValue("num_threads", mapper->num_threads);
			ReconstructionOptionsKey.setValue("min_num_matches", mapper->min_num_matches);
			ReconstructionOptionsKey.setValue("ignore_watermarks", mapper->ignore_watermarks);
			ReconstructionOptionsKey.setValue("snapshot_path", StdString2QString(mapper->snapshot_path));
			ReconstructionOptionsKey.setValue("snapshot_images_freq", mapper->snapshot_images_freq);
			ReconstructionOptionsKey.setValue("init_image_id1", mapper->init_image_id1);
			ReconstructionOptionsKey.setValue("init_image_id2", mapper->init_image_id2);
			ReconstructionOptionsKey.setValue("init_num_trials", mapper->init_num_trials);
			ReconstructionOptionsKey.setValue("init_min_num_inliers", mapper->mapper.init_min_num_inliers);
			ReconstructionOptionsKey.setValue("init_max_error", mapper->mapper.init_max_error);
			ReconstructionOptionsKey.setValue("init_max_forward_motion", mapper->mapper.init_max_forward_motion);
			ReconstructionOptionsKey.setValue("init_min_tri_angle", mapper->mapper.init_min_tri_angle);
			ReconstructionOptionsKey.setValue("init_max_reg_trials", mapper->mapper.init_max_reg_trials);
			ReconstructionOptionsKey.setValue("abs_pose_max_error", mapper->mapper.abs_pose_max_error);
			ReconstructionOptionsKey.setValue("abs_pose_min_num_inliers", mapper->mapper.abs_pose_min_num_inliers);
			ReconstructionOptionsKey.setValue("abs_pose_min_inlier_ratio", mapper->mapper.abs_pose_min_inlier_ratio);
			ReconstructionOptionsKey.setValue("max_reg_trials", mapper->mapper.max_reg_trials);
			ReconstructionOptionsKey.setValue("max_transitivity", mapper->triangulation.max_transitivity);
			ReconstructionOptionsKey.setValue("create_max_angle_error", mapper->triangulation.create_max_angle_error);
			ReconstructionOptionsKey.setValue("continue_max_angle_error", mapper->triangulation.continue_max_angle_error);
			ReconstructionOptionsKey.setValue("merge_max_reproj_error", mapper->triangulation.merge_max_reproj_error);
			ReconstructionOptionsKey.setValue("re_max_angle_error", mapper->triangulation.re_max_angle_error);
			ReconstructionOptionsKey.setValue("re_min_ratio", mapper->triangulation.re_min_ratio);
			ReconstructionOptionsKey.setValue("re_max_trials", mapper->triangulation.re_max_trials);
			ReconstructionOptionsKey.setValue("complete_max_reproj_error", mapper->triangulation.complete_max_reproj_error);
			ReconstructionOptionsKey.setValue("complete_max_transitivity", mapper->triangulation.complete_max_transitivity);
			ReconstructionOptionsKey.setValue("min_angle", mapper->triangulation.min_angle);
			ReconstructionOptionsKey.setValue("ignore_two_view_tracks", mapper->triangulation.ignore_two_view_tracks);
			ReconstructionOptionsKey.setValue("refine_focal_length", mapper->ba_refine_focal_length);
			ReconstructionOptionsKey.setValue("refine_principal_point", mapper->ba_refine_principal_point);
			ReconstructionOptionsKey.setValue("refine_extra_params", mapper->ba_refine_extra_params);
			ReconstructionOptionsKey.setValue("num_images", mapper->ba_local_num_images);
			ReconstructionOptionsKey.setValue("max_num_iterations", mapper->ba_local_max_num_iterations);
			ReconstructionOptionsKey.setValue("max_refinements", mapper->ba_local_max_refinements);
			ReconstructionOptionsKey.setValue("max_refinement_change", mapper->ba_local_max_refinement_change);
			ReconstructionOptionsKey.setValue("images_ratio", mapper->ba_global_images_ratio);
			ReconstructionOptionsKey.setValue("images_freq", mapper->ba_global_images_freq);
			ReconstructionOptionsKey.setValue("points_ratio", mapper->ba_global_points_ratio);
			ReconstructionOptionsKey.setValue("points_freq", mapper->ba_global_points_freq);
			ReconstructionOptionsKey.setValue("max_num_iterations", mapper->ba_global_max_num_iterations);
			ReconstructionOptionsKey.setValue("max_refinements", mapper->ba_global_max_refinements);
			ReconstructionOptionsKey.setValue("max_refinement_change", mapper->ba_global_max_refinement_change);
			ReconstructionOptionsKey.setValue("min_focal_length_ratio", mapper->min_focal_length_ratio);
			ReconstructionOptionsKey.setValue("max_focal_length_ratio", mapper->max_focal_length_ratio);
			ReconstructionOptionsKey.setValue("max_extra_param", mapper->max_extra_param);
			ReconstructionOptionsKey.setValue("filter_max_reproj_error", mapper->mapper.filter_max_reproj_error);
			ReconstructionOptionsKey.setValue("filter_min_tri_angle", mapper->mapper.filter_min_tri_angle);
		}



	private:
		template <typename T>
		void AddAndRegisterRequiredOption(const std::string& name, T* option, const std::string& help_text = "");
		template <typename T>
		void AddAndRegisterDefaultOption(const std::string& name, T* option, const std::string& help_text = "");

		template <typename T>
		void RegisterOption(const std::string& name, const T* option);

		std::shared_ptr<boost::program_options::options_description> desc_;

		std::vector<std::pair<std::string, const bool*>> options_bool_;
		std::vector<std::pair<std::string, const int*>> options_int_;
		std::vector<std::pair<std::string, const double*>> options_double_;
		std::vector<std::pair<std::string, const std::string*>> options_string_;

		bool added_log_options_;
		bool added_random_options_;
		bool added_database_options_;
		bool added_image_options_;
		bool added_extraction_options_;
		bool added_match_options_;
		bool added_exhaustive_match_options_;
		bool added_sequential_match_options_;
		bool added_vocab_tree_match_options_;
		bool added_spatial_match_options_;
		bool added_transitive_match_options_;
		bool added_image_pairs_match_options_;
		bool added_ba_options_;
		bool added_mapper_options_;
		bool added_patch_match_stereo_options_;
		bool added_stereo_fusion_options_;
		bool added_poisson_meshing_options_;
		bool added_delaunay_meshing_options_;
		bool added_render_options_;

		inline std::string QString2StdString(QString QString)
		{
			return std::string(QString.toLocal8Bit());
		}
		inline QString StdString2QString(std::string string)
		{
			return QString::fromLocal8Bit(string.data());
		}
	};

	////////////////////////////////////////////////////////////////////////////////
	// Implementation
	////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	void OptionManager::AddRequiredOption(const std::string& name, T* option, const std::string& help_text)
	{
		desc_->add_options()(name.c_str(),
			boost::program_options::value<T>(option)->required(),
			help_text.c_str());
	}

	template <typename T>
	void OptionManager::AddDefaultOption(const std::string& name, T* option,
		const std::string& help_text) {
		desc_->add_options()(
			name.c_str(),
			boost::program_options::value<T>(option)->default_value(*option),
			help_text.c_str());
	}

	template <typename T>
	void OptionManager::AddAndRegisterRequiredOption(const std::string& name,
		T* option,
		const std::string& help_text) {
		desc_->add_options()(name.c_str(),
			boost::program_options::value<T>(option)->required(),
			help_text.c_str());
		RegisterOption(name, option);
	}

	template <typename T>
	void OptionManager::AddAndRegisterDefaultOption(const std::string& name,
		T* option,
		const std::string& help_text) {
		desc_->add_options()(
			name.c_str(),
			boost::program_options::value<T>(option)->default_value(*option),
			help_text.c_str());
		RegisterOption(name, option);
	}

	template <typename T>
	void OptionManager::RegisterOption(const std::string& name, const T* option) {
		if (std::is_same<T, bool>::value) {
			options_bool_.emplace_back(name, reinterpret_cast<const bool*>(option));
		}
		else if (std::is_same<T, int>::value) {
			options_int_.emplace_back(name, reinterpret_cast<const int*>(option));
		}
		else if (std::is_same<T, double>::value) {
			options_double_.emplace_back(name, reinterpret_cast<const double*>(option));
		}
		else if (std::is_same<T, std::string>::value) {
			options_string_.emplace_back(name,
				reinterpret_cast<const std::string*>(option));
		}
		else {
			LOG(FATAL) << "Unsupported option type";
		}
	}

}  // namespace colmap

#endif  // COLMAP_SRC_UTIL_OPTION_MANAGER_H_
