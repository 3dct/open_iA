#include "iANModalWidget.h"

#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QHBoxLayout>

iANModalWidgetAttachment::iANModalWidgetAttachment(MainWindow * mainWnd, iAChildData childData) :
	iAModuleAttachmentToChild(mainWnd, childData)
{
	// Do nothing
}

iANModalWidgetAttachment* iANModalWidgetAttachment::create(MainWindow * mainWnd, iAChildData childData) {
	auto newAttachment = new iANModalWidgetAttachment(mainWnd, childData);
	return newAttachment;
}

void iANModalWidgetAttachment::start() {
	if (!m_nModalWidget) {
		m_nModalWidget = new iANModalWidget(m_childData.child);
		m_childData.child->tabifyDockWidget(m_childData.child->logs, m_nModalWidget);
	}
	m_nModalWidget->show();
	m_nModalWidget->raise();
}

void iANModalWidgetModuleInterface::Initialize() {
	if (!m_mainWnd) // if m_mainWnd is not set, we are running in command line mode
		return;     // in that case, we do not do anything as we can not add a menu entry there
	QMenu *toolsMenu = m_mainWnd->getToolsMenu();
	QMenu *menuMultiModalChannel = getMenuWithTitle(toolsMenu, QString("Multi-Modal/-Channel Images"), false);

	QAction *action = new QAction(m_mainWnd);
	action->setText(QApplication::translate("MainWindow", "n-Modal Transfer Function", 0));
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, action, true);
	connect(action, SIGNAL(triggered()), this, SLOT(onMenuItemSelected()));
}

iAModuleAttachmentToChild* iANModalWidgetModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData) {
	return iANModalWidgetAttachment::create(mainWnd, childData);
}

void iANModalWidgetModuleInterface::onMenuItemSelected() {
	PrepareActiveChild();
	auto attach = GetAttachment<iANModalWidgetAttachment>();
	if (!attach)
	{
		AttachToMdiChild(m_mdiChild);
		attach = GetAttachment<iANModalWidgetAttachment>();
		if (!attach)
		{
			DEBUG_LOG("Attaching failed!");
			return;
		}
	}
	attach->start();
}

iANModalWidget::iANModalWidget(MdiChild *mdiChild) {

}