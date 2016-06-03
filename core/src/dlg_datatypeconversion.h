/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef DLG_DATATYPECONVERSION_H
#define DLG_DATATYPECONVERSION_H

#include <string>
using namespace std;

#include <QDialog>
#include <QString>

#include "ui_DataTypeConversion.h"

class QCheckBox;
class QComboBox;

class QVTKWidget;
class vtkActor;
class vtkImageData;
class vtkPlaneSource;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class iAConnector;

#include "iAAbstractDiagramData.h"

class dlg_datatypeconversion : public QDialog, public Ui_DataTypeConversion
{
	Q_OBJECT

public:
	dlg_datatypeconversion ( QWidget *parent, vtkImageData* input, const char* filename, const char* intype, double* b, double* c, double* inPara );
	~dlg_datatypeconversion();

	void DataTypeConversion(string m_filename, double* b);
	void DataTypeConversionROI(string m_filename, double* b, double *roi);
	void histogramdrawing(iAAbstractDiagramData::DataType* histbinlist, float min, float max, int m_bins, double discretization);

	void xyprojectslices();
	void xzprojectslices();
	void yzprojectslices();
	QString coreconversionfunction(QString filename, QString & finalfilename, double* para, string indatatype, string outdatatype, double minrange, double maxrange, double minout, double maxout, int check);
	QString coreconversionfunctionforroi(QString filename, QString & finalfilename, double* para, string indatatype, string outdatatype, double minrange, double maxrange, double minout, double maxout, int check, double* roi);
	void updatevalues(double* inPara);
	virtual void changeEvent();
	void updateroi( );

	double getlabelWidget1 ();
	double getlabelWidget2 ();
	double getlabelWidget3 ();
	double getlabelWidget4 ();
	double getlabelWidget6 ();
	double getlabelWidget7 ();
	double getlabelWidget8 ();
	double getlabelWidget9 ();
	double getlabelWidget10();
	double getlabelWidget11();

	string getcombobox ();
	int getcheckbox1 ();
	private slots:
		void update(QString a);

private:
	QString text11;

	iAConnector* m_roiconvertimage;

	double * m_bptr;
	QTabWidget* TabWidget;
	int m_bins;
	vtkImageData* imageData;
	string m_intype;
	double m_sliceskip, m_insizex,	m_insizey, m_insizez, m_inspacex, m_inspacey, m_inspacez;
	string m_filename;
	iAAbstractDiagramData::DataType * m_histbinlist;
	float m_min, m_max, m_dis;
	vtkImageData* m_testxyimage;	vtkImageData* m_testxzimage;	vtkImageData* m_testyzimage; vtkImageData* m_roiimage;
	QVTKWidget* vtkWidgetXY, *vtkWidgetXZ, *vtkWidgetYZ;

	iAConnector* xyconvertimage;	iAConnector* xzconvertimage; iAConnector* yzconvertimage;

	int m_xstart, m_xend, m_ystart, m_yend, m_zstart, m_zend;
	QLineEdit* lineEdit1, *lineEdit2, *lineEdit3,*lineEdit4, *lineEdit5, *lineEdit6, *lineEdit7, *lineEdit8, *lineEdit9, *lineEdit10;
	QComboBox* comboBox;
	QCheckBox* checkbox1, * checkbox2;
	double m_roi[6];
	double m_spacing[3];

	vtkPlaneSource *xyroiSource, *xzroiSource;
	vtkPolyDataMapper *xyroiMapper, *xzroiMapper;
	vtkActor *xyroiActor, *xzroiActor;
	vtkRenderer* xyrenderer, *xzrenderer;
	vtkRenderWindowInteractor* xyinteractor, *xzinteractor;
	vtkRenderWindow* xywindow, *xzwindow;
};
#endif