#pragma once
#include "base.h"
#include "UI.h"
#include "Database.h"

class CImageViewer :public QWidget
{
	Q_OBJECT
public:
	CImageViewer(QWidget* parent, std::string ImagePath);

public slots:
	void EnableButton(); //使"显示特征点"、"显示重叠影像"按钮可用

private:
	QWidget* parent;
	std::string ImagePath;
	QLabel ImageLabel;
	QPixmap Pixmap;
	QImage Image;
	QImage Image_KeyPoints;
	bool StopFlag;

	QPushButton* KeyPointsButton;
	QPushButton* SaveButton;
	QPushButton* MatchButton;
	QVBoxLayout* Layout;
	QHBoxLayout* SubLayout;
	QHBoxLayout* ButtonLayout;
	COverlappingInfoWidget* OverlappingInfoWidget;


	void Daemon();
	void ShowKeyPoints();
	void Save();
	void ShowOverlappingWidget();
	void closeEvent(QCloseEvent* event);
	void resizeEvent(QResizeEvent* event) override;


signals:
	void EnableButton_SIGNAL();
};
