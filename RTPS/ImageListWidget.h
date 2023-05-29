#pragma once
#include "Base.h"
#include "ImageViewer.h"


class CImageListWidget :public QListWidget
{
	Q_OBJECT
public:
	CImageListWidget(QDockWidget* parent);
	void AddImage(std::string ImagePath);
	void ClearImage();
public slots:
	void itemDoubleClicked_SLOT(QListWidgetItem* item);

private:
	QDockWidget* parent;
	std::vector<QListWidgetItem*> Items;
	std::vector<std::string> ImagePaths;

	~CImageListWidget();
	QSize CalculateImgSize(QSize& OriginSize);

};