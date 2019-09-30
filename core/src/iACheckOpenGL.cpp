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

#include <QOpenGLFunctions>
#include <QString>
#include <QWindow>

bool checkOpenGLVersion(QString & msg)
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
			msg = "Creating OpenGL context failed!";
			delete context;
			return false;
		}
		test.show();
		if (!context->makeCurrent(&test))
		{
			msg = "Making OpenGL context current failed!";
			delete context;
			return false;
		}
		QOpenGLFunctions *f = context->functions();
		if (!f)
		{
			msg = "Fetching OpenGL functions failed!";
			delete context;
			return false;
		}
		const char *openGLVersion = reinterpret_cast<const char *>(f->glGetString(GL_VERSION));
		if (!openGLVersion)
		{
			msg = "No version available!";
			delete context;
			return false;
		}
		QString qopenGLVersion(openGLVersion);
		auto versionStringParts = qopenGLVersion.split(" ");
		if (qopenGLVersion.size() == 0 || versionStringParts.size() < 1)
		{
			msg = QString("Invalid OpenGL version string: %1").arg(openGLVersion);
			delete context;
			return false;
		}
		auto majorMinorPatch = versionStringParts[0].split(".");
		if (majorMinorPatch.size() < 2)
		{
			msg = QString("Invalid OpenGL version format; expected 3 parts (Major.Minor.Patch) but got %1 (%2).")
					.arg(majorMinorPatch.size())
					.arg(versionStringParts[0]);
			delete context;
			return false;
		}
		bool ok1, ok2;
		int major = majorMinorPatch[0].toInt(&ok1);
		int minor = majorMinorPatch[1].toInt(&ok2);
		if (!ok1 || !ok2)
		{
			msg = QString("OpenGL version does not consist of valid integer numbers: %1.").arg(versionStringParts[0]);
			delete context;
			return false;
		}
#if (defined(VTK_OPENGL2_BACKEND))
		const int minMajor = 3;
		const int minMinor = 2;
#else
		const int minMajor = 1;
		const int minMinor = 1;
#endif
		if (major < minMajor || (major == minMajor && minor < minMinor))
		{
			msg = QString("The OpenGL version currently available on your system (%1.%2) is insufficient, required is at least %3.%4.\n\n"
				"If you are using Remote Desktop, you might be able to get better OpenGL support when using other remote access software "
				"such as VNC, TeamViewer, NoMachine or similar. "
#if (defined(VTK_OPENGL2_BACKEND))
				"You can also use the compatibility version with 'c' suffix, or if you have built yourself, "
				"compile VTK with 'OpenGL' VTK_RENDERING_BACKEND instead of 'OpenGL2' "
				"(note that the 'OpenGL' option is only available up until VTK version 8.1.2, it is not available in newer versions). "
#endif
				"It might also help to update the display drivers; "
				"or if you have the chance, use the program on newer hardware.")
				.arg(major).arg(minor).arg(minMajor).arg(minMinor);
			delete context;
			return false;
		}

		delete context;
	}
	catch (std::exception const & e)
	{
		msg = QString("Error checking OpenGL version: %1").arg(e.what());
		return false;
	}
	return true;
}
