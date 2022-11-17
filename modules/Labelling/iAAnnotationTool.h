/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *O
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

#include <iATool.h>
#include <iAVec3.h>

#include <QColor>
#include <QObject>
#include <QString>

#include <memory>

class iAAnnotationToolUI;

class iAMainWindow;
class iAMdiChild;

struct iAAnnotation
{
	iAAnnotation(size_t id, iAVec3d coord, QString const& name, QColor color);
	iAAnnotation(iAAnnotation const& a) = default;
	size_t m_id;
	iAVec3d m_coord;
	QString m_name;
	QColor m_color;
};

class iAAnnotationTool : public QObject, public iATool
{
	Q_OBJECT
public:
	static const QString Name;
	iAAnnotationTool(iAMainWindow* mainWin, iAMdiChild* child);
	size_t addAnnotation(iAVec3d const & coord);
	void renameAnnotation(size_t id, QString const& newName);
	void removeAnnotation(size_t id);
	std::vector<iAAnnotation> const & annotations() const;

public slots:
	void startAddMode();

private slots:
	void slicerPointClicked(double x, double y, double z);

private:
	std::shared_ptr<iAAnnotationToolUI> m_ui;
};