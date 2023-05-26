#include "UI.h"

using namespace std;
using namespace colmap;

CFeatureExtractionSettingWidget::CFeatureExtractionSettingWidget(QWidget* parent_, OptionManager* options_) :QWidget(parent_)
{
	parent = parent_;
	options = options_;
	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("Feature Extraction Settings"));

	Layout = new QVBoxLayout(this);
	CameraModel_GroupBox = new QGroupBox(tr("Camera model"), this);
	GroupBox_Layout = new QVBoxLayout(CameraModel_GroupBox);

	CameraModel_ComboBox = new QComboBox(this);
	QStringList CameraModels;
	CameraModels << "SIMPLE_PINHOLE" << "PINHOLE" << "SIMPLE_RADIAL" << "SIMPLE_RADIAL_FISHEYE" << "RADIAL" << "RADIAL_FISHEYE" << "OPENCV" << "OPENCV_FISHEYE" << "FULL_OPENCV" << "FOV" << "THIN_PRISM_FISHEYE";
	CameraModel_ComboBox->addItems(CameraModels);
	CameraParamsInfo_Label = new QLabel("Parameters: f, cx, cy, k");
	connect(CameraModel_ComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index)
		{
			if (index == 0)
			{
				CameraParamsInfo_Label->setText("Parameters: f, cx, cy");
			}
			else if (index == 1)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy");
			}
			else if (index == 2 || index == 3)
			{
				CameraParamsInfo_Label->setText("Parameters: f, cx, cy, k");
			}
			else if (index == 4 || index == 5)
			{
				CameraParamsInfo_Label->setText("Parameters: f, cx, cy, k1, k2");
			}
			else if (index == 6)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy, k1, k2, p1, p2");
			}
			else if (index == 7)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy, k1, k2, k3, k4");
			}
			else if (index == 8)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy, k1, k2, p1, p2, k3, k4, k5, k6");
			}
			else if (index == 9)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy, omega");
			}
			else if (index == 10)
			{
				CameraParamsInfo_Label->setText("Parameters: fx, fy, cx, cy, k1, k2, p1, p2, k3, k4, sx1, sy1");
			}
		});
	GroupBox_Layout->addWidget(CameraModel_ComboBox);

	QPalette pal(CameraParamsInfo_Label->palette());
	pal.setColor(QPalette::WindowText, QColor(130, 130, 130));
	CameraParamsInfo_Label->setPalette(pal);
	QFont LabelFont;
	LabelFont.setPointSize(8);
	CameraParamsInfo_Label->setFont(LabelFont);
	GroupBox_Layout->addWidget(CameraParamsInfo_Label);

	IsSingleCamera_CheckBox = new QCheckBox(tr("Shared for all images"), this);
	GroupBox_Layout->addWidget(IsSingleCamera_CheckBox);

	IsSingleCameraPerFolder_CheckBox = new QCheckBox(tr("Shared per sub-folder"), this);
	GroupBox_Layout->addWidget(IsSingleCameraPerFolder_CheckBox);

	FromEXIF_RadioButton = new QRadioButton(tr("Parameters from EXIF"), this);
	FromEXIF_RadioButton->setChecked(true);
	GroupBox_Layout->addWidget(FromEXIF_RadioButton);
	FromCustom_RadioButton = new QRadioButton(tr("Custom parameters"), this);
	FromCustom_RadioButton->setChecked(false);
	GroupBox_Layout->addWidget(FromCustom_RadioButton);

	CameraParams_Textbox = new QLineEdit(this);
	CameraParams_Textbox->setEnabled(false);
	GroupBox_Layout->addWidget(CameraParams_Textbox);
	CameraModel_GroupBox->setLayout(GroupBox_Layout);
	Layout->addWidget(CameraModel_GroupBox);

	TabWidget = new QTabWidget(this);
	ExtractionOptionsWidget = new QWidget(this);
	MaxImageSize_SpinBox = new QSpinBox();
	MaxImageSize_SpinBox->setRange(0, INT_MAX);
	MaxNumFeatures_SpinBox = new QSpinBox();
	MaxNumFeatures_SpinBox->setRange(0, INT_MAX);
	FirstOctave_SpinBox = new QSpinBox();
	FirstOctave_SpinBox->setRange(INT_MIN, INT_MAX);
	NumOctaves_SpinBox = new QSpinBox();
	NumOctaves_SpinBox->setRange(0, INT_MAX);
	OctaveResolution_SpinBox = new QSpinBox();
	OctaveResolution_SpinBox->setRange(0, INT_MAX);
	PeakThreshold_SpinBox = new QDoubleSpinBox();
	PeakThreshold_SpinBox->setRange(0, INT_MAX);
	PeakThreshold_SpinBox->setDecimals(5);
	PeakThreshold_SpinBox->setSingleStep(0.00001);
	EdgeThreshold_SpinBox = new QDoubleSpinBox();
	EdgeThreshold_SpinBox->setRange(0, INT_MAX);
	EdgeThreshold_SpinBox->setDecimals(2);
	EstimateAffineShape_CheckBox = new QCheckBox();
	MaxNumOrientations_SpinBox = new QSpinBox();
	MaxNumOrientations_SpinBox->setRange(0, INT_MAX);
	UpRight_CheckBox = new QCheckBox();
	DomainSizePooling_CheckBox = new QCheckBox();
	DspMinScale_SpinBox = new QDoubleSpinBox();
	DspMinScale_SpinBox->setRange(0, INT_MAX);
	DspMinScale_SpinBox->setDecimals(5);
	DspMinScale_SpinBox->setSingleStep(0.00001);
	DspMaxScale_SpinBox = new QDoubleSpinBox();
	DspMaxScale_SpinBox->setRange(0, INT_MAX);
	DspMaxScale_SpinBox->setDecimals(5);
	DspMaxScale_SpinBox->setSingleStep(0.00001);
	DspNumScales_SpinBox = new QSpinBox();
	DspNumScales_SpinBox->setRange(1, INT_MAX);

	connect(FromEXIF_RadioButton, &QRadioButton::clicked, CameraParams_Textbox, &QLineEdit::setDisabled);
	connect(FromCustom_RadioButton, &QRadioButton::clicked, CameraParams_Textbox, &QLineEdit::setEnabled);


	ExtractionOptionsWidgetLayout = GenerateMultiOptionsLayout({ "max_image_size" ,"max_num_features" ,"first_octaves" ,"num_octaves" ,"octave_resolution" ,
		"peak_threshold" ,"edge_threshold" ,"estimate_affine_shape" ,"max_num_orientations" ,"upright" ,"domain_size_pooling" ,"dsp_min_scale" ,"dsp_max_scale" ,
		"dsp_num_scales" },
		{ MaxImageSize_SpinBox ,MaxNumFeatures_SpinBox ,FirstOctave_SpinBox ,NumOctaves_SpinBox ,OctaveResolution_SpinBox ,PeakThreshold_SpinBox ,
		EdgeThreshold_SpinBox ,EstimateAffineShape_CheckBox ,MaxNumOrientations_SpinBox ,UpRight_CheckBox ,DomainSizePooling_CheckBox ,DspMinScale_SpinBox ,
		DspMaxScale_SpinBox ,DspNumScales_SpinBox });
	ExtractionOptionsWidget->setLayout(ExtractionOptionsWidgetLayout);

	TabWidget->addTab(ExtractionOptionsWidget, tr("Extract"));
	Layout->addWidget(TabWidget);

	SaveButton = new QPushButton(tr("Save"), this);
	connect(SaveButton, &QPushButton::released, this, &CFeatureExtractionSettingWidget::Save);
	Layout->addWidget(SaveButton);

	setFixedWidth(Size(450, true));
	setFixedHeight(Size(650, false));
	setWindowIcon(QIcon(":/media/feature-extraction.png"));
}
void CFeatureExtractionSettingWidget::showEvent(QShowEvent* event)
{
	options->RegReadOptions();
	CameraModel_ComboBox->setCurrentText(StdString2QString(options->image_reader->camera_model));
	IsSingleCamera_CheckBox->setChecked(options->image_reader->single_camera);
	IsSingleCameraPerFolder_CheckBox->setChecked(options->image_reader->single_camera_per_folder);
	MaxImageSize_SpinBox->setValue(options->sift_extraction->max_image_size);
	MaxNumFeatures_SpinBox->setValue(options->sift_extraction->max_num_features);
	FirstOctave_SpinBox->setValue(options->sift_extraction->first_octave);
	NumOctaves_SpinBox->setValue(options->sift_extraction->num_octaves);
	OctaveResolution_SpinBox->setValue(options->sift_extraction->octave_resolution);
	PeakThreshold_SpinBox->setValue(options->sift_extraction->peak_threshold);
	EdgeThreshold_SpinBox->setValue(options->sift_extraction->edge_threshold);
	EstimateAffineShape_CheckBox->setChecked(options->sift_extraction->estimate_affine_shape);
	MaxNumOrientations_SpinBox->setValue(options->sift_extraction->max_num_orientations);
	UpRight_CheckBox->setChecked(options->sift_extraction->upright);
	DomainSizePooling_CheckBox->setChecked(options->sift_extraction->domain_size_pooling);
	DspMinScale_SpinBox->setValue(options->sift_extraction->dsp_min_scale);
	DspMaxScale_SpinBox->setValue(options->sift_extraction->dsp_max_scale);
	DspNumScales_SpinBox->setValue(options->sift_extraction->dsp_num_scales);
}
void CFeatureExtractionSettingWidget::closeEvent(QCloseEvent* event)
{
	hide();
}
void CFeatureExtractionSettingWidget::Save()
{
	if (FromCustom_RadioButton->isChecked())
	{
		string CameraParams_String = QString2StdString(CameraParams_Textbox->text());
		vector<double> CameraParams = CSVToVector<double>(CameraParams_String);
		int CameraCode = CameraModelNameToId(QString2StdString(CameraModel_ComboBox->currentText()));
		if (!CameraModelVerifyParams(CameraCode, CameraParams))
		{
			QMessageBox::critical(this, "", tr("Invalid camera parameters"));
			return;
		}
		else
		{
			options->image_reader->camera_params = CameraParams_String;
		}
	}
	options->image_reader->camera_model = QString2StdString(CameraModel_ComboBox->currentText());
	options->image_reader->single_camera = IsSingleCamera_CheckBox->isChecked();
	options->image_reader->single_camera_per_folder = IsSingleCameraPerFolder_CheckBox->isChecked();
	options->sift_extraction->max_image_size = MaxImageSize_SpinBox->value();
	options->sift_extraction->max_num_features = MaxNumFeatures_SpinBox->value();
	options->sift_extraction->first_octave = FirstOctave_SpinBox->value();
	options->sift_extraction->num_octaves = NumOctaves_SpinBox->value();
	options->sift_extraction->octave_resolution = OctaveResolution_SpinBox->value();
	options->sift_extraction->peak_threshold = PeakThreshold_SpinBox->value();
	options->sift_extraction->edge_threshold = EdgeThreshold_SpinBox->value();
	options->sift_extraction->estimate_affine_shape = EstimateAffineShape_CheckBox->isChecked();
	options->sift_extraction->max_num_orientations = MaxNumOrientations_SpinBox->value();
	options->sift_extraction->upright = UpRight_CheckBox->isChecked();
	options->sift_extraction->domain_size_pooling = DomainSizePooling_CheckBox->isChecked();
	options->sift_extraction->dsp_min_scale = DspMinScale_SpinBox->value();
	options->sift_extraction->dsp_max_scale = DspMaxScale_SpinBox->value();
	options->sift_extraction->dsp_num_scales = DspNumScales_SpinBox->value();
	options->RegWriteOptions();
	emit NewOptions_SIGNAL();
	hide();
}


CMatchingSettingTab::CMatchingSettingTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	this->options = options;
}
void CMatchingSettingTab::CreateGeneralOptions()
{
	AddSpacer();
	AddSection("General Options");
	AddSpacer();
	max_ratio_spinbox = AddOptionDouble( &options->sift_matching->max_ratio, "max_ratio");
	max_distance_spinbox = AddOptionDouble(&options->sift_matching->max_distance, "max_distance");
	max_num_matches_spinbox = AddOptionInt(&options->sift_matching->max_num_matches, "max_num_matches");
	max_error_spinbox = AddOptionDouble(&options->sift_matching->max_error, "max_error");
	confidence_spinbox = AddOptionDouble(&options->sift_matching->confidence, "confidence", 0, 1, 0.00001, 5);
	max_num_trials_spinbox = AddOptionInt(&options->sift_matching->max_num_trials, "max_num_trials");
	min_inlier_ratio_spinbox = AddOptionDouble(&options->sift_matching->min_inlier_ratio, "min_inlier_ratio", 0, 1, 0.001, 3);
	min_num_inliers_spinbox = AddOptionInt(&options->sift_matching->min_num_inliers, "min_num_inliers");
	AddSpacer();
}
CExhaustiveTabWidget::CExhaustiveTabWidget(QWidget* parent, OptionManager* options) : CMatchingSettingTab(parent, options)
{
	CreateGeneralOptions();
}
CRetrievalTabWidget::CRetrievalTabWidget(QWidget* parent, OptionManager* options) :CMatchingSettingTab(parent, options)
{
	VGG_pth_file_path_lineedit = AddOptionFilePath(options->PthPath.get(), "VGG_pth_file_path");
	HDF5_file_path_lineedit = AddOptionFilePath(options->HDF5Path.get(), "HDF5_file_path");
	checkpoint_file_path_lineedit = AddOptionFilePath(options->CheckPointPath.get(), "checkpoint_file_path");
	PCA_model_path_lineedit = AddOptionFilePath(options->PCA_ModelPath.get(), "PCA_model_path");
	tree_path_lineedit = AddOptionFilePath(options->TreeModelPath.get(), "tree_path");
	TopN_spinbox = AddOptionInt(options->RetrievalTopN.get(), "TopN", 1, INT_MAX);
	CreateGeneralOptions();
}
CFeatureMatchingSettingWidget::CFeatureMatchingSettingWidget(QWidget* parent, OptionManager* options) :QWidget(parent)
{
	this->parent = parent;
	this->options = options;
	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("Feature Matching Settings"));

	grid = new QGridLayout(this);
	MatchingMethod_GroupBox = new QGroupBox(tr("Select a matching method:"), this);
	vLayout_0 = new QVBoxLayout;
	MatchingMethod_ComboBox = new QComboBox(this);
	MatchingMethod_ComboBox->addItem(tr("Exhaustive"));
	MatchingMethod_ComboBox->addItem(tr("Retrieval"));
	connect(MatchingMethod_ComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(ShowMethodSettings(QString)));
	vLayout_0->addWidget(MatchingMethod_ComboBox);
	MatchingMethod_GroupBox->setLayout(vLayout_0);
	grid->addWidget(MatchingMethod_GroupBox, 0, 0);
	TabWidget = new QTabWidget(this);
	ExhaustiveTabWidget = new CExhaustiveTabWidget(TabWidget, options);
	RetrievalTabWidget = new CRetrievalTabWidget(TabWidget, options);

	grid->addWidget(TabWidget, 1, 0);

	SaveButton = new QPushButton(tr("Save"), this);
	connect(SaveButton, &QPushButton::released, this, &CFeatureMatchingSettingWidget::Save);
	grid->addWidget(SaveButton, grid->rowCount(), 0);


	this->setWindowIcon(QIcon(":/media/feature-matching.png"));
}
void CFeatureMatchingSettingWidget::Save()
{
	QString CurrentMethod = MatchingMethod_ComboBox->currentText();
	if (CurrentMethod == tr("Exhaustive"))
	{
		*options->FeatureMatchMethod = CFeatureMatchMethod::Exhaustive;
		options->sift_matching->max_ratio = ExhaustiveTabWidget->max_ratio_spinbox->value();
		options->sift_matching->max_distance = ExhaustiveTabWidget->max_distance_spinbox->value();
		options->sift_matching->max_num_matches = ExhaustiveTabWidget->max_num_matches_spinbox->value();
		options->sift_matching->max_error = ExhaustiveTabWidget->max_error_spinbox->value();
		options->sift_matching->confidence = ExhaustiveTabWidget->confidence_spinbox->value();
		options->sift_matching->max_num_trials = ExhaustiveTabWidget->max_num_trials_spinbox->value();
		options->sift_matching->min_inlier_ratio = ExhaustiveTabWidget->min_inlier_ratio_spinbox->value();
		options->sift_matching->min_num_inliers = ExhaustiveTabWidget->min_num_inliers_spinbox->value();
	}
	else if (CurrentMethod == tr("Retrieval"))
	{
		*options->FeatureMatchMethod = CFeatureMatchMethod::Retrieval;
		*options->PthPath = QString2StdString(RetrievalTabWidget->VGG_pth_file_path_lineedit->text());
		*options->HDF5Path = QString2StdString(RetrievalTabWidget->HDF5_file_path_lineedit->text());
		*options->CheckPointPath = QString2StdString(RetrievalTabWidget->checkpoint_file_path_lineedit->text());
		*options->PCA_ModelPath = QString2StdString(RetrievalTabWidget->PCA_model_path_lineedit->text());
		*options->TreeModelPath = QString2StdString(RetrievalTabWidget->tree_path_lineedit->text());
		*options->RetrievalTopN = RetrievalTabWidget->TopN_spinbox->value();
	}
	options->RegWriteOptions();
	emit NewOptions_SIGNAL();
	hide();
}
void CFeatureMatchingSettingWidget::showEvent(QShowEvent* event)
{
	options->RegReadOptions();
	if (*options->FeatureMatchMethod == CFeatureMatchMethod::Exhaustive)
	{
		TabWidget->clear();
		TabWidget->addTab(ExhaustiveTabWidget, tr("Exhaustive"));

		MatchingMethod_ComboBox->setCurrentIndex(0);
		this->setFixedSize(Size(350, true), Size(400, false));
	}
	else if (*options->FeatureMatchMethod == CFeatureMatchMethod::Retrieval)
	{
		TabWidget->clear();
		TabWidget->addTab(RetrievalTabWidget, tr("Retrieval"));
		MatchingMethod_ComboBox->setCurrentIndex(1);
		this->setFixedSize(Size(465, true), Size(700, false));
	}
}
void CFeatureMatchingSettingWidget::closeEvent(QCloseEvent* event)
{
	hide();
}
void CFeatureMatchingSettingWidget::ShowMethodSettings(QString MethodName)
{
	if (MethodName == tr("Exhaustive"))
	{
		TabWidget->clear();
		TabWidget->addTab(ExhaustiveTabWidget, tr("Exhaustive"));
		this->setFixedSize(Size(350, true), Size(400, false));
	}
	else if (MethodName == tr("Retrieval"))
	{
		TabWidget->clear();
		TabWidget->addTab(RetrievalTabWidget, tr("Retrieval"));
		this->setFixedSize(Size(465, true), Size(700, false));
	}
}

CReconstructionSettingWidget::GeneralTab::GeneralTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddOptionInt(&options->mapper->max_model_overlap, "max_model_overlap");
	AddOptionInt(&options->mapper->min_model_size, "min_model_size");
	AddOptionInt(&options->mapper->min_num_matches, "min_num_matches");
	AddOptionBool(&options->mapper->ignore_watermarks, "ignore_watermarks");
}
CReconstructionSettingWidget::InitializationTab::InitializationTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddOptionInt(&options->mapper->init_num_trials, "init_num_trials");
	AddOptionInt(&options->mapper->mapper.init_min_num_inliers, "init_min_num_inliers");
	AddOptionDouble(&options->mapper->mapper.init_max_error, "init_max_error");
	AddOptionDouble(&options->mapper->mapper.init_max_forward_motion, "init_max_forward_motion");
	AddOptionDouble(&options->mapper->mapper.init_min_tri_angle, "init_min_tri_angle [deg]");
	AddOptionInt(&options->mapper->mapper.init_max_reg_trials, "init_max_reg_trials", 1);
}
CReconstructionSettingWidget::RegistrationTab::RegistrationTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddOptionDouble(&options->mapper->mapper.abs_pose_max_error, "abs_pose_max_error [px]");
	AddOptionInt(&options->mapper->mapper.abs_pose_min_num_inliers, "abs_pose_min_num_inliers");
	AddOptionDouble(&options->mapper->mapper.abs_pose_min_inlier_ratio, "abs_pose_min_inlier_ratio");
	AddOptionInt(&options->mapper->mapper.max_reg_trials, "max_reg_trials", 1);
}
CReconstructionSettingWidget::TriangulationTab::TriangulationTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddOptionInt(&options->mapper->triangulation.max_transitivity, "max_transitivity");
	AddOptionDouble(&options->mapper->triangulation.create_max_angle_error, "create_max_angle_error [deg]");
	AddOptionDouble(&options->mapper->triangulation.continue_max_angle_error, "continue_max_angle_error [deg]");
	AddOptionDouble(&options->mapper->triangulation.merge_max_reproj_error, "merge_max_reproj_error [px]");
	AddOptionDouble(&options->mapper->triangulation.re_max_angle_error, "re_max_angle_error [deg]");
	AddOptionDouble(&options->mapper->triangulation.re_min_ratio, "re_min_ratio");
	AddOptionInt(&options->mapper->triangulation.re_max_trials, "re_max_trials");
	AddOptionDouble(&options->mapper->triangulation.complete_max_reproj_error, "complete_max_reproj_error [px]");
	AddOptionInt(&options->mapper->triangulation.complete_max_transitivity, "complete_max_transitivity");
	AddOptionDouble(&options->mapper->triangulation.min_angle, "min_angle [deg]", 0, 180);
	AddOptionBool(&options->mapper->triangulation.ignore_two_view_tracks, "ignore_two_view_tracks");
}
CReconstructionSettingWidget::BundleAdjustmentTab::BundleAdjustmentTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddSection("Camera parameters");
	AddOptionBool(&options->mapper->ba_refine_focal_length, "refine_focal_length");
	AddOptionBool(&options->mapper->ba_refine_principal_point, "refine_principal_point");
	AddOptionBool(&options->mapper->ba_refine_extra_params, "refine_extra_params");

	AddSpacer();

	AddSection("Local Bundle Adjustment");
	AddOptionInt(&options->mapper->ba_local_num_images, "num_images");
	AddOptionInt(&options->mapper->ba_local_max_num_iterations, "max_num_iterations");
	AddOptionInt(&options->mapper->ba_local_max_refinements, "max_refinements", 1);
	AddOptionDouble(&options->mapper->ba_local_max_refinement_change, "max_refinement_change", 0, 1, 1e-6, 6);

	AddSpacer();

	AddSection("Global Bundle Adjustment");
	AddOptionDouble(&options->mapper->ba_global_images_ratio, "images_ratio");
	AddOptionInt(&options->mapper->ba_global_images_freq, "images_freq");
	AddOptionDouble(&options->mapper->ba_global_points_ratio, "points_ratio");
	AddOptionInt(&options->mapper->ba_global_points_freq, "points_freq");
	AddOptionInt(&options->mapper->ba_global_max_num_iterations, "max_num_iterations");
	AddOptionInt(&options->mapper->ba_global_max_refinements, "max_refinements", 1);
	AddOptionDouble(&options->mapper->ba_global_max_refinement_change, "max_refinement_change", 0, 1, 1e-6, 6);
}
CReconstructionSettingWidget::FilterTab::FilterTab(QWidget* parent, OptionManager* options) : OptionsWidget(parent)
{
	AddOptionDouble(&options->mapper->min_focal_length_ratio, "min_focal_length_ratio");
	AddOptionDouble(&options->mapper->max_focal_length_ratio, "max_focal_length_ratio");
	AddOptionDouble(&options->mapper->max_extra_param, "max_extra_param");
	AddOptionDouble(&options->mapper->mapper.filter_max_reproj_error, "filter_max_reproj_error [px]");
	AddOptionDouble(&options->mapper->mapper.filter_min_tri_angle, "filter_min_tri_angle [deg]");
}
CReconstructionSettingWidget::CReconstructionSettingWidget(QWidget* parent, OptionManager* options)
{
	this->parent = parent;
	this->options = options;

	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("Reconstruction Settings"));

	Grid = new QGridLayout(this);
	TabWidget = new QTabWidget(this);

	GeneralTabWidget = new GeneralTab(this, this->options);
	InitializationTabWidget = new InitializationTab(this, this->options);
	RegistrationTabWidget = new RegistrationTab(this, this->options);
	TriangulationTabWidget = new TriangulationTab(this, this->options);
	BundleAdjustmentTabWidget = new BundleAdjustmentTab(this, this->options);
	FilterTabWidget = new FilterTab(this, this->options);


	TabWidget->addTab(GeneralTabWidget, tr("General"));
	TabWidget->addTab(InitializationTabWidget, tr("Initialization"));
	TabWidget->addTab(RegistrationTabWidget, tr("Registration"));
	TabWidget->addTab(TriangulationTabWidget, tr("Triangulation"));
	TabWidget->addTab(BundleAdjustmentTabWidget, tr("BundleAdjustment"));
	TabWidget->addTab(FilterTabWidget, tr("Filter"));
	Grid->addWidget(TabWidget, 0, 0);

	SaveButton = new QPushButton(tr("Save"), this);
	connect(SaveButton, &QPushButton::released, this, &CReconstructionSettingWidget::Save);
	Grid->addWidget(SaveButton, Grid->rowCount(), 0);
	this->setFixedSize(Size(600, true), Size(500, false));
	this->setWindowIcon(QIcon(":/media/bundle-adjustment.png"));
}
void CReconstructionSettingWidget::Save()
{
	GeneralTabWidget->WriteOptions();
	InitializationTabWidget->WriteOptions();
	RegistrationTabWidget->WriteOptions();
	TriangulationTabWidget->WriteOptions();
	BundleAdjustmentTabWidget->WriteOptions();
	FilterTabWidget->WriteOptions();
	options->RegWriteOptions();
	hide();
}
void CReconstructionSettingWidget::showEvent(QShowEvent* event)
{
	options->RegReadOptions();
	GeneralTabWidget->ReadOptions();
	InitializationTabWidget->ReadOptions();
	RegistrationTabWidget->ReadOptions();
	TriangulationTabWidget->ReadOptions();
	BundleAdjustmentTabWidget->ReadOptions();
	FilterTabWidget->ReadOptions();
}
void CReconstructionSettingWidget::closeEvent(QCloseEvent* event)
{
	hide();
}

CReceiverSettingWidget::CReceiverSettingWidget(QWidget* parent) :QWidget(parent)
{
	this->parent = parent;
	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("CamFi image receiver settings"));

	UrlTextbox = new QLineEdit();
	QRegExp regExp("^[1-9]\\d*$");
	PortTextbox = new QLineEdit();
	PortTextbox->setValidator(new QRegExpValidator(regExp, this)); //通过正则表达式限制输入框只能输入正整数
	PasswordTextbox = new QLineEdit();
	PasswordTextbox->setEchoMode(QLineEdit::Password); //输入模式为"密码"

	SaveButton = new QPushButton(tr("Save"), this);
	connect(SaveButton, &QPushButton::released, this, &CReceiverSettingWidget::Save);

	Layout = new QVBoxLayout(this);
	Grid = new QGridLayout(this);
	Grid->addWidget(new QLabel(tr("Url:"), this), 0, 0, Qt::AlignRight);
	Grid->addWidget(UrlTextbox, 0, 1);
	Grid->addWidget(new QLabel(tr("Port:"), this), 1, 0, Qt::AlignRight);
	Grid->addWidget(PortTextbox, 1, 1);
	Grid->addWidget(new QLabel(tr("Password:"), this), 2, 0, Qt::AlignRight);
	Grid->addWidget(PasswordTextbox, 2, 1);
	Layout->addLayout(Grid);
	Layout->addWidget(SaveButton);


	setFixedSize(Size(350, true), Size(120, false));
	PreloadSettings(); //加载注册表中之前设置好的"传输设置"(避免每次都要更改设置)
	setWindowIcon(QIcon(":/media/automatic-reconstruction.png"));
}
void CReceiverSettingWidget::PreloadSettings()
{
	QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\RTPS\\CamFiTransmit", QSettings::NativeFormat);
	if (reg.contains("Url"))
	{
		QString Url = reg.value("Url", QVariant()).toString();
		this->Url = Url.toLocal8Bit().data();
		UrlTextbox->setText(Url);
	}
	else
	{
		UrlTextbox->setText("http://192.168.9.67");
	}
	if (reg.contains("Port"))
	{
		QString Port = reg.value("Port", QVariant()).toString();
		this->Port = Port.toLocal8Bit().data();
		PortTextbox->setText(Port);
	}
	else
	{
		PortTextbox->setText("8080");
	}
	if (reg.contains("Password"))
	{
		QString Password = reg.value("Password", QVariant()).toString();
		if (Password != "")
		{
			//注册表中的密码非空, 则解密后写入输入框
			Password = CReceiver::xorData(Password);
			this->Password = Password.toLocal8Bit().data();
			PasswordTextbox->setText(Password);
		}
	}
}
void CReceiverSettingWidget::Save()
{
	Url = UrlTextbox->text().toLocal8Bit().data();
	Port = PortTextbox->text().toLocal8Bit().data();
	Password = PasswordTextbox->text().toLocal8Bit().data();

	QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\RTPS\\CamFiTransmit", QSettings::NativeFormat);
	reg.setValue("Url", UrlTextbox->text());
	reg.setValue("Port", PortTextbox->text());
	if (Password == "")reg.setValue("Password", "");
	else reg.setValue("Password", CReceiver::xorData(PasswordTextbox->text())); //如果有密码, 则加密后写入注册表
	hide();
}

CProjectSettingWidget::CProjectSettingWidget(QWidget* parent, OptionManager* options) :QWidget(parent)
{
	this->parent = parent;
	this->options = options;

	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle(tr("Project settings"));

	NewDatabaseButton = new QPushButton(tr("New"), this);
	connect(NewDatabaseButton, &QPushButton::released, this, &CProjectSettingWidget::NewDatabase);

	OpenDatabaseButton = new QPushButton(tr("Open"), this);
	connect(OpenDatabaseButton, &QPushButton::released, this, &CProjectSettingWidget::OpenDatabase);

	SelectSaveDirButton = new QPushButton(tr("Select"), this);
	connect(SelectSaveDirButton, &QPushButton::released, this, &CProjectSettingWidget::SelectSaveDir);

	SaveButton = new QPushButton(tr("Save"), this);
	connect(SaveButton, &QPushButton::released, this, &CProjectSettingWidget::Save);

	DatabaseTextbox = new QLineEdit();
	SaveDirTextbox = new QLineEdit();

	Layout = new QVBoxLayout(this);
	Grid = new QGridLayout(this);
	Grid->addWidget(new QLabel(tr("Database Path:"), this), 0, 0);
	Grid->addWidget(DatabaseTextbox, 0, 1);
	Grid->addWidget(NewDatabaseButton, 0, 2);
	Grid->addWidget(OpenDatabaseButton, 0, 3);
	Grid->addWidget(new QLabel(tr("Image Save Path:"), this), 1, 0);
	Grid->addWidget(SaveDirTextbox, 1, 1);
	Grid->addWidget(SelectSaveDirButton, 1, 2);
	Layout->addLayout(Grid);
	Layout->addWidget(SaveButton);

	setFixedHeight(Size(100, false));
	this->setWindowIcon(QIcon(":/media/project-new.png"));
}
void CProjectSettingWidget::NewDatabase()
{
	QString DatabasePath = QFileDialog::getSaveFileName(this, tr("Select database save path"), QString(), tr("SQLite3 database (*.db)"));
	if (DatabasePath != "")
	{
		if (!HasFileExtension(DatabasePath.toUtf8().constData(), ".db"))
		{
			DatabasePath += ".db";
		}
		DatabaseTextbox->setText(DatabasePath);
		DatabaseTextbox->setReadOnly(true);
		Status = 0;
	}
}
void CProjectSettingWidget::OpenDatabase()
{
	QString DatabasePath = QFileDialog::getOpenFileName(this, tr("Open a database"), QString(), tr("SQLite3 database (*.db)"));
	if (DatabasePath != "")
	{
		DatabaseTextbox->setText(DatabasePath);
		DatabaseTextbox->setReadOnly(true);
		Status = 1;
	}
}
void CProjectSettingWidget::SelectSaveDir()
{
	QString ImgSaveDir = QFileDialog::getExistingDirectory(this, tr("Select image save path..."));
	if (ImgSaveDir != "")
	{
		SaveDirTextbox->setText(ImgSaveDir);
		SaveDirTextbox->setReadOnly(true);
	}
}
void CProjectSettingWidget::showEvent(QShowEvent* event)
{
	Status = -1;
}
void CProjectSettingWidget::Save()
{
	if (Status == -1)
	{
		hide();
		return;
	}
	string ImgSaveDir_Temp = QString2StdString(SaveDirTextbox->text());
	string DatabasePath_Temp = QString2StdString(DatabaseTextbox->text());
	if (ExistsDir(ImgSaveDir_Temp) && ExistsDir(GetParentDir(DatabasePath_Temp)))
	{
		*options->image_path = ImgSaveDir_Temp;
		*options->database_path = DatabasePath_Temp;
		options->image_reader->database_path = DatabasePath_Temp;
		if (Status == 0)
		{
			if (ExistsFile(*options->database_path))
			{
				QFile file(StdString2QString(*options->database_path));
				file.remove();
			}
			QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
			CDatabase::NewDatabase(*options->database_path, db);
			ReleaseDatabaseConnect(db);
		}
		else
		{
			QSqlDatabase db = CreateDatabaseConnect(*options->database_path);
			CDatabase::OpenDatabase(*options->database_path, db);
			ReleaseDatabaseConnect(db);
			emit LoadDirImages_SIGNAL();
		}
		hide();
	}
	else
	{
		QMessageBox::critical(this, "", tr("Invalid Paths!"));
	}
}

CShowMatchWidget::CShowMatchWidget(QWidget* parent, size_t CurrentImgID, size_t MatchedImgID, string ImageSaveDir, bool IsTwoViewGeometry) :QWidget(parent)
{
	this->parent = parent;
	this->CurrentImgID = CurrentImgID;
	this->MatchedImgID = MatchedImgID;
	this->ImageSaveDir = ImageSaveDir;
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	string CurrentImgPath = ImageSaveDir + "/" + CDatabase::GetImageName(CurrentImgID, db);
	string MatchedImgPath = ImageSaveDir + "/" + CDatabase::GetImageName(MatchedImgID, db);

	QImage CurrentImg(StdString2QString(CurrentImgPath));
	QImage MatchedImg(StdString2QString(MatchedImgPath));

	//横向拼接两张原始影像
	Image = QImage(CurrentImg.width() + MatchedImg.width(), max(CurrentImg.height(), MatchedImg.height()), QImage::Format_RGB888);
	ImagePainter = new QPainter(&Image);
	ImagePainter->drawPixmap(0, 0, CurrentImg.width(), CurrentImg.height(), QPixmap::fromImage(CurrentImg));
	ImagePainter->drawPixmap(CurrentImg.width(), 0, MatchedImg.width(), MatchedImg.height(), QPixmap::fromImage(MatchedImg));
	Image_Matches = Image; //拼接后的影像拷贝到Image_Matches

	FeatureKeypoints CurrentImg_Keypoints;
	FeatureKeypoints MatchedImg_Keypoints;
	CDatabase::GetKeypoints(CurrentImgID, CurrentImg_Keypoints, db);
	CDatabase::GetKeypoints(MatchedImgID, MatchedImg_Keypoints, db);

	Painter = new QPainter(&Image_Matches);
	Painter->setRenderHint(QPainter::Antialiasing, true);
	int PenSize = 2, CircleSize = 3;
	if (Image_Matches.width() < 1000 || Image_Matches.height() < 1000) //影像太小, 特征点也画小一些
	{
		PenSize = 1;
		CircleSize = 1;
	}
	Painter->setPen(QPen(Qt::red, PenSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	for (FeatureKeypoint& Keypoint : CurrentImg_Keypoints)
	{
		Painter->drawEllipse(QPoint(Keypoint.x, Keypoint.y), CircleSize, CircleSize);
	}
	for (FeatureKeypoint& Keypoint : MatchedImg_Keypoints)
	{
		Painter->drawEllipse(QPoint(Keypoint.x + CurrentImg.width(), Keypoint.y), CircleSize, CircleSize);
	}
	Painter->end();
	Painter->~QPainter();

	FeatureMatches Matches;
	if (IsTwoViewGeometry)
	{
		TwoViewGeometry TwoViewGeometries;
		CDatabase::GetTwoViewGeometries(CurrentImgID, MatchedImgID, TwoViewGeometries, db);
		Matches = TwoViewGeometries.inlier_matches;

		setWindowTitle(tr("Two-view Matches between [ ") + StdString2QString(CDatabase::GetImageName(CurrentImgID, db)) + tr(" ] and [ ") + StdString2QString(CDatabase::GetImageName(MatchedImgID, db)) + tr(" ]"));
	}
	else
	{
		CDatabase::GetMatches(CurrentImgID, MatchedImgID, Matches, db);

		setWindowTitle(tr("Matches between [ ") + StdString2QString(CDatabase::GetImageName(CurrentImgID, db)) + tr(" ] and [ ") + StdString2QString(CDatabase::GetImageName(MatchedImgID, db)) + tr(" ]"));
	}
	QList<QColor> Colors = GenerateRandomColor(Matches.size());

	//画匹配线
	//为了提高QPainter的绘制速度, 所以先把Image_Matches(QImage)转成Pixmap(QPixmap), 在Pixmap上画
	Pixmap = QPixmap::fromImage(Image_Matches);
	QPainter DrawLinePainter(&Pixmap);
	for (size_t i = 0; i < Matches.size(); i++)
	{
		DrawLinePainter.setPen(QPen(Colors[i], PenSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		FeatureKeypoint Point1 = CurrentImg_Keypoints[Matches[i].point2D_idx1];
		FeatureKeypoint Point2 = MatchedImg_Keypoints[Matches[i].point2D_idx2];
		DrawLinePainter.drawEllipse(QPoint(Point1.x, Point1.y), CircleSize, CircleSize);
		DrawLinePainter.drawEllipse(QPoint(Point2.x + CurrentImg.width(), Point2.y), CircleSize, CircleSize);
		DrawLinePainter.drawLine(QPoint(Point1.x, Point1.y), QPoint(Point2.x + CurrentImg.width(), Point2.y));
	}
	DrawLinePainter.end();
	Image_Matches = Pixmap.toImage();

	ImageLabel = new QLabel();
	ImageLabel->setScaledContents(true);
	ImageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	Pixmap.scaled(this->width(), this->height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
	ImageLabel->setPixmap(Pixmap);

	SaveButton = new QPushButton(tr("Save"), this);
	HideMatchesButton = new QPushButton(tr("Hide matches"), this);
	connect(HideMatchesButton, &QPushButton::released, this, &CShowMatchWidget::HideMatches);
	connect(SaveButton, &QPushButton::released, this, &CShowMatchWidget::Save);

	ReleaseDatabaseConnect(db);

	QVBoxLayout* Layout = new QVBoxLayout();
	Layout->addWidget(ImageLabel);
	QHBoxLayout* SubLayout = new QHBoxLayout();
	QHBoxLayout* ButtonLayout = new QHBoxLayout();
	ButtonLayout->addSpacerItem(new QSpacerItem(Size(20, true), Size(20, false), QSizePolicy::Expanding));
	ButtonLayout->addWidget(HideMatchesButton);
	ButtonLayout->addWidget(SaveButton);
	SubLayout->addLayout(ButtonLayout);
	Layout->addLayout(SubLayout);
	this->setLayout(Layout);

	resize(Size(900, true), Size(700, false));
	setWindowIcon(QIcon(":/media/undistort.png"));
}
QList<QColor> CShowMatchWidget::GenerateRandomColor(int Num)
{
	QList<QColor> ColorList;
	int ValidNum = min(Num, 280); //最大只能生成280个不重复且差异尽可能大的颜色
	int Length = 280 / ValidNum;
	int h = rand() % Length + 10;
	for (int i = 0; i < ValidNum; i++)
	{
		int s = rand() % 200 + 55;
		int v = rand() % 155 + 100;
		QColor randColor = QColor::fromHsv(h, s, v);
		h = h + Length;
		ColorList << randColor;
	}
	while (ColorList.size() < Num) //如果需要的颜色超过280个, 那么把前面生成的差异尽可能大的280个颜色加进来, 后面不足的颜色就开始摆烂, 随机生成
	{
		ColorList << QColor(rand() % 256, rand() % 256, rand() % 256);
	}
	return ColorList;
}
void CShowMatchWidget::Save()
{
	QString filter("PNG (*.png)");
	const QString SavePath = QFileDialog::getSaveFileName(this, tr("Select destination..."), "", "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)", &filter).toUtf8().constData();
	if (SavePath == "")
	{
		return;
	}
	if (HideMatchesButton->text() == tr("Hide matches")) //当前显示的影像是带匹配线的
	{
		Image_Matches.save(SavePath, nullptr, 100);
	}
	else //当前显示的影像是不带匹配线的
	{
		Image.save(SavePath, nullptr, 100);
	}
}
void CShowMatchWidget::HideMatches()
{
	if (HideMatchesButton->text() == tr("Hide matches")) //当前显示的影像是带匹配线的, 则显示不带匹配线的
	{
		Pixmap = QPixmap::fromImage(Image).scaled(this->width(), this->height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel->setScaledContents(true);
		ImageLabel->setPixmap(Pixmap);
		HideMatchesButton->setText(tr("Show matches"));
	}
	else //当前显示的影像是不带匹配线的, 则显示带匹配线的
	{
		Pixmap = QPixmap::fromImage(Image_Matches).scaled(this->width(), this->height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel->setScaledContents(true);
		ImageLabel->setPixmap(Pixmap);
		HideMatchesButton->setText(tr("Hide matches"));
	}
}
void CShowMatchWidget::closeEvent(QCloseEvent* event)
{
	Image.~QImage();
	Image_Matches.~QImage();
	deleteLater();
}


CMatchesTab::CMatchesTab(QWidget* parent, string ImagePath)
{
	this->parent = parent;
	this->ImagePath = ImagePath;

	GetMatchInfo();
	MatchedImageNum = MatchInfo.size();

	QGridLayout* Layout = new QGridLayout(this);
	QLabel* MatchedImgNum_Label = new QLabel(tr("Matched images: ") + QString::number(MatchedImageNum), this);
	Layout->addWidget(MatchedImgNum_Label, 0, 0);
	QPushButton* ShowMatchedButton = new QPushButton(tr("Show matches"), this);
	connect(ShowMatchedButton, &QPushButton::released, this, &CMatchesTab::ShowMatches);
	Layout->addWidget(ShowMatchedButton, 0, 1, Qt::AlignRight);

	QStringList TableHeader({ tr("Image ID"),tr("Image Name"),tr("Matches Num") });
	Table = new QTableWidget(this);
	Table->setSortingEnabled(false); //为了使表格可以自动/手动排序, 在表格完成之前需要先关闭排序功能, 全部填完之后再打开
	Table->setColumnCount(3);
	Table->setHorizontalHeaderLabels(TableHeader);
	Table->setShowGrid(true);
	Table->setSelectionBehavior(QAbstractItemView::SelectRows);
	Table->setSelectionMode(QAbstractItemView::SingleSelection);
	Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	Table->horizontalHeader()->setStretchLastSection(true);
	Table->verticalHeader()->setVisible(false);
	Table->verticalHeader()->setDefaultSectionSize(20);
	Table->clearContents();
	Table->setRowCount(MatchedImageNum);

	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	for (size_t i = 0; i < MatchedImageNum; i++)
	{
		size_t MatchedImageID = MatchInfo[i].first;
		size_t MatchesNum = MatchInfo[i].second;

		QTableWidgetItem* item1 = new QTableWidgetItem;
		item1->setData(Qt::DisplayRole, MatchedImageID);
		Table->setItem(i, 0, item1);

		QTableWidgetItem* item2 = new QTableWidgetItem;
		item2->setData(Qt::DisplayRole, StdString2QString(CDatabase::GetImageName(MatchedImageID, db)));
		Table->setItem(i, 1, item2);

		QTableWidgetItem* item3 = new QTableWidgetItem;
		item3->setData(Qt::DisplayRole, MatchesNum);
		Table->setItem(i, 2, item3);
	}
	ReleaseDatabaseConnect(db);
	Table->setSortingEnabled(true);
	Table->setCurrentCell(0, 0);
	Layout->addWidget(Table, 1, 0, 1, 2);
}
void CMatchesTab::GetMatchInfo()
{
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	MatchInfo.resize(0);
	string ImageName = GetFileName(ImagePath);
	size_t CurrentImgID = CDatabase::GetImageID(ImageName, db);
	size_t ImagesNum = CDatabase::GetImagesNum(db);
	MatchInfo.reserve(ImagesNum);
	for (size_t i = 0; i < ImagesNum; i++)
	{
		if (i == CurrentImgID)continue;
		size_t MatchesNum = CDatabase::GetMatchesNum(i, CurrentImgID, db);
		if (MatchesNum == 0)continue;
		MatchInfo.emplace_back(i, MatchesNum);
	}
	ReleaseDatabaseConnect(db);
}
void CMatchesTab::ShowMatches()
{
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	QList<QTableWidgetItem*> items = Table->selectedItems();
	if (items.count() == 0)
	{
		QMessageBox::critical(this, "Error", tr("No image pair selected!"));
		return;
	}
	size_t MatchedImgID = Table->item(Table->row(items[0]), 0)->text().toInt();
	size_t CurrentImgID = CDatabase::GetImageID(GetFileName(ImagePath), db);
	ShowMatchWidget = new CShowMatchWidget(this, CurrentImgID, MatchedImgID, GetParentDir(ImagePath), false);
	ShowMatchWidget->show();
	ReleaseDatabaseConnect(db);
}

CTwoViewGeometriesTab::CTwoViewGeometriesTab(QWidget* parent, string ImagePath)
{
	this->parent = parent;
	this->ImagePath = ImagePath;
	GetTwoViewGeometriesInfo();
	MatchedImageNum = TwoviewGeometryInfo.size();

	QGridLayout* Layout = new QGridLayout(this);
	QLabel* MatchedImgNum_Label = new QLabel(tr("Matched images: ") + QString::number(MatchedImageNum), this);
	Layout->addWidget(MatchedImgNum_Label, 0, 0);
	QPushButton* ShowMatchedButton = new QPushButton(tr("Show matches"), this);
	connect(ShowMatchedButton, &QPushButton::released, this, &CTwoViewGeometriesTab::ShowMatches);
	Layout->addWidget(ShowMatchedButton, 0, 1, Qt::AlignRight);

	QStringList TableHeader({ tr("Image ID"),tr("Image Name"),tr("Two-view Matches Num"),tr("Config") });
	Table = new QTableWidget(this);
	Table->setSortingEnabled(false); //为了使表格可以自动/手动排序, 在表格完成之前需要先关闭排序功能, 全部填完之后再打开
	Table->setColumnCount(4);
	Table->setHorizontalHeaderLabels(TableHeader);
	Table->setShowGrid(true);
	Table->setSelectionBehavior(QAbstractItemView::SelectRows);
	Table->setSelectionMode(QAbstractItemView::SingleSelection);
	Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	Table->horizontalHeader()->setStretchLastSection(true);
	Table->verticalHeader()->setVisible(false);
	Table->verticalHeader()->setDefaultSectionSize(20);
	Table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	Table->clearContents();
	Table->setRowCount(MatchedImageNum);
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	for (size_t i = 0; i < MatchedImageNum; i++)
	{
		size_t MatchedImgID = TwoviewGeometryInfo[i].first;
		size_t MatchesNum = TwoviewGeometryInfo[i].second;

		QTableWidgetItem* item1 = new QTableWidgetItem;
		item1->setData(Qt::DisplayRole, MatchedImgID);
		Table->setItem(i, 0, item1);

		QTableWidgetItem* item2 = new QTableWidgetItem;
		
		item2->setData(Qt::DisplayRole, StdString2QString(CDatabase::GetImageName(MatchedImgID, db)));
		Table->setItem(i, 1, item2);

		QTableWidgetItem* item3 = new QTableWidgetItem;
		item3->setData(Qt::DisplayRole, MatchesNum);
		Table->setItem(i, 2, item3);

		QTableWidgetItem* item4 = new QTableWidgetItem;
		item4->setData(Qt::DisplayRole, QString::number(Config[i]));
		Table->setItem(i, 3, item4);
	}
	Table->setSortingEnabled(true);
	Table->setCurrentCell(0, 0);
	Layout->addWidget(Table, 1, 0, 1, 2);
	ReleaseDatabaseConnect(db);
}
void CTwoViewGeometriesTab::GetTwoViewGeometriesInfo()
{
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	TwoviewGeometryInfo.resize(0);
	Config.resize(0);
	size_t CurrentImgID = CDatabase::GetImageID(GetFileName(ImagePath), db);
	size_t ImagesNum = CDatabase::GetImagesNum(db);
	TwoviewGeometryInfo.reserve(ImagesNum);
	Config.reserve(ImagesNum);
	for (size_t i = 0; i < ImagesNum; i++)
	{
		if (i == CurrentImgID)continue;
		TwoViewGeometry Geometry;
		CDatabase::GetTwoViewGeometries(i, CurrentImgID, Geometry, db);
		if (Geometry.inlier_matches.empty())continue;
		TwoviewGeometryInfo.emplace_back(i, Geometry.inlier_matches.size());
		Config.push_back(Geometry.config);
	}
	ReleaseDatabaseConnect(db);
}
void CTwoViewGeometriesTab::ShowMatches()
{
	QList<QTableWidgetItem*> items = Table->selectedItems();
	if (items.count() == 0)
	{
		QMessageBox::critical(this, "Error", tr("No image pair selected!"));
		return;
	}
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	int MatchedImgID = Table->item(Table->row(items[0]), 0)->text().toInt();
	int CurrentImgID = CDatabase::GetImageID(GetFileName(ImagePath), db);
	ShowMatchWidget = new CShowMatchWidget(nullptr, CurrentImgID, MatchedImgID, GetParentDir(ImagePath), true);
	ShowMatchWidget->show();
	ReleaseDatabaseConnect(db);
}

COverlappingInfoWidget::COverlappingInfoWidget(QWidget* parent, string ImagePath) :QWidget(parent)
{
	this->parent = parent;
	this->ImagePath = ImagePath;
	setWindowFlags(Qt::Window);
	grid = new QGridLayout(this);
	Tab = new QTabWidget(this);
	Tab->addTab(new CMatchesTab(this, ImagePath), tr("Matches"));
	Tab->addTab(new CTwoViewGeometriesTab(this, ImagePath), tr("Two-view Geometries"));
	grid->addWidget(Tab, 0, 0);
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	size_t ImageID = CDatabase::GetImageID(GetFileName(ImagePath), db);
	ReleaseDatabaseConnect(db);
	setWindowTitle(tr("Matches for image [ ") + StdString2QString(GetFileName(ImagePath)) + tr(" ] ID=") + QString::number(ImageID));
	setMinimumWidth(Size(600, true));
	setWindowIcon(QIcon(":/media/database-management.png"));
}
void COverlappingInfoWidget::closeEvent(QCloseEvent* event)
{
	grid->~QGridLayout();
	deleteLater();
}


CModelImportWidget::CModelImportWidget(QWidget* parent) :QWidget(parent)
{
	this->parent = parent;
	setWindowFlags(Qt::Window);
	setWindowModality(Qt::ApplicationModal);
	this->setWindowIcon(QIcon(":/media/import.png"));
	setWindowTitle(tr("Import Model"));

	QVBoxLayout* Layout = new QVBoxLayout(this);
	QHBoxLayout* RadioButtonLayout = new QHBoxLayout(this);
	FromFolder_RadioButton = new QRadioButton(tr("Import from directory"), this);
	FromFolder_RadioButton->setChecked(true);
	connect(FromFolder_RadioButton, &QRadioButton::clicked, this, &CModelImportWidget::FromFolderChecked_SLOT);
	RadioButtonLayout->addWidget(FromFolder_RadioButton, 0, Qt::AlignLeft);
	FromPLY_RadioButton = new QRadioButton(tr("Import from PLY file"), this);
	connect(FromPLY_RadioButton, &QRadioButton::clicked, this, &CModelImportWidget::FromPLYChecked_SLOT);
	RadioButtonLayout->addWidget(FromPLY_RadioButton, 9999, Qt::AlignRight);
	Layout->addLayout(RadioButtonLayout);

	FromFolder_Widget = new QWidget(this);
	FolderDir_LineEdit = new QLineEdit();
	FolderDir_LineEdit->setReadOnly(true);
	SelectFolderDir_Button = new QPushButton(tr("Select"), FromFolder_Widget);
	connect(SelectFolderDir_Button, &QPushButton::released, this, &CModelImportWidget::SelectFolder_SLOT);
	FolderOverwrite_RadioButton = new QRadioButton(tr("Overwrite current models"), FromFolder_Widget);
	FolderAdd_RadioButton = new QRadioButton(tr("Add as new models"), FromFolder_Widget);
	FolderAdd_RadioButton->setChecked(true);
	QGridLayout* FromFolder_Layout = new QGridLayout(FromFolder_Widget);
	FromFolder_Layout->addWidget(new QLabel(tr("Model directory:"), this), 0, 0);
	FromFolder_Layout->addWidget(FolderDir_LineEdit, 0, 1);
	FromFolder_Layout->addWidget(SelectFolderDir_Button, 0, 2);
	QVBoxLayout* FromFolderRadioButton_Layout = new QVBoxLayout();
	FromFolderRadioButton_Layout->addWidget(FolderOverwrite_RadioButton);
	FromFolderRadioButton_Layout->addWidget(FolderAdd_RadioButton);
	FromFolder_Layout->addLayout(FromFolderRadioButton_Layout, 1, 0, 2, 3, Qt::AlignHCenter);
	FromFolder_Widget->setLayout(FromFolder_Layout);
	FromFolder_Widget->setVisible(true);

	FromPLY_Widget = new QWidget(this);
	PLYPath_LineEdit = new QLineEdit();
	PLYPath_LineEdit->setReadOnly(true);
	SelectPLY_Button = new QPushButton(tr("Select"), FromPLY_Widget);
	connect(SelectPLY_Button, &QPushButton::released, this, &CModelImportWidget::SelectPLY_SLOT);
	PLYOverwrite_RadioButton = new QRadioButton(tr("Overwrite current models"), FromPLY_Widget);
	PLYAdd_RadioButton = new QRadioButton(tr("Add as a new model"), FromPLY_Widget);
	PLYAdd_RadioButton->setChecked(true);
	QGridLayout* FromPLY_Layout = new QGridLayout(FromPLY_Widget);
	FromPLY_Layout->addWidget(new QLabel(tr("PLY file directory:"), this), 0, 0);
	FromPLY_Layout->addWidget(PLYPath_LineEdit, 0, 1);
	FromPLY_Layout->addWidget(SelectPLY_Button, 0, 2);
	QVBoxLayout* FromPLYRadioButton_Layout = new QVBoxLayout();
	FromPLYRadioButton_Layout->addWidget(PLYOverwrite_RadioButton);
	FromPLYRadioButton_Layout->addWidget(PLYAdd_RadioButton);
	FromPLY_Layout->addLayout(FromPLYRadioButton_Layout, 1, 0, 2, 3, Qt::AlignHCenter);
	FromPLY_Widget->setLayout(FromPLY_Layout);
	FromPLY_Widget->setVisible(false);

	TabImportSetting_TabWidget = new QTabWidget(this);
	TabImportSetting_TabWidget->addTab(FromFolder_Widget, tr("From directory"));
	Layout->addWidget(TabImportSetting_TabWidget);

	LoadButton = new QPushButton(tr("Import Models"), this);
	LoadButton->setFixedWidth(Size(200, true));
	connect(LoadButton, &QPushButton::released, this, &CModelImportWidget::Load_SLOT);
	Layout->addWidget(LoadButton, 0, Qt::AlignHCenter);

	this->setLayout(Layout);
	this->setFixedSize(Size(420, true), Size(180, false));
}
void CModelImportWidget::DeleteAllTab()
{
	int TabNum = TabImportSetting_TabWidget->count();
	for (int i = TabNum - 1; i >= 0; i--)
	{
		TabImportSetting_TabWidget->removeTab(i);
	}
}
void CModelImportWidget::FolderFileDirInfo(QString FolderPath, QStringList& FileList, QStringList& FolderList)
{
	QDir dir(FolderPath);
	FolderPath = dir.fromNativeSeparators(FolderPath);
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Name);
	FileList = dir.entryList();
	for (int i = 0; i < FileList.size(); i++)
	{
		FileList[i] = FolderPath + "/" + FileList[i];
	}

	dir = QDir(FolderPath);
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	FolderList = dir.entryList();
	for (int i = 0; i < FolderList.size(); i++)
	{
		FolderList[i] = FolderPath + "/" + FolderList[i];
	}
}
bool CModelImportWidget::IsValidModelFolder(string ModelPath)
{
	string project_path = JoinPaths(ModelPath, "project.ini");
	string cameras_bin_path = JoinPaths(ModelPath, "cameras.bin");
	string images_bin_path = JoinPaths(ModelPath, "images.bin");
	string points3D_bin_path = JoinPaths(ModelPath, "points3D.bin");
	string cameras_txt_path = JoinPaths(ModelPath, "cameras.txt");
	string images_txt_path = JoinPaths(ModelPath, "images.txt");
	string points3D_txt_path = JoinPaths(ModelPath, "points3D.txt");
	if ((!ExistsFile(cameras_bin_path) || !ExistsFile(images_bin_path) || !ExistsFile(points3D_bin_path)) &&
		(!ExistsFile(cameras_txt_path) || !ExistsFile(images_txt_path) || !ExistsFile(points3D_txt_path)))
	{
		return false;
	}
	return true;
}
void CModelImportWidget::FromFolderChecked_SLOT()
{
	DeleteAllTab();
	FromFolder_Widget->setVisible(true);
	FromPLY_Widget->setVisible(false);
	TabImportSetting_TabWidget->addTab(FromFolder_Widget, tr("From directory"));
}
void CModelImportWidget::FromPLYChecked_SLOT()
{
	DeleteAllTab();
	FromFolder_Widget->setVisible(false);
	FromPLY_Widget->setVisible(true);
	TabImportSetting_TabWidget->addTab(FromPLY_Widget, tr("From PLY"));
}
void CModelImportWidget::SelectFolder_SLOT()
{
	string ImportPath = QString2StdString(QFileDialog::getExistingDirectory(this, tr("Select Model Folder..."), "", QFileDialog::ShowDirsOnly));
	if (ImportPath == "")
	{
		return;
	}
	ModelFolders.clear();
	QStringList FileList, FolderList;
	FolderFileDirInfo(StdString2QString(ImportPath), FileList, FolderList);
	if (FolderList.size() == 0)
	{
		if (!IsValidModelFolder(ImportPath))
		{
			FolderDir_LineEdit->setText("");
			QMessageBox::critical(this, tr("Import Error"), tr("Cameras, images, and points3D files do not exist in the chosen directory!"));
			return;
		}
		else
		{
			FolderDir_LineEdit->setText(StdString2QString(ImportPath));
			ModelFolders = { ImportPath };
		}
	}
	else
	{
		if (IsValidModelFolder(ImportPath))
		{
			FolderDir_LineEdit->setText(StdString2QString(ImportPath));
			ModelFolders = { ImportPath };
		}
		for (QString SubFolder : FolderList)
		{
			if (IsValidModelFolder(QString2StdString(SubFolder)))
			{
				ModelFolders.push_back(QString2StdString(SubFolder));
				if (FolderDir_LineEdit->text().isEmpty())
				{
					FolderDir_LineEdit->setText(SubFolder);
				}
			}
		}
		if (ModelFolders.empty())
		{
			FolderDir_LineEdit->setText("");
			QMessageBox::critical(this, tr("Import Error"), tr("Cameras, images, and points3D files do not exist in the chosen directory!"));
			return;
		}
		else
		{
			QMessageBox::information(this, tr("Multiple Models"), tr("Find ") + QString::number(ModelFolders.size()) + tr(" models!"));
		}
	}
}
void CModelImportWidget::SelectPLY_SLOT()
{
	string PLYPath = QString2StdString(QFileDialog::getOpenFileName(this, tr("Open a PLY File"), "", tr("PLY File (*.ply)")));
	if (PLYPath == "")
	{
		return;
	}
	PLYPath_LineEdit->setText(StdString2QString(PLYPath));
	this->PLYPath = PLYPath;
}
void CModelImportWidget::Load_SLOT()
{
	int Type = FromFolder_RadioButton->isChecked() ? 0 : 1;
	if ((Type == 0 && FolderDir_LineEdit->text().isEmpty()) || (Type == 1 && PLYPath_LineEdit->text().isEmpty()))
	{
		QMessageBox::critical(this, tr("Error"), tr("Invalid Path!"));
		return;
	}
	vector<string> ModelFolders = (Type == 0 ? this->ModelFolders : vector<string>());
	string PLYPath = (Type == 0 ? "" : this->PLYPath);
	bool IsOverwrite = (Type == 0 ? FolderOverwrite_RadioButton->isChecked() : PLYOverwrite_RadioButton->isChecked());
	emit ModelImport_SIGNAL(Type, ModelFolders, PLYPath, IsOverwrite);
	
	closeEvent(nullptr);
}
void CModelImportWidget::closeEvent(QCloseEvent* event)
{
	FromFolder_RadioButton->setChecked(true);
	FromPLY_RadioButton->setChecked(false);
	FromFolderChecked_SLOT();
	vector<string>().swap(ModelFolders);
	PLYPath = "";
	close();
}


size_t CModelSelectWidget::NewstModelIndex = INT_MAX;
CModelSelectWidget::CModelSelectWidget(QWidget* parent, CModelManager* ModelManager) :QComboBox(parent)
{
	this->ModelManager = ModelManager;
	QFont font;
	font.setPointSize(Size(10, true));
	setFont(font);
}
void CModelSelectWidget::Update()
{
	if (view()->isVisible() || !ModelManager)return;
	blockSignals(true);
	size_t PreIndex = max(0, currentIndex());
	clear();
	addItem("Newest model");

	size_t MaxWidth = 0;
	ModelManager->Lock();
	for (size_t i = 0; i < ModelManager->Size(); i++)
	{
		CModel& CurModel = ModelManager->GetRefer(i);
		size_t RegImagesNum = CurModel.GetModelRegImagesNum();
		size_t Points3DNum = CurModel.GetModelPoints3DNum();
		QString item = QString().asprintf("Model %d (%d images, %d points)", i + 1, RegImagesNum, Points3DNum);
		QFontMetrics FontMetrics(view()->font());
		MaxWidth = max(MaxWidth, (size_t)FontMetrics.width(item));
		addItem(item);
	}
	ModelManager->UnLock();
	view()->setMinimumWidth(MaxWidth);

	if (ModelManager->Size() == 0)
	{
		setCurrentIndex(0);
	}
	else
	{
		setCurrentIndex(PreIndex);
	}
	blockSignals(false);
}
size_t CModelSelectWidget::GetSelectedModelIndex()
{
	if (!ModelManager)
	{
		throw "Model manager is nullptr!";
	}
	if (ModelManager->Size() == 0 || currentIndex() == 0)return NewstModelIndex;
	return currentIndex() - 1;
}
void CModelSelectWidget::SelectModel(size_t index)
{
	if (!ModelManager)
	{
		throw "Model manager is nullptr!";
	}
	blockSignals(true);
	if (ModelManager->Size() == 0)
	{
		setCurrentIndex(0);
	}
	else
	{
		setCurrentIndex(index + 1);
	}
	blockSignals(false);
}
