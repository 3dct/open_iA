// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// charts
#include "iAPlotData.h"

#include <vtkSmartPointer.h>

#include <QDialog>
#include <QString>

class iAConnector;
struct iARawFileParameters;

class iAQVTKWidget;

class vtkImageData;
class vtkPlaneSource;

class QCheckBox;
class QComboBox;
class QLineEdit;

//! Dialog for opening raw files with preview and conversion options.
class dlg_datatypeconversion : public QDialog
{
	Q_OBJECT

public:
	dlg_datatypeconversion ( QWidget *parent, QString const & filename, iARawFileParameters const & p,
		unsigned int zSkip, double* inPara );
	~dlg_datatypeconversion();

	QString convert(QString const & filename, int outdatatype,
		double minrange, double maxrange, double minout, double maxout);
	QString convertROI(QString const & filename, iARawFileParameters const & p, int outdatatype,
		double minrange, double maxrange, double minout, double maxout, double* roi);

	double getRangeLower() const;
	double getRangeUpper() const;
	double getOutputMin() const;
	double getOutputMax() const;
	double getXOrigin() const;
	double getXSize() const;
	double getYOrigin() const;
	double getYSize() const;
	double getZOrigin() const;
	double getZSize() const;
	QString getDataType() const;
	int getConvertROI() const;

private slots:
	void update(QString a);

private:
	void loadPreview(QString const & filename, iARawFileParameters const & p, unsigned int zSkip, size_t numBins);
	void DataTypeConversionROI(QString const & filename, iARawFileParameters const & p, double *roi);
	void createHistogram(iAPlotData::DataType* histbinlist, double minVal, double maxVal, int bins);
	void updatevalues(double* inPara);
	void updateROI();

	iAPlotData::DataType * m_histbinlist;
	double m_min, m_max;
	vtkSmartPointer<vtkPlaneSource> m_xyroiSource, m_xzroiSource, m_yzroiSource;
	iAConnector *m_roiimage, *m_xyimage, *m_xzimage, *m_yzimage;
	iAQVTKWidget* m_xyWidget, *m_xzWidget, *m_yzWidget;
	QLineEdit* leRangeLower, *leRangeUpper, *leOutputMin,*leOutputMax, *leXOrigin, *leXSize, *leYOrigin, *leYSize, *leZOrigin, *leZSize;
	QComboBox* cbDataType;
	QCheckBox* chConvertROI;
	double m_roi[6];
	double m_spacing[3];
};
