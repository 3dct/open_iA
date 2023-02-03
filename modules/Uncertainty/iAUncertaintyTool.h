// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iATool.h>

#include <iAITKIO.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QObject>
#include <QVector>

class iADockWidgetWrapper;
class iAEnsemble;
class iAEnsembleDescriptorFile;
class iAEnsembleView;
class iAHistogramView;
class iAMemberView;
class iAScatterPlotView;
class iASpatialView;

class vtkLookupTable;

class iAUncertaintyTool: public QObject, public iATool
{
	Q_OBJECT
public:
	iAUncertaintyTool(iAMainWindow* mainWnd, iAMdiChild* child);
	static std::shared_ptr<iATool> create(iAMainWindow* mainWnd, iAMdiChild* child);
	void toggleDockWidgetTitleBars();
	void toggleSettings();
	void calculateNewSubEnsemble();
	void loadState(QSettings & projectFile, QString const& fileName) override;
	void saveState(QSettings & projectFile, QString const& fileName) override;
	void writeFullDataFile();
private slots:
	void memberSelected(int memberIdx);
	void ensembleSelected(QSharedPointer<iAEnsemble> ensemble);
private:
	iAHistogramView * m_labelDistributionView, * m_uncertaintyDistributionView;
	iAMemberView* m_memberView;
	iAScatterPlotView* m_scatterplotView;
	iASpatialView* m_spatialView;
	iAEnsembleView* m_ensembleView;
	QVector<iADockWidgetWrapper*> m_dockWidgets;
	QVector<iAITKIO::ImagePointer> m_shownMembers;
	QSharedPointer<iAEnsemble> m_currentEnsemble;
	vtkSmartPointer<vtkLookupTable> m_labelLut;
	int m_newSubEnsembleID;
	// cache for ensemble loading:
	QSharedPointer<iAEnsembleDescriptorFile> m_ensembleFile;
};
