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
	if (modality->ComponentCount() > 1)
	{
		lbChannel->setText("Components");
		edChannel->setText(QString("%1").arg(modality->ComponentCount()));
	}
	else
	{
		edChannel->setText(QString("%1").arg(modality->GetChannel()));
	}
	cbMagicLens->setChecked(modality->hasRenderFlag(iAModality::MagicLens));
	cbBoundingBox->setChecked(modality->hasRenderFlag(iAModality::BoundingBox));

	double const * orientation = modality->GetRenderer()->GetOrientation();
	double const * position = modality->GetRenderer()->GetPosition();
	double const * spacing = modality->GetSpacing();
	double const * origin = modality->GetOrigin();

	edPositionX   ->setText(QString::number(position[0]));
	edPositionY   ->setText(QString::number(position[1]));
	edPositionZ   ->setText(QString::number(position[2]));
	edOrientationX->setText(QString::number(orientation[0]));
	edOrientationY->setText(QString::number(orientation[1]));
	edOrientationZ->setText(QString::number(orientation[2]));
	edOriginX     ->setText(QString::number(origin[0]));
	edOriginY     ->setText(QString::number(origin[1]));
	edOriginZ     ->setText(QString::number(origin[2]));
	edSpacingX    ->setText(QString::number(spacing[0]));
	edSpacingY    ->setText(QString::number(spacing[1]));
	edSpacingZ    ->setText(QString::number(spacing[2]));

	connect(pbOK, SIGNAL(clicked()), this, SLOT(OKButtonClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

double getValueAndCheck(QLineEdit * le, QString const & caption, QStringList & notOKList)
{
	bool isOK;
	double returnVal = le->text().toDouble(&isOK);
	if (!isOK) {
		notOKList.append(caption);
	}
	return returnVal;
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
	QStringList notOKValues;
	position[0]    = getValueAndCheck(edPositionX   , "Position X"   , notOKValues);
	position[1]    = getValueAndCheck(edPositionY   , "Position Y"   , notOKValues);
	position[2]    = getValueAndCheck(edPositionZ   , "Position Z"   , notOKValues);
	orientation[0] = getValueAndCheck(edOrientationX, "Orientation X", notOKValues);
	orientation[1] = getValueAndCheck(edOrientationY, "Orientation Y", notOKValues);
	orientation[2] = getValueAndCheck(edOrientationZ, "Orientation Z", notOKValues);
	spacing[0]     = getValueAndCheck(edSpacingX    , "Spacing X"    , notOKValues);
	spacing[1]     = getValueAndCheck(edSpacingY    , "Spacing Y"    , notOKValues);
	spacing[2]     = getValueAndCheck(edSpacingZ    , "Spacing Z"    , notOKValues);
	origin[0]      = getValueAndCheck(edOriginX     , "Origin X"     , notOKValues);
	origin[1]      = getValueAndCheck(edOriginY     , "Origin Y"     , notOKValues);
	origin[2]      = getValueAndCheck(edOriginZ     , "Origin Z"     , notOKValues);
	if (notOKValues.size() > 0) {
		lbError->setText(QString("One or mor values are not valid: %1").arg(notOKValues.join(",")));
		return;
	}
	m_modality->SetOrigin(origin);
	m_modality->SetSpacing(spacing);
	m_modality->GetRenderer()->SetOrientation(orientation);
	m_modality->GetRenderer()->SetPosition(position);
	done(QDialog::Accepted);
}
