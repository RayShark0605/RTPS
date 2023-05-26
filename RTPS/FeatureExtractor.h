#pragma once
#include "Base.h"
#include "Database.h"


class CFeatureExtractor
{
public:
	CFeatureExtractor(colmap::OptionManager* options);
	bool Extract(std::string ImagePath);

private:
	int ReadImage(std::string ImagePath, colmap::Camera& camera, colmap::Image& image, colmap::Bitmap& bitmap, QSqlDatabase& db);
	colmap::OptionManager* options;
	void ScaleKeypoints(colmap::Bitmap& bitmap, colmap::Camera& camera, colmap::FeatureKeypoints& keypoints);

};
