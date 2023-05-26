#include "Reconstructor.h"


using namespace std;
using namespace colmap;


CReconstructor::CReconstructor(OptionManager* options, CModelManager* ModelManager)
{
	CHECK(options->mapper.get()->Check());
	this->options = options;
	this->ModelManager = ModelManager;
	this->queue = std::queue<string>();
	IsContinue = true;
	LastReconstructTimeConsuming = 0;

	RegisterCallback(INITIAL_IMAGE_PAIR_REG_CALLBACK);
	RegisterCallback(NEXT_IMAGE_REG_CALLBACK);
	RegisterCallback(LAST_IMAGE_REG_CALLBACK);

	ReconstructThread = std::thread(&CReconstructor::Run, this);
	ReconstructThread.detach();
}
CReconstructor::~CReconstructor()
{
	IsContinue = false;
}
void CReconstructor::AddImage(string ImagePath)
{
	queue_mutex.lock();
	queue.push(ImagePath);
	queue_mutex.unlock();
}
void CReconstructor::StopReconstruct()
{
	IsContinue = false;
}
void CReconstructor::UpdateRegImgIDs()
{
	size_t NewRegImagesNum = 0;
	if (ModelManager)
	{
		size_t ModelsNum = ModelManager->Size();
		ModelManager->Lock();
		for (size_t i = 0; i < ModelsNum; i++)
		{
			vector<size_t> RegImagesID = ModelManager->GetRefer(i).GetAllRegImagesIDs();
			for (image_t ImageID : RegImagesID)
			{
				RegImages.insert(ImageID);
				NewRegImagesNum++;
			}
		}
		ModelManager->UnLock();
	}
	cout << StringPrintf("Updated %d images that have been registered!", NewRegImagesNum) << endl;
}
void CReconstructor::TryMergeModels()
{
	if (!ModelManager || ModelManager->Size() < 2)return;
	cout << StringPrintf("[Merge model] Trying to merge models...") << endl;
	bool IsFinished = true;
	do
	{
		IsFinished = true;
		for (size_t i = 0; i < ModelManager->Size(); i++)
		{
			CModel Model = ModelManager->Get(i);
			vector<size_t> BaseRegImages = Model.GetAllRegImagesIDs();
			if (BaseRegImages.size() < 2)continue;
			sort(BaseRegImages.begin(), BaseRegImages.end());
			bool IsMerge = false;
			for (size_t j = 0; j < ModelManager->Size(); j++)
			{
				if (j == i)continue;
				CModel RefModel = ModelManager->Get(j);
				vector<size_t> RefRegImages = RefModel.GetAllRegImagesIDs();
				if (RefRegImages.size() < 2)continue;
				sort(RefRegImages.begin(), RefRegImages.end());
				vector<size_t> intersection(BaseRegImages.size() + RefRegImages.size());
				set_intersection(BaseRegImages.begin(), BaseRegImages.end(), RefRegImages.begin(), RefRegImages.end(), intersection.begin());
				size_t Threshold = min({ (size_t)options->mapper->max_model_overlap ,BaseRegImages.size() ,RefRegImages.size() });
				if (intersection.size() < Threshold)continue;
				if (MergeModel(i, j))
				{
					IsMerge = true;
					break;
				}
			}
			if (IsMerge)
			{
				IsFinished = false;
				break;
			}
		}
	} while (!IsFinished);
	Callback(LAST_IMAGE_REG_CALLBACK);
	cout << StringPrintf("[Merge model] Models merging completed!") << endl;
}
bool CReconstructor::MergeModel(size_t Model1ID, size_t Model2ID)
{
	if (!ModelManager)return false;
	ModelManager->Lock();
	CModel& Model1 = ModelManager->GetRefer(Model1ID);
	CModel& Model2 = ModelManager->GetRefer(Model2ID);
	size_t Point1Num = Model1.GetModelPoints3DNum();
	size_t Point2Num = Model2.GetModelPoints3DNum();
	double MaxReprojectError = 100;

	CModel& Base = (Point1Num > Point2Num ? Model1 : Model2);
	CModel& Ref = (Point1Num > Point2Num ? Model2 : Model1);
	size_t BaseID = (Point1Num > Point2Num ? Model1ID : Model2ID);
	size_t RefID = (Point1Num > Point2Num ? Model2ID : Model1ID);
	if (Base.Merge(Ref, MaxReprojectError)) //���ȳ��԰�Ref����Base
	{
		ModelManager->UnLock();
		cout << StringPrintf("[Merge model] Successfully merged model %d into model %d and will delete model %d!", RefID + 1, BaseID + 1, RefID + 1) << endl;
		ModelManager->Delete(RefID);
		return true;
	}
	//���ʧ��, �ٳ��԰�Base����Ref
	// cout << StringPrintf("[Merge model] Attempt to merge model %d into model %d failed!", RefID + 1, BaseID + 1) << endl;
	// cout << StringPrintf("[Merge model] Automatically try to swap merging sequence...") << endl;
	if (Ref.Merge(Base, MaxReprojectError))
	{
		ModelManager->UnLock();
		cout << StringPrintf("[Merge model] Successfully merged model %d into model %d and will delete model %d!", BaseID + 1, RefID + 1, BaseID + 1) << endl;
		ModelManager->Delete(BaseID);
		return true;
	}
	ModelManager->UnLock();
	return false;
}

void CReconstructor::Run()
{
	DebugTimer timer(__FUNCTION__);
	ThreadStart();
	while (!ExistsFile(CDatabase::DatabasePath))
	{
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	while (IsContinue)
	{
		this_thread::sleep_for(chrono::milliseconds(1000));
		queue_mutex.lock();
		size_t WaitNum = queue.size();

		if (WaitNum == 0 || (WaitNum < 5 && LastReconstructTimeConsuming >= 10) || (ModelManager->Size() == 0 && WaitNum < 5))
		{
			queue_mutex.unlock();
			continue;
		}
		vector<string> WaitImages(WaitNum);
		for (size_t i = 0; i < WaitNum; i++)
		{
			WaitImages[i] = queue.front();
			queue.pop();
		}
		queue_mutex.unlock();
		for (string ImagePath : WaitImages)
		{
			string ImageName = GetFileName(ImagePath);
			if (!CDatabase::IsExistImage(ImageName, db))
			{
				cout << StringPrintf("[Reconstructor] Warning! %s does not exists in database!", ImageName) << endl;
				continue;
			}
			size_t ImageID = CDatabase::GetImageID(ImageName, db);
			if (RegImages.count(ImageID))continue;
			cout << StringPrintf("=====> Reconstruction of image %s is being done... <=====", ImageName) << endl;
			Base::Reconstruction_Mutex.lock();
			Reconstruct(ImagePath,db);
			Callback(LAST_IMAGE_REG_CALLBACK);
			Base::Reconstruction_Mutex.unlock();
		}
	}
	ReleaseDatabaseConnect(db);
	ThreadEnd();
}
void CReconstructor::Reconstruct(string ImagePath, QSqlDatabase& db)
{
	DebugTimer timer(__FUNCTION__);
	cout << "Loading database cache..." << endl;
	DatabaseCache DbCache;
	CDatabase::ExportDatabaseCache(options, DbCache, db);
	if (DbCache.NumImages() == 0)
	{
		cout << "No images with matches found in the database!" << endl;
		return;
	}
	for (size_t ImageID : RegImages)
	{
		if (DbCache.ExistsImage(ImageID))
		{
			DbCache.Image(ImageID).SetRegistered(true);
		}
	}
	cout << StringPrintf("Successfully loaded %d images, %d cameras, %d image pairs in database!", DbCache.NumImages(), DbCache.NumCameras(), DbCache.CorrespondenceGraph().NumImagePairs()) << endl;
	CMapperOptions MapperOptions(*options->mapper);
	int ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
	if (ReconstructResult == -1)
	{
		cout << "Trying to relax the initialization constraints..." << endl;
		size_t kNumInitRelaxations = 2; //��������
		for (size_t i = 0; i < kNumInitRelaxations; i++)
		{
			MapperOptions.mapper.init_min_num_inliers = MapperOptions.mapper.init_min_num_inliers / 2;
			ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
			if (ReconstructResult == -1)
			{
				cout << "Trying to relax the initialization constraints again..." << endl;
				MapperOptions.mapper.init_min_tri_angle = MapperOptions.mapper.init_min_tri_angle / 2;
				ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
				if (ReconstructResult != -1)
				{
					cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
					TryMergeModels();
					return;
				}
			}
			else
			{
				cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
				TryMergeModels();
				return;
			}
		}
		cout << StringPrintf("[%d s] All attempts to reconstruct have ended in failure!", LastReconstructTimeConsuming) << endl;
	}
	else
	{
		cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
		TryMergeModels();
	}
}
int CReconstructor::Reconstruct(string ImagePath, CMapperOptions* MapperOptions, DatabaseCache& DbCache, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	QElapsedTimer timer;
	timer.start();
	CMapper mapper(&DbCache);
	db = CreateDatabaseConnect(CDatabase::DatabasePath);
	size_t ModelID = ChooseModel(ImagePath,db); //��ǰӰ��Ӧ�������ĸ�ģ��
	cout << StringPrintf("=====================> Current model: %d <=====================", ModelID + 1) << endl;
	ModelManager->Lock();
	CModel& Model = ModelManager->GetRefer(ModelID);
	mapper.BeginReconstruction(&Model);

	size_t InitImageID1 = kInvalidImageId, InitImageID2 = kInvalidImageId, NextTryImageID;
	vector<image_t> ThisRegImgIDs; //�����ؽ�������, ��ע���Ӱ������ؽ��ɹ�, ��ô��������ֵ��ȫ����ӵ�RegImgIDs�С�����ؽ�ʧ��, ���������
	if (Model.GetModelRegImagesNum() == 0)
	{
		cout << "This is a new model! Trying to find initial pair..." << endl;
		if (!RegisterInitialPair(MapperOptions, mapper, ModelID, InitImageID1, InitImageID2, db))
		{
			LastReconstructTimeConsuming = timer.elapsed() / 1000;
			ModelManager->UnLock();
			return -1;
		}
		ThisRegImgIDs.push_back(InitImageID1);
		ThisRegImgIDs.push_back(InitImageID2);
		cout << StringPrintf("Successfully set and registered images with [%s, id=%d] and [%s, id=%d] as initial image pair", CDatabase::GetImageName(InitImageID1, db), InitImageID1, CDatabase::GetImageName(InitImageID2, db), InitImageID2) << endl;
	}
	Callback(INITIAL_IMAGE_PAIR_REG_CALLBACK);

	size_t OriginRegImgNum = Model.GetModelRegImagesNum(); //ע����Ӱ��֮ǰ, ģ��ԭ�е�ע��Ӱ�����͵���
	size_t OriginPointNum = Model.GetModelPoints3DNum();
	bool IsThisRegSuccess = true, IsPreRegSuccess = true;
	size_t UnGlobalBAImagesNum = 0;
	while (IsThisRegSuccess) //�˳�����: ��һ��ע��(IsPreRegSuccess)ʧ����, ��һ��ע��(IsThisRegSuccess)Ҳʧ����
	{
		if (!IsContinue)
		{
			ModelManager->UnLock();
			return 1;
		}
		IsThisRegSuccess = false;
		cout << StringPrintf("[Model %d]: Trying to register other images...", ModelID + 1) << endl;
		vector<size_t> NextImages = mapper.FindNextImages(MapperOptions->Mapper()); //Ѱ����һ��Ҫע���Ӱ��
		if (NextImages.empty()) //����Ҳ�����һ��Ҫע���Ӱ��
		{
			cout << StringPrintf("[Model %d]: No more other images!", ModelID + 1) << endl;
			break;
		}
		size_t TryNextCount = 0;
		for (image_t NextImageID : NextImages) //������ע���Ӱ��, ֱ���ҵ�һ��ע��ɹ���Ӱ��
		{
			if (!IsContinue)
			{
				ModelManager->UnLock();
				return 1;
			}
			cout << StringPrintf("[Model %d]: Trying to register next image [%s, id=%d]...", ModelID + 1, CDatabase::GetImageName(NextImageID, db), NextImageID) << endl;
			bool IsThisRegSuccess = mapper.RegisterNextImage(MapperOptions->Mapper(), NextImageID); //����ע���Ӱ��
			TryNextCount++;
			if (IsThisRegSuccess) //���ע��ɹ�
			{
				cout << "Success!" << endl;
				Callback(NEXT_IMAGE_REG_CALLBACK);
				Image image = Model.GetModelImage(NextImageID);
				ImageTriangulate(*MapperOptions, image, &mapper); //Ӱ�����ǻ�
				emit ChangeImageColor_SIGNAL(NextImageID); //���ĵ�ǰע��ɹ���Ӱ�����ɫ
				cout << StringPrintf("[Model %d]: Performing iterative local refinement...", ModelID + 1) << endl;
				IterativeLocalRefinement(*MapperOptions, NextImageID, &mapper);

				UnGlobalBAImagesNum++;
				if (UnGlobalBAImagesNum >= 3)
				{
					cout << StringPrintf("[Model %d]: Global adjustment will be performed....", ModelID + 1) << endl;
					IterativeGlobalRefinement(*MapperOptions, &mapper); //���е���ʽȫ�־���
					UnGlobalBAImagesNum = 0;
				}

				//�����ǰ�����˹����ע��Ӱ����߹���ĵ�
				//if (Model.NumRegImages() >= MapperOptions->ba_global_images_ratio * OriginRegImgNum || Model.NumRegImages() >= MapperOptions->ba_global_images_freq + OriginRegImgNum || Model.NumPoints3D() >= MapperOptions->ba_global_points_ratio * OriginPointNum || Model.NumPoints3D() >= MapperOptions->ba_global_points_freq + OriginPointNum)
				//{
				//	cout << StringPrintf("[Model %d]: Added too many new images and points, performing iterative global refinement...", ModelID + 1) << endl;
				//	IterativeGlobalRefinement(*MapperOptions, &mapper); //���е���ʽȫ�־���
				//	OriginRegImgNum = Model.NumRegImages();
				//	OriginPointNum = Model.NumPoints3D();
				//}
				ExtractColors(NextImageID, &Model);
				ThisRegImgIDs.emplace_back(NextImageID);
				break; //��ǰ�Ѿ��ɹ�ע����һ��Ӱ��, ����ע������Ӱ��
			}
			else
			{
				cout << "Fail!" << endl;
				// If initial pair fails to continue for some time, abort and try different initial pair.
				// ���������50��Ӱ��ûע��ɹ�, ���ҵ�ǰģ�͵�ע��Ӱ������̫��, ��ô������������
				if (TryNextCount >= 50 && Model.GetModelRegImagesNum() < MapperOptions->min_model_size)
				{
					break;
				}
			}
		}
		if (mapper.NumSharedRegImages() >= MapperOptions->max_model_overlap)
		{
			cout << StringPrintf("[Model %d]: There are too many shared registered images, the reconstruction will stop!", ModelID + 1) << endl;
			break;
		}
		//������, ���IsPreRegSuccess��IsThisRegSuccess��Ϊfalse, ��ô֮����˳�whileѭ��
		if (!IsThisRegSuccess && IsPreRegSuccess) //�����һ�γ���ʧ����, ������һ���ǳɹ���, ��ô��һ��ȫ�־���֮��, �ٸ�һ�λ���
		{
			IsThisRegSuccess = true;
			//IterativeGlobalRefinement(*MapperOptions, &mapper); //���е���ʽȫ�־���
		}
		IsPreRegSuccess = IsThisRegSuccess;
	} //whileѭ��

	if (Model.GetModelRegImagesNum() >= 2 && Model.GetModelRegImagesNum() != OriginRegImgNum && Model.GetModelPoints3DNum() != OriginPointNum)
	{
		//IterativeGlobalRefinement(*MapperOptions, &mapper);
	}
	size_t MinModelSize = min(DbCache.NumImages(), size_t(MapperOptions->min_model_size));
	ModelManager->UnLock();
	if (Model.GetModelRegImagesNum() < MinModelSize)
	{
		cout << StringPrintf("[Model %d]: Too few registered images! the model will be deleted!", ModelID + 1) << endl;
		mapper.EndReconstruction(true);
		ThisRegImgIDs.clear(); //��ո��б�, ����RegImgIDs��д��
		ModelManager->Delete(ModelID); //ɾ����ģ��
	}
	else
	{
		cout << StringPrintf("=====================> [Model %d]: This reconstruction is complete <=====================", ModelID + 1) << endl;
		cout << StringPrintf("[Model %d]: %d images were registered for this reconstruction: ", ModelID + 1, ThisRegImgIDs.size());
		for (size_t index = 0; index < ThisRegImgIDs.size(); index++) //��ThisRegImgIDs�����ֵȫ����ӵ�RegImgIDs��
		{
			size_t ID = ThisRegImgIDs[index];
			RegImages.insert(ID);
			//SetImageReg(ID, true);
			cout << StringPrintf("%s[%d]", CDatabase::GetImageName(ID, db), ID);
			if (index != ThisRegImgIDs.size() - 1)
			{
				cout << ", ";
			}
		}
		cout << endl;
		mapper.EndReconstruction(false);
	}
	LastReconstructTimeConsuming = timer.elapsed() / 1000;
	return 1;
}
size_t CReconstructor::ChooseModel(string ImagePath, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	//���ص�һ������������ģ��
	for (size_t i = 0; i < ModelManager->Size(); i++)
	{
		CModel Model = ModelManager->Get(i);
		if (IsImageBelongsModel(ImagePath, Model, db))
		{
			return i;
		}
	}
	cout << "Add a new model!" << endl;
	return ModelManager->Add();
}
bool CReconstructor::IsImageBelongsModel(string ImagePath, CModel& Model, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	//��ImagePath��Ӧ��Ӱ��Ϊa, ģ����ע�������һ��Ӱ��Ϊb, ���a��b֮���˫�Ӽ���ƥ��������MinMatchedPointsNum, ��ô����Ϊa��b��"��Ч��Ӱ���"
	//����ڵ�ǰģ�͵�����ע��Ӱ����, ��a����"��Ч��Ӱ���"����������MinMatchedImagesNum, ��ô����ΪaӰ�����ڸ�ģ��
	size_t MinMatchedPointsNum = 10, MinMatchedImagesNum = 3;
	vector<size_t> ModelRegImages = Model.GetAllRegImagesIDs(); //��ȡģ�͵�ȫ��ע��Ӱ��
	size_t ImageID = CDatabase::GetImageID(GetFileName(ImagePath),db);
	size_t ValidMatchedImageNum = 0;
	for (size_t ModelImageID : ModelRegImages)
	{
		if (CDatabase::GetTwoViewGeometriesNum(ImageID, ModelImageID, db) >= MinMatchedPointsNum)
		{
			ValidMatchedImageNum++;
		}
	}
	if (ModelRegImages.size() <= MinMatchedImagesNum)
	{
		return ValidMatchedImageNum >= 1;
	}
	return ValidMatchedImageNum >= MinMatchedImagesNum;
}
bool CReconstructor::RegisterInitialPair(CMapperOptions* MapperOptions, CMapper& mapper, size_t ModelID, size_t& InitImageID1, size_t& InitImageID2, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	CModel& Model = ModelManager->GetRefer(ModelID);
	db = CreateDatabaseConnect(CDatabase::DatabasePath);
	// Try to find good initial pair.
	bool IsFindInitSuccess = mapper.FindInitialImagePair(MapperOptions->Mapper(), &InitImageID1, &InitImageID2);
	if (!IsFindInitSuccess || !Model.IsModelExistImage(InitImageID1) || !Model.IsModelExistImage(InitImageID2) || RegImages.count(InitImageID1) || RegImages.count(InitImageID2))
	{
		if (!IsFindInitSuccess)
		{
			cout << "[WARNING] Could not find good initial image pair!" << endl;
		}
		else if (!Model.IsModelExistImage(InitImageID1) || !Model.IsModelExistImage(InitImageID2))
		{
			cout << StringPrintf("[WARNING] The initial image pair found [%d and %d] does not exist!", InitImageID1, InitImageID2) << endl;
		}
		else
		{
			cout << StringPrintf("[WARNING] The initial image pair found [%d and %d] is registered images!", InitImageID1, InitImageID2) << endl;
		}
		ModelManager->UnLock();
		mapper.EndReconstruction(true);
		ModelManager->Delete(ModelID);
		ModelManager->Lock();
		return false;
	}
	cout << StringPrintf("[Model %d]: Initializing with image pair: [%s, ID=%d] and [%s, ID=%d]!", ModelID + 1, CDatabase::GetImageName(InitImageID1, db), InitImageID1, CDatabase::GetImageName(InitImageID2, db), InitImageID2) << endl;
	
	bool IsRegInitSuccess = mapper.RegisterInitialImagePair(MapperOptions->Mapper(), InitImageID1, InitImageID2);
	if (!IsRegInitSuccess)
	{
		cout << "[WARNING] Initialization failed: failed to register initial image pair!" << endl;
		ModelManager->UnLock();
		mapper.EndReconstruction(true);
		ModelManager->Delete(ModelID);
		ModelManager->Lock();
		return false;
	}
	GlobalBundleAdjust(*MapperOptions, &mapper);
	FilterPoints(*MapperOptions, &mapper);
	FilterImages(*MapperOptions, &mapper);
	if (Model.GetModelRegImagesNum() == 0 || Model.GetModelPoints3DNum() == 0)
	{
		cout << "[WARNING] After global adjustment, the number of registered images or 3D points is 0!" << endl;
		ModelManager->UnLock();
		mapper.EndReconstruction(true);
		ModelManager->Delete(ModelID);
		ModelManager->Lock();
		return false;
	}
	ExtractColors(InitImageID1, &Model);
	return true;
}


void CReconstructor::GlobalBundleAdjust(CMapperOptions& options, CMapper* mapper)
{
	cout << "Performing global bundle adjustment..." << endl;
	CBundleAdjustmentOptions BA_Options = options.GlobalBundleAdjustment();
	size_t RegImagesNum = mapper->GetModel()->GetModelRegImagesNum();
	// Use stricter convergence criteria for first registered images.
	size_t kMinNumRegImagesForFastBA = 10;
	if (RegImagesNum < kMinNumRegImagesForFastBA)
	{
		BA_Options.solver_options.function_tolerance /= 10;
		BA_Options.solver_options.gradient_tolerance /= 10;
		BA_Options.solver_options.parameter_tolerance /= 10;
		BA_Options.solver_options.max_num_iterations *= 2;
		BA_Options.solver_options.max_linear_solver_iterations = 200;
	}
	BA_Options.solver_options.logging_type = ceres::LoggingType::SILENT;
	BA_Options.print_summary = false;
	CMapper::Options CMapperOptions = options.Mapper();
	mapper->AdjustGlobalBundle(CMapperOptions, BA_Options);
	cout << "Global bundle adjustment completed!" << endl;
}
size_t CReconstructor::ImageTriangulate(CMapperOptions& options, Image& image, CMapper* mapper)
{
	cout << StringPrintf("[Image triangulate] Continued observations: %d", image.NumPoints3D()) << endl;
	size_t num_tris = mapper->TriangulateImage(options.Triangulation(), image.ImageId());
	cout << StringPrintf("[Image triangulate] Added observations: %d", num_tris) << endl;
	return num_tris;
}
void CReconstructor::IterativeLocalRefinement(CMapperOptions& options, image_t ImageID, CMapper* mapper)
{
	CBundleAdjustmentOptions BA_Options = options.LocalBundleAdjustment();
	BA_Options.solver_options.logging_type = ceres::LoggingType::SILENT;
	BA_Options.print_summary = false;
	for (int i = 0; i < options.ba_local_max_refinements; i++)
	{
		CMapper::Options CMapperOptions = options.Mapper();
		CTriangulator::Options CTriangulatorOptions = options.Triangulation();

		auto Report = mapper->AdjustLocalBundle(CMapperOptions, BA_Options, CTriangulatorOptions, ImageID, mapper->GetModifiedPoints3D());
		//cout << StringPrintf("[Iterative local refinement] Merged observations: %d", Report.num_merged_observations) << endl;
		//cout << StringPrintf("[Iterative local refinement] Completed observations: %d", Report.num_completed_observations) << endl;
		//cout << StringPrintf("[Iterative local refinement] Filtered observations: %d", Report.num_filtered_observations) << endl;
		double Changed = 0;
		if (Report.num_adjusted_observations != 0)
		{
			Changed = (Report.num_merged_observations + Report.num_completed_observations + Report.num_filtered_observations) * 1.0 / Report.num_adjusted_observations;
		}
		//cout << StringPrintf("[Iterative local refinement] Changed observations: %.6f", Changed) << endl;
		if (Changed < options.ba_local_max_refinement_change)
		{
			break;
		}
		// Only use robust cost function for first iteration.
		BA_Options.loss_function_type = CBundleAdjustmentOptions::LossFunctionType::TRIVIAL;
	}
	//cout << StringPrintf("[Iterative local refinement] Clear modified points3D...") << endl;
	mapper->ClearModifiedPoints3D();
	cout << StringPrintf("[Iterative local refinement] All steps completed!") << endl;
}
void CReconstructor::IterativeGlobalRefinement(CMapperOptions& options, CMapper* mapper)
{
	cout << StringPrintf("[Iterative global refinement] Complete and merge tracks...") << endl;
	CompleteAndMergeTracks(options, mapper);
	cout << StringPrintf("[Iterative global refinement] Retriangulated observations: %d", mapper->Retriangulate(options.Triangulation())) << endl;
	for (int i = 0; i < options.ba_global_max_refinements; i++)
	{
		size_t num_observations = mapper->GetModel()->GetObservationsNum();
		size_t num_changed_observations = 0;
		GlobalBundleAdjust(options, mapper);
		num_changed_observations += CompleteAndMergeTracks(options, mapper);
		num_changed_observations += FilterPoints(options, mapper);
		double changed = (num_observations == 0 ? 0 : static_cast<double>(num_changed_observations) / num_observations);
		cout << StringPrintf("[Iterative global refinement] Changed observations: %.6f", changed) << endl;
		if (changed < options.ba_global_max_refinement_change)
		{
			break;
		}
	}
	cout << StringPrintf("[Iterative global refinement] Filter images...") << endl;
	FilterImages(options, mapper);
	cout << StringPrintf("[Iterative global refinement] All steps completed!") << endl;
}
void CReconstructor::ExtractColors(image_t ImageID, CModel* Model)
{
	if (!Model->ExtractColors_SingleImage(ImageID, *options->image_path))
	{
		cout << StringPrintf("[WARNING] Could not read image %d at path %s!", ImageID, *options->image_path) << endl;
	}
}
size_t CReconstructor::FilterPoints(CMapperOptions& options, CMapper* mapper)
{
	size_t num_filtered_observations = mapper->FilterPoints(options.Mapper());
	std::cout << "  => Filtered observations: " << num_filtered_observations << std::endl;
	return num_filtered_observations;
}
size_t CReconstructor::FilterImages(CMapperOptions& options, CMapper* mapper)
{
	const size_t num_filtered_images = mapper->FilterImages(options.Mapper());
	std::cout << "  => Filtered images: " << num_filtered_images << std::endl;
	return num_filtered_images;
}
size_t CReconstructor::CompleteAndMergeTracks(CMapperOptions& options, CMapper* mapper)
{
	const size_t num_completed_observations = mapper->CompleteTracks(options.Triangulation());
	std::cout << "  => Completed observations: " << num_completed_observations << std::endl;
	const size_t num_merged_observations = mapper->MergeTracks(options.Triangulation());
	std::cout << "  => Merged observations: " << num_merged_observations << std::endl;
	return num_completed_observations + num_merged_observations;
}