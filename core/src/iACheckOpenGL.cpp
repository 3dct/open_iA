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
#include "iAConsole.h"

#include <QOpenGLFunctions>
#include <QWindow>

bool checkOpenGLVersion()
{
	try
	{
		QWindow test;
		QSurfaceFormat fmt;
		fmt.setVersion(1, 0);
		test.setSurfaceType(QWindow::OpenGLSurface);
		test.setFormat(fmt);
		auto context = new QOpenGLContext(&test);
		context->setFormat(test.requestedFormat());
		if (!context->create())
		{
			DEBUG_LOG("Creating OpenGL context failed!");
			delete context;
			return false;
		}
		test.show();
		if (!context->makeCurrent(&test))
		{
			DEBUG_LOG("Making OpenGL context current failed!");
			delete context;
			return false;
		}
		QOpenGLFunctions *f = context->functions();
		if (!f)
		{
			DEBUG_LOG("Fetching OpenGL functions failed!");
			delete context;
			return false;
		}
		const char *openGLVersion = reinterpret_cast<const char *>(f->glGetString(GL_VERSION));
		DEBUG_LOG(QString("OpenGL Version: %1").arg(openGLVersion ? openGLVersion : "null"));
		delete context;
	}
	catch (std::exception const & e)
	{
		DEBUG_LOG(QString("Error checking OpenGL version: %1").arg(e.what()));
		return false;
	}
	return true;
}