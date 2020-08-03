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
#include "iAXmlSettings.h"

#include "iAConsole.h"
#include "iATransferFunction.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QFile>
#include <QMessageBox>
#include <QTextStream>

iAXmlSettings::iAXmlSettings()
{
	QDomElement root = domDocument.createElement("settings");
	domDocument.appendChild(root);
}

bool iAXmlSettings::read(QString const & filename)
{
	QFile file(filename);
	// TODO: better error handling!
	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}
	if (!domDocument.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();
	if (!domDocument.hasChildNodes() ||
		domDocument.documentElement().tagName() != "settings")
	{
		//QDomElement root = domDocument.createElement("settings");
		//domDocument.appendChild(root);
		return false;
	}
	return true;
}

void iAXmlSettings::loadTransferFunction(iATransferFunction* transferFunction)
{
	QDomElement root = domDocument.documentElement();
	QDomNode functionsNode = root.namedItem("functions");
	if (!functionsNode.isElement())
		return;
	iAXmlSettings::loadTransferFunction(functionsNode, transferFunction);
}

QDomNode iAXmlSettings::node(QString const & nodeName)
{
	QDomElement root = domDocument.documentElement();
	return root.namedItem(nodeName);
}

bool iAXmlSettings::hasElement(QString const & nodeName) const
{
	QDomNode node = domDocument.documentElement().namedItem(nodeName);
	return node.isElement();
}

QDomElement iAXmlSettings::createElement(QString const & elementName)
{
	return createElement(elementName, domDocument.documentElement());
}

QDomElement iAXmlSettings::createElement(QString const & elementName, QDomNode parent)
{
	auto element = domDocument.createElement(elementName);
	parent.appendChild(element);
	return element;
}

QDomElement iAXmlSettings::documentElement()
{
	return domDocument.documentElement();
}

QDomDocument & iAXmlSettings::document()
{
	return domDocument;
}

void iAXmlSettings::loadTransferFunction(QDomNode const & functionsNode, iATransferFunction* transferFunction)
{
	QDomNode transferNode = functionsNode.namedItem("transfer");
	if (!transferNode.isElement())
	{
		DEBUG_LOG("'transfer' node not found in given XML file, aborting load of transfer function!");
		return;
	}
	transferFunction->opacityTF()->RemoveAllPoints();
	transferFunction->colorTF()->RemoveAllPoints();
	QDomNodeList list = transferNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		QDomNamedNodeMap attributes = node.attributes();
		double value = attributes.namedItem("value").nodeValue().toDouble();
		double opacity = attributes.namedItem("opacity").nodeValue().toDouble();
		double red = attributes.namedItem("red").nodeValue().toDouble();
		double green = attributes.namedItem("green").nodeValue().toDouble();
		double blue = attributes.namedItem("blue").nodeValue().toDouble();
		//value = clamp(range[0], range[1], value);
		transferFunction->opacityTF()->AddPoint(value, opacity);
		transferFunction->colorTF()->AddRGBPoint(value, red, green, blue);
	}
	transferFunction->colorTF()->Build();
}

void iAXmlSettings::saveTransferFunction(iATransferFunction* transferFunction)
{
	QDomNode functionsNode = domDocument.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = createElement("functions");
	}
	QDomElement transferElement = domDocument.createElement("transfer");

	for (int i = 0; i < transferFunction->opacityTF()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->opacityTF()->GetNodeValue(i, opacityTFValue);
		transferFunction->colorTF()->GetNodeValue(i, colorTFValue);

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

void iAXmlSettings::save(QString const & filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QTextStream ts(&file);
	ts << domDocument.toString();
	file.close();
}

void iAXmlSettings::removeNode(QString const & str)
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
