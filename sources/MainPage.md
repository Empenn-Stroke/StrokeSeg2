@mainpage StrokeSeg2 Documentation

# Welcome to StrokeSeg2

StrokeSeg2 is a C++ application for medical image processing, specifically designed for stroke segmentation and pipeline management.

## Project Structure
This documentation covers the core components of the application:
- **Core Processing** (`sources/core/`): Inference, preprocessing (brain extraction, resampling), and post-processing modules.
- **GUI** (`sources/gui/`): The Qt-based graphical user interface, including slice viewers and parameter forms.
- **Workers & Managers**: Pipeline synchronization, app configuration, and logging.
- **Utilities**: DICOM/NIfTI conversions and Anima wrappers.

## Getting Started
To navigate the codebase, use the sidebar to browse **Classes**, **Files**, or the **Class Hierarchy**.

### Key Classes to Explore:
* `PipelineWorker`: Handles the execution of the segmentation pipeline.
* `Preprocessor` / `Postprocessor`: Image manipulation before and after inference.
* `MainWindow`: The main entry point for the user interface.