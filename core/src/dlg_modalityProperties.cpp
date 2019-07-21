/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_modalityProperties.h"

#include "iAModality.h"
#include "iAToolsVTK.h"
#include "iAVolumeRenderer.h"
#include "mainwindow.h"

#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkRenderer.h>

dlg_modalityProperties::dlg_modalityProperties(QWidget * parent, QSharedPointer<iAModality> modality):
	dlg_modalityPropertiesUI(parent), m_modality(modality)
{
	edName->setText(modality->name());
	edFilename->setText(modality->fileName());

	if (modality->componentCount() > 1)
	{
		lbChannel->setText("Components");
		edChannel->setText(QString("%1").arg(modality->componentCount()));
	}
	else
	{
		edChannel->setText(QString("%1").arg(modality->channel()));
	}

	cbMagicLens->setChecked(modality->hasRenderFlag(iAModality::MagicLens));
	cbBoundingBox->setChecked(modality->hasRenderFlag(iAModality::BoundingBox));
	cbShowInSlicer->setChecked(modality->hasRenderFlag(iAModality::Slicer));

	double const * orientation = modality->renderer()->orientation();
	double const * spacing = modality->spacing();
	double const * origin = modality->origin();

	edOrientationX->setText(QString::number(orientation[0]));
	edOrientationY->setText(QString::number(orientation[1]));
	edOrientationZ->setText(QString::number(orientation[2]));
	edOriginX     ->setText(QString::number(origin[0]));
	edOriginY     ->setText(QString::number(origin[1]));
	edOriginZ     ->setText(QString::number(origin[2]));
	edSpacingX    ->setText(QString::number(spacing[0]));
	edSpacingY    ->setText(QString::number(spacing[1]));
	edSpacingZ    ->setText(QString::number(spacing[2]));

	iAVolumeSettings const & vs = m_modality->renderer()->volumeSettings();
	cb_LinearInterpolation->setChecked(vs.LinearInterpolation);
	cb_Shading->setChecked(vs.Shading);
	ed_SampleDistance->setText(QString::number(vs.SampleDistance));
	ed_AmbientLighting->setText(QString::number(vs.AmbientLighting));
	ed_DiffuseLighting->setText(QString::number(vs.DiffuseLighting));
	ed_SpecularLighting->setText(QString::number(vs.SpecularLighting));
	ed_SpecularPower->setText(QString::number(vs.SpecularPower));
	ed_ScalarOpacityUnitDistance->setText(QString::number(vs.ScalarOpacityUnitDistance));
	cb_RenderMode->setCurrentText(RenderModeMap().value(vs.RenderMode));

	connect(pbOK, SIGNAL(clicked()), this, SLOT(OKButtonClicked()));
	connect(pbCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

double getValueAndCheck(QLineEdit * le, QString const & caption, QStringList & notOKList)
{
	bool isOK;
	double returnVal = le->text().toDouble(&isOK);
	if (!isOK)
		notOKList.append(caption);
	return returnVal;
}

void dlg_modalityProperties::OKButtonClicked()
{
	m_modality->setName(edName->text());
	bool hasRenderFlag = m_modality->hasRenderFlag(iAModality::MainRenderer);
	m_modality->setRenderFlag(
		(cbMagicLens->isChecked() ? iAModality::MagicLens : 0) |
		(hasRenderFlag ? iAModality::MainRenderer : 0) |
		(cbBoundingBox->isChecked() ? iAModality::BoundingBox : 0) |
		(cbShowInSlicer->isChecked() ? iAModality::Slicer : 0)
	);
	double orientation[3];
	double spacing[3];
	double origin[3];
	QStringList notOKValues;
	orientation[0] = getValueAndCheck(edOrientationX, "Orientation X", notOKValues);
	orientation[1] = getValueAndCheck(edOrientationY, "Orientation Y", notOKValues);
	orientation[2] = getValueAndCheck(edOrientationZ, "Orientation Z", notOKValues);
	spacing[0]     = getValueAndCheck(edSpacingX    , "Spacing X"    , notOKValues);
	spacing[1]     = getValueAndCheck(edSpacingY    , "Spacing Y"    , notOKValues);
	spacing[2]     = getValueAndCheck(edSpacingZ    , "Spacing Z"    , notOKValues);
	origin[0]      = getValueAndCheck(edOriginX     , "Origin X"     , notOKValues);
	origin[1]      = getValueAndCheck(edOriginY     , "Origin Y"     , notOKValues);
	origin[2]      = getValueAndCheck(edOriginZ     , "Origin Z"     , notOKValues);

	m_volumeSettings.LinearInterpolation = cb_LinearInterpolation->isChecked();
	m_volumeSettings.Shading = cb_Shading->isChecked();
	m_volumeSettings.SampleDistance = getValueAndCheck(ed_SampleDistance, "Sample Distance", notOKValues);
	m_volumeSettings.AmbientLighting = getValueAndCheck(ed_AmbientLighting, "AmbientLighting", notOKValues);
	m_volumeSettings.DiffuseLighting = getValueAndCheck(ed_DiffuseLighting, "DiffuseLighting", notOKValues);
	m_volumeSettings.SpecularLighting = getValueAndCheck(ed_SpecularLighting, "SpecularLighting", notOKValues);
	m_volumeSettings.SpecularPower = getValueAndCheck(ed_SpecularPower, "SpecularPower", notOKValues);
	m_volumeSettings.ScalarOpacityUnitDistance = getValueAndCheck(ed_ScalarOpacityUnitDistance, "ScalarOpacityUnitDistance", notOKValues);
	m_volumeSettings.RenderMode = mapRenderModeToEnum(cb_RenderMode->currentText());

	if (notOKValues.size() > 0)
	{
		lbError->setText(QString("One or mor values are not valid: %1").arg(notOKValues.join(",")));
		return;
	}

	double const * oldSpacing = m_modality->spacing();
	m_spacingChanged = false;
	
	for (int i = 0; i < 3; i++)
	{
		if (oldSpacing[i] != spacing[i])
		{
			m_spacingChanged = true;
			break;
		}
	}

	m_currentSpacing = m_modality->spacing();

	m_modality->setOrigin(origin);
	m_modality->setSpacing(spacing);
	m_modality->renderer()->setOrientation(orientation);
	//m_modality->renderer()->setPosition(position);
	m_modality->renderer()->applySettings(m_volumeSettings);
	done(QDialog::Accepted);
}

bool dlg_modalityProperties::spacingChanged()
{
	return m_spacingChanged;
}

double const * dlg_modalityProperties::newSpacing()
{
	return m_currentSpacing;
}