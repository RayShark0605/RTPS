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
	}; //�ؽ����̵�����״̬, �ᴥ����Ӧ�Ļص�����
	CReconstructor(colmap::OptionManager* options, CModelManager* ModelManager);
	~CReconstructor();
	void AddImage(std::string ImagePath);
	void StopReconstruct();
	void UpdateRegImgIDs(); //��������ģ���Ѿ�ע���Ӱ��(һ����"����ģ��"������)
	void TryMergeModels(); //���Ժϲ�ģ��
	bool MergeModel(size_t Model1ID, size_t Model2ID);
signals:
	void ChangeImageColor_SIGNAL(int);  //�������������"��ǰ����ע���Ӱ��"����ɫ
private:
	colmap::OptionManager* options;
	CModelManager* ModelManager;
	std::unordered_set<colmap::image_t> RegImages; //ModelManager������ģ�͵�"�ɹ�ע���Ӱ��"�Ĳ���
	
	std::thread ReconstructThread;
	std::queue<std::string> queue;
	std::mutex queue_mutex;
	size_t LastReconstructTimeConsuming; //��λ: ��
	bool IsContinue;
	void Run();

	void Reconstruct(std::string ImagePath, QSqlDatabase& db); //�ؽ�
	int Reconstruct(std::string ImagePath, CMapperOptions* MapperOptions, colmap::DatabaseCache& DbCache, QSqlDatabase& db);
	size_t ChooseModel(std::string ImagePath, QSqlDatabase& db);
	bool IsImageBelongsModel(std::string ImagePath, CModel& Model, QSqlDatabase& db);
	bool RegisterInitialPair(CMapperOptions* MapperOptions, CMapper& mapper, size_t ModelID, size_t& InitImageID1, size_t& InitImageID2, QSqlDatabase& db);

	void GlobalBundleAdjust(CMapperOptions& options, CMapper* mapper);
	size_t ImageTriangulate(CMapperOptions& options, colmap::Image& image, CMapper* mapper);
	void IterativeLocalRefinement(CMapperOptions& options, colmap::image_t ImageID, CMapper* mapper);
	void IterativeGlobalRefinement(CMapperOptions& options, CMapper* mapper);
	void ExtractColors(colmap::image_t ImageID, CModel* Model);
	size_t FilterPoints(CMapperOptions& options, CMapper* mapper);
	size_t FilterImages(CMapperOptions& options, CMapper* mapper);
	size_t CompleteAndMergeTracks(CMapperOptions& options, CMapper* mapper);
};