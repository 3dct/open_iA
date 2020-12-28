/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

// charts
#include "iAPlotData.h"

#include "iAVtkWidgetFwd.h"
#include "ui_DataTypeConversion.h"

#include <vtkSmartPointer.h>

#include <QDialog>
#include <QString>

class iAConnector;
struct iARawFileParameters;

class vtkImageData;
class vtkPlaneSource;

class QCheckBox;
class QComboBox;
class QLineEdit;

class dlg_datatypeconversion : public QDialog, public Ui_DataTypeConversion
{
	Q_OBJECT

public:
	dlg_datatypeconversion ( QWidget *parent, QString const & filename, iARawFileParameters const & p,
		unsigned int zSkip, size_t numBins, double* inPara );
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
	iAVtkWidget* m_xyWidget, *m_xzWidget, *m_yzWidget;
	QLineEdit* leRangeLower, *leRangeUpper, *leOutputMin,*leOutputMax, *leXOrigin, *leXSize, *leYOrigin, *leYSize, *leZOrigin, *leZSize;
	QComboBox* cbDataType;
	QCheckBox* chConvertROI;
	double m_roi[6];
	double m_spacing[3];
};
