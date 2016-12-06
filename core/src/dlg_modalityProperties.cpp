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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_modalityProperties.h"

#include "iAModality.h"
#include "iAVolumeRenderer.h"

dlg_modalityProperties::dlg_modalityProperties(QWidget * parent, QSharedPointer<iAModality> modality):
	dlg_modalityPropertiesUI(parent),
	m_modality(modality)

{
	edName->setText(modality->GetName());
	edFilename->setText(modality->GetFileName());
	edChannel->setText(QString("%1").arg(modality->GetChannel()));
	cbMagicLens->setChecked(modality->hasRenderFlag(iAModality::MagicLens));
	cbBoundingBox->setChecked(modality->hasRenderFlag(iAModality::BoundingBox));

	double const * orientation = modality->GetRenderer()->GetOrientation();
	double const * position = modality->GetRenderer()->GetPosition();

	double const * spacing = modality->GetSpacing();
	double const * origin = modality->GetOrigin();

	double minSpacing = std::min(modality->GetSpacing()[0], std::min(modality->GetSpacing()[1], modality->GetSpacing()[2]));
	int placesAfterComma = 1 - (std::min(static_cast<int>(std::log10(minSpacing)), 0));

	dsbPositionX->setValue(position[0]);
	dsbPositionY->setValue(position[1]);
	dsbPositionZ->setValue(position[2]);

	dsbOrientationX->setValue(orientation[0]);
	dsbOrientationY->setValue(orientation[1]);
	dsbOrientationZ->setValue(orientation[2]);

	dsbOriginX->setValue(origin[0]);
	dsbOriginY->setValue(origin[1]);
	dsbOriginZ->setValue(origin[2]);

	dsbSpacingX->setValue(spacing[0]);
	dsbSpacingY->setValue(spacing[1]);
	dsbSpacingZ->setValue(spacing[2]);

	connect(pbOK, SIGNAL(clicked()), this, SLOT(OKButtonClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));

}

void dlg_modalityProperties::OKButtonClicked()
{
	m_modality->SetName(edName->text());
	m_modality->SetRenderFlag(
		(cbMagicLens->isChecked() ? iAModality::MagicLens : 0) |
		iAModality::MainRenderer |  
		(cbBoundingBox->isChecked() ? iAModality::BoundingBox : 0)
	);
	double orientation[3];
	double position[3];
	double spacing[3];
	double origin[3];
	position[0] = dsbPositionX->value();
	position[1] = dsbPositionY->value();
	position[2] = dsbPositionZ->value();

	orientation[0] = dsbOrientationX->value();
	orientation[1] = dsbOrientationY->value();
	orientation[2] = dsbOrientationZ->value();

	spacing[0] = dsbSpacingX->value();
	spacing[1] = dsbSpacingY->value();
	spacing[2] = dsbSpacingZ->value();

	origin[0] = dsbOriginX->value();
	origin[1] = dsbOriginY->value();
	origin[2] = dsbOriginZ->value();

	m_modality->SetOrigin(origin);
	m_modality->SetSpacing(spacing);
	m_modality->GetRenderer()->SetOrientation(orientation);
	m_modality->GetRenderer()->SetPosition(position);
	done(QDialog::Accepted);
}
