#pragma once

#include "ui_AdaptiveThreshold.h"
#include <QSharedPointer>
#include <QDialog>
#include <QChartView>


struct Threshold{
	
};

class  AdaptiveThreshold : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	AdaptiveThreshold(QWidget * parent = 0, Qt::WindowFlags f = 0);
	
	void calculateMovingAverege();

	void loadValues(const QString &filename);

private: 
	double resThreshold; 

};

