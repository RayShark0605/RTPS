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


	QAction* FeatureExtractionSettingAction;     //����"������ȡ����"�Ի���
	CFeatureExtractionSettingWidget* FeatureExtractionSettingWidget;
	void ShowFeatureExtractionSettingWidget();
	
	QAction* FeatureMatchingSettingAction;       //����"����ƥ������"�Ի���
	CFeatureMatchingSettingWidget* FeatureMatchingSettingWidget;
	void ShowFeatureMatchingSettingWidget();
	
	QAction* ReconstructionSettingAction;        //����"�ؽ�����"�Ի���
	CReconstructionSettingWidget* ReconstructionSettingWidget;
	void ShowReconstructionSettingWidget();

	QAction* TransmitSettingAction;              //����"��������"�Ի���
	CReceiverSettingWidget* ReceiverSettingWidget;
	void ShowReceiverSettingWidget();

	QAction* NewProjectAction;                   //����"�����¹���"�Ի���
	CProjectSettingWidget* ProjectSettingWidget;
	void ShowProjectSettingWidget();
	void LoadDirImages_SLOT();
	
	QAction* OpenProjectAction;                  //��һ������
	void OpenProject();
	
	QAction* SaveProjectAction;                  //���浱ǰ����
	void SaveProject();
	
	QAction* SaveAsProjectAction;                //��������Ϊ
	void SaveAsProject();

	QAction* ImportModelAction;                  //����ģ��
	CModelImportWidget* ModelImportWidget;
	void ShowModelImportWidget();
	
	
	QAction* ExportModelAction;                  //����ģ��

	QAction* StartReceiveAction;                //��ʼ����
	CReceiver* Receiver = nullptr;
	void StartReceive();
	void ProcessImage(std::string ImagePath);

	QAction* StopReceiveAction;                 //��������
	void StopReceive();

	QAction* ShowRenderOptionsAction;            //����"��Ⱦ����"�Ի���
	QAction* ShowMatchMatrixAction;              //����"ƥ���ϵ����"�Ի���
	QAction* RenderModelAction;                  //��Ⱦģ��

	void UpdateWindowTitle(QString Extras = "");
	void SetupWidgets();
	void SetupActions();
	void CreateMenus();
	void CreateToolbar();
	void CreateStatusbar();
	void CreateExtractor();
	void CreateMatcher();

	//�ؽ���غ���
	bool IsRenderNow = false;
	colmap::Reconstruction* ModelToRender = nullptr;

	void RenderModel_SLOT();
	void GetRenderModel();
	void CreateReconstructor();

signals:
	void RenderNow_SIGNAL();
	void ClearRenderModel_SIGNAL();


	










};













