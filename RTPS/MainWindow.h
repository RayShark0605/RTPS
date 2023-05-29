#pragma once
#include "Base.h"
#include "Logger.h"
#include "ImageListWidget.h"
#include "UI.h"
#include "Reconstructor.h"
#include "FeatureExtractor.h"
#include "FeatureMatcher.h"
#include "Model.h"
#include "Database.h"

#include <ui/model_viewer_widget.h>


Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(std::vector<std::string>);

class CMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	CMainWindow(QWidget* parent = nullptr);



public slots:
	void ImportModel(int, std::vector<std::string>, std::string, bool);
	void ChangeNewImageColor(int ImageID);
	void NewImage_SLOT(std::string ImagePath);
	void SenderQuit_SLOT();
	void RenderModel();
	void ClearRenderModel();


private:
	colmap::OptionManager* options;

	colmap::ModelViewerWidget* ModelViewer;
	colmap::LogWidget* LogWindow;
	CModelSelectWidget* ModelSelectWidget;
	CModelManager* ModelManager = nullptr;

	CReconstructor* Reconstructor = nullptr;
	CFeatureExtractor* FeatureExtractor = nullptr;
	CFeatureMatcher* FeatureMatcher = nullptr;

	QDockWidget* LogWindowDock;
	QDockWidget* ImgListViewerDock;
	CImageListWidget* ImageListWidget;
	QLabel* StatusLabel;

	QMenu* SettingMenu;
	QMenu* ProjectMenu;
	QMenu* TransmitMenu;
	QMenu* ToolsMenu;
	QToolBar* SettingToolBar;
	QToolBar* ProjectToolBar;
	QToolBar* TransmitToolBar;
	QToolBar* ReconstructToolBar;
	QToolBar* ToolsToolBar;


	QAction* FeatureExtractionSettingAction;     //弹出"特征提取设置"对话框
	CFeatureExtractionSettingWidget* FeatureExtractionSettingWidget;
	void ShowFeatureExtractionSettingWidget();
	
	QAction* FeatureMatchingSettingAction;       //弹出"特征匹配设置"对话框
	CFeatureMatchingSettingWidget* FeatureMatchingSettingWidget;
	void ShowFeatureMatchingSettingWidget();
	
	QAction* ReconstructionSettingAction;        //弹出"重建设置"对话框
	CReconstructionSettingWidget* ReconstructionSettingWidget;
	void ShowReconstructionSettingWidget();

	QAction* TransmitSettingAction;              //弹出"传输设置"对话框
	CReceiverSettingWidget* ReceiverSettingWidget;
	void ShowReceiverSettingWidget();

	QAction* NewProjectAction;                   //弹出"创建新工程"对话框
	CProjectSettingWidget* ProjectSettingWidget;
	void ShowProjectSettingWidget();
	void LoadDirImages_SLOT();
	
	QAction* OpenProjectAction;                  //打开一个工程
	void OpenProject();
	
	QAction* SaveProjectAction;                  //保存当前工程
	void SaveProject();
	
	QAction* SaveAsProjectAction;                //工程另存为
	void SaveAsProject();

	QAction* ImportModelAction;                  //导入模型
	CModelImportWidget* ModelImportWidget;
	void ShowModelImportWidget();
	
	QAction* ExportModelAction;                  //导出模型
	CModelExportWidget* ModelExportWidget;
	void ShowModelExportWidget();

	QAction* StartReceiveAction;                //开始传输
	CReceiver* Receiver = nullptr;
	void StartReceive();
	void ProcessImage(std::string ImagePath);

	QAction* StopReceiveAction;                 //结束传输
	void StopReceive();

	QAction* ShowRenderOptionsAction;            //弹出"渲染设置"对话框
	CRenderOptionsWidget* RenderOptionsWidget;
	void ShowRenderOptionsWidget();

	QAction* ShowMatchMatrixAction;              //弹出"匹配关系矩阵"对话框
	CShowMatchMatrixWidget* ShowMatchMatrixWidget;
	void ShowShowMatchMatrixWidget();
	
	QAction* RenderModelAction;                  //渲染模型

	void UpdateWindowTitle(QString Extras = "");
	void SetupWidgets();
	void SetupActions();
	void CreateMenus();
	void CreateToolbar();
	void CreateStatusbar();
	void CreateExtractor();
	void CreateMatcher();

	QTimer UpdateStatusBar_Timer;
	void UpdateStatusBar();


	//重建相关函数
	bool IsRenderNow = false;
	std::string LastWaitImagePath;
	colmap::Reconstruction* ModelToRender = nullptr;
	bool IsContinueReconstruct = true;
	std::atomic<size_t> WaitforReconstructImageNum = 0;
	std::thread ReconstructionThread;
	void DetectReconstruct();
	void RenderModel_SLOT();
	void GetRenderModel();
	void CreateReconstructor();
	void AutoSaveModels();

signals:
	void RenderNow_SIGNAL();
	void ClearRenderModel_SIGNAL();

};














