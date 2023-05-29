#include "Reconstructor.h"


using namespace std;
using namespace colmap;

size_t CReconstructor::LastReconstructTimeConsuming = 0;
CReconstructor::CReconstructor(OptionManager* options, CModelManager* ModelManager)
{
	CHECK(options->mapper.get()->Check());
	this->options = options;
	this->ModelManager = ModelManager;
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
		cout << StringPrintf("[Reconstructor] Warning! %s does not exists in database!", ImageName) << endl;
	}
	size_t ImageID = CDatabase::GetImageID(ImageName, db);
	cout << StringPrintf("=====> Reconstruction of image %s is being done... <=====", ImageName) << endl;
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
	double MaxReprojectError = 80000;

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
		size_t kNumInitRelaxations = 2; //放松条件
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
	size_t ModelID = ChooseModel(ImagePath,db); //当前影像应该属于哪个模型
	cout << StringPrintf("=====================> Current model: %d <=====================", ModelID + 1) << endl;
	//ModelManager->Lock();
	CModel& Model = ModelManager->GetRefer(ModelID);
	mapper.BeginReconstruction(&Model);

	size_t InitImageID1 = kInvalidImageId, InitImageID2 = kInvalidImageId, NextTryImageID;
	vector<image_t> ThisRegImgIDs; //本次重建过程中, 新注册的影像。如果重建成功, 那么这个里面的值会全部添加到RegImgIDs中。如果重建失败, 则会清空这个
	if (Model.GetModelRegImagesNum() == 0)
	{
		cout << "This is a new model! Trying to find initial pair..." << endl;
		if (!RegisterInitialPair(MapperOptions, mapper, ModelID, InitImageID1, InitImageID2, db))
		{
			LastReconstructTimeConsuming = timer.elapsed() / 1000;
			//ModelManager->UnLock();
			return -1;
		}
		ThisRegImgIDs.push_back(InitImageID1);
		ThisRegImgIDs.push_back(InitImageID2);
		cout << StringPrintf("Successfully set and registered images with [%s, id=%d] and [%s, id=%d] as initial image pair", CDatabase::GetImageName(InitImageID1, db), InitImageID1, CDatabase::GetImageName(InitImageID2, db), InitImageID2) << endl;
	}
	Callback(INITIAL_IMAGE_PAIR_REG_CALLBACK);

	size_t OriginRegImgNum = Model.GetModelRegImagesNum(); //注册新影像之前, 模型原有的注册影像数和点数
	size_t OriginPointNum = Model.GetModelPoints3DNum();
	size_t ContinuedFailedNum = 0; //连续失败次数
	while (ContinuedFailedNum <= 3)
	{
		if (!IsContinue)
		{
			//ModelManager->UnLock();
			return 1;
		}
		cout << StringPrintf("[Model %d]: Trying to register other images...", ModelID + 1) << endl;
		vector<size_t> NextImages = mapper.FindNextImages(MapperOptions->Mapper()); //寻找下一个要注册的影像
		if (NextImages.empty()) //如果找不到下一个要注册的影像
		{
			cout << StringPrintf("[Model %d]: No more other images!", ModelID + 1) << endl;
			break;
		}
		for (image_t NextImageID : NextImages)
		{
			if (!IsContinue)
			{
				//ModelManager->UnLock();
				return 1;
			}
			cout << StringPrintf("[Model %d]: Trying to register next image [%s, id=%d]...", ModelID + 1, CDatabase::GetImageName(NextImageID, db), NextImageID) << endl;
			bool IsThisRegSuccess = mapper.RegisterNextImage(MapperOptions->Mapper(), NextImageID); //尝试注册该影像
			if (IsThisRegSuccess)
			{
				cout << "Success!" << endl;
				emit ChangeImageColor_SIGNAL(NextImageID); //更改当前注册成功的影像的颜色
				ContinuedFailedNum = 0; //连续失败次数置0
				Image image = Model.GetModelImage(NextImageID);
				ImageTriangulate(*MapperOptions, image, &mapper); //影像三角化
				cout << StringPrintf("[Model %d]: Performing iterative local refinement...", ModelID + 1) << endl;
				try
				{
					IterativeLocalRefinement(*MapperOptions, NextImageID, &mapper);
				}
				catch (...)
				{
					qDebug() << "Catch an error!";
				}

				//如果当前引入了过多的注册影像或者过多的点
				if (Model.GetModelRegImagesNum() >= MapperOptions->ba_global_images_ratio * OriginRegImgNum || Model.GetModelRegImagesNum() >= MapperOptions->ba_global_images_freq + OriginRegImgNum || Model.GetModelPoints3DNum() >= MapperOptions->ba_global_points_ratio * OriginPointNum || Model.GetModelPoints3DNum() >= MapperOptions->ba_global_points_freq + OriginPointNum)
				{
					//cout << StringPrintf("[Model %d]: Added too many new images and points, performing iterative global refinement...", ModelID + 1) << endl;
					//IterativeGlobalRefinement(*MapperOptions, &mapper); //进行迭代式全局精化
					//OriginRegImgNum = Model.GetModelRegImagesNum();
					//OriginPointNum = Model.GetModelPoints3DNum();
				}

				ExtractColors(NextImageID, &Model);
				Callback(NEXT_IMAGE_REG_CALLBACK);
				ThisRegImgIDs.emplace_back(NextImageID);
				break; //当前已经成功注册了一个影像, 不再注册其他影像
			}
			else
			{
				cout << "Fail!" << endl;
				ContinuedFailedNum++;
				if (ContinuedFailedNum >= 3)
				{
					break;
				}
			}
		}
	}
	if (Model.GetModelRegImagesNum() >= 2 && Model.GetModelRegImagesNum() != OriginRegImgNum && Model.GetModelPoints3DNum() != OriginPointNum)
	{
		IterativeGlobalRefinement(*MapperOptions, &mapper);
	}
	size_t MinModelSize = min(DbCache.NumImages(), size_t(MapperOptions->min_model_size));
	if (Model.GetModelRegImagesNum() < MinModelSize)
	{
		cout << StringPrintf("[Model %d]: Too few registered images! the model will be deleted!", ModelID + 1) << endl;
		mapper.EndReconstruction(true);
		ThisRegImgIDs.clear(); //清空该列表, 不往RegImgIDs中写入
		ModelManager->Delete(ModelID); //删除该模型
	}
	else
	{
		cout << StringPrintf("=====================> [Model %d]: This reconstruction is complete <=====================", ModelID + 1) << endl;
		cout << StringPrintf("[Model %d]: %d images were registered for this reconstruction: ", ModelID + 1, ThisRegImgIDs.size());
		for (size_t index = 0; index < ThisRegImgIDs.size(); index++) //把ThisRegImgIDs里面的值全部添加到RegImgIDs中
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
	//返回第一个符合条件的模型
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
	//设ImagePath对应的影像为a, 模型中注册的其中一张影像为b, 如果a和b之间的双视几何匹配数超过MinMatchedPointsNum, 那么就认为a和b是"有效的影像对"
	//如果在当前模型的所有注册影像中, 与a构成"有效的影像对"的数量超过MinMatchedImagesNum, 那么就认为a影像属于该模型
	size_t MinMatchedPointsNum = 10, MinMatchedImagesNum = 3;
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
	DebugTimer timer(__FUNCTION__);
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
	DebugTimer timer(__FUNCTION__);
	cout << StringPrintf("[Image triangulate] Continued observations: %d", image.NumPoints3D()) << endl;
	size_t num_tris = mapper->TriangulateImage(options.Triangulation(), image.ImageId());
	cout << StringPrintf("[Image triangulate] Added observations: %d", num_tris) << endl;
	return num_tris;
}
void CReconstructor::IterativeLocalRefinement(CMapperOptions& options, image_t ImageID, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
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
	DebugTimer timer(__FUNCTION__);
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
	DebugTimer timer(__FUNCTION__);
	if (!Model->ExtractColors_SingleImage(ImageID, *options->image_path))
	{
		cout << StringPrintf("[WARNING] Could not read image %d at path %s!", ImageID, *options->image_path) << endl;
	}
}
size_t CReconstructor::FilterPoints(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	size_t num_filtered_observations = mapper->FilterPoints(options.Mapper());
	std::cout << "  => Filtered observations: " << num_filtered_observations << std::endl;
	return num_filtered_observations;
}
size_t CReconstructor::FilterImages(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	const size_t num_filtered_images = mapper->FilterImages(options.Mapper());
	std::cout << "  => Filtered images: " << num_filtered_images << std::endl;
	return num_filtered_images;
}
size_t CReconstructor::CompleteAndMergeTracks(CMapperOptions& options, CMapper* mapper)
{
	DebugTimer timer(__FUNCTION__);
	const size_t num_completed_observations = mapper->CompleteTracks(options.Triangulation());
	std::cout << "  => Completed observations: " << num_completed_observations << std::endl;
	const size_t num_merged_observations = mapper->MergeTracks(options.Triangulation());
	std::cout << "  => Merged observations: " << num_merged_observations << std::endl;
	return num_completed_observations + num_merged_observations;
}