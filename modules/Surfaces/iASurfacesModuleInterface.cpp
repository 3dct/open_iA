// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASurfacesModuleInterface.h"

#include "iAGeometricObjectMappings.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAParameterDlg.h>

#include <QAction>
#include <QMessageBox>

namespace
{
	const QString Title("Add Polygon Object");
}

void iASurfacesModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction* actionAddObject = new QAction(Title, m_mainWnd);
	connect(actionAddObject, &QAction::triggered, this, &iASurfacesModuleInterface::addObject);
	m_mainWnd->makeActionChildDependent(actionAddObject);
	addToMenuSorted(m_mainWnd->toolsMenu(), actionAddObject);
}

void iASurfacesModuleInterface::addObject()
{
	auto child = m_mainWnd->activeMdiChild();
	if (!child)
	{
		QMessageBox::information(m_mainWnd, Title, "Requires an opened child window.");
		return;
	}
	iAAttributes params;
	QStringList objects = QStringList() << "Cube" << "Line" << "Sphere";
	addAttr(params, "Object type", iAValueType::Categorical, objects);
	iAParameterDlg dlg(m_mainWnd, "Object type", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	QString name = dlg.parameterValues()["Object type"].toString();
	auto source = createSource(name);
	auto dataSet = std::make_shared<iAGeometricObject>(name, std::move(source));
	child->addDataSet(dataSet);
}
