/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAAlgorithm.h"

#include <iAConnector.h>
#include <iALogger.h>
#include <mdichild.h>
#include <iAProgress.h>

#include <vtkImageData.h>
#include <vtkPolyData.h>

#include <QLocale>
#include <QMessageBox>

iAAlgorithm::iAAlgorithm( QString fn, vtkImageData* idata, vtkPolyData* p, iALogger * logger, QObject *parent )
	: QThread( parent ),
	m_isRunning(false),
	m_filterName(fn),
	m_image(idata),
	m_polyData(p),
	m_progressObserver(new iAProgress),
	m_logger(logger)
{
	m_connectors.push_back(new iAConnector());
	if (parent && qobject_cast<MdiChild*>(parent))
	{
		connect(qobject_cast<MdiChild*>(parent), &MdiChild::rendererDeactivated, this, &iAAlgorithm::updateVtkImageData);
	}
	connect(m_progressObserver, &iAProgress::progress, this, &iAAlgorithm::aprogress );
}

iAAlgorithm::~iAAlgorithm()
{
	for (iAConnector* c : m_connectors)
	{
		delete c;
	}
	m_connectors.clear();
	delete m_progressObserver;
}

void iAAlgorithm::run()
{
	Start();
	try
	{
		getConnector()->setImage(getVtkImageData());
		getConnector()->modified();
		performWork();
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1 terminated unexpectedly. Error: %2; in File %3, Line %4. Elapsed time: %5 ms.")
			.arg(getFilterName())
			.arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine())
			.arg(Stop()));
		return;
	}
	catch (const std::exception& e)
	{
		addMsg(tr("%1 terminated unexpectedly. Error: %2. Elapsed time: %3 ms.")
			.arg(getFilterName())
			.arg(e.what())
			.arg(Stop()));
		return;
	}
	addMsg(tr("%1 finished. Elapsed time: %2 ms.")
		.arg(getFilterName())
		.arg(Stop()));
	emit startUpdate();
}

void iAAlgorithm::performWork()
{
	addMsg(tr("Unknown filter type"));
}

void iAAlgorithm::setImageData(vtkImageData* imgData)
{
	m_image = imgData;
}

void iAAlgorithm::Start()
{
	m_time.start();
	m_isRunning = true;
}

int iAAlgorithm::Stop()
{
	m_isRunning = false;
	return m_time.elapsed();
}

void iAAlgorithm::setup(QString fn, vtkImageData* i, vtkPolyData* p, iALogger * l)
{
	m_filterName = fn;
	m_image = i;
	m_polyData = p;
	m_logger = l;
}

void iAAlgorithm::addMsg(QString txt)
{
	if (m_logger)
	{
		m_logger->log(txt);
	}
}

iALogger* iAAlgorithm::logger() const
{
	return m_logger;
}

QString iAAlgorithm::getFilterName() const
{
	return m_filterName;
}

vtkImageData* iAAlgorithm::getVtkImageData()
{
	return m_image;
}

vtkPolyData* iAAlgorithm::getVtkPolyData()
{
	return m_polyData;
}

iAConnector* iAAlgorithm::getConnector() const
{
	return m_connectors[0];
}

QVector<iAConnector*> const & iAAlgorithm::Connectors() const
{
	return m_connectors;
}

void iAAlgorithm::AddImage(vtkImageData* i)
{
	auto con = new iAConnector();
	con->setImage(i);
	con->modified();
	m_connectors.push_back(con);
}

bool iAAlgorithm::deleteConnector(iAConnector* c)
{
	bool isDeleted = false;
	int ind = m_connectors.indexOf(c);
	if (ind >= 0)
	{
		m_connectors.remove(ind);
		isDeleted = true;
	}
	delete c;
	return isDeleted;
}

void iAAlgorithm::allocConnectors(int size)
{
	while (m_connectors.size() < size)
	{
		m_connectors.push_back(new iAConnector());
	}
}

iAProgress* iAAlgorithm::ProgressObserver()
{
	return m_progressObserver;
}

void iAAlgorithm::updateVtkImageData(int ch)
{	// updates the vtk image data in the mdi child to be the one contained
	// in the m_connectors[ch].
	if (m_image == m_connectors[ch]->vtkImage().GetPointer())
	{
		return;
	}
	m_image->ReleaseData();
	m_image->Initialize();
	m_image->DeepCopy(m_connectors[ch]->vtkImage());
	m_image->CopyInformationFromPipeline(m_connectors[ch]->vtkImage()->GetInformation());
	m_image->Modified();
}


void iAAlgorithm::SafeTerminate()
{
	if(isRunning())
	{
		terminate();
	}
}
