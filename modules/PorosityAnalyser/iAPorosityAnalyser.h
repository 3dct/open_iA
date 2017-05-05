/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#ifndef iAPorosityAnalyser_h__
#define iAPorosityAnalyser_h__

#include "ui_PorosityAnalyser.h"
#include "iAQTtoUIConnector.h"

#include <QTableWidget>
#include <QFileInfo>
#include <QMap>
#include <QList>

#include <vtkSmartPointer.h>

typedef iAQTtoUIConnector<QMainWindow, Ui_PorosityAnalyser> PorosityAnalyserConnector;

class iASPMView;
class iATreeView;
class iAPDMView;
class iAPCView;
class iASSView;
class iARangeSliderDiagramView;
class iASelectionsView;
class QTreeWidgetItem;
class QTableWidget;
class vtkIdTypeArray;
class vtkSelection;
struct iASelection;
class iASegm3DView;
class iAPreviewSPLOMView;
class QButtonGroup;

class iAPorosityAnalyser : public PorosityAnalyserConnector
{
	Q_OBJECT

public:
	iAPorosityAnalyser( const QString & resDir, const QString & datasetsDir, QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iAPorosityAnalyser();
	void LoadStateAndShow();

protected:
	void LoadData();
	void AddSubdirectory( const QString & subDirName );
	void ParseComputerCSV( const QFileInfo & fi );
	void CalculateRunsOffset();
	//void GenerateMasksData();

signals:
	void loadTreeDataToViews();
	void loadOverviewSelectionToSPM( QModelIndexList indices );
	void runsOffsetChanged( int );

	protected slots:
	void ShowSelections( bool checked );
	void ShowTreeView( bool checked );
	void selectionLoaded( iASelection * sel );
	void tabChanged( int index );
	void message( QString text );

protected:
	QTableWidget m_data;
	QTableWidget m_referenceData;
	QMap<QString, double> m_gtPorosityMap;
	QString m_dataDir;
	QString m_datasetsDir;
	iASPMView * m_spmView;
	iATreeView * m_treeView;
	iAPDMView * m_pdmView;
	iAPCView * m_pcView;
	iASSView * m_ssView;
	iARangeSliderDiagramView * m_rangeSliderDiagramView;
	iASelectionsView * m_selView;
	iASegm3DView * m_segm3DView;
	iAPreviewSPLOMView * m_prvSplomView;
	int m_runsOffset;
	QMainWindow *m_visanMW;
};

#endif // iAPorosityAnalyser_h__
