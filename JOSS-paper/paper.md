---
title: 'open\_iA: A tool for processing and visual analysis of industrial computed tomography datasets'
tags:
  - C++
  - computed tomography
  - industrial computed tomography
  - image processing
  - volume processing
  - volume visualization
  - visual analytics
authors:
  - name: Bernhard Fröhler
    orcid: 0000-0003-1271-0838
    affiliation: "1, 2"
  - name: Johannes Weissenböck
    orcid: 0000-0001-8062-4590
    affiliation: "1, 3"
  - name: Marcel Schiwarth
    orcid: 0000-0003-2339-1879
    affiliation: "1"
  - name: Johann Kastner
    orcid: 0000-0002-3163-0916
    affiliation: "1"
  - name: Christoph Heinzl
    orcid: 0000-0002-3173-8871
    affiliation: "1"
affiliations:
  - name: University of Applied Sciences Upper Austria, Research Group Computed Tomography
    index: 1
  - name: University of Vienna, Research Group Visualization and Data Analysis
    index: 2
  - name: TU Wien, Visualization Working Group
    index: 3
date: December 21, 2018
bibliography: paper.bib
---
# Summary 
open\_iA is a platform for visual analysis and processing of volumetric datasets.
The main driver behind its development is to provide a common framework for performing visual analytics on industrial Computed Tomography (CT) data.
In contrast to general volume visualization or processing software, it offers specialized tools, which address domain-specific analysis scenarios such as porosity determination, fiber characterization and image processing parameter space analysis.
The wide range of building blocks, which these tools consist of, facilitate the development of new research prototypes in this application domain.
It currently provides a variety of image processing filters, e.g. for noise reduction, segmentation, data type conversion, convolution, geometric transformations, and morphological operations.
open\_iA is written in C++ using Qt, VKT and ITK, as well as some other open source libraries.
open\_iA is continuously improved and extended.
The core of open\_iA provides functionality for loading and displaying volumetric datasets in several file formats, as well as support for loading polygonal datasets.
A comparison of volumes is facilitated through a magic lens as well as optional position indicators in all open child windows.
In addition, it provides a view for showing the image histogram, where also the transfer function used for the slicer views and the 3D renderer is configured.

open\_iA is highly extensible through what we call modules, which makes it an ideal platform for research prototypes and tools.
These modules can contain anything from simple image processing filters to complex visual analytics tools.
Several publications so far were already based on modules implemented in the open\_iA framework.
The modules in the following list are included in the open\_iA repository:

- DreamCaster [@Amirkhanov:2010] is a tool for finding the best scanning parameters in a CT device for a given specimen.
- FiberScout [@Weissenboeck:2014], later extended to FeatureScout, is a tool for analyzing the properties of collections of similar objects.
- MObjects [@Reh:2013] now included in the FeatureScout tool, provides a way to visualize the average shape of a collection of similar objects, such as fibers or pores.
- The 4DCT tools [@Amirkhanov:2016] enable exploring multiple CT datasets from different stages of fatigue testing of fiber-reinforced polymers, where one can classify and analyze types of defects and defect formations.
- Fuzzy Feature Tracking [@Reh:2015] provides graphs for tracking the creation, continuation, and merge of defects between different stages of fatigue testing.
- GEMSe [@Froehler:2016] supports users in finding optimal parameters for their volume segmentation tasks without requiring a ground truth.
- The PorosityAnalyzer [@Weissenboeck:2016] similarly supports users in finding the ideal segmentation algorithm and parameterization when they are determining porosity values, e.g., in fiber-reinforced polymers.
- InSpectr [@Amirkhanov:2014] makes it possible to analyze spectral data, e.g., from X-Ray fluorescence spectral tomography, alongside with data from computed tomography for the same specimen.
- Dynamic Volume Lines [@Weissenboeck:2019] facilitate the comparison of multiple slightly varying volumetric datasets, by mapping them to 1D and applying a nonlinear scaling to highlight regions with large differences.
- With MetaTracts [@Bhattacharya:2015, @Bhattacharya:2017] one can characterize and analyze fiber bundles as well as weaving patterns in fiber-reinforced polymers.

# Acknowledgments

The work leading to the results shown here was supported by the K-Project for “non-destructive testing and tomography plus” (ZPT+) and by the COMET program of FFG and the federal government of Upper Austria and Styria. It also received funding by the project "Multimodal and in-situ characterization of inhomogeneous materials" (MiCi) by the federal government of Upper Austria and the European Regional Development Fund (EFRE) in the framework of the EU-program IWB2020. Furthermore, it received funding from the FFG BRIDGE early stage project no. 851249, "Advanced multimodal data analysis and visualization of composites based on grating interferometer micro-CT data" (ADAM), as well as from the Research Foundation Flanders (FWO) and the Austrian Science Fund (FWF) project "Quantitative X-ray tomography of advanced polymer composites", under the grant numbers G0F9117N and I3261-N36 respectively. This work was also supported by the project "Interpretation and evaluation of defects in complex CFK structures based on 3D-CT data and structural simulation" (DigiCT-Sim; FFG proj. no. 862015) funded by the federal government of Upper Austria and FFG.

# References
