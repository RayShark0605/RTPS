#pragma once
#include <estimators/triangulation.h>
#include "Base.h"
#include "Model.h"

class CTriangulator
{
public:
    struct Options 
    {
        // Maximum transitivity to search for correspondences.
        int max_transitivity = 1;

        // Maximum angular error to create new triangulations.
        double create_max_angle_error = 2.0;

        // Maximum angular error to continue existing triangulations.
        double continue_max_angle_error = 2.0;

        // Maximum reprojection error in pixels to merge triangulations.
        double merge_max_reproj_error = 4.0;

        // Maximum reprojection error to complete an existing triangulation.
        double complete_max_reproj_error = 4.0;

        // Maximum transitivity for track completion.
        int complete_max_transitivity = 5;

        // Maximum angular error to re-triangulate under-reconstructed image pairs.
        double re_max_angle_error = 5.0;

        // Minimum ratio of common triangulations between an image pair over the
        // number of correspondences between that image pair to be considered
        // as under-reconstructed.
        double re_min_ratio = 0.2;

        // Maximum number of trials to re-triangulate an image pair.
        int re_max_trials = 1;

        // Minimum pairwise triangulation angle for a stable triangulation.
        double min_angle = 1.5;

        // Whether to ignore two-view tracks.
        bool ignore_two_view_tracks = true;

        // Thresholds for bogus camera parameters. Images with bogus camera
        // parameters are ignored in triangulation.
        double min_focal_length_ratio = 0.1;
        double max_focal_length_ratio = 10.0;
        double max_extra_param = 1.0;

        bool Check() const;
    };
    CTriangulator(const colmap::CorrespondenceGraph* correspondence_graph, CModel* reconstruction);
    size_t TriangulateImage(const Options& options, const colmap::image_t image_id);
    size_t CompleteImage(const Options& options, const colmap::image_t image_id);
    size_t CompleteTracks(const Options& options, const std::unordered_set<colmap::point3D_t>& point3D_ids);
    size_t CompleteAllTracks(const Options& options);
    size_t MergeTracks(const Options& options, const std::unordered_set<colmap::point3D_t>& point3D_ids);
    size_t MergeAllTracks(const Options& options);
    size_t Retriangulate(const Options& options);
    void AddModifiedPoint3D(const colmap::point3D_t point3D_id);
    const std::unordered_set<colmap::point3D_t>& GetModifiedPoints3D();
    void ClearModifiedPoints3D();
    struct CorrData 
    {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        colmap::image_t image_id;
        colmap::point2D_t point2D_idx;
        const colmap::Image* image;
        const colmap::Camera* camera;
        const colmap::Point2D* point2D;
    };
private:
    const colmap::CorrespondenceGraph* correspondence_graph_;
    CModel* reconstruction_;
    std::unordered_map<colmap::camera_t, bool> camera_has_bogus_params_;
    std::unordered_map<colmap::point3D_t, std::unordered_set<colmap::point3D_t>> merge_trials_;
    std::vector<colmap::CorrespondenceGraph::Correspondence> found_corrs_;
    std::unordered_map<colmap::image_pair_t, int> re_num_trials_;
    std::unordered_set<colmap::point3D_t> modified_point3D_ids_;


    void ClearCaches();
    size_t Find(const Options& options, const colmap::image_t image_id, const colmap::point2D_t point2D_idx, const size_t transitivity, std::vector<CorrData>* corrs_data);
    size_t Create(const Options& options, const std::vector<CorrData>& corrs_data);
    size_t Continue(const Options& options, const CorrData& ref_corr_data, const std::vector<CorrData>& corrs_data);
    size_t Merge(const Options& options, const colmap::point3D_t point3D_id);
    size_t Complete(const Options& options, const colmap::point3D_t point3D_id);
    bool HasCameraBogusParams(const Options& options, const colmap::Camera& camera);
};

EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION_CUSTOM(CTriangulator::CorrData)