/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAAbstractDiagramData.h"
#include "iAAbstractDrawableFunction.h"
#include "iADiagramWidget.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>
#include <QPoint>
#include <QString>

#include <vector>

class QDomNode;
class QMenu;
class QPaintEvent;
class QPainter;

class vtkPiecewiseFunction;
class vtkColorTransferFunction;

class dlg_function;
class iAFunctionChangeListener;
class MdiChild;

class open_iA_Core_API iADiagramFctWidget : public iADiagramWidget
{
	Q_OBJECT
public:
	enum AdditionalMode { MOVE_NEW_POINT_MODE=Y_ZOOM_MODE+1 };
	
	static const int SELECTED_POINT_RADIUS = 10;
	static const int SELECTED_POINT_SIZE = 2*SELECTED_POINT_RADIUS;
	static const int POINT_RADIUS = 4;
	static const int POINT_SIZE = 2*POINT_RADIUS;
	//static const int OFFSET_FROM_TOP = 5; // Offset from the highest bar to the top in pixels AMA 08.03.2010
	static const int TEXT_Y = 15;
	static const int TEXT_X = 15;
	
	static const int SELECTED_PIE_RADIUS = 16;
	static const int SELECTED_PIE_SIZE = 2 * SELECTED_PIE_RADIUS;
	static const int PIE_RADIUS = 16;
	static const int PIE_SIZE = 2 * PIE_RADIUS;

	enum DrawModeType {
		Linear,
		Logarithmic
	};

	iADiagramFctWidget(QWidget *parent, MdiChild *mdiChild, vtkPiecewiseFunction* oTF, vtkColorTransferFunction* cTF,
		QString const & label = "Greyvalue", QString const & yLabel = "");
	virtual ~iADiagramFctWidget();

	int getSelectedFuncPoint() const;
	bool isFuncEndPoint(int index) const;
	bool isUpdateAutomatically() const;

	virtual void drawEverything();
	void redraw();

	void updateTransferFunctions(vtkColorTransferFunction* ctf, vtkPiecewiseFunction* pwf);

	dlg_function *getSelectedFunction();
	int getHeight() const;
	int getChartHeight() const;
	std::vector<dlg_function*> &getFunctions();
	iAAbstractDiagramData::DataType getMax();

	virtual QSharedPointer<iAAbstractDiagramRangedData> GetData() =0;
	virtual QSharedPointer<iAAbstractDiagramRangedData> const GetData() const =0;
	
	void GetDataRange(double* range);
	double GetDataRange();

	void setColorTransferFunctionChangeListener(iAFunctionChangeListener* listener);

	void AddDataset(QSharedPointer<iAAbstractDrawableFunction> dataset);
	void AddImageOverlay(QSharedPointer<QImage> imgOverlay);
	void RemoveImageOverlay(QImage * imgOverlay);
	void RemoveDataset(QSharedPointer<iAAbstractDrawableFunction> dataset);

	void UpdatePrimaryDrawer();
	void SetShowPrimaryDrawer(bool showPrimaryDrawer);
	
	void SetYDrawMode(DrawModeType drawMode);

	QSharedPointer<CoordinateConverter> const GetCoordinateConverter() const;

	void SetXAxisSteps( int xSteps );
	void SetYAxisSteps( int ySteps );

	void SetRequiredPlacesAfterComma( int requiredPlaces );

	void SetAllowTrfReset(bool allow);
	void SetEnableAdditionalFunctions(bool enable);

	int GetTFGradientHeight() const;

	virtual int getBottomMargin() const;

	bool IsDrawnDiscrete() const;

protected:
	void paintEvent(QPaintEvent * );
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event); 
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void enterEvent(QEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);

	QPoint        lastpoint;
	MdiChild*      activeChild;
	QMenu*        contextMenu;
	QAction*      contextMenuResult;
	QPoint        contextPos;

	std::vector<dlg_function*> functions;

	bool contextMenuVisible;
	bool updateAutomatically;
	unsigned int selectedFunction;
	double min_intensity[3];
	double max_intensity[3];

	QString xCaption, yCaption;

	virtual void drawDatasets(QPainter &painter);
	virtual void drawImageOverlays(QPainter &painter);
	virtual void drawAxes(QPainter &painter);
	virtual void changeMode(int mode, QMouseEvent *event);

	// conversion between screen and data coordinates/bins:
	int diagram2PaintX(double x);
	long screenX2DataBin(int x);
	int  dataBin2ScreenX(long x);

signals:
	void updateViews();
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void active();
	void autoUpdateChanged(bool toggled);
	void applyTFForAll();
	void DblClicked();

public slots:
	int deletePoint();
	void changeColor(QMouseEvent *event = NULL);
	void autoUpdate(bool toggled);
	void resetView();
	void resetTrf();
	void updateTrf();
	bool loadTransferFunction();
	void loadTransferFunction(QDomNode &functionsNode);
	bool saveTransferFunction();
	void applyTransferFunctionForAll();
	void addBezierFunction();
	void addGaussianFunction();
	bool loadFunctions();
	bool saveFunctions();
	void removeFunction();
protected:
	virtual void drawFunctions(QPainter &painter);
	virtual QString GetXAxisCaption(double value, int placesBeforeComma, int requiredPlacesAfterComma);

	QFlags<Qt::AlignmentFlag> m_captionPosition;
	bool                      m_showXAxisLabel;
	bool                      m_showFunctions;
private:
	QList< QSharedPointer< QImage > >						m_overlays;
	QVector< QSharedPointer< iAAbstractDrawableFunction > >	m_datasets;
	QSharedPointer< iAAbstractDrawableFunction >			m_primaryDrawer;
	bool													m_showPrimaryDrawer;
	bool													m_allowTrfReset;
	bool													m_enableAdditionalFunctions;
	DrawModeType											m_yDrawMode;
	QSharedPointer<CoordinateConverter>						m_yConverter;
	
	int m_xAxisSteps;
	int m_yAxisSteps;
	int m_requiredPlacesAfterComma;

	virtual QSharedPointer<iAAbstractDrawableFunction> CreatePrimaryDrawer();

	virtual void drawXAxis(QPainter &painter);
	virtual void drawYAxis(QPainter &painter);
	virtual double getMaxXZoom() const;
	virtual void selectBin(QMouseEvent *event);


};
