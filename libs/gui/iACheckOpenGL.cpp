// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QOpenGLFunctions>
#include <QString>
#include <QWindow>

bool checkOpenGLVersion(QString & msg)
{
	try
	{
		QWindow test;
		QSurfaceFormat fmt;
		fmt.setVersion(3, 2);
		fmt.setProfile(QSurfaceFormat::CoreProfile);
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
		const int minMajor = 3;
		const int minMinor = 2;
//		std::cout << "OpenGL version: " << major << "." << minor << std::endl;
		if (major < minMajor || (major == minMajor && minor < minMinor))
		{
			msg = QString("The OpenGL version currently available on your system (%1.%2) is insufficient, required is at least %3.%4.\n\n"
				"If you are using Remote Desktop, you might be able to get better OpenGL support when using other remote access software "
				"such as VNC, TeamViewer, NoMachine or similar. "
				"It might also help to update the display drivers; "
				"or if you have the chance, use the program on newer hardware."
#ifndef _WIN32
				"\n\nIf you are running with MESA OpenGL, you can bypass this warning by setting MESA_GL_VERSION_OVERRIDE to at least %3.%4."
				" Be warned that this might result in unexpected glitches."
#endif
				)
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
