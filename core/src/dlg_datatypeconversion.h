/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "charts/iAPlotData.h"
#include "iAVtkWidgetFwd.h"
#include "ui_DataTypeConversion.h"

#include <vtkSmartPointer.h>

#include <QDialog>
#include <QString>

class iAConnector;

class vtkImageData;
class vtkPlaneSource;

class QCheckBox;
class QComboBox;
class QLineEdit;

class dlg_datatypeconversion : public QDialog, public Ui_DataTypeConversion
{
	Q_OBJECT

public:
	dlg_datatypeconversion ( QWidget *parent, QString const & filename, int intype, double* b, double* c, double* inPara );
	~dlg_datatypeconversion();

	void DataTypeConversion(QString const & filename, double* b);
	void DataTypeConversionROI(QString const & filename, double* b, double *roi);
	void createHistogram(iAPlotData::DataType* histbinlist, double minVal, double maxVal, int m_bins, double discretization);

	QString coreconversionfunction(QString filename, QString & finalfilename, double* para, int indatatype, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check);
	QString coreconversionfunctionforroi(QString filename, QString & finalfilename, double* para, int outdatatype, double minrange, double maxrange, double minout, double maxout, int check, double* roi);
	void updatevalues(double* inPara);
	void updateROI( );

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
	double * m_bptr;
	int m_intype;
	double m_insizez;
	iAPlotData::DataType * m_histbinlist;
	double m_min, m_max, m_dis;
	vtkSmartPointer<vtkPlaneSource> m_xyroiSource, m_xzroiSource, m_yzroiSource;
	iAConnector *m_roiimage, *m_xyimage, *m_xzimage, *m_yzimage;
	iAVtkWidget* m_xyWidget, *m_xzWidget, *m_yzWidget;
	QLineEdit* leRangeLower, *leRangeUpper, *leOutputMin,*leOutputMax, *leXOrigin, *leXSize, *leYOrigin, *leYSize, *leZOrigin, *leZSize;
	QComboBox* cbDataType;
	QCheckBox* chConvertROI;
	double m_roi[6];
	double m_spacing[3];
};
