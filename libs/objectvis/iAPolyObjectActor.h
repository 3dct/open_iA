class vtkActor;
class vtkOutlineFilter;
class vtkPlane;
class vtkPolyDataMapper;

//! Displays some class derived from iA3DColoredPolyObjectVis
class iAPolyObjectActor
{
	iAPolyObjectActor(vtkRenderer* ren, QSharedPointer<iA3DColoredPolyObjectVis> obj);
	void show();
	void hide();
	//! Triggers an update of the color mapper and the renderer.
	void updatePolyMapper();
	//! Prepare the filters providing the bounding box.
	void setupBoundingBox();

	//! Updates the renderer; but only if the own actor is actually shown.
	void updateRenderer();

	virtual void setShowLines(bool /*lines*/)
	{
	}  // not ideal, should not be here
	void setClippingPlanes(vtkPlane* planes[3]);
	void removeClippingPlanes();

private:
	bool m_visible, m_clippingPlanesEnabled;
	vtkRenderer* m_ren;
	QSharedPointer<iA3DColoredPolyObjectVis> m_obj;

	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkActor> m_actor;

	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;
};
