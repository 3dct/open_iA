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
#pragma once

#include <QFileInfoList>
#include <QSharedPointer>
#include <QThread>

class iAModalityList;
class iAMultiStepProgressObserver;
class MdiChild;

class iATLGICTLoader : public QThread
{
	Q_OBJECT
public:
	iATLGICTLoader();
	bool setup(QString const & baseDirectory, QWidget* parent);
	void start(MdiChild* child);
protected:
	~iATLGICTLoader();	// destructur private to make sure we can only be constructed on the heap
						// and will destruct ourselves in finishUp()
private:

	QString m_baseDirectory;
	MdiChild* m_child;

	QSharedPointer<iAModalityList> m_modList;
	double m_spacing[3];
	double m_origin[3];
	QFileInfoList m_subDirs;
	iAMultiStepProgressObserver* m_multiStepObserver;
	
	virtual void run();
private slots:
	void finishUp();
};
