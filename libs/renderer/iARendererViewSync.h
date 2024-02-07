// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iarenderer_export.h"

#include <QMap>

#include <vector>

class vtkCamera;
class vtkObject;
class vtkRenderer;

//! Class synchronizing the viewing parameters between multiple renderers.
//! Two "modes":
//! - Shared camera (default, i.e. sharedCamera = true in constructor),
//!   each added renderer gets assigned the same camera (the one from the
//!   renderer added first).
//!   Pro: Less overhead on updates (only triggering an update on other renderer windows)
//!   Con: The views share the visible area; so if the renderers have different
//!        pixel dimensions, viewed objects might be distorted (i.e., the aspect ratio is
//!        only correct in one window if they have different width/height ratios)
//! - Synchronized camera settings: An observer is registered to each
//!   added renderer's camera, and whenever in one of them, the camera is modified,
//!   its main camera parameters (camera position, focal point, view up
//!   direction, clipping planes, and parallel scale if source camera has
//!   parallel projection turned on) are copied to all other renderers.
//!   Pro: Avoids sharing "too much", i.e. avoids distortions and renderers can have
//!        separate visible areas
//!   Con: Slightly more (coding) effort to synchronize relevant viewing parameters
//!        (and potentially error prone - relevant parameters for specific viewing
//!        modes might be missed)
class iArenderer_API iARendererViewSync
{
public:
	//! Creates a new renderer view synchronization service.
	//! @param sharedCamera whether to use shared camera (true)
	//!        or just synchronize required view parameters (false).
	//!        see class description for details.
	iARendererViewSync(bool sharedCamera = true);

	//! destructor, disconnects from all renderers
	~iARendererViewSync();

	//! Adds given renderer to the bundle of renderers with synced views.
	//! Any change in that renderer's viewing parameters will be synced to all other
	//! renderers in the bundle.
	//! @param renderer the renderer to add to the bundle
	void addToBundle(vtkRenderer* renderer);
	//! Remove given renderer from bundle. Stops synchronizing this renderers'
	//! viewing parameters with the other renderers in the bundle.
	//! @param renderer the renderer to remove from the bundle
	//! @param resetCamera whether the camera of the removed renderer should be reset to a new one
	//!        with a copy of the settings of the common renderer; set to false for example if the
	//!        renderer is not used after removing from bundle, or if you set your own camera.
	//!        Note that when resetCamera=false, and you reuse the renderer afterwards, but don't
	//!        set your own camera to the renderer, camera updates will still be shared to other
	//!        renderers added to the bundle, but updates will not propagate anymore!
	bool removeFromBundle(vtkRenderer* renderer, bool resetCamera = true);
	//! Remove all renderers from the bundle synchronized by this class.
	void removeAll();

	//! whether this renderer view sync instance uses a shared camera or not
	bool sharedCamera() const;

private:
	//! @{ No copy construction or assignment
	iARendererViewSync(const iARendererViewSync&) = delete;
	iARendererViewSync& operator=(const iARendererViewSync&) = delete;
	//! @}

	//! callback for when one renderer is updated. updates viewing parameters in all
	//! other renderers in the bundle
	void redrawOtherRenderers(vtkObject* caller, long unsigned int eventId, void* callData);

	bool m_updateInProgress;    //!< avoids recursion in redrawOtherRenderers.
	vtkCamera* m_commonCamera;  //!< the common camera (if m_sharedCamera is true).
	bool m_sharedCamera;        //!< the viewing parameter share mode
	std::vector<vtkRenderer*> m_renderers;
	QMap<vtkCamera*, unsigned long> m_rendererObserverTags; //!< list of observed cameras and associated observer tags
};
