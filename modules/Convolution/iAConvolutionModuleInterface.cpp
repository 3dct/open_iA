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
#include "pch.h"
#include "iAConvolutionModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAConvolutionFilter.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QFileDialog>
#include <QMessageBox>


void iAConvolutionModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * filterMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuConvolution_Filter = getMenuWithTitle(filterMenu, QString("Convolution & Correlation"));

	QAction * actionConvolutionFilter = new QAction(m_mainWnd);
	QAction * actionfftConvolutionFilter = new QAction(m_mainWnd);
	QAction * actionCorrelationFilter = new QAction(m_mainWnd); 
	QAction * actionFFTCorrelationFilter = new QAction(m_mainWnd);
	QAction * actionFFTCPPCorrelationFilter = new QAction(m_mainWnd);

	actionConvolutionFilter->setText(QApplication::translate("MainWindow", "Convolution", 0));
	actionfftConvolutionFilter->setText(QApplication::translate("MainWindow", "FFT Convolution", 0));
	actionCorrelationFilter->setText(QApplication::translate("MainWindow", "Correlation", 0));
	actionFFTCorrelationFilter->setText(QApplication::translate("MainWindow", "FFT Correlation", 0));
	actionFFTCPPCorrelationFilter->setText(QApplication::translate("Mainwindow", "FFT CPP Correlation", 0));

	menuConvolution_Filter->addAction(actionConvolutionFilter);
	menuConvolution_Filter->addAction(actionfftConvolutionFilter);
	menuConvolution_Filter->addAction(actionCorrelationFilter);
	menuConvolution_Filter->addAction(actionFFTCorrelationFilter);
	menuConvolution_Filter->addAction(actionFFTCPPCorrelationFilter); 

	//connect signals to slots
	connect(actionConvolutionFilter, SIGNAL(triggered()), this, SLOT(convolve()));
	connect(actionfftConvolutionFilter, SIGNAL(triggered()), this, SLOT(FFT_convolve()));
	connect(actionCorrelationFilter, SIGNAL(triggered()), this, SLOT(correlate()));
	connect(actionFFTCorrelationFilter, SIGNAL(triggered()), this, SLOT(FFT_correlate()));
	connect(actionFFTCPPCorrelationFilter, SIGNAL(triggered()), this, SLOT(FFT_CPP_correlate())); 
}

void iAConvolutionModuleInterface::convolve()
{
	QString filename = QFileDialog::getOpenFileName(m_mainWnd, tr("Load template .mhd file"), QDir::currentPath(), tr("MHD Files (*.mhd *.mha *.MHD *.MHA)"));
	if (filename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Class load error: No source file was specified!");
		msgBox.setWindowTitle("ConvFilter");
		msgBox.exec();
		return;
	}

	//path to template image
	QStringList inList = (QStringList() << tr("#Path to template image"));
	QList<QVariant> inPara; inPara << tr("%1").arg(filename);

	dlg_commoninput dlg(m_mainWnd, "Convolution Filter", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;
	
	//prepare
	QString filterName = "Convolution";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAConvolutionFilter* thread = new iAConvolutionFilter(filterName, CONVOLUTION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);

	thread->setParameters( filename.toStdString() );
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}

void iAConvolutionModuleInterface::correlate()
{
	QString filename = QFileDialog::getOpenFileName(m_mainWnd, tr("Load template .mhd file"), QDir::currentPath(), tr("MHD Files (*.mhd *.mha *.MHD *.MHA)"));
	if (filename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Class load error: No source file was specified!");
		msgBox.setWindowTitle("ConvFilter");
		msgBox.exec();
		return;
	}

	//path to template image
	QStringList inList = (QStringList() << tr("#Path to template image"));
	QList<QVariant> inPara; inPara << tr("%1").arg(filename);

	dlg_commoninput dlg(m_mainWnd, "Correlation Filter", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//prepare
	QString filterName = "Correlation";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAConvolutionFilter* thread = new iAConvolutionFilter(filterName, CORRELATION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);

	thread->setParameters(filename.toStdString());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}

void iAConvolutionModuleInterface::FFT_correlate()
{
	QString filename = QFileDialog::getOpenFileName(m_mainWnd, tr("Load template .mhd file"), QDir::currentPath(), tr("MHD Files (*.mhd *.mha *.MHD *.MHA)"));
	if (filename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Class load error: No source file was specified!");
		msgBox.setWindowTitle("ConvFilter");
		msgBox.exec();
		return;
	}

	//path to template image
	QStringList inList = (QStringList() << tr("#Path to template image"));
	QList<QVariant> inPara; inPara << tr("%1").arg(filename);

	dlg_commoninput dlg(m_mainWnd, "FFT Correlation Filter", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//prepare
	QString filterName = "FFT Correlation";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAConvolutionFilter* thread = new iAConvolutionFilter(filterName, FFT_CORRELATION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);

	thread->setParameters(filename.toStdString());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}

void iAConvolutionModuleInterface::FFT_convolve()
{
	QString filename = QFileDialog::getOpenFileName(m_mainWnd, tr("Load template .mhd file"), QDir::currentPath(), tr("MHD Files (*.mhd *.mha *.MHD *.MHA)"));
	if (filename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Class load error: No source file was specified!");
		msgBox.setWindowTitle("FFT ConvFilter");
		msgBox.exec();
		return;
	}

	//path to template image
	QStringList inList = (QStringList() << tr("#Path to template image"));
	QList<QVariant> inPara; inPara << tr("%1").arg(filename);

	dlg_commoninput dlg(m_mainWnd, "FFT Convolution Filter", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//prepare
	QString filterName = "FFT Convolution";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAConvolutionFilter* thread = new iAConvolutionFilter(filterName, FFT_CONVOLUTION_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);

	thread->setParameters(filename.toStdString());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}

void iAConvolutionModuleInterface::FFT_CPP_correlate()
{
	QString filename = QFileDialog::getOpenFileName(m_mainWnd, tr("Load template .mhd file"), QDir::currentPath(), tr("MHD Files (*.mhd *.mha *.MHD *.MHA)"));
	if (filename.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Class load error: No source file was specified!");
		msgBox.setWindowTitle("FFT CPP CorrFilter");
		msgBox.exec();
		return;
	}

	//path to template image
	QStringList inList = (QStringList() << tr("#Path to template image"));
	QList<QVariant> inPara; inPara << tr("%1").arg(filename);

	dlg_commoninput dlg(m_mainWnd, "FFT CPP correlation filter", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	//prepare
	QString filterName = "FFT CPP correlation";
	PrepareResultChild(filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iAConvolutionFilter* thread = new iAConvolutionFilter(filterName, FFT_NCC_CPP_FILTER,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);

	thread->setParameters(filename.toStdString());
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);

}
