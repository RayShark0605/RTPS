#include "ImageListWidget.h"

using namespace std;
using namespace colmap;


CImageListWidget::CImageListWidget(QDockWidget* parent): QListWidget(parent)
{
	this->parent = parent;
	Items.resize(0);
	ImagePaths.resize(0);
	parent->setWindowTitle(tr("Image list"));

	setSelectionMode(QAbstractItemView::SingleSelection);
	setViewMode(QListWidget::IconMode);
	setIconSize(QSize(Size(180, true), Size(180, false)));
	setSpacing(2);
	setResizeMode(QListView::Adjust);
	setMovement(QListView::Static);
	connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked_SLOT(QListWidgetItem*)));
}

void CImageListWidget::AddImage(string ImagePath)
{
	QString QImgPath = StdString2QString(ImagePath);

	//为了加快读取速度并且降低内存占用, 影像预览并不读取原图, 而是使用ImageReader以图标(Icon)的形式直接读取设定大小的影像图标
	QImageReader ImageReader(QImgPath);
	ImageReader.setAutoTransform(true);
	QSize OriginSize = ImageReader.size();
	QSize TargetSize = OriginSize.scaled(CalculateImgSize(OriginSize), Qt::KeepAspectRatio);
	ImageReader.setScaledSize(TargetSize);
	if (!ImageReader.canRead()) //影像读取失败, 影像可能是损坏的
	{
		cout << "[Warning] Image [ " + ImagePath + " ] is damaged!" << endl;
		return;
	}
	QListWidgetItem* item = new QListWidgetItem();
	item->setIcon(QIcon(QPixmap::fromImageReader(&ImageReader)));
	item->setText(StdString2QString(GetFileName(ImagePath)));
	TargetSize.setHeight(TargetSize.height() + 10);
	item->setSizeHint(TargetSize);
	item->setData(Qt::UserRole, StdString2QString(GetFileName(ImagePath)));
	addItem(item);
	Items.push_back(item);
	ImagePaths.push_back(ImagePath);
	parent->setWindowTitle(tr("Image list ") + "(" + QString::number(Items.size()) + ")");
}
void CImageListWidget::ClearImage()
{
	clear();
	Items.resize(0);
}
void CImageListWidget::itemDoubleClicked_SLOT(QListWidgetItem* item)
{
	//弹出"影像浏览"窗口
	QListWidgetItem* currentItem = this->currentItem();
	if (currentItem)
	{
		size_t index = this->row(currentItem);
		CImageViewer* ImageViewer = new CImageViewer(this, ImagePaths[index]);
	}
}


CImageListWidget::~CImageListWidget()
{
	ClearImage();
}
QSize CImageListWidget::CalculateImgSize(QSize& OriginSize)
{
	//计算影像图标合适的大小
	float dWidth = OriginSize.width() / 200.0;
	float dHeight = OriginSize.height() / 200.0;
	float Shrink = max(dHeight, dWidth);
	QSize dsize(OriginSize.width() / Shrink, OriginSize.height() / Shrink);
	return dsize;
}
