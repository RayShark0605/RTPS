#include "Reconstructor.h"


using namespace std;
using namespace colmap;

size_t CReconstructor::LastReconstructTimeConsuming = 0;
CReconstructor::CReconstructor(OptionManager* options, CModelManager* ModelManager)
{
	CHECK(options->mapper.get()->Check());
	this->options = options;
	this->ModelManager = ModelManager;
	CurrentModelID = INT_MAX;
	IsContinue = true;

	RegisterCallback(INITIAL_IMAGE_PAIR_REG_CALLBACK);
	RegisterCallback(NEXT_IMAGE_REG_CALLBACK);
	RegisterCallback(LAST_IMAGE_REG_CALLBACK);
}
CReconstructor::~CReconstructor()
{
	IsContinue = false;
}
void CReconstructor::Reconstruct(string ImagePath)
{
	DebugTimer timer(__FUNCTION__);
	while (!ExistsFile(CDatabase::DatabasePath))
	{
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	string ImageName = GetFileName(ImagePath);
	if (!CDatabase::IsExistImage(ImageName, db))
	{
		cout << StringPrintf("[Reconstructor] Warning! %s does not exists in database!", ImageName.c_str()) << endl;
		return;
	}
	if (!CDatabase::ExistTwoViewGeometriesImagesNum(CDatabase::GetImageID(ImageName, db), db))
	{
		cout << StringPrintf("[Reconstructor] Warning! Image %s is not matched to any images!", ImageName.c_str()) << endl;
		return;
	}
	cout << StringPrintf("=====> Reconstruction of image %s is being done... <=====", ImageName.c_str()) << endl;
	Base::Reconstruction_Mutex.lock();
	Reconstruct(ImagePath, db);
	Callback(LAST_IMAGE_REG_CALLBACK);
	Base::Reconstruction_Mutex.unlock();
	ReleaseDatabaseConnect(db);
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
	DebugTimer timer(__FUNCTION__);
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
	DebugTimer timer(__FUNCTION__);
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
	if (Base.Merge(Ref, MaxReprojectError)) //优先尝试把Ref融入Base
	{
		ModelManager->UnLock();
		cout << StringPrintf("[Merge model] Successfully merged model %d into model %d and will delete model %d!", RefID + 1, BaseID + 1, RefID + 1) << endl;
		ModelManager->Delete(RefID);
		Callback(LAST_IMAGE_REG_CALLBACK);
		return true;
	}
	//如果失败, 再尝试把Base融入Ref
	// cout << StringPrintf("[Merge model] Attempt to merge model %d into model %d failed!", RefID + 1, BaseID + 1) << endl;
	// cout << StringPrintf("[Merge model] Automatically try to swap merging sequence...") << endl;
	if (Ref.Merge(Base, MaxReprojectError))
	{
		ModelManager->UnLock();
		cout << StringPrintf("[Merge model] Successfully merged model %d into model %d and will delete model %d!", BaseID + 1, RefID + 1, BaseID + 1) << endl;
		ModelManager->Delete(BaseID);
		Callback(LAST_IMAGE_REG_CALLBACK);
		return true;
	}
	ModelManager->UnLock();
	return false;
}

void CReconstructor::Run() {};

void CReconstructor::Reconstruct(string ImagePath, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);

	QElapsedTimer TotalTimer;
	TotalTimer.start();

	QElapsedTimer timer;
	if (Base::IsQuit)
	{
		return;
	}
	cout << "Loading database cache..." << endl;
	DatabaseCache DbCache;

	timer.start();
	CDatabase::ExportDatabaseCache(options, DbCache, db);
	qDebug() << "ExportDatabaseCache:   " << timer.elapsed();


	if (DbCache.NumImages() < 4)
	{
		cout << "Current images are too few!" << endl;
		return;
	}
	for (size_t ImageID : RegImages)
	{
		if (DbCache.ExistsImage(ImageID))
		{
			DbCache.Image(ImageID).SetRegistered(true);
		}
	}
	if (Base::IsQuit)
	{
		return;
	}
	CMapper mapper(&DbCache);
	CMapperOptions MapperOptions(*options->mapper);

	size_t ThisImageID = CDatabase::GetImageID(GetFileName(ImagePath), db);

	timer.restart();
	vector<size_t> ModelIDs = ChooseModel(ImagePath, db); //当前影像应该属于哪些模型
	qDebug() << "ChooseModel:   " << timer.elapsed();

	size_t OriginRegImagesNum = RegImages.size();
	for (size_t ModelID : ModelIDs)
	{
		cout << StringPrintf("=====================> Current model: %d <=====================", ModelID + 1) << endl;
		emit ChangeImageColor_SIGNAL(ThisImageID, ModelID); //更改当前注册成功的影像的颜色
		CModel& Model = ModelManager->GetRefer(ModelID);
		mapper.BeginReconstruction(&Model);
		if (!Model.IsModelExistImage(GetFileName(ImagePath)))
		{
#ifdef OUTPUTLOG_MODE
			qDebug() << "Model " << ModelID + 1 << " does not exists " << GetFileName(ImagePath);
#endif
			continue;
		}
		unordered_set<size_t> ThisRegImagesID;
		if (Model.GetModelRegImagesNum() == 0) //该模型还是一个空模型
		{
			size_t InitImageID1 = kInvalidImageId, InitImageID2 = kInvalidImageId;
			bool IsInitialSuccess = false;
			for (size_t InitialTrial = 0; InitialTrial < 2; InitialTrial++) //多次尝试构建初始模型
			{
				if (Base::IsQuit)
				{
					return;
				}
				if (RegisterInitialPair(&MapperOptions, mapper, ModelID, InitImageID1, InitImageID2, db))
				{
					IsInitialSuccess = true;
					break;
				}
				MapperOptions.mapper.init_min_num_inliers /= 2;
				if (RegisterInitialPair(&MapperOptions, mapper, ModelID, InitImageID1, InitImageID2, db))
				{
					IsInitialSuccess = true;
					break;
				}
				MapperOptions.mapper.init_min_tri_angle /= 2;
			}
			if (Base::IsQuit)
			{
				return;
			}
			if (!IsInitialSuccess) //初始模型构建不成功
			{
				cout << StringPrintf("[Model %d] Cannot find initial image pair!", ModelID + 1) << endl;
				mapper.EndReconstruction(true);
				ModelManager->Delete(ModelID);
				LastReconstructTimeConsuming = timer.elapsed() / 1000;
				continue;
			}
			else //初始模型构建成功
			{
				Callback(INITIAL_IMAGE_PAIR_REG_CALLBACK);
				ThisRegImagesID.insert(InitImageID1);
				ThisRegImagesID.insert(InitImageID2);
				cout << StringPrintf("Successfully set and registered images with [%s, id=%d] and [%s, id=%d] as initial image pair", CDatabase::GetImageName(InitImageID1, db).c_str(), InitImageID1, CDatabase::GetImageName(InitImageID2, db).c_str(), InitImageID2) << endl;
			}
			if (Base::IsQuit)
			{
				return;
			}
			bool IsContinue = false;
			if (!ThisRegImagesID.count(ThisImageID)) //当前影像还没有被用作初始影像对
			{
				if (mapper.RegisterNextImage(MapperOptions.Mapper(), ThisImageID)) //注册当前影像
				{
					cout << StringPrintf("[Model %d] Current image registration successful!", ModelID + 1) << endl;
					ImageTriangulate(MapperOptions, ThisImageID, &mapper); //空三
					if (Base::IsQuit)
					{
						return;
					}
					IterativeLocalRefinement(MapperOptions, ThisImageID, &mapper);
					ExtractColors(ThisImageID, &Model);
					Callback(NEXT_IMAGE_REG_CALLBACK);
					ThisRegImagesID.insert(ThisImageID);
				}
				else
				{
					cout << StringPrintf("[Model %d] Current image registration failed!", ModelID + 1) << endl;
				}
			}
		}
		cout << StringPrintf("[Model %d] Try registering another images...", ModelID + 1) << endl;
		while (!Base::IsQuit)
		{
			vector<size_t> NextImages = mapper.FindNextImages(MapperOptions.Mapper()); //寻找下一个要注册的影像
			bool IsContinue = false;
			for (size_t NextImageID : NextImages)
			{
				if (Base::IsQuit)
				{
					return;
				}
				if (ThisRegImagesID.count(NextImageID) || Model.IsImageRegistered(NextImageID))continue;
				if (!mapper.RegisterNextImage(MapperOptions.Mapper(), NextImageID))continue;
				if (NextImageID == ThisImageID)
				{
					cout << StringPrintf("[Model %d] Current image registration successful!", ModelID + 1) << endl;
				}
				else
				{
					cout << StringPrintf("[Model %d] Successfully registered image %s!", ModelID + 1, CDatabase::GetImageName(NextImageID, db).c_str()) << endl;
				}
				ImageTriangulate(MapperOptions, NextImageID, &mapper); //空三
				if (Base::IsQuit)
				{
					return;
				}
				IterativeLocalRefinement(MapperOptions, NextImageID, &mapper);
				ExtractColors(NextImageID, &Model);
				Callback(NEXT_IMAGE_REG_CALLBACK);
				ThisRegImagesID.insert(NextImageID);
				IsContinue = true;
				break;
			}
			if (!IsContinue)break;
		}
		cout << StringPrintf("[Model %d] Registration of other images has been completed!", ModelID + 1) << endl;
		size_t CurrentRegImagesNum = Model.GetModelRegImagesNum();
		size_t MinModelSize = min(DbCache.NumImages(), size_t(MapperOptions.min_model_size));
		if (CurrentRegImagesNum < MinModelSize)
		{
			cout << StringPrintf("[Model %d]: Too few registered images! the model will be deleted!", ModelID + 1) << endl;
			mapper.EndReconstruction(true);
			ModelManager->Delete(ModelID); //删除该模型
			LastReconstructTimeConsuming = timer.elapsed() / 1000;
			continue;
		}
		if (ThisRegImagesID.size() >= 20 && !Base::IsQuit)
		{
			cout << StringPrintf("[Model %d] Iterative global refinement is being performed...", ModelID + 1) << endl;
			IterativeGlobalRefinement(MapperOptions, &mapper);
			cout << StringPrintf("[Model %d] Iterative global refinement finished!", ModelID + 1) << endl;
		}
		cout << StringPrintf("[Model %d]: %d images were registered for this reconstruction: ", ModelID + 1, ThisRegImagesID.size());
		for (size_t index : ThisRegImagesID)
		{
			RegImages.insert(index);
			cout << StringPrintf("%s[%d]", CDatabase::GetImageName(index, db).c_str(), index) << ", ";
		}
		cout << endl;
		if (Base::IsQuit)
		{
			return;
		}
		mapper.EndReconstruction(false);
		LastReconstructTimeConsuming = TotalTimer.elapsed() / 1000;
		cout << StringPrintf("=====================> [Model %d, %d s]: This reconstruction is complete <=====================", ModelID + 1, LastReconstructTimeConsuming) << endl;
	}
	if (RegImages.size() > OriginRegImagesNum && !Base::IsQuit)
	{
		TryMergeModels();
	}

	//DebugTimer timer(__FUNCTION__);
	//cout << "Loading database cache..." << endl;
	//DatabaseCache DbCache;
	//CDatabase::ExportDatabaseCache(options, DbCache, db);
	//if (DbCache.NumImages() == 0)
	//{
	//	cout << "No images with matches found in the database!" << endl;
	//	return;
	//}
	//for (size_t ImageID : RegImages)
	//{
	//	if (DbCache.ExistsImage(ImageID))
	//	{
	//		DbCache.Image(ImageID).SetRegistered(true);
	//	}
	//}
	//cout << StringPrintf("Successfully loaded %d images, %d cameras, %d image pairs in database!", DbCache.NumImages(), DbCache.NumCameras(), DbCache.CorrespondenceGraph().NumImagePairs()) << endl;
	//CMapperOptions MapperOptions(*options->mapper);
	//int ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
	//if (ReconstructResult == -1)
	//{
	//	cout << "Trying to relax the initialization constraints..." << endl;
	//	size_t kNumInitRelaxations = 2; //放松条件
	//	for (size_t i = 0; i < kNumInitRelaxations; i++)
	//	{
	//		MapperOptions.mapper.init_min_num_inliers = MapperOptions.mapper.init_min_num_inliers / 2;
	//		ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
	//		if (ReconstructResult == -1)
	//		{
	//			cout << "Trying to relax the initialization constraints again..." << endl;
	//			MapperOptions.mapper.init_min_tri_angle = MapperOptions.mapper.init_min_tri_angle / 2;
	//			ReconstructResult = Reconstruct(ImagePath, &MapperOptions, DbCache, db);
	//			if (ReconstructResult != -1)
	//			{
	//				cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
	//				TryMergeModels();
	//				return;
	//			}
	//		}
	//		else
	//		{
	//			cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
	//			TryMergeModels();
	//			return;
	//		}
	//	}
	//	cout << StringPrintf("[%d s] All attempts to reconstruct have ended in failure!", LastReconstructTimeConsuming) << endl;
	//}
	//else
	//{
	//	cout << StringPrintf("[%d s] Reconstruction success!", LastReconstructTimeConsuming) << endl;
	//	TryMergeModels();
	//}
}
vector<size_t> CReconstructor::ChooseModel(string ImagePath, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	vector<size_t> re;
	for (size_t i = 0; i < ModelManager->Size(); i++)
	{
		CModel Model = ModelManager->Get(i);
		if (IsImageBelongsModel(ImagePath, Model, db))
		{
			re.push_back(i);
		}
	}
	if (re.empty())
	{
		cout << "Add a new model!" << endl;
		return { ModelManager->Add() };
	}
	return re;
}
bool CReconstructor::IsImageBelongsModel(string ImagePath, CModel& Model, QSqlDatabase& db)
{
	DebugTimer DebugTimer(__FUNCTION__);
	//设ImagePath对应的影像为a, 模型中注册的其中一张影像为b, 如果a和b之间的双视几何匹配数超过MinMatchedPointsNum, 那么就认为a和b是"有效的影像对"
	//如果在当前模型的所有注册影像中, 与a构成"有效的影像对"的数量超过MinMatchedImagesNum, 那么就认为a影像属于该模型
	size_t MinMatchedPointsNum = 20, MinMatchedImagesNum = 2;
	vector<size_t> ModelRegImages = Model.GetAllRegImagesIDs(); //获取模型的全部注册影像
	size_t ImageID = CDatabase::GetImageID(GetFileName(ImagePath),db);
	size_t ValidMatchedImageNum = 0;
	for (size_t ModelImageID : ModelRegImages)
	{
		if (ModelImageID == ImageID)continue;
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
		/*ModelManager->UnLock();
		mapper.EndReconstruction(true);
		ModelManager->Delete(ModelID);
		ModelManager->Lock();*/
		return false;
	}
	cout << StringPrintf("[Model %d]: Initializing with image pair: [%s, ID=%d] and [%s, ID=%d]!", ModelID + 1, CDatabase::GetImageName(InitImageID1, db).c_str(), InitImageID1, CDatabase::GetImageName(InitImageID2, db).c_str(), InitImageID2) << endl;
	
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
	DebugTimer timer(__FUNCTION__);
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
}
size_t CReconstructor::ImageTriangulate(CMapperOptions& options, size_t ImageID, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	size_t num_tris = mapper->TriangulateImage(options.Triangulation(), ImageID);
#ifdef OUTPUTLOG_MODE
	qDebug() << StdString2QString(StringPrintf("[Image triangulate] Added observations: %d", num_tris));
#endif
	return num_tris;
}
void CReconstructor::IterativeLocalRefinement(CMapperOptions& options, image_t ImageID, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	CBundleAdjustmentOptions BA_Options = options.LocalBundleAdjustment();
	BA_Options.solver_options.logging_type = ceres::LoggingType::SILENT;
	BA_Options.print_summary = false;
	BA_Options.solver_options.num_threads = omp_get_max_threads();
	for (int i = 0; i < options.ba_local_max_refinements; i++)
	{
		if (Base::IsQuit)
		{
			return;
		}
		CMapper::Options CMapperOptions = options.Mapper();
		CTriangulator::Options CTriangulatorOptions = options.Triangulation();

		auto Report = mapper->AdjustLocalBundle(CMapperOptions, BA_Options, CTriangulatorOptions, ImageID, mapper->GetModifiedPoints3D());
		double Changed = 0;
		if (Report.num_adjusted_observations != 0)
		{
			Changed = (Report.num_merged_observations + Report.num_completed_observations + Report.num_filtered_observations) * 1.0 / Report.num_adjusted_observations;
		}
		if (Changed < options.ba_local_max_refinement_change)
		{
			break;
		}
		// Only use robust cost function for first iteration.
		BA_Options.loss_function_type = CBundleAdjustmentOptions::LossFunctionType::TRIVIAL;
	}
	mapper->ClearModifiedPoints3D();
#ifdef OUTPUTLOG_MODE
	qDebug() << "[Iterative local refinement] All steps completed!";
#endif
}
void CReconstructor::IterativeGlobalRefinement(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	if (Base::IsQuit)
	{
		return;
	}
	CompleteAndMergeTracks(options, mapper);
	for (size_t i = 0; i < options.ba_global_max_refinements; i++)
	{
		if (Base::IsQuit)
		{
			return;
		}
		size_t num_observations = mapper->GetModel()->GetObservationsNum();
		size_t num_changed_observations = 0;
		GlobalBundleAdjust(options, mapper);
		num_changed_observations += CompleteAndMergeTracks(options, mapper);
		num_changed_observations += FilterPoints(options, mapper);
		double changed = (num_observations == 0 ? 0 : static_cast<double>(num_changed_observations) / num_observations);
		if (changed < options.ba_global_max_refinement_change)
		{
			break;
		}
	}
	FilterImages(options, mapper);
#ifdef OUTPUTLOG_MODE
	qDebug() << "[Iterative global refinement] All steps completed!";
#endif
}
void CReconstructor::ExtractColors(image_t ImageID, CModel* Model)
{
	DebugTimer timer(__FUNCTION__);
	if (!Model->ExtractColors_SingleImage(ImageID, *options->image_path))
	{
		cout << StringPrintf("[WARNING] Could not read image %d at path %s!", ImageID, (*options->image_path).c_str()) << endl;
	}
}
size_t CReconstructor::FilterPoints(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	size_t num_filtered_observations = mapper->FilterPoints(options.Mapper());
#ifdef OUTPUTLOG_MODE
	qDebug() << "  => Filtered observations: " << num_filtered_observations;
#endif
	return num_filtered_observations;
}
size_t CReconstructor::FilterImages(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	const size_t num_filtered_images = mapper->FilterImages(options.Mapper());
#ifdef OUTPUTLOG_MODE
	qDebug() << "  => Filtered images: " << num_filtered_images;
#endif
	return num_filtered_images;
}
size_t CReconstructor::CompleteAndMergeTracks(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	const size_t num_completed_observations = mapper->CompleteTracks(options.Triangulation());
#ifdef OUTPUTLOG_MODE
	qDebug() << "  => Completed observations: " << num_completed_observations;
#endif
	const size_t num_merged_observations = mapper->MergeTracks(options.Triangulation());
#ifdef OUTPUTLOG_MODE
	qDebug() << "  => Merged observations: " << num_merged_observations;
#endif
	return num_completed_observations + num_merged_observations;
}