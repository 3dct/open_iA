// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
	return MainWindow::runGUI(argc, argv, "open_iA", Open_iA_Version, BuildInfo,
		":/images/splashscreen.png", ":/images/ia.png");
}
