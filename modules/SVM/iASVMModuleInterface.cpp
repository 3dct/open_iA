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
#include "iASVMModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iASVMImageFilter.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkImageData.h>

#include <QFileDialog>
#include <QMessageBox>


void iASVMModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSegm = getMenuWithTitle(filtersMenu, QString( "Segmentation" ) );
	QMenu * menuGraphSegm = getMenuWithTitle(menuSegm, QString("SVM"));
	QAction * actionSVM = new QAction( m_mainWnd );
	actionSVM->setText( QApplication::translate( "MainWindow", "Probabilistic SVM", 0 ) );
	menuGraphSegm->addAction(actionSVM);
	connect(actionSVM, SIGNAL(triggered()), this, SLOT(CalculateSVM()));
}


bool iASVMModuleInterface::CalculateSVM()
{
	double gamma = 0.1;
	double c = 10;
	int dimension = 2;
	double r = 1;

	MdiChild* activeChild = m_mainWnd->activeMdiChild();

	QStringList inParaDescr = (QStringList()
		<< tr("+Kernel")
		<< tr("^Gamma")
		<< tr("*Dimension")
		<< tr("^Coef0")
		<< tr("^C"));
	QStringList kernels = (QStringList()
		<< tr("Linear")
		<< tr("Polynomial")
		<< tr("!RBF")
		<< tr("Sigmoid"));
	QList<QVariant> inParaValue;
	inParaValue << kernels << gamma << dimension << r << c;
	QTextDocument *fDescr = new QTextDocument(0);
	fDescr->setHtml(
		"<p><font size=+1>Classify pixels with Support Vector Machines (SVM).</font></p>"
		"<p>Choose the parameters to use! Note that the parameters (gamma, dimension and r are depending on the choice of the kernel:"
		"<ul>"
		"<li>The Linear kernel does not consider any of these parameters</li>"
		"<li>The Polynomial kernel considers the Gamma and the Dimension parameter</li>"
		"<li>The RBF kernel only considers the Gamma parameter</li>"
		"<li>The Sigmoid kernel considers the Coef0 and Gamma parameters</li>"
		"</ul></p>"
		"<p>In the following dialog, specify a file with one seed point per line in the following format :"
		"<pre>x y z label</pre>"
		"where x, y and z are the coordinates(set z = 0 for 2D images) and label is the index of the label "
		"for this seed point.Label indices should start at 0 and be contiguous(so if you have N different "
		"labels, you should use label indices 0..N - 1 and make sure that there is at least one seed per label).</p>"
	);
	dlg_commoninput dlg(m_mainWnd, "SVM Pixel Segmentation", inParaDescr, inParaValue, fDescr, true);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}

	QString seedFileName = QFileDialog::getOpenFileName(m_mainWnd,
		"SVM",
		activeChild->getFilePath(),
		"Plain Seed File (*.txt);;"
		"XML Seed File (*.xml);;");
	if (seedFileName.isEmpty())
	{
		return false;
	}

	int kernelIdx = dlg.getComboBoxIndices()[0];
	gamma = dlg.getDoubleSpinBoxValues()[1];
	dimension = dlg.getSpinBoxValues()[2];
	r = dlg.getDoubleSpinBoxValues()[3];
	c = dlg.getDoubleSpinBoxValues()[4];

	vtkSmartPointer<vtkImageData> img = activeChild->GetModality(0)->GetImage();
	int dim[3];
	img->GetDimensions(dim);
	QString seedString;

	iASeedsPointer seeds;
	if (seedFileName.endsWith("xml"))
	{
		// move to common location from GEMSe!
		DEBUG_LOG("XML file loading not yet implemented!");
		return false;
	}
	else
	{
		seeds = ExtractSeedVector(seedString, dim[0], dim[1], dim[2]);
	}

	PrepareResultChild("SVM");
	iASVMImageFilter * svm = new iASVMImageFilter(m_childData.imgData, activeChild->getLogger());
	svm->SetParameters(kernelIdx, c, gamma, dimension, r);
	svm->SetSeeds(seeds);
	for (int i = 0; i < activeChild->GetModalities()->size(); ++i)
	{
		QSharedPointer<iAModality> mod = activeChild->GetModality(i);
		for (int m = 0; m < mod->ComponentCount(); ++m)
		{
			svm->AddInput(mod->GetComponent(m));
		}
	}
	//m_mdiChild->connectThreadSignalsToChildSlots(svm);
	svm->start();
	connect(svm, SIGNAL(finished()), this, SLOT(SVMFinished()));
	return true;
}

void iASVMModuleInterface::SVMFinished()
{
	iASVMImageFilter* svm = qobject_cast<iASVMImageFilter*>(QObject::sender());
	iASVMImageFilter::ImagesPointer probImg = svm->GetResult();

	QSharedPointer<iAModalityList> mods(new iAModalityList);
	for (int p = 0; p < probImg->size(); ++p)
	{
		QSharedPointer<iAModality> mod(new iAModality(QString("SVM Probability Label %1").arg(p),
			QString(""), -1, probImg->at(p), iAModality::MainRenderer));
		mods->Add(mod);
	}
	m_mdiChild->SetModalities(mods);
}