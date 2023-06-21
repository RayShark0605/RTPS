#pragma once
#include "Base.h"
#include "Receiver.h"
#include "Model.h"


class CFeatureExtractionSettingWidget :public QWidget
{
	Q_OBJECT
public:
	CFeatureExtractionSettingWidget(QWidget* parent_, colmap::OptionManager* options_);
signals:
	void NewOptions_SIGNAL();
private:
	QWidget* parent;
	colmap::OptionManager* options;

	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void Save();

	QVBoxLayout* Layout;
	QVBoxLayout* GroupBox_Layout;
	QTabWidget* TabWidget;
	QWidget* ExtractionOptionsWidget;
	QGridLayout* ExtractionOptionsWidgetLayout;
	QGroupBox* CameraModel_GroupBox;
	QComboBox* CameraModel_ComboBox;
	QLabel* CameraParamsInfo_Label;
	QCheckBox* IsSingleCamera_CheckBox;
	QCheckBox* IsSingleCameraPerFolder_CheckBox;
	QRadioButton* FromEXIF_RadioButton;
	QRadioButton* FromCustom_RadioButton;
	QLineEdit* CameraParams_Textbox;
	QPushButton* SaveButton;

	QSpinBox* MaxImageSize_SpinBox;
	QSpinBox* MaxNumFeatures_SpinBox;
	QSpinBox* FirstOctave_SpinBox;
	QSpinBox* NumOctaves_SpinBox;
	QSpinBox* OctaveResolution_SpinBox;
	QDoubleSpinBox* PeakThreshold_SpinBox;
	QDoubleSpinBox* EdgeThreshold_SpinBox;
	QCheckBox* EstimateAffineShape_CheckBox;
	QSpinBox* MaxNumOrientations_SpinBox;
	QCheckBox* UpRight_CheckBox;
	QCheckBox* DomainSizePooling_CheckBox;
	QDoubleSpinBox* DspMinScale_SpinBox;
	QDoubleSpinBox* DspMaxScale_SpinBox;
	QSpinBox* DspNumScales_SpinBox;
};

/**************************************************
"特征匹配设置"对话框的TabWidget模板, 后面的几个类都以该类为模板
**************************************************/
class CFeatureMatchingSettingWidget;
class CMatchingSettingTab :public colmap::OptionsWidget
{
	friend class CFeatureMatchingSettingWidget;
public:
	CMatchingSettingTab(QWidget* parent, colmap::OptionManager* options);
protected:
	colmap::OptionManager* options;
	
	QDoubleSpinBox* max_ratio_spinbox;
	QDoubleSpinBox* max_distance_spinbox;
	QDoubleSpinBox* max_error_spinbox;
	QDoubleSpinBox* confidence_spinbox;
	QDoubleSpinBox* min_inlier_ratio_spinbox;

	QSpinBox* max_num_matches_spinbox;
	QSpinBox* max_num_trials_spinbox;
	QSpinBox* min_num_inliers_spinbox;
	QSpinBox* TopN_spinbox;

	QLineEdit* VGG_pth_file_path_lineedit;
	QLineEdit* HDF5_file_path_lineedit;
	QLineEdit* checkpoint_file_path_lineedit;
	QLineEdit* PCA_model_path_lineedit;
	QLineEdit* tree_path_lineedit;
	

	void CreateGeneralOptions(); //生成"所有匹配方法都会用到的设置"
};
/**************************************************
"特征匹配设置"对话框的"暴力匹配"TabWidget
**************************************************/
class CExhaustiveTabWidget :public CMatchingSettingTab
{
public:
	CExhaustiveTabWidget(QWidget* parent, colmap::OptionManager* options);
};
/**************************************************
"特征匹配设置"对话框的"检索匹配"TabWidget
**************************************************/
class CRetrievalTabWidget :public CMatchingSettingTab
{
public:
	CRetrievalTabWidget(QWidget* parent, colmap::OptionManager* options);
};

/**************************************************
"特征匹配设置"对话框
**************************************************/
class CFeatureMatchingSettingWidget :public QWidget
{
	Q_OBJECT
public:
	CFeatureMatchingSettingWidget(QWidget* parent, colmap::OptionManager* options);
signals:
	void NewOptions_SIGNAL();
private:
	QWidget* parent;
	colmap::OptionManager* options;
	QVBoxLayout* vLayout_0;
	QComboBox* MatchingMethod_ComboBox;
	QGroupBox* MatchingMethod_GroupBox;
	QPushButton* SaveButton;
	QGridLayout* grid;
	QTabWidget* TabWidget;
	CExhaustiveTabWidget* ExhaustiveTabWidget;
	CRetrievalTabWidget* RetrievalTabWidget;

	void Save();
	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
public slots:
	void ShowMethodSettings(QString MethodName); //ComboBox改变时, 动态地改变TabWidget中的内容, 动态地改变窗口大小
};

class CReconstructionSettingWidget :public QWidget
{
	Q_OBJECT
public:
	CReconstructionSettingWidget(QWidget* parent, colmap::OptionManager* options);
private:
	QWidget* parent;
	colmap::OptionManager* options;
	QGridLayout* Grid;
	QTabWidget* TabWidget;
	QPushButton* SaveButton;
	
	void Save();
	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

private:
	class GeneralTab :public colmap::OptionsWidget
	{
	public:
		GeneralTab(QWidget* parent, colmap::OptionManager* options);
	};
	class InitializationTab :public colmap::OptionsWidget
	{
	public:
		InitializationTab(QWidget* parent, colmap::OptionManager* options);
	};
	class RegistrationTab :public colmap::OptionsWidget
	{
	public:
		RegistrationTab(QWidget* parent, colmap::OptionManager* options);
	};
	class TriangulationTab :public colmap::OptionsWidget
	{
	public:
		TriangulationTab(QWidget* parent, colmap::OptionManager* options);
	};
	class BundleAdjustmentTab :public colmap::OptionsWidget
	{
	public:
		BundleAdjustmentTab(QWidget* parent, colmap::OptionManager* options);
	};
	class FilterTab :public colmap::OptionsWidget
	{
	public:
		FilterTab(QWidget* parent, colmap::OptionManager* options);
	};

	GeneralTab* GeneralTabWidget;
	InitializationTab* InitializationTabWidget;
	RegistrationTab* RegistrationTabWidget;
	TriangulationTab* TriangulationTabWidget;
	BundleAdjustmentTab* BundleAdjustmentTabWidget;
	FilterTab* FilterTabWidget;
};

class CReceiverSettingWidget :public QWidget
{
	Q_OBJECT
public:
	CReceiverSettingWidget(QWidget* parent);
private:
	std::string Url;
	std::string Port;
	std::string Password;
	QWidget* parent;
	QLineEdit* UrlTextbox;
	QLineEdit* PortTextbox;
	QLineEdit* PasswordTextbox;
	QPushButton* SaveButton;
	QVBoxLayout* Layout;
	QGridLayout* Grid;

	void Save();
	void PreloadSettings();
};

class CProjectSettingWidget :public QWidget
{
	Q_OBJECT
public:
	CProjectSettingWidget(QWidget* parent, colmap::OptionManager* options);
signals:
	void LoadDirImages_SIGNAL();
private:
	QWidget* parent;
	colmap::OptionManager* options;

	QLineEdit* DatabaseTextbox;
	QLineEdit* SaveDirTextbox;
	QPushButton* NewDatabaseButton;
	QPushButton* OpenDatabaseButton;
	QPushButton* SelectSaveDirButton;
	QPushButton* SaveButton;
	QVBoxLayout* Layout;
	QGridLayout* Grid;

	int Status;

	void NewDatabase();
	void OpenDatabase();
	void SelectSaveDir();
	void showEvent(QShowEvent* event) override;
	void Save();
};

class CShowMatchWidget :public QWidget
{
	Q_OBJECT
public:
	CShowMatchWidget(QWidget* parent, size_t CurrentImgID, size_t MatchedImgID, std::string ImageSaveDir, bool IsTwoViewGeometry);
private:
	QWidget* parent;
	size_t CurrentImgID;
	size_t MatchedImgID;
	std::string ImageSaveDir;
	QLabel* ImageLabel;
	QImage Image;              //不带匹配线的两张原始影像的拼接影像
	QImage Image_Matches;      //带匹配线的拼接影像
	QPixmap Pixmap;
	QPushButton* HideMatchesButton;
	QPushButton* SaveButton;
	QPainter* ImagePainter;
	QPainter* Painter;

	QList<QColor> GenerateRandomColor(int Num);
	void Save();
	void HideMatches();
	void closeEvent(QCloseEvent* event);
};
class CMatchesTab :public QWidget
{
	Q_OBJECT
public:
	CMatchesTab(QWidget* parent, std::string ImagePath);
private:
	CShowMatchWidget* ShowMatchWidget;
	QWidget* parent;
	std::string ImagePath;
	size_t MatchedImageNum;
	QTableWidget* Table;
	std::vector<std::pair<size_t, size_t>> MatchInfo;

	void GetMatchInfo();
	void ShowMatches();
};
class CTwoViewGeometriesTab :public QWidget
{
	Q_OBJECT
public:
	CTwoViewGeometriesTab(QWidget* parent, std::string ImagePath);
private:
	CShowMatchWidget* ShowMatchWidget;
	QWidget* parent;
	std::string ImagePath;
	std::vector<std::pair<size_t, size_t>> TwoviewGeometryInfo;
	std::vector<int> Config;
	size_t MatchedImageNum;
	QTableWidget* Table;

	void GetTwoViewGeometriesInfo();
	void ShowMatches();
};
class COverlappingInfoWidget :public QWidget
{
	Q_OBJECT
public:
	COverlappingInfoWidget(QWidget* parent, std::string ImagePath);
	
private:
	QWidget* parent;
	QTabWidget* Tab;
	QGridLayout* grid;
	std::string ImagePath;

	void closeEvent(QCloseEvent* event);
};

class CModelImportWidget :public QWidget
{
	Q_OBJECT
public:
	CModelImportWidget(QWidget* parent);

private:
	QWidget* parent;
	QRadioButton* FromFolder_RadioButton;
	QRadioButton* FromPLY_RadioButton;
	QTabWidget* TabImportSetting_TabWidget;
	QPushButton* LoadButton;

	QWidget* FromFolder_Widget;
	QLineEdit* FolderDir_LineEdit;
	QPushButton* SelectFolderDir_Button;
	QRadioButton* FolderOverwrite_RadioButton;
	QRadioButton* FolderAdd_RadioButton;

	QWidget* FromPLY_Widget;
	QLineEdit* PLYPath_LineEdit;
	QPushButton* SelectPLY_Button;
	QRadioButton* PLYOverwrite_RadioButton;
	QRadioButton* PLYAdd_RadioButton;

	std::vector<std::string> ModelFolders;
	std::string PLYPath;

	void DeleteAllTab(); //清除TabWidget中的所有控件
	void FolderFileDirInfo(QString FolderPath, QStringList& FileList, QStringList& FolderList);
	bool IsValidModelFolder(std::string ModelPath);
	void FromFolderChecked_SLOT();
	void FromPLYChecked_SLOT();
	void SelectFolder_SLOT();
	void SelectPLY_SLOT();
	void Load_SLOT();
	void closeEvent(QCloseEvent* event) override;

signals:
	void ModelImport_SIGNAL(int, std::vector<std::string>, std::string, bool); //提醒主界面"导入模型", 三个参数分别为: 模型类别(0表示"目录",1表示"PLY"), 模型文件夹路径("PLY"时为空字符串), PLY路径("目录"时为空字符串), 是否覆盖当前模型
};

class CModelExportWidget :public QWidget
{
	Q_OBJECT
public:
	CModelExportWidget(QWidget* parent, CModelManager* ModelManager, colmap::OptionManager* options);
private:
	QWidget* parent;
	CModelManager* ModelManager;
	colmap::OptionManager* options;
	std::string SavePath;

	QCheckBox* Folder_Checkbox;
	QCheckBox* Text_Checkbox;
	QCheckBox* VRML_Checkbox;
	QCheckBox* NVM_Checkbox;
	QCheckBox* Bundler_Checkbox;
	QCheckBox* Ply_Checkbox;

	QLineEdit* SavePath_LineEdit;
	QPushButton* SelectSavePath_Button;
	QPushButton* Export_Button;

	QTableWidget* ModelTable;

	void showEvent(QShowEvent* event) override;
	std::vector<size_t> GetAllSelectedRow();
	void ExportFolder(int ModelID);
	void ExportText(int ModelID);
	void ExportVRML(int ModelID);
	void ExportNVM(int ModelID);
	void ExportBundler(int ModelID);
	void ExportPLY(int ModelID);


public slots:
	void SelectSavePath();
	void Export();

};

class CModelSelectWidget :public QComboBox
{
	Q_OBJECT
public:
	CModelSelectWidget(QWidget* parent, CModelManager* ModelManager);
	void Update();
	size_t GetSelectedModelIndex();
	void SelectModel(size_t index);

private:
	CModelManager* ModelManager;
	QTimer UpdateTimer;
	std::mutex ModelSelectWidget_Mutex;
};

class CRenderOptionsWidget :public QWidget
{
	Q_OBJECT
public:
	CRenderOptionsWidget(QWidget* parent, colmap::OptionManager* options, colmap::ModelViewerWidget* ModelViewer);
private:
	QWidget* parent;
	colmap::OptionManager* options;
	colmap::ModelViewerWidget* ModelViewer;
	QTimer UpdateTimer;

	Eigen::Vector4f BackgroundColor;
	Eigen::Vector4f ImagePlaneColor;
	Eigen::Vector4f ImageFrameColor;
	double PointColormapMinQ;
	double PointColormapMaxQ;
	double PointColormapScale;
	colmap::ImageColormapNameFilter ImageColormapFilter;
	colmap::ImageColormapBase* ImageColorMap;

	QDoubleSpinBox* PointSizeSpinBox;
	QDoubleSpinBox* ImageSizeSpinBox;
	QDoubleSpinBox* MaxPointErrorSpinBox;
	QDoubleSpinBox* PointColormapMinQSpinBox;
	QDoubleSpinBox* PointColormapMaxQSpinBox;
	QDoubleSpinBox* PointColormapScaleSpinBox;
	QSpinBox* RefreshRateSpinbox;
	QSpinBox* MinTrackLenSpinBox;

	QComboBox* ProjectionComboBox;
	QComboBox* PointColormapComboBox;
	QComboBox* ImageColormapComboBox;
	QGroupBox* PointColormapGroupBox;
	QGroupBox* ImageColormapGroupBox;

	QPushButton* SelectBackgroundButton;
	QPushButton* SelectImagePlaneButton;
	QPushButton* SelectImageFrameButton;
	QPushButton* AddWordsButton;
	QPushButton* ClearWordsButton;

	QCheckBox* AdaptiveRateCheckBox;
	QCheckBox* ImageConnectCheckBox;

	QFrame* UniformColorFrame;
	QFrame* WithNameFrame;

	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

	void RefreshValues();
	void SetImageConnection();
	void SetBackgroundColor();
	void SelectImagePlaneColor();
	void SelectImageFrameColor();
	void AddWord();
	void ClearWord();
	void SetAdaptiveRate();
};


class CMatchMatrixWidget :public QWidget
{
	Q_OBJECT
public:
	CMatchMatrixWidget(QWidget* parent, colmap::OptionManager* options);
private:
	QWidget* parent;
	colmap::OptionManager* options;
	QTimer UpdateTimer;
	size_t ImagesNum;
	void showEvent(QShowEvent* event) override;
	void paintEvent(QPaintEvent* event)override;
	void closeEvent(QCloseEvent* event) override;
	size_t GetValue(size_t i, size_t j);
	QColor GetColor(size_t value);
};
class CShowMatchMatrixWidget :public QWidget
{
	Q_OBJECT
public:
	CShowMatchMatrixWidget(QWidget* parent, colmap::OptionManager* options);
	
private:
	QWidget* parent;
	colmap::OptionManager* options;
	CMatchMatrixWidget* MatchMatrix;
	QPushButton* SaveButton;

	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void Save();
};





