#pragma once
#include "Base.h"
#include "Model.h"
#include "Mapper.h"

class CReconstructor :public QObject, public colmap::Thread
{
	Q_OBJECT
public:
	enum
	{
		INITIAL_IMAGE_PAIR_REG_CALLBACK,
		NEXT_IMAGE_REG_CALLBACK,
		LAST_IMAGE_REG_CALLBACK,
	}; //重建过程的三个状态, 会触发相应的回调函数

	static size_t LastReconstructTimeConsuming; //单位: 秒
	size_t CurrentModelID;

	CReconstructor(colmap::OptionManager* options, CModelManager* ModelManager);
	~CReconstructor();
	void Reconstruct(std::string ImagePath);

	void UpdateRegImgIDs(); //更新所有模型已经注册的影像(一般是"导入模型"后会调用)
	void TryMergeModels(); //尝试合并模型
	bool MergeModel(size_t Model1ID, size_t Model2ID);
signals:
	void ChangeImageColor_SIGNAL(int, int);  //提醒主界面更改"当前尝试注册的影像"的颜色
private:
	colmap::OptionManager* options;
	CModelManager* ModelManager;
	std::unordered_set<colmap::image_t> RegImages; //ModelManager中所有模型的"成功注册的影像"的并集
	
	bool IsContinue;

	void Run();
	void Reconstruct(std::string ImagePath, QSqlDatabase& db); //重建
	std::vector<size_t> ChooseModel(std::string ImagePath, QSqlDatabase& db);
	bool IsImageBelongsModel(std::string ImagePath, CModel& Model, QSqlDatabase& db);
	bool RegisterInitialPair(CMapperOptions* MapperOptions, CMapper& mapper, size_t ModelID, size_t& InitImageID1, size_t& InitImageID2, QSqlDatabase& db);

	void GlobalBundleAdjust(CMapperOptions& options, CMapper* mapper);
	size_t ImageTriangulate(CMapperOptions& options, size_t ImageID, CMapper* mapper);
	void IterativeLocalRefinement(CMapperOptions& options, colmap::image_t ImageID, CMapper* mapper);
	void IterativeGlobalRefinement(CMapperOptions& options, CMapper* mapper);
	void ExtractColors(colmap::image_t ImageID, CModel* Model);
	size_t FilterPoints(CMapperOptions& options, CMapper* mapper);
	size_t FilterImages(CMapperOptions& options, CMapper* mapper);
	size_t CompleteAndMergeTracks(CMapperOptions& options, CMapper* mapper);
};