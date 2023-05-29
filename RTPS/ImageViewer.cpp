#include "ImageViewer.h"

using namespace std;
using namespace colmap;


CImageViewer::CImageViewer(QWidget* parent, string ImagePath)
{
	this->parent = parent;
	this->ImagePath = ImagePath;
	if (!ExistsFile(ImagePath))
	{
		ThrowError(ImagePath + " does not exists!");
	}
	StopFlag = false;

	Image = QImage(StdString2QString(ImagePath));
	Pixmap = QPixmap::fromImage(Image).scaled(this->width(), this->height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
	ImageLabel.setScaledContents(true);
	ImageLabel.setPixmap(Pixmap);
	ImageLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	KeyPointsButton = new QPushButton(tr("Show Keypoints"), this);
	KeyPointsButton->setEnabled(false);
	KeyPointsButton->setFixedWidth(Size(100, true));

	SaveButton = new QPushButton(tr("Save"), this);
	SaveButton->setFixedWidth(Size(60, true));

	MatchButton = new QPushButton(tr("Overlapping Images"), this);
	MatchButton->setEnabled(false);
	MatchButton->setFixedWidth(Size(120, true));

	connect(KeyPointsButton, &QPushButton::released, this, &CImageViewer::ShowKeyPoints);
	connect(SaveButton, &QPushButton::released, this, &CImageViewer::Save);
	connect(MatchButton, &QPushButton::released, this, &CImageViewer::ShowOverlappingWidget);

	Layout = new QVBoxLayout();
	Layout->addWidget(&ImageLabel);
	SubLayout = new QHBoxLayout();
	ButtonLayout = new QHBoxLayout();
	ButtonLayout->addSpacerItem(new QSpacerItem(Size(20, true), Size(60, false), QSizePolicy::Expanding));
	ButtonLayout->addWidget(KeyPointsButton);
	ButtonLayout->addWidget(MatchButton);
	ButtonLayout->addWidget(SaveButton);
	SubLayout->addLayout(ButtonLayout);
	Layout->addLayout(SubLayout);
	setLayout(Layout);

	QFileInfo fileInfo(StdString2QString(ImagePath));
	setWindowTitle(fileInfo.fileName());
	connect(this, SIGNAL(EnableButton_SIGNAL()), this, SLOT(EnableButton()));

	std::thread DaemonThread(&CImageViewer::Daemon, this); //检测当前影像是否已完成"特征提取"和"特征匹配"
	DaemonThread.detach();

	resize(Size(900, true), Size(700, false));
	setWindowIcon(QIcon(":/media/undistort.png"));
	show();
}
void CImageViewer::EnableButton()
{
	if (KeyPointsButton && MatchButton && !StopFlag)
	{
		KeyPointsButton->setEnabled(true);
		MatchButton->setEnabled(true);
		OverlappingInfoWidget = new COverlappingInfoWidget(this, ImagePath);
	}
}
void CImageViewer::Daemon()
{
	ThreadStart();
	QSqlDatabase db = CreateDatabaseConnect(CDatabase::DatabasePath);
	Image_KeyPoints = Image;
	string ImgName = GetFileName(ImagePath);
	while (!StopFlag && (!CDatabase::IsExistImage(ImgName,db) || !CDatabase::GetImageKeypointsNum(CDatabase::GetImageID(ImgName, db), db)))
	{
		this_thread::sleep_for(chrono::milliseconds(500));
	}
	if (!StopFlag)
	{
		size_t ImageID = CDatabase::GetImageID(ImgName, db);
		FeatureKeypoints Keypoints;
		CDatabase::GetKeypoints(ImageID, Keypoints, db);

		QPainter Painter(&Image_KeyPoints);
		Painter.setRenderHint(QPainter::Antialiasing, true);
		Painter.setPen(QPen(Qt::red, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		for (FeatureKeypoint& Keypoint : Keypoints)
		{
			Painter.drawEllipse(QPoint(Keypoint.x, Keypoint.y), 3, 3);
		}
	}
	//while (!StopFlag && CDatabase::MatchedImagesNum(ImageID) == 0) //只有当前影像的状态是Matched, Reconstructing, Finished的时候才退出循环
	//{
	//	this_thread::sleep_for(chrono::milliseconds(500));
	//}
	if (!StopFlag)
	{
		emit EnableButton_SIGNAL();
	}
	ReleaseDatabaseConnect(db);
	ThreadEnd();
}
void CImageViewer::ShowKeyPoints()
{
	if (KeyPointsButton->text() == tr("Show Keypoints")) //当前显示的影像是"原影像", 需要显示"带特征点的影像"
	{
		Pixmap = QPixmap::fromImage(Image_KeyPoints).scaled(width(), height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel.setScaledContents(true);
		ImageLabel.setPixmap(Pixmap);
		KeyPointsButton->setText(tr("Hide Keypoints"));
	}
	else //当前显示的影像是"带特征点的影像", 需要显示"原影像"
	{
		Pixmap = QPixmap::fromImage(Image).scaled(width(), height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel.setScaledContents(true);
		ImageLabel.setPixmap(Pixmap);
		KeyPointsButton->setText(tr("Show Keypoints"));
	}
}
void CImageViewer::Save()
{
	QString filter("BMP (*.bmp)");
	const QString SavePath = QFileDialog::getSaveFileName(this, tr("Select destination..."), "", "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)", &filter).toUtf8().constData();
	if (SavePath == "")
	{
		return;
	}
	if (KeyPointsButton->text() == tr("Show Keypoints"))
	{
		Image.save(SavePath, nullptr, 100);
	}
	else
	{
		Image_KeyPoints.save(SavePath, nullptr, 100);
	}
}
void CImageViewer::ShowOverlappingWidget()
{
	OverlappingInfoWidget = new COverlappingInfoWidget(this, ImagePath);
	OverlappingInfoWidget->show();
}
void CImageViewer::resizeEvent(QResizeEvent* event)
{
	if (KeyPointsButton->text() == tr("Show Keypoints"))
	{
		Pixmap = QPixmap::fromImage(Image).scaled(ImageLabel.width(), ImageLabel.height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel.setScaledContents(true);
		ImageLabel.setPixmap(Pixmap);
	}
	else
	{
		Pixmap = QPixmap::fromImage(Image_KeyPoints).scaled(ImageLabel.width(), ImageLabel.height(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
		ImageLabel.setScaledContents(true);
		ImageLabel.setPixmap(Pixmap);
	}
}
void CImageViewer::closeEvent(QCloseEvent* event)
{
	StopFlag = true;
	Image.~QImage();
	Image_KeyPoints.~QImage();
	Layout->deleteLater();
}
