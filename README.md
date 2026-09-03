# StrokeSeg2: Automated Stroke Segmentation Application

# ⚠️ This tool is for research purpose only!

StrokeSeg2 is the new, enhanced version of the StrokeSeg application. It is now a C++/Qt application 
designed to automate the segmentation of stroke lesions from medical imaging data. It provides 
a complete workflow including preprocessing, inference using a selection of lightweight models, and postprocessing 
with visualization tools. The modular architecture allows for easy customization and extension.

## Application Architecture Overview


The project is organized into several key modules, each responsible for a specific aspect of the 
stroke segmentation workflow:

- [**preprocessing/**](./core/preprocessing): Handles the preparation of medical imaging data, 
including normalization, resizing, and other necessary transformations to ensure 
compatibility with the inference models.
- [**inference/**](./core/inference): Contains the implementation of light trained models for 
stroke segmentation, optimized for performance and accuracy.
- [**postprocessing/**](./core/postprocessing): Responsible for refining the segmentation results, 
including techniques for noise reduction, morphological operations, and other enhancements to 
improve the quality of the output.
- [**managers/**](./core/managers): Manages the application's configuration, progress tracking, and
logging functionalities.
- [**workers/**](./core/workers): Contains worker classes that handle the execution of the pipeline.
- [**gui/**](./gui): Implements the graphical user interface using Qt, allowing users to interact with
the application, configure settings, and visualize results.

Each module is designed to be modular and maintainable, facilitating easy updates and extensions to 
the application's functionality.


## User Guide

For detailed usage instructions, please refer to the [USER_GUIDE.md](./USER_GUIDE.md) file.


## Setup Instructions

To set up the StrokeSeg2 application, follow the instructions in the [SETUP.md](./SETUP.md) file, which 
includes all necessary steps to build the application from source, including dependencies and configuration.
If you encounter any issues during setup, please refer to the troubleshooting section in the setup guide 
or contact the development team for assistance.

In case you prefer installing the application using a package manager, please check the [RELEASES](./RELEASES)
section for available binaries and installation instructions.

