// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "labelling_export.h"

#include <iATool.h>
#include <iAVec3.h>

#include <vtkSmartPointer.h>

#include <QColor>
#include <QObject>
#include <QString>

#include <memory>

class iAAnnotationToolUI;

class iAMainWindow;
class iAMdiChild;

class vtkCaptionWidget;
class vtkRenderWindowInteractor;

struct iAAnnotation
{
	iAAnnotation(size_t id, iAVec3d coord, QString const& name, QColor color);
	size_t m_id;
	iAVec3d m_coord;
	QString m_name;
	QColor m_color;
	bool m_show;
};

class Labelling_API iAAnnotationTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString Name;
	iAAnnotationTool(iAMainWindow* mainWin, iAMdiChild* child);
	size_t addAnnotation(iAVec3d const & coord);
	std::vector<iAAnnotation> const & annotations() const;
	void loadState(QSettings& projectFile, QString const& fileName) override;
	void saveState(QSettings& projectFile, QString const& fileName) override;

public slots:
	void startAddMode();
	void renameAnnotation(size_t id, QString const& newName);
	void removeAnnotation(size_t id);
	void focusToAnnotation(size_t id);
	void toggleAnnotation(size_t id);

signals:
	void annotationsUpdated(std::vector<iAAnnotation> const &);
	void annotionMode(bool activated);
	void focusedToAnnotation(size_t id);

private slots:
	void slicerPointClicked(double x, double y, double z);

private:
	void addAnnotation(iAAnnotation a);
	void showAnnotation(size_t id, bool show);
	void showActors(size_t id, bool show);
	void adjustTableItemShown(int row, bool show);
	std::shared_ptr<iAAnnotationToolUI> m_ui;
};

//! create a 3D widget for a given render window interactor and a given annotation
Labelling_API vtkSmartPointer<vtkCaptionWidget> create3DWidget(vtkRenderWindowInteractor* interactor, iAAnnotation& a);