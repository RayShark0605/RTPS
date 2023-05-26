#pragma once
#include <memory>
#include <unordered_set>
#include <Eigen/Core>
#include <ceres/ceres.h>
#include "Model.h"

class CModel;
struct CBundleAdjustmentOptions
{
	enum class LossFunctionType { TRIVIAL, SOFT_L1, CAUCHY };
	LossFunctionType loss_function_type = LossFunctionType::TRIVIAL;

	double loss_function_scale = 1.0;
	bool refine_focal_length = true;
	bool refine_principal_point = false;
	bool refine_extra_params = true;
	bool refine_extrinsics = true;
	bool print_summary = true;
	int min_num_residuals_for_multi_threading = 50000;
	ceres::Solver::Options solver_options;
	CBundleAdjustmentOptions()
	{
		solver_options.function_tolerance = 0.0;
		solver_options.gradient_tolerance = 0.0;
		solver_options.parameter_tolerance = 0.0;
		solver_options.minimizer_progress_to_stdout = false;
		solver_options.max_num_iterations = 100;
		solver_options.max_linear_solver_iterations = 200;
		solver_options.max_num_consecutive_invalid_steps = 10;
		solver_options.max_consecutive_nonmonotonic_steps = 10;
		solver_options.num_threads = -1;
#if CERES_VERSION_MAJOR < 2
		solver_options.num_linear_solver_threads = -1;
#endif
	}
	ceres::LossFunction* CreateLossFunction() const;
	bool Check() const;
};
class CBundleAdjustmentConfig
{
public:
	CBundleAdjustmentConfig();

	size_t NumImages() const;
	size_t NumPoints() const;
	size_t NumConstantCameras() const;
	size_t NumConstantPoses() const;
	size_t NumConstantTvecs() const;
	size_t NumVariablePoints() const;
	size_t NumConstantPoints() const;

	size_t NumResiduals(CModel& reconstruction) const;

	// Add / remove images from the configuration.
	void AddImage(const size_t image_id);
	bool HasImage(const size_t image_id) const;
	void RemoveImage(const size_t image_id);

	// Set cameras of added images as constant or variable. By default all
	// cameras of added images are variable. Note that the corresponding images
	// have to be added prior to calling these methods.
	void SetConstantCamera(const size_t camera_id);
	void SetVariableCamera(const size_t camera_id);
	bool IsConstantCamera(const size_t camera_id) const;

	// Set the pose of added images as constant. The pose is defined as the
	// rotational and translational part of the projection matrix.
	void SetConstantPose(const size_t image_id);
	void SetVariablePose(const size_t image_id);
	bool HasConstantPose(const size_t image_id) const;

	// Set the translational part of the pose, hence the constant pose
	// indices may be in [0, 1, 2] and must be unique. Note that the
	// corresponding images have to be added prior to calling these methods.
	void SetConstantTvec(const size_t image_id, const std::vector<int>& idxs);
	void RemoveConstantTvec(const size_t image_id);
	bool HasConstantTvec(const size_t image_id) const;

	// Add / remove points from the configuration. Note that points can either
	// be variable or constant but not both at the same time.
	void AddVariablePoint(const size_t point3D_id);
	void AddConstantPoint(const size_t point3D_id);
	bool HasPoint(const size_t point3D_id) const;
	bool HasVariablePoint(const size_t point3D_id) const;
	bool HasConstantPoint(const size_t point3D_id) const;
	void RemoveVariablePoint(const size_t point3D_id);
	void RemoveConstantPoint(const size_t point3D_id);

	// Access configuration data.
	const std::unordered_set<size_t>& Images() const;
	const std::unordered_set<size_t>& VariablePoints() const;
	const std::unordered_set<size_t>& ConstantPoints() const;
	const std::vector<int>& ConstantTvec(const size_t image_id) const;
private:
	std::unordered_set<size_t> constant_camera_ids_;
	std::unordered_set<size_t> image_ids_;
	std::unordered_set<size_t> variable_point3D_ids_;
	std::unordered_set<size_t> constant_point3D_ids_;
	std::unordered_set<size_t> constant_poses_;
	std::unordered_map<size_t, std::vector<int>> constant_tvecs_;
};
class CBundleAdjuster
{
public:
	CBundleAdjuster(const CBundleAdjustmentOptions& options, const CBundleAdjustmentConfig& config);
	bool Solve(CModel* reconstruction);
	const ceres::Solver::Summary& Summary() const;

private:

	void SetUp(CModel* reconstruction, ceres::LossFunction* loss_function);
	void TearDown(CModel* reconstruction);
	void AddImageToProblem(const size_t image_id, CModel* reconstruction, ceres::LossFunction* loss_function);
	void AddPointToProblem(const size_t point3D_id, CModel* reconstruction, ceres::LossFunction* loss_function);

protected:
	const CBundleAdjustmentOptions options_;
	CBundleAdjustmentConfig config_;
	std::unique_ptr<ceres::Problem> problem_;
	ceres::Solver::Summary summary_;
	std::unordered_set<size_t> camera_ids_;
	std::unordered_map<size_t, size_t> point3D_num_observations_;

	void ParameterizeCameras(CModel* reconstruction);
	void ParameterizePoints(CModel* reconstruction);
};
/*
class CRigBundleAdjuster :public CBundleAdjuster
{
public:
	struct Options
	{
		bool refine_relative_poses = true;
		double max_reproj_error = 1000.0;
	};
	CRigBundleAdjuster(const CBundleAdjustmentOptions& options, const Options& rig_options, const CBundleAdjustmentConfig& config);
	bool Solve(CModel* reconstruction, std::vector<colmap::CameraRig>* camera_rigs);

private:
	const Options rig_options_;
	std::unordered_map<size_t, colmap::CameraRig*> image_id_to_camera_rig_;
	std::unordered_map<size_t, Eigen::Vector4d*> image_id_to_rig_qvec_;
	std::unordered_map<size_t, Eigen::Vector3d*> image_id_to_rig_tvec_;
	std::vector<std::vector<Eigen::Vector4d>> camera_rig_qvecs_;
	std::vector<std::vector<Eigen::Vector3d>> camera_rig_tvecs_;
	std::unordered_set<double*> parameterized_qvec_data_;


	void SetUp(CModel* reconstruction, std::vector<colmap::CameraRig>* camera_rigs, ceres::LossFunction* loss_function);
	void TearDown(CModel* reconstruction, const std::vector<colmap::CameraRig>& camera_rigs);
	void AddImageToProblem(const size_t image_id, CModel* reconstruction, std::vector<colmap::CameraRig>* camera_rigs, ceres::LossFunction* loss_function);
	void AddPointToProblem(const size_t point3D_id, CModel* reconstruction, ceres::LossFunction* loss_function);
	void ComputeCameraRigPoses(const CModel& reconstruction, const std::vector<colmap::CameraRig>& camera_rigs);
	void ParameterizeCameraRigs(CModel* reconstruction);
};
*/
