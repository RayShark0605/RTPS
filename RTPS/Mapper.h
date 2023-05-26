#pragma once
#include "Base.h"
#include "Model.h"
#include "Triangulator.h"
#include "BundleAdjuster.h"

class CMapper
{
public:
    struct Options 
    {
        // Minimum number of inliers for initial image pair.
        int init_min_num_inliers = 100;

        // Maximum error in pixels for two-view geometry estimation for initial
        // image pair.
        double init_max_error = 4.0;

        // Maximum forward motion for initial image pair.
        double init_max_forward_motion = 0.95;

        // Minimum triangulation angle for initial image pair.
        double init_min_tri_angle = 16.0;

        // Maximum number of trials to use an image for initialization.
        int init_max_reg_trials = 2;

        // Maximum reprojection error in absolute pose estimation.
        double abs_pose_max_error = 12.0;

        // Minimum number of inliers in absolute pose estimation.
        int abs_pose_min_num_inliers = 30;

        // Minimum inlier ratio in absolute pose estimation.
        double abs_pose_min_inlier_ratio = 0.25;

        // Whether to estimate the focal length in absolute pose estimation.
        bool abs_pose_refine_focal_length = true;

        // Whether to estimate the extra parameters in absolute pose estimation.
        bool abs_pose_refine_extra_params = true;

        // Number of images to optimize in local bundle adjustment.
        int local_ba_num_images = 6;

        // Minimum triangulation for images to be chosen in local bundle adjustment.
        double local_ba_min_tri_angle = 6;

        // Thresholds for bogus camera parameters. Images with bogus camera
        // parameters are filtered and ignored in triangulation.
        double min_focal_length_ratio = 0.1;  // Opening angle of ~130deg
        double max_focal_length_ratio = 10;   // Opening angle of ~5deg
        double max_extra_param = 1;

        // Maximum reprojection error in pixels for observations.
        double filter_max_reproj_error = 4.0;

        // Minimum triangulation angle in degrees for stable 3D points.
        double filter_min_tri_angle = 1.5;

        // Maximum number of trials to register an image.
        int max_reg_trials = 3;

        // If reconstruction is provided as input, fix the existing image poses.
        bool fix_existing_images = false;

        // Number of threads.
        int num_threads = -1;

        // Method to find and select next best image to register.
        enum class ImageSelectionMethod 
        {
            MAX_VISIBLE_POINTS_NUM,
            MAX_VISIBLE_POINTS_RATIO,
            MIN_UNCERTAINTY,
        };
        ImageSelectionMethod image_selection_method = ImageSelectionMethod::MIN_UNCERTAINTY;

        bool Check();
    };
    struct LocalBundleAdjustmentReport 
    {
        size_t num_merged_observations = 0;
        size_t num_completed_observations = 0;
        size_t num_filtered_observations = 0;
        size_t num_adjusted_observations = 0;
    };
    explicit CMapper(colmap::DatabaseCache* database_cache);
    void BeginReconstruction(CModel* reconstruction);
    void EndReconstruction(bool IsDiscard);
    bool FindInitialImagePair(Options options, size_t* image_id1, size_t* image_id2);
    std::vector<size_t> FindNextImages(Options options);
    bool RegisterInitialImagePair(Options options, size_t image_id1, size_t image_id2);
    bool RegisterNextImage(Options options, size_t image_id);
    size_t TriangulateImage(CTriangulator::Options tri_options, size_t image_id);
    size_t Retriangulate(CTriangulator::Options tri_options);
    size_t CompleteTracks(CTriangulator::Options tri_options);
    size_t MergeTracks(CTriangulator::Options tri_options);
    LocalBundleAdjustmentReport AdjustLocalBundle(Options options, CBundleAdjustmentOptions ba_options, CTriangulator::Options tri_options, size_t image_id, const std::unordered_set<size_t>& point3D_ids);
    bool AdjustGlobalBundle(Options& options, CBundleAdjustmentOptions& ba_options);
    size_t FilterImages(Options options);
    size_t FilterPoints(Options options);
    CModel* GetModel();
    size_t NumTotalRegImages();
    size_t NumSharedRegImages();
    const std::unordered_set<size_t>& GetModifiedPoints3D();
    void ClearModifiedPoints3D();

private:
    CModel* Model_;
    colmap::DatabaseCache* database_cache_;
    std::unique_ptr<CTriangulator> triangulator_;
    size_t num_total_reg_images_;
    size_t num_shared_reg_images_;
    colmap::image_pair_t prev_init_image_pair_id_;
    colmap::TwoViewGeometry prev_init_two_view_geometry_;
    std::unordered_map<size_t, size_t> init_num_reg_trials_;
    std::unordered_set<colmap::image_pair_t> init_image_pairs_;
    std::unordered_map<size_t, size_t> num_reg_images_per_camera_;
    std::unordered_map<size_t, size_t> num_registrations_;
    std::unordered_set<size_t> filtered_images_;
    std::unordered_map<size_t, size_t> num_reg_trials_;
    std::unordered_set<size_t> existing_image_ids_;

    std::vector<size_t> FindFirstInitialImage(Options& options);
    std::vector<size_t> FindSecondInitialImage(Options& options, size_t image_id1);
    std::vector<size_t> FindLocalBundle(Options& options, size_t image_id);
    void RegisterImageEvent(size_t image_id);
    void DeRegisterImageEvent(size_t image_id);

    bool EstimateInitialTwoViewGeometry(Options& options, size_t image_id1, size_t image_id2);
};

struct CMapperOptions
{
public:

    CMapperOptions(colmap::IncrementalMapperOptions options);

    CMapper::Options mapper;
    CTriangulator::Options triangulation;

    // The minimum number of matches for inlier matches to be considered.
    int min_num_matches = 15;

    // Whether to ignore the inlier matches of watermark image pairs.
    bool ignore_watermarks = false;

    // Whether to reconstruct multiple sub-models.
    bool multiple_models = true;

    // The number of sub-models to reconstruct.
    int max_num_models = 50;

    // The maximum number of overlapping images between sub-models. If the
    // current sub-models shares more than this number of images with another
    // model, then the reconstruction is stopped.
    int max_model_overlap = 4;

    // The minimum number of registered images of a sub-model, otherwise the
    // sub-model is discarded.
    int min_model_size = 10;

    // The image identifiers used to initialize the reconstruction. Note that
    // only one or both image identifiers can be specified. In the former case,
    // the second image is automatically determined.
    int init_image_id1 = -1;
    int init_image_id2 = -1;

    // The number of trials to initialize the reconstruction.
    int init_num_trials = 200;

    // Whether to extract colors for reconstructed points.
    bool extract_colors = true;

    // The number of threads to use during reconstruction.
    int num_threads = -1;

    // Thresholds for filtering images with degenerate intrinsics.
    double min_focal_length_ratio = 0.1;
    double max_focal_length_ratio = 10.0;
    double max_extra_param = 1.0;

    // Which intrinsic parameters to optimize during the reconstruction.
    bool ba_refine_focal_length = true;
    bool ba_refine_principal_point = false;
    bool ba_refine_extra_params = true;

    // The minimum number of residuals per bundle adjustment problem to
    // enable multi-threading solving of the problems.
    int ba_min_num_residuals_for_multi_threading = 50000;

    // The number of images to optimize in local bundle adjustment.
    int ba_local_num_images = 6;

    // Ceres solver function tolerance for local bundle adjustment
    double ba_local_function_tolerance = 0.0;

    // The maximum number of local bundle adjustment iterations.
    int ba_local_max_num_iterations = 25;

    // The growth rates after which to perform global bundle adjustment.
    double ba_global_images_ratio = 1.1;
    double ba_global_points_ratio = 1.1;
    int ba_global_images_freq = 500;
    int ba_global_points_freq = 250000;

    // Ceres solver function tolerance for global bundle adjustment
    double ba_global_function_tolerance = 0.0;

    // The maximum number of global bundle adjustment iterations.
    int ba_global_max_num_iterations = 30;

    // The thresholds for iterative bundle adjustment refinements.
    int ba_local_max_refinements = 2;
    double ba_local_max_refinement_change = 0.001;
    int ba_global_max_refinements = 2;
    double ba_global_max_refinement_change = 0.0005;

    // Path to a folder with reconstruction snapshots during incremental
    // reconstruction. Snapshots will be saved according to the specified
    // frequency of registered images.
    std::string snapshot_path = "";
    int snapshot_images_freq = 0;

    // Which images to reconstruct. If no images are specified, all images will
    // be reconstructed by default.
    std::unordered_set<std::string> image_names;

    // If reconstruction is provided as input, fix the existing image poses.
    bool fix_existing_images = false;

    CMapper::Options Mapper();
    CTriangulator::Options Triangulation();
    CBundleAdjustmentOptions LocalBundleAdjustment();
    CBundleAdjustmentOptions GlobalBundleAdjustment();
    bool Check();
};


