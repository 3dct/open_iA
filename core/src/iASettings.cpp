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
#include "iASettings.h"

#include "dlg_transfer.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QFile>
#include <QMessageBox>
#include <QTextStream>

Settings::Settings()
{
	QDomElement root = domDocument.createElement("settings");
	domDocument.appendChild(root);
}

Settings::Settings(QString const & filename)
{
	QFile file(filename);
	// TODO: better error handling!
	if (file.open(QIODevice::ReadOnly))
	{
		if (!domDocument.setContent(&file)) {
			QMessageBox msgBox;
			msgBox.setText("An error occurred during xml parsing!");
			msgBox.exec();
		}
		if (!domDocument.hasChildNodes() ||
			domDocument.documentElement().tagName() != "settings")
		{
			QDomElement root = domDocument.createElement("settings");
			domDocument.appendChild(root);
		}
	}
	else
	{
		QDomElement root = domDocument.createElement("settings");
		domDocument.appendChild(root);
	}
	file.close();
}

void Settings::LoadTransferFunction(iATransferFunction* transferFunction, double range[2])
{
	QDomElement root = domDocument.documentElement();
	QDomNode functionsNode = root.namedItem("functions");
	if (!functionsNode.isElement())
		return;
	QDomNode transferElement = functionsNode.namedItem("transfer");
	if (!transferElement.isElement())
		return;

	transferFunction->GetOpacityFunction()->RemoveAllPoints();
	transferFunction->GetColorFunction()->RemoveAllPoints();

	QDomNodeList list = transferElement.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);

		QDomNamedNodeMap attributes = node.attributes();
		double value = attributes.namedItem("value").nodeValue().toDouble();
		double opacity = attributes.namedItem("opacity").nodeValue().toDouble();
		double red = attributes.namedItem("red").nodeValue().toDouble();
		double green = attributes.namedItem("green").nodeValue().toDouble();
		double blue = attributes.namedItem("blue").nodeValue().toDouble();

		//if (value < range[0]) value = range[0];
		//if (value > range[1]) value = range[1];
		transferFunction->GetOpacityFunction()->AddPoint(value, opacity);
		transferFunction->GetColorFunction()->AddRGBPoint(value, red, green, blue);
	}
	transferFunction->GetColorFunction()->Build();
}

void Settings::StoreTransferFunction(iATransferFunction* transferFunction)
{
	// does functions node exist
	QDomNode functionsNode = domDocument.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = domDocument.createElement("functions");
		domDocument.documentElement().appendChild(functionsNode);
	}
	removeNode("transfer");

	QDomElement transferElement = domDocument.createElement("transfer");

	for (int i = 0; i < transferFunction->GetOpacityFunction()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->GetOpacityFunction()->GetNodeValue(i, opacityTFValue);
		transferFunction->GetColorFunction()->GetNodeValue(i, colorTFValue);

		QDomElement nodeElement = domDocument.createElement("node");
		nodeElement.setAttribute("value", QObject::tr("%1").arg(opacityTFValue[0]));
		nodeElement.setAttribute("opacity", QObject::tr("%1").arg(opacityTFValue[1]));
		nodeElement.setAttribute("red", QObject::tr("%1").arg(colorTFValue[1]));
		nodeElement.setAttribute("green", QObject::tr("%1").arg(colorTFValue[2]));
		nodeElement.setAttribute("blue", QObject::tr("%1").arg(colorTFValue[3]));
		transferElement.appendChild(nodeElement);
	}

	functionsNode.appendChild(transferElement);
}

void Settings::Save(QString const & filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QTextStream ts(&file);
	ts << domDocument.toString();
	file.close();
}

void Settings::removeNode(QString const & str)
{
	QDomNode rootNode = domDocument.documentElement();
	QDomNodeList list = rootNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		if (node.nodeName() == str)
		{
			rootNode.removeChild(node);
			break;
		}
	}
}
