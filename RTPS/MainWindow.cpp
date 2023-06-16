#include "MainWindow.h"

using namespace std;
using namespace colmap;

CMainWindow::CMainWindow(QWidget* parent) :QMainWindow(parent)
{
	ThreadStart();
	options = new OptionManager();
	options->AddAllOptions();

	SetupWidgets();
	SetupActions();
	CreateMenus();
	CreateToolbar();
	CreateStatusbar();

	CreateExtractor();
	CreateMatcher();
	CreateReconstructor();
}

void CMainWindow::ImportModel(int Type, vector<string> ModelFolders, string PLYPath, bool IsOverwrite)
{
	if (ModelManager->Size() == 0 || IsOverwrite)
	{
		ModelManager->Clear();
		ModelSelectWidget->Update();
		ModelSelectWidget->SelectModel(ReconstructionManagerWidget::kNewestReconstructionIdx);
		ModelViewer->ClearReconstruction();
		if (Reconstructor)
		{
			Reconstructor->~CReconstructor();
			Reconstructor = nullptr;
		}
		CreateReconstructor();
	}
	if (Type == 0)
	{
		bool IsCommonSourceModel = true;
		string OriginImagePath = "";
		if (ExistsDir(*options->image_path))
		{
			OriginImagePath = *options->image_path;
		}
		string ProjectPath = JoinPaths(ModelFolders[ModelFolders.size() - 1], "project.ini");
		if (ExistsFile(ProjectPath))
		{
			options->ReRead(ProjectPath);
			*options->project_path = ProjectPath;
			UpdateWindowTitle(StdString2QString(ProjectPath));
			if (ExistsFile(*options->database_path))
			{
				options->image_reader->database_path = *options->database_path;
				QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
				CDatabase::OpenDatabase(*options->database_path, db);
				ReleaseDatabaseConnect(db);
			}
			if (ExistsDir(*options->image_path))
			{
				if (OriginImagePath != "" && OriginImagePath != *options->image_path)
				{
					if (!IsOverwrite)
					{
						QMessageBox::StandardButton re = QMessageBox::question(this, tr("Different image sources"), tr("The model to be imported has a different image source than the existing models, please set to \"Overwrite current models\" in the import options! Or you can choose to ignore this message, but the subsequent process may report an error or the program will crash! Continue to import?"));
						if (re != QMessageBox::Yes)
						{
							return;
						}
						IsCommonSourceModel = false;
					}
				}
				if (IsOverwrite)
				{
					ImageListWidget->ClearImage();
				}
				LoadDirImages_SLOT();
			}
		}
		for (string Folder : ModelFolders)
		{
			size_t idx = ModelManager->Read(Folder);
			ModelSelectWidget->Update();
			ModelSelectWidget->SelectModel(idx);
		}
		RenderModelAction->triggered();
		if (IsCommonSourceModel)
		{
			Reconstructor->UpdateRegImgIDs();
			Reconstructor->TryMergeModels();
		}
		ModelSelectWidget->setCurrentIndex(0);
		RenderModelAction->triggered();
		if (!ExistsFile(*options->database_path))
		{
			QMessageBox::StandardButton re = QMessageBox::question(this, tr("Can't find database"), tr("The database corresponding to this model could not be found! Is feature extraction and feature matching performed on all images at once?"));
			if (re == QMessageBox::Yes)
			{
				for (string ImagePath : ImageListWidget->ImagePaths)
				{
					if (ExistsFile(ImagePath))
					{
						ScaleImage(ImagePath, options->sift_extraction->max_image_size);
						cout << StringPrintf("Start processing image %s", GetFileName(ImagePath)) << endl;
						if (!FeatureExtractor)
						{
							cout << "Error! Feature extractor is nullptr!" << endl;
							continue;
						}
						if (FeatureExtractor->Extract(ImagePath))
						{
							if (!FeatureMatcher)
							{
								cout << "Error! Feature matcher is nullptr!" << endl;
								continue;
							}
							FeatureMatcher->Match(ImagePath);
						}
						cout << StringPrintf("All processes for Image %s have been completed!", GetFileName(ImagePath)) << endl;
					}
				}
			}
		}
	}
	else if (Type == 1)
	{
		ThreadControlWidget* thread_control_widget_ = new ThreadControlWidget(this);
		thread_control_widget_->StartFunction(tr("Importing Model from PLY File..."), [this, PLYPath]()
			{
				size_t reconstruction_idx = ModelManager->Add();
				ModelManager->Get(reconstruction_idx).ReadPLY(PLYPath);
				options->render->min_track_len = 0;
				ModelSelectWidget->Update();
				ModelSelectWidget->SelectModel(reconstruction_idx);
				RenderModelAction->triggered();
			});
	}
	QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
	CDatabase::GenerateMatchInfo(db);
	ReleaseDatabaseConnect(db);
	
}
void CMainWindow::ChangeNewImageColor(int ImageID, int ModelID)
{
	if (IsTrackModel_Checkbox->isChecked() && ModelID < ModelManager->Size() && ModelID + 1 < ModelSelectWidget->count())
	{
		ModelSelectWidget->setCurrentIndex(ModelID + 1);
	}
	ImageColormapNameFilter Filter;
	Eigen::Vector4f NewImagePlaneColor;
	NewImagePlaneColor(0) = 0.0 / 255.0;
	NewImagePlaneColor(1) = 0.0 / 255.0;
	NewImagePlaneColor(2) = 128.0 / 255.0;
	NewImagePlaneColor(3) = 0.6;

	Eigen::Vector4f NewImageFrameColor;
	NewImageFrameColor(0) = 0.0 / 255.0;
	NewImageFrameColor(1) = 0.0 / 255.0;
	NewImageFrameColor(2) = 255.0 / 255.0;
	NewImageFrameColor(3) = 1.0;

	QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
	string ImageName = CDatabase::GetImageName(ImageID, db);
	Filter.AddColorForWord(ImageName, NewImagePlaneColor, NewImageFrameColor);
	ImageColormapBase* ColorMap = new ImageColormapNameFilter(Filter);
	ModelViewer->SetImageColormap(ColorMap);
	ModelViewer->ReloadReconstruction();

	/*ImageColormapNameFilter Filter;
	ImageColormapBase* ColorMap = new ImageColormapNameFilter(Filter);
	ModelViewer->SetImageColormap(ColorMap);
	ModelViewer->ReloadReconstruction();

	QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
	string ImageName = CDatabase::GetImageName(ImageID, db);
	ReleaseDatabaseConnect(db);

	Eigen::Vector4f NewImageColor(ImageColormapBase::kDefaultPlaneColor);
	NewImageColor(0) = 0;
	NewImageColor(1) = 0;
	NewImageColor(2) = 1;
	NewImageColor(3) = 0.8;

	Eigen::Vector4f NewImageFlameColor(ImageColormapBase::kDefaultPlaneColor);
	NewImageFlameColor(0) = 0.0 / 255;
	NewImageFlameColor(1) = 0.0 / 255;
	NewImageFlameColor(2) = 255.0 / 255;
	NewImageFlameColor(3) = 1;

	Filter.AddColorForWord(ImageName, NewImageColor, NewImageFlameColor);
	ColorMap = new ImageColormapNameFilter(Filter);
	ModelViewer->SetImageColormap(ColorMap);
	ModelViewer->ReloadReconstruction();*/
	//this_thread::sleep_for(chrono::milliseconds(330));
}
void CMainWindow::NewImage_SLOT(string ImagePath)
{
	cout << StringPrintf("Image %s detected!", GetFileName(ImagePath)) << endl;
	std::thread ProcessThread(&CMainWindow::ProcessImage, this, ImagePath);
	ProcessThread.detach();
}

void CMainWindow::SenderQuit_SLOT()
{
	StopReceive();
}

void CMainWindow::ShowFeatureExtractionSettingWidget()
{
	FeatureExtractionSettingWidget->show();
}
void CMainWindow::ShowFeatureMatchingSettingWidget()
{
	FeatureMatchingSettingWidget->show();
}
void CMainWindow::ShowReconstructionSettingWidget()
{
	ReconstructionSettingWidget->show();
}
void CMainWindow::ShowReceiverSettingWidget()
{
	ReceiverSettingWidget->show();
}
void CMainWindow::ShowProjectSettingWidget()
{
	ProjectSettingWidget->show();
}
void CMainWindow::LoadDirImages_SLOT()
{
	auto AddImagesFunc = [&]()
	{
		ThreadStart();
		QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
		string DirPath = *options->image_path;
		if (DirPath.back() != '/' && DirPath.back() != '\\')
		{
			DirPath += '/';
		}
		vector<string> ImageNames;
		CDatabase::GetAllImageNames(ImageNames, db);
		ReleaseDatabaseConnect(db);
		for (string& ImageName : ImageNames)
		{
			string ImagePath = DirPath + ImageName;
			if (ExistsFile(ImagePath))
			{
				ImageListWidget->AddImage(ImagePath);
			}
		}
		std::cout << "Successfully loaded database and images!" << std::endl;
		SaveProjectAction->setEnabled(true);
		SaveAsProjectAction->setEnabled(true);
		ThreadEnd();
	};
	std::thread AddImagesThread(AddImagesFunc);
	AddImagesThread.detach();
}
void CMainWindow::OpenProject()
{
	if (ExistsFile(*options->database_path) || ExistsDir(*options->image_path))
	{
		QMessageBox::StandardButton re = QMessageBox::question(this, tr("Project is not empty"), tr("Want to open a new project to overwrite the current project?"));
		if (re != QMessageBox::Yes)
		{
			return;
		}
	}
	string ProjectPath = QString2StdString(QFileDialog::getOpenFileName(this, tr("Open a project file"), "", tr("Project file (*.ini)")));
	if (ProjectPath == "")return;
	if (!options->ReRead(ProjectPath))
	{
		QMessageBox::critical(this, tr("Project file error"), tr("The selected project file is incorrect!"));
		return;
	}
	*options->project_path = ProjectPath;
	UpdateWindowTitle(StdString2QString(ProjectPath));
	if (ExistsDir(*options->image_path) && ExistsFile(*options->database_path))
	{
		options->image_reader->database_path = *options->database_path;
		QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
		CDatabase::OpenDatabase(*options->database_path, db);
		ReleaseDatabaseConnect(db);
		LoadDirImages_SLOT();
	}
	SaveProjectAction->setEnabled(true);
	SaveAsProjectAction->setEnabled(true);
}
void CMainWindow::SaveProject()
{
	if (!ExistsFile(*options->database_path) || !ExistsDir(*options->image_path))
	{
		QMessageBox::critical(this, tr("Project is empty"), tr("Create a new project or load a project first!"));
		return;
	}
	if (!ExistsFile(*options->project_path))
	{
		string ProjectPath = QString2StdString(QFileDialog::getSaveFileName(this, tr("Select a project file"), "", tr("Project file (*.ini)")));
		if (ProjectPath != "")
		{
			if (!HasFileExtension(ProjectPath, ".ini"))
			{
				ProjectPath += ".ini";
			}
			*options->project_path = ProjectPath;
			options->Write(*options->project_path);
			UpdateWindowTitle(StdString2QString(ProjectPath));
		}
	}
	else
	{
		options->Write(*options->project_path);
		UpdateWindowTitle(StdString2QString(*options->project_path));
	}
}
void CMainWindow::SaveAsProject()
{
	if (!ExistsFile(*options->database_path) || !ExistsDir(*options->image_path))
	{
		QMessageBox::critical(this, tr("Project is empty"), tr("Create a new project or load a project first!"));
		return;
	}
	string ProjectPath = QString2StdString(QFileDialog::getSaveFileName(this, tr("Select a project file"), "", tr("Project file (*.ini)")));
	if (ProjectPath != "")
	{
		if (!HasFileExtension(ProjectPath, ".ini"))
		{
			ProjectPath += ".ini";
		}
		*options->project_path = ProjectPath;
		options->Write(*options->project_path);
		UpdateWindowTitle(StdString2QString(ProjectPath));
	}
}
void CMainWindow::ShowModelImportWidget()
{
	ModelImportWidget->show();
}
void CMainWindow::ShowModelExportWidget()
{
	ModelExportWidget->show();
}

void CMainWindow::StartReceive()
{
	QString SaveDir = StdString2QString(*options->image_path);
	QDir dir(SaveDir);
	if (!dir.exists() || SaveDir == "")
	{
		QMessageBox::critical(this, tr("Error"), tr("Image save path has not been set!"));
		return;
	}
	QFileInfo TransmitFile(QCoreApplication::applicationDirPath() + "/CamFiTransmit.exe");
	if (!TransmitFile.isFile())
	{
		QMessageBox::critical(this, "", tr("Error! \"CamFiTransmit.exe\" does not exist in the current path!"));
		return;
	}
	StartReceiveAction->setEnabled(false);
	Receiver = new CReceiver(*options->image_path);
	qRegisterMetaType<std::string>();
	connect(Receiver, SIGNAL(NewImage_SIGNAL(std::string)), this, SLOT(NewImage_SLOT(std::string)));
	connect(Receiver, SIGNAL(SenderQuit_SIGNAL()), this, SLOT(SenderQuit_SLOT()));
	Receiver->StartReceive();

	StopReceiveAction->setEnabled(true);
	FeatureExtractionSettingAction->setEnabled(false);
	FeatureMatchingSettingAction->setEnabled(false);
	TransmitSettingAction->setEnabled(false);
	NewProjectAction->setEnabled(false);
	OpenProjectAction->setEnabled(false);
}
void CMainWindow::ProcessImage(string ImagePath)
{
	if (Base::IsQuit)
	{
		return;
	}
	//ScaleImage(ImagePath, options->sift_extraction->max_image_size);
	ImageListWidget->AddImage(ImagePath);
	while (GetThreadCount() >= 8 && !Base::IsQuit)
	{
		size_t SleepTime = QRandomGenerator::global()->bounded(100, 1001);
		this_thread::sleep_for(chrono::milliseconds(SleepTime));
	}

	ThreadStart();
	QElapsedTimer timer;
	timer.start();
	cout << StringPrintf("Start processing image %s", GetFileName(ImagePath)) << endl;
	
	if (!FeatureExtractor)
	{
		cout << "Error! Feature extractor is nullptr!" << endl;
		ThreadEnd();
		return;
	}
	if (Base::IsQuit)
	{
		return;
	}
	if (FeatureExtractor->Extract(ImagePath))
	{
		if (Base::IsQuit)
		{
			return;
		}
		if (!FeatureMatcher)
		{
			cout << "Error! Feature matcher is nullptr!" << endl;
			ThreadEnd();
			return;
		}
		if (FeatureMatcher->Match(ImagePath))
		{
			if (Base::IsQuit)
			{
				return;
			}
			if (!Reconstructor)
			{
				cout << "Error! Reconstructor is nullptr!" << endl;
				ThreadEnd();
				return;
			}
			LastWaitImagePath = ImagePath;
			WaitforReconstructImageNum++;
			WaitforReconstructImagePath_Mutex.lock();
			WaitforReconstructImagePath.push(ImagePath);
			WaitforReconstructImagePath_Mutex.unlock();
		}
	}
	cout << StringPrintf("[%d s] All processes for Image %s have been completed!", timer.elapsed() / 1000, GetFileName(ImagePath)) << endl;
	ThreadEnd();
}
void CMainWindow::StopReceive()
{
	Receiver->StopReceive();
	Receiver->~CReceiver();
	Receiver = nullptr;
	StopReceiveAction->setEnabled(false);
	StartReceiveAction->setEnabled(true);
	FeatureExtractionSettingAction->setEnabled(true);
	FeatureMatchingSettingAction->setEnabled(true);
	TransmitSettingAction->setEnabled(true);
	NewProjectAction->setEnabled(true);
	OpenProjectAction->setEnabled(true);
}
void CMainWindow::ShowRenderOptionsWidget()
{
	RenderOptionsWidget->show();
}
void CMainWindow::ShowShowMatchMatrixWidget()
{
	ShowMatchMatrixWidget->show();
}

void CMainWindow::closeEvent(QCloseEvent* event)
{
	QMessageBox::StandardButton resBtn = QMessageBox::question(this, tr("Close"), tr("Are you sure to quit?"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
	if (resBtn != QMessageBox::Yes) 
	{
		event->ignore();
	}
	else 
	{
		Base::IsQuit = true;
		IsContinueReconstruct = false;
		event->accept();
		if (Receiver)
		{
			StopReceive();
		}
	}
}
void CMainWindow::UpdateWindowTitle(QString Extras)
{
	if (Extras == "")
	{
		this->setWindowTitle(tr("RealTimePhotogrammetrySystem"));
	}
	else
	{
		this->setWindowTitle(tr("RealTimePhotogrammetrySystem") + " - " + Extras);
	}

}
void CMainWindow::SetupWidgets()
{
	resize(Size(1000, true), Size(800, false));
	UpdateWindowTitle();
	setWindowIcon(QIcon(":/media/WindowIcon.png"));

	ModelViewer = new ModelViewerWidget(this, options);
	setCentralWidget(ModelViewer);

	ModelManager = new CModelManager();
	ModelSelectWidget = new CModelSelectWidget(this, ModelManager);
	ModelSelectWidget->setFixedWidth(Size(270, true));
	connect(ModelSelectWidget, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CMainWindow::RenderModel_SLOT);

	ImgListViewerDock = new QDockWidget(tr("Image Preview"), this);
	ImgListViewerDock->setMinimumHeight(Size(100, false));
	ImgListViewerDock->setMinimumWidth(Size(250, true));
	ImageListWidget = new CImageListWidget(ImgListViewerDock);
	ImgListViewerDock->setWidget(ImageListWidget);
	addDockWidget(Qt::RightDockWidgetArea, ImgListViewerDock);
	IsTrackModel_Checkbox = new QCheckBox(tr("Track the latest progress"));
	IsTrackModel_Checkbox->setChecked(true);

	LogWindowDock = new QDockWidget(tr("Log"), this);
	LogWindow = new LogWidget(this);
	LogWindowDock->setWidget(LogWindow);
	addDockWidget(Qt::BottomDockWidgetArea, LogWindowDock);
	LogWindowDock->show();
	LogWindowDock->raise();

	FeatureExtractionSettingWidget = new CFeatureExtractionSettingWidget(this, options);
	connect(FeatureExtractionSettingWidget, &CFeatureExtractionSettingWidget::NewOptions_SIGNAL, this, &CMainWindow::CreateExtractor);
	FeatureMatchingSettingWidget = new CFeatureMatchingSettingWidget(this, options);
	connect(FeatureMatchingSettingWidget, &CFeatureMatchingSettingWidget::NewOptions_SIGNAL, this, &CMainWindow::CreateMatcher);
	ReconstructionSettingWidget = new CReconstructionSettingWidget(this, options);
	ReceiverSettingWidget = new CReceiverSettingWidget(this);
	
	ProjectSettingWidget = new CProjectSettingWidget(this, options);
	connect(ProjectSettingWidget, &CProjectSettingWidget::LoadDirImages_SIGNAL, this, &CMainWindow::LoadDirImages_SLOT);
	
	ModelImportWidget = new CModelImportWidget(this);
	connect(ModelImportWidget, SIGNAL(ModelImport_SIGNAL(int, std::vector<std::string>, std::string, bool)), this, SLOT(ImportModel(int, std::vector<std::string>, std::string, bool)));
	
	ModelExportWidget = new CModelExportWidget(this, ModelManager, options);

	ShowMatchMatrixWidget = new CShowMatchMatrixWidget(this, options);

	RenderOptionsWidget = new CRenderOptionsWidget(this, options, ModelViewer);
}
void CMainWindow::SetupActions()
{
	FeatureExtractionSettingAction = new QAction(QIcon(":/media/feature-extraction.png"), tr("Feature Extraction Setting"), this);
	connect(FeatureExtractionSettingAction, &QAction::triggered, this, &CMainWindow::ShowFeatureExtractionSettingWidget);

	FeatureMatchingSettingAction = new QAction(QIcon(":/media/feature-matching.png"), tr("Feature Matching Setting"), this);
	connect(FeatureMatchingSettingAction, &QAction::triggered, this, &CMainWindow::ShowFeatureMatchingSettingWidget);

	ReconstructionSettingAction = new QAction(QIcon(":/media/bundle-adjustment.png"), tr("Reconstruction Setting"), this);
	connect(ReconstructionSettingAction, &QAction::triggered, this, &CMainWindow::ShowReconstructionSettingWidget);

	TransmitSettingAction = new QAction(QIcon(":/media/automatic-reconstruction.png"), tr("Transmit Setting"), this);
	connect(TransmitSettingAction, &QAction::triggered, this, &CMainWindow::ShowReceiverSettingWidget);

	NewProjectAction = new QAction(QIcon(":/media/project-new.png"), tr("New Project"), this);
	connect(NewProjectAction, &QAction::triggered, this, &CMainWindow::ShowProjectSettingWidget);

	OpenProjectAction = new QAction(QIcon(":/media/project-open.png"), tr("Open A Project"), this);
	connect(OpenProjectAction, &QAction::triggered, this, &CMainWindow::OpenProject);

	SaveProjectAction = new QAction(QIcon(":/media/project-save.png"), tr("Save the Project"), this);
	connect(SaveProjectAction, &QAction::triggered, this, &CMainWindow::SaveProject);
	SaveProjectAction->setEnabled(false);

	SaveAsProjectAction = new QAction(QIcon(":/media/project-save-as.png"), tr("Save the Project as..."), this);
	connect(SaveAsProjectAction, &QAction::triggered, this, &CMainWindow::SaveAsProject);
	SaveAsProjectAction->setEnabled(false);

	ImportModelAction = new QAction(QIcon(":/media/import.png"), tr("Import Models"), this);
	connect(ImportModelAction, &QAction::triggered, this, &CMainWindow::ShowModelImportWidget);

	ExportModelAction = new QAction(QIcon(":/media/export.png"), tr("Export Models"), this);
	connect(ExportModelAction, &QAction::triggered, this, &CMainWindow::ShowModelExportWidget);

	StartReceiveAction = new QAction(QIcon(":/media/reconstruction-start.png"), tr("Start Receive"), this);
	connect(StartReceiveAction, &QAction::triggered, this, &CMainWindow::StartReceive);

	StopReceiveAction = new QAction(QIcon(":/media/reconstruction-pause.png"), tr("Stop Receive"), this);
	connect(StopReceiveAction, &QAction::triggered, this, &CMainWindow::StopReceive);
	StopReceiveAction->setEnabled(false);

	ShowRenderOptionsAction = new QAction(QIcon(":/media/reconstruction-reset.png"), tr("Show Render options"), this);
	connect(ShowRenderOptionsAction, &QAction::triggered, this, &CMainWindow::ShowRenderOptionsWidget);

	ShowMatchMatrixAction = new QAction(QIcon(":/media/match-matrix.png"), tr("Show Match Matrix"), this);
	connect(ShowMatchMatrixAction, &QAction::triggered, this, &CMainWindow::ShowShowMatchMatrixWidget);

	RenderModelAction = new QAction(this);
	connect(RenderModelAction, &QAction::triggered, this, &CMainWindow::RenderModel_SLOT);

	connect(this, SIGNAL(RenderNow_SIGNAL()), this, SLOT(RenderModel()));
	connect(this, SIGNAL(ClearRenderModel_SIGNAL()), this, SLOT(ClearRenderModel()));
}
void CMainWindow::CreateMenus()
{
	SettingMenu = new QMenu(tr("Work Settings"), this);
	SettingMenu->addAction(FeatureExtractionSettingAction);
	SettingMenu->addAction(FeatureMatchingSettingAction);
	SettingMenu->addAction(ReconstructionSettingAction);
	SettingMenu->addAction(TransmitSettingAction);
	menuBar()->addAction(SettingMenu->menuAction());

	ProjectMenu = new QMenu(tr("Project"), this);
	ProjectMenu->addAction(NewProjectAction);
	ProjectMenu->addAction(OpenProjectAction);
	ProjectMenu->addAction(SaveProjectAction);
	ProjectMenu->addAction(SaveAsProjectAction);
	ProjectMenu->addSeparator();
	ProjectMenu->addAction(ImportModelAction);
	ProjectMenu->addAction(ExportModelAction);
	menuBar()->addAction(ProjectMenu->menuAction());

	TransmitMenu = new QMenu(tr("Transmit"), this);
	TransmitMenu->addAction(StartReceiveAction);
	TransmitMenu->addAction(StopReceiveAction);
	menuBar()->addAction(TransmitMenu->menuAction());

	ToolsMenu = new QMenu(tr("Tools"), this);
	ToolsMenu->addAction(ShowRenderOptionsAction);
	ToolsMenu->addAction(ShowMatchMatrixAction);
	menuBar()->addAction(ToolsMenu->menuAction());
}
void CMainWindow::CreateToolbar()
{
	SettingToolBar = addToolBar(tr("Settings"));
	SettingToolBar->addAction(FeatureExtractionSettingAction);
	SettingToolBar->addAction(FeatureMatchingSettingAction);
	SettingToolBar->addAction(ReconstructionSettingAction);
	SettingToolBar->addAction(TransmitSettingAction);
	SettingToolBar->setIconSize(QSize(16, 16));
	SettingToolBar->setMovable(false);

	ProjectToolBar = addToolBar(tr("Project"));
	ProjectToolBar->addAction(NewProjectAction);
	ProjectToolBar->addAction(OpenProjectAction);
	ProjectToolBar->addAction(SaveProjectAction);
	ProjectToolBar->addAction(SaveAsProjectAction);
	ProjectToolBar->setIconSize(QSize(16, 16));
	ProjectToolBar->setMovable(false);

	TransmitToolBar = addToolBar(tr("Transmit"));
	TransmitToolBar->addAction(StartReceiveAction);
	TransmitToolBar->addAction(StopReceiveAction);
	TransmitToolBar->setIconSize(QSize(16, 16));
	TransmitToolBar->setMovable(false);

	ReconstructToolBar = addToolBar(tr("Reconstruct"));
	ReconstructToolBar->addWidget(ModelSelectWidget);
	ReconstructToolBar->addSeparator();
	ReconstructToolBar->addWidget(IsTrackModel_Checkbox);
	ReconstructToolBar->addAction(ImportModelAction);
	ReconstructToolBar->addAction(ExportModelAction);
	ReconstructToolBar->setIconSize(QSize(16, 16));
	ReconstructToolBar->setMovable(false);


	ToolsToolBar = addToolBar(tr("Tools"));
	ToolsToolBar->addAction(ShowRenderOptionsAction);
	ToolsToolBar->addAction(ShowMatchMatrixAction);
	ToolsToolBar->setIconSize(QSize(16, 16));
	ToolsToolBar->setMovable(false);
}
void CMainWindow::CreateStatusbar()
{
	QFont Font;
	Font.setPointSize(11);

	StatusLabel = new QLabel(tr(""), this);
	StatusLabel->setFont(Font);
	StatusLabel->setAlignment(Qt::AlignCenter);
	statusBar()->addWidget(StatusLabel, 0);

	ModelViewer->statusbar_status_label = new QLabel("0 Images - 0 Points", this);
	ModelViewer->statusbar_status_label->setFont(Font);
	ModelViewer->statusbar_status_label->setAlignment(Qt::AlignCenter);
	statusBar()->addWidget(ModelViewer->statusbar_status_label, 1);

	connect(&UpdateStatusBar_Timer, &QTimer::timeout, this, &CMainWindow::UpdateStatusBar);
	UpdateStatusBar_Timer.start(500);
}
void CMainWindow::CreateExtractor()
{
	if (FeatureExtractor)
	{
		delete FeatureExtractor;
	}
	FeatureExtractor = new CFeatureExtractor(options);
}
void CMainWindow::CreateMatcher()
{
	if (FeatureMatcher)
	{
		if(dynamic_cast<CRetrievalMatcher*>(FeatureMatcher))
		{
			dynamic_cast<CRetrievalMatcher*>(FeatureMatcher)->Uninstall();
		}
		delete FeatureMatcher;
	}
	CFeatureMatchMethod MatchMethod = *options->FeatureMatchMethod;
	if (MatchMethod == CFeatureMatchMethod::Exhaustive)
	{
		FeatureMatcher = new CExhaustiveMatcher(options);
	}
	else if (MatchMethod == CFeatureMatchMethod::Retrieval)
	{
		FeatureMatcher = new CRetrievalMatcher(options);
	}
}
void CMainWindow::UpdateStatusBar()
{
	double TotalGB, UsedGB, FreeGB;
	GetMemoryUsage(TotalGB, UsedGB, FreeGB);
	QString MemoryUsage = tr("Memory: ") + QString("%1GB / %2GB, ").arg(UsedGB, 0, 'f', 1).arg(TotalGB, 0, 'f', 1);
	GetGPUMemUsage(TotalGB, UsedGB, FreeGB);
	QString GPUMemUsage = tr("GPU Memory: ") + QString("%1GB / %2GB, ").arg(UsedGB, 0, 'f', 1).arg(TotalGB, 0, 'f', 1);
	QString Info = MemoryUsage + GPUMemUsage + tr("Number of threads: ") + QString::number(GetThreadCount()) + "  ";
	StatusLabel->setText(Info);
}

void CMainWindow::DetectReconstruct()
{
	ThreadStart();

	while (IsContinueReconstruct)
	{
		this_thread::sleep_for(chrono::milliseconds(1000));
		WaitforReconstructImagePath_Mutex.lock();
		if (WaitforReconstructImagePath.empty())
		{
			WaitforReconstructImagePath_Mutex.unlock();
			continue;
		}
		string CurrentImagePath = WaitforReconstructImagePath.front();
		WaitforReconstructImagePath.pop();
		WaitforReconstructImagePath_Mutex.unlock();

		Reconstructor->Reconstruct(CurrentImagePath);
		AutoSaveModels();
		/*if (WaitforReconstructImageNum.load() == 0 || !Reconstructor)continue;
		if (WaitforReconstructImageNum.load() >= 3 || CReconstructor::LastReconstructTimeConsuming <= 5)
		{
			WaitforReconstructImageNum.store(0);
			Reconstructor->Reconstruct(LastWaitImagePath);
			AutoSaveModels();
		}*/
	}
	ThreadEnd();
}
void CMainWindow::RenderModel_SLOT()
{
	ModelSelectWidget->Update();
	std::thread RenderThread(&CMainWindow::GetRenderModel, this);
	RenderThread.detach();

	/*if (!ModelManager || !Reconstructor)
	{
		return;
	}

	if (ModelManager->Size() == 0)
	{
		ModelViewer->ClearReconstruction();
		return;
	}
	ModelSelectWidget->Update();
	size_t SelectedModelID = ModelSelectWidget->GetSelectedModelIndex();
	if (SelectedModelID == CModelSelectWidget::NewstModelIndex)
	{
		SelectedModelID = ModelManager->Size() - 1;
	}
	Reconstruction* Model = new Reconstruction();
	ModelManager->Get(SelectedModelID).ConvertToReconstruction(*Model);
	ModelViewer->reconstruction = Model;
	ModelViewer->ReloadReconstruction();*/
}
void CMainWindow::GetRenderModel()
{
	DebugTimer timer(__FUNCTION__);
	if (IsRenderNow)return;
	IsRenderNow = true;
	if (!ModelManager || !Reconstructor)
	{
		IsRenderNow = false;
		return;
	}
	if (ModelManager->Size() == 0)
	{
		emit ClearRenderModel_SIGNAL();
		IsRenderNow = false;
		return;
	}
	ModelSelectWidget->Update();
	Reconstruction* Model = new Reconstruction();
	if (IsTrackModel_Checkbox->isChecked() && Reconstructor->CurrentModelID < ModelManager->Size())
	{
		ModelManager->Get(Reconstructor->CurrentModelID).ConvertToReconstruction(*Model);
	}
	else
	{
		size_t SelectedModelID = ModelSelectWidget->GetSelectedModelIndex();
		if (SelectedModelID == CModelSelectWidget::NewstModelIndex)
		{
			SelectedModelID = ModelManager->Size() - 1;
		}
		ModelManager->Get(SelectedModelID).ConvertToReconstruction(*Model);
	}
	RenderModel_Mutex.lock();
	if (ModelToRender)
	{
		delete ModelToRender;
	}
	ModelToRender = Model;
	RenderModel_Mutex.unlock();
	emit RenderNow_SIGNAL();
	IsRenderNow = false;
}
void CMainWindow::RenderModel()
{
	DebugTimer timer(__FUNCTION__);
	if (!ModelToRender)return;
	RenderModel_Mutex.lock();
	ModelViewer->reconstruction = ModelToRender;
	ModelViewer->cameras = ModelToRender->cameras_;
	ModelViewer->points3D = ModelToRender->points3D_;
	ModelViewer->reg_image_ids = ModelToRender->reg_image_ids_;
	ModelViewer->images.clear();
	ModelViewer->images = ModelToRender->GetAllRegImages();
	ModelViewer->ReloadReconstruction();
	RenderModel_Mutex.unlock();
}
void CMainWindow::ClearRenderModel()
{
	ModelViewer->ClearReconstruction();
}

void CMainWindow::CreateReconstructor()
{
	if (!ModelManager)return;
	if (Reconstructor)
	{
		Reconstructor->~CReconstructor();
	}
	Reconstructor = new CReconstructor(options, ModelManager);
	connect(Reconstructor, SIGNAL(ChangeImageColor_SIGNAL(int, int)), this, SLOT(ChangeNewImageColor(int, int)));
	
	Reconstructor->AddCallback(CReconstructor::INITIAL_IMAGE_PAIR_REG_CALLBACK, [this]() 
	{
		RenderModelAction->trigger();
	});
	Reconstructor->AddCallback(CReconstructor::NEXT_IMAGE_REG_CALLBACK, [this]()
	{
		RenderModelAction->trigger();
	});
	Reconstructor->AddCallback(CReconstructor::LAST_IMAGE_REG_CALLBACK, [this]()
	{
		RenderModelAction->trigger();
	});
	Reconstructor->AddCallback(CReconstructor::FINISHED_CALLBACK, [this]()
	{
		RenderModelAction->trigger();
	});

	IsContinueReconstruct = true;
	WaitforReconstructImageNum = 0;

	std::thread ReconstructThread(&CMainWindow::DetectReconstruct, this);
	ReconstructThread.detach();
}
void CMainWindow::AutoSaveModels()
{
	cout << "Model is being saved automatically..." << endl;
	QString DirectoryDir = StdString2QString(GetFileDir(*options->database_path)) + "/AutoSaveModels";
	QDir dir(DirectoryDir);
	if (!dir.exists())
	{
		dir.mkdir(DirectoryDir);
	}
	DirectoryDir = dir.fromNativeSeparators(DirectoryDir);
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	QStringList FolderList = dir.entryList();
	for (int i = 0; i < FolderList.size() - 10 + 1; i++)
	{
		DeleteDir(DirectoryDir + "/" + FolderList[i]);
	}
	QString NewPath = DirectoryDir + "/" + QDateTime::currentDateTime().toString("MM-dd HH-mm-ss");
	dir = QDir(NewPath);
	if (!dir.exists())
	{
		dir.mkdir(NewPath);
	}
	for (int i = 0; i < ModelManager->Size(); i++)
	{
		if (Base::IsQuit)
		{
			return;
		}
		QString SubModelDir = NewPath + "/" + QString::number(i + 1);
		dir = QDir(SubModelDir);
		if (!dir.exists())
		{
			dir.mkdir(SubModelDir);
		}
		string ProjectPath = JoinPaths(QString2StdString(SubModelDir), "project.ini");
		ModelManager->Get(i).WriteBinary(QString2StdString(SubModelDir));
		options->Write(ProjectPath);
	}
	cout << "All models have been saved!" << endl;
}


