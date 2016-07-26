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
 
#include "pch.h"
#include "dlg_modalityProperties.h"

#include "iAIOProvider.h"
#include "iAModality.h"

#include <QFileDialog>

dlg_modalityProperties::dlg_modalityProperties(QWidget * parent, QSharedPointer<iAModality> modality):
	dlg_modalityPropertiesUI(parent),
	m_modality(modality)
{
	edName->setText(modality->GetName());
	edFilename->setText(modality->GetFileName());
	cbMagicLens->setChecked(modality->hasRenderFlag(iAModality::MagicLens));
	cbMainRenderer->setChecked(modality->hasRenderFlag(iAModality::MainRenderer));
	cbBoundingBox->setChecked(modality->hasRenderFlag(iAModality::BoundingBox));
	connect(pbChooseFile, SIGNAL(clicked()), this, SLOT(FileChooserClicked()));
	connect(pbOK, SIGNAL(clicked()), this, SLOT(OKButtonClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void dlg_modalityProperties::OKButtonClicked()
{
	m_modality->SetName(edName->text());
	m_modality->SetFileName(edFilename->text());
	m_modality->SetRenderFlag(
		(cbMagicLens->isChecked() ? iAModality::MagicLens : 0) |
		(cbMainRenderer->isChecked() ? iAModality::MainRenderer : 0) |
		(cbBoundingBox->isChecked() ? iAModality::BoundingBox : 0)
	);
	if (m_modality->LoadData())
	{
		done(QDialog::Accepted);
	}
}

void dlg_modalityProperties::FileChooserClicked()
{
	QFileDialog dlg;
	QString fileName = QFileDialog::getOpenFileName(this, tr("Load"),
		edFilename->text(),
		iAIOProvider::GetSupportedLoadFormats() + tr("Volume Stack (*.volstack);;" ) );
	if (fileName != "")
		edFilename->setText(fileName);
}
