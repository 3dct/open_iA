---
title: 'open_iA: A tool for the visual analysis and processing of volumetric datasets, with a focus on industrial computed tomography.'
tags:
  - C++
  - computed tomography
  - industrial computed tomography
  - image processing
  - volume processing
  - volume visualization
  - visual analytics
authors:
  - name: Bernhard Froehler
    orcid: 0000-0003-1271-0838
    affiliation: "1, 2"
  - name: Christoph Heinzl
    orcid: 0000-0002-3173-8871
    affiliation: "1"
  - name: Marcel Schiwarth
    orcid: 0000-xxxx-mmmm-nnnn
    affiliation: "1"
  - name: Johannes Weissenboeck
    orcid: 0000-0001-8062-4590
    affiliation: "1, 3"
affiliation:
  - name: University of Applied Sciences Upper Austria, Research Group Computed Tomography
    index: 1
  - name: University of Vienna, Research Group Visualization and Data Analysis
    index: 2
  - name: Technical University of Vienna, Visualization Working Group
    index: 3
date: 13 November 2018
bibliography: paper.bib
---
# Summary 
open_iA is a platform for visual analysis and processing of volumetric datasets. The main driver behind its development is to provide a common framework for performing visual analytics on industrial Computed Tomography (CT) data. It currently provides a variety of image processing filters, for example for noise reduction, segmentation, datatype conversion, convolution, geometric transformations, and morphological operations. open_iA is written in C++ using Qt, VKT and ITK, as well as some other open source libraries. open_iA is continuously improved and extended. It is open source and available on GitHub under https://github.com/3dct/open_iA.
The core of open_iA provides functionality for loading and displaying volumetric datasets in several file formats, as well as support for loading polygonal datasets. A comparison of volumes is facilitated through a magic lens as well as optional position indicators in all open child windows. In addition, it provides a view for showing the image histogram, where also the transfer function used for the slicer views and the 3D renderer is configured.

open_iA is highly extensible through what we call modules, which makes it an ideal platform for research prototypes and tools. These modules can contain anything from simple image processing filters to complex visual analytics tools.  Several publications so far were already based on modules implemented in the open_iA framework, and these modules are also included in the open_iA repository:

- DreamCaster [@Amirkhanov:2010] is a tool for finding the best scanning parameters in a Computed Tomography device for a given specimen.
- FiberScout [@Weissenboeck:2014], later extended to FeatureScout, is a tool for analyzing the properties of collections of similar objects.
- MObjects [@Reh:2013] now included in the FeatureScout tool, provides a way to visualize the average shape of a collection of similar objects, such as fibers or pores.
- The 4DCT tools [@Amirkhanov:2016] enable exploring multiple CT datasets from different stages of fatigue testing of fiber-reinforced polymers, where one can classify and analyze types of defects and defect formations.
- Fuzzy Feature Tracking [@Reh:2015] provides graphs for tracking the creation, continuation and merge of defects between different stages of fatigue testing.
- GEMSe [@Froehler:2016] supports users in finding optimal parameters for their volume segmentation tasks without requiring a ground truth.
- The PorosityAnalyzer [@Weissenboeck:2016] similarly supports users in finding the ideal segmentation algorithm and parameterization when they are determining porosity values for example in fiber-reinforced polymers.
- InSpectr [@Amirkhanov:2014} makes it possible to analyze spectral data, for example from X-Ray fluorescence spectral tomography, alongside with data from computed tomography for the same specimen.
- Dynamic Volume Lines [@Weissenboeck:2019] facilitate the comparison of multiple slightly varying volumetric datasets, by mapping them to 1D and applying a nonlinear scaling to highlight regions with large differences.

# Acknowledgements

The work leading to the results shown here was supported by the K-Project for “non-destructive testing and tomography plus” (ZPT+) and by the COMET program of FFG and the federal government of Upper Austria and Styria. It also received funding by the project "Multimodal and in-situ characterization of inhomogeneous materials" (MiCi) by the federal government of Upper Austria and the European Regional Development Fund (EFRE) in the framework of the EU-program IWB2020. Furthermore, it received funding from the FFG BRIDGE early stage project no. 851249, "Advanced multimodal data analysis and visualization of composites based on grating interferometer micro-CT data", as well as from the Research Foundation Flanders (FWO) and the Austrian Science Fund (FWF) project "Quantitative X-ray tomography of advanced polymer composites", under the grant numbers G0F9117N and I3261-N36 respectively.

# References