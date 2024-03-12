// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkObject;
class vtkRenderer;
class vtkPolyData;
class vtkPoints;
class vtkPolyDataMapper;
class vtkActor;
class vtkFloatArray;
class vtkIntArray;
class vtkLookupTable;
class vtkScalarBarActor;
class vtkScalarBarWidget;
class vtkDepthSortPolyData;
class vtkPointPicker;
class vtkCubeAxesActor2D;

struct iAPICKDATA
{
	unsigned int xInd, zInd;
	double pos[3];
	long pntnum;
};

class iAPlot3DVtk
{
public:
	iAPlot3DVtk();
	~iAPlot3DVtk();
	//TODO: some functions might be not used, throw out all the rubbish
	void loadFromData(double * plotData, double * scalars, int cntX, int cntZ, float scale = 1.0);
	void SetSolidColor(double r, double g, double b);
	void SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
	void Update();
	void SetPalette(int count, double *colors);
	void SetPalette(int count, unsigned int r1, unsigned int g1, unsigned int b1, unsigned int r2, unsigned int g2, unsigned int b2);
	void SetGridCellNumbers(long *data);
	void SetAutoScalarRange();//TODO: not used?
	void SetUserScalarRange(double _Smin, double _Smax);//TODO: not used
	void ShowWireGrid(int show, float r, float g, float b);
	void SetOpacity(double opacity);
	void RenderWthCorrectTransparency(int correctTransparency);
	void Pick(double xpos, double ypos);
	int GetShowGrid(){return m_showgrid;}
	void SetShowGrid(int showgrid){m_showgrid = showgrid;}
	vtkRenderer * GetRenderer(){ return m_renderer;}
	void SetAxesParams(int showaxes, int showlabels, double color[3], long fontfactor);
	void HighlightPickedPoint();
	long long GetNumberOfLookupTableValues();
	//! return false if points not set
	bool setPicked(int indX, int indZ);
private:
	int m_correctTransparency;
	int m_showgrid;
	vtkRenderer *m_renderer;
	vtkPolyData *m_grid;
	vtkPoints *m_points;
	vtkPolyDataMapper *m_mapper;
	vtkActor *m_actor;
	vtkPolyDataMapper *m_wireMapper;
	vtkActor *m_wireActor;
	vtkFloatArray *m_cellScalars;
	vtkIntArray *m_cellNumbers;
	vtkLookupTable *m_lookupTable;
	vtkScalarBarActor *m_scalarBarActor;
	vtkScalarBarWidget *m_scalarBarWidget;
	//vtkDepthSortPolyData *m_depthSort;
	vtkPointPicker *m_picker;
	vtkCubeAxesActor2D *m_cubeAxes;
	vtkPolyDataMapper *m_pickedMapper;
	vtkActor *m_pickedActor;
	double m_Smin;
	double m_Smax;
	double MinX,MaxX;
	double MinY,MaxY;
	double MinZ,MaxZ;
	unsigned int m_sizeX, m_sizeZ;
public:
	iAPICKDATA pickData;
	unsigned int lastPickSuccessful;
};
