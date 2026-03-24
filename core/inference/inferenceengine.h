#pragma once
#include <vector>
#include <QString>
#include <utils/niftiVolume.h>
#include <QDir>
#include <QStringList>
#include <dml_provider_factory.h>
#include <onnxruntime_cxx_api.h>
#include <utils/path.h>

class InferenceEngine {

private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;

        /* @brief Get the list of available ONNX models in the predefined models directory.
     * The models should have a .onnx extension.
     * @return QStringList containing the names of the available models (without path).
     * @throws std::runtime_error if the models directory cannot be accessed.
     */
    QStringList getAvailableModels();

    /* @brief Load the specified ONNX model from the predefined models directory and
     * initialize the ONNX Runtime session.
     * @param modelName The name of the model to load (without path).
     * @throws std::runtime_error if the model cannot be found or loaded.
     * @throws Ort::Exception if there is an error initializing the ONNX Runtime session.
     * @note This method must be called before running inference with the run() method.
     *       The model will be loaded from the path: getModelsPath() + "/" + modelName + ".onnx"
     *       For example, if modelName is "stroke_segmentation", the method will attempt
     *       to load "C:/ProgramData/StrokeSeg/Models/stroke_segmentation.onnx"
     *       (assuming getModelsPath() returns "C:/ProgramData/StrokeSeg/Models").
     *       The method will also initialize the ONNX Runtime session with the loaded model, which
     is required for running inference.

    */
    void loadModel(const QString &modelName);

    /* @brief Initialize the ONNX Runtime session with the specified model path. This method is
     * called internally by loadModel() after loading the model.
     * @param modelPath The full path to the ONNX model file to load (including the .onnx
     * extension).
     * @throws std::runtime_error if the model cannot be loaded or if there is an error initializing
     * the ONNX Runtime session.
     */
    void initSession(const QString &modelPath);

    /* @brief Slice a 3D image volume into overlapping patches based on the specified image size,
     * patch size, and step size.
     * @param image_size An array of three integers specifying the dimensions of the input image
     * volume (e.g., {width, height, depth}).
     * @param patch_size An array of three integers specifying the dimensions of each patch to
     * extract from the image volume (e.g., {patch_width, patch_height, patch_depth}).
     * @param step_size A float between 0 and 1 specifying the fraction of the patch size to use
     * as the step between patches. For example, a step_size of 0.5 means that patches will overlap
     * by 50%.
     * @return A vector of vectors of integers, where each inner vector contains the starting
     * indices (x, y, z) for a patch in the image volume. The outer vector contains one entry for
     * each patch.
     * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative
     * sizes, step_size not in (0, 1)).
     * @note The method calculates the starting indices for patches in a way that covers the entire
     * image volume with overlapping patches. The last patches in each dimension may be smaller than
     * the specified patch size if they extend beyond the image boundaries.
     * The method does not actually extract or return the patch data; it only returns the
     * starting indices for where patches would be extracted from the image volume.
     */
    std::vector<std::vector<int>> sliceVolume(std::array<int, 3> image_size,
                                              std::array<int, 3> patch_size, float step_size);

    /* @brief Compute a 3D Gaussian weighting function (kernel) based on the specified patch size,
     * sigma scale, and value scaling factor.
     * @param patch_size An array of three integers specifying the dimensions of the patch for
     * which to compute the Gaussian kernel (e.g., {patch_width, patch_height, patch_depth}).
     * @param sigma_scale A float specifying the scale factor for the standard deviation (sigma)
     * of the Gaussian function. The actual sigma will be calculated as sigma_scale multiplied
     * by the corresponding dimension of the patch size. For example, if patch_size is {128, 128,
     * 128} and sigma_scale is 0.125, then sigma will be {16, 16, 16}.
     * @param value_scaling_factor A float specifying a scaling factor to apply to the values of the
     * Gaussian kernel. This can be used to increase or decrease the overall magnitude of the
     * weights in the kernel.
     * @return An Eigen::Tensor<float, 3, Eigen::ColMajor> representing the computed 3D Gaussian
     * kernel with dimensions matching the specified patch size. The values in the tensor will be
     * scaled by the value_scaling_factor.
     * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative
     * sizes, non-positive sigma_scale).
     * @note The computed Gaussian kernel can be used for weighting patches during inference to give
     * more importance to voxels near the center of the patch and less importance to voxels near the
     * edges. This can help reduce edge artifacts when combining predictions from overlapping
     * patches. The method does not perform any normalization on the kernel values; it simply
     * applies the specified value scaling factor after computing the Gaussian function. The output
     * tensor will have its dimensions ordered according to Eigen's column-major storage format,
     * which is consistent with how NIfTI data is stored on disk.
     */
    Eigen::Tensor<float, 3, Eigen::ColMajor> compute_gaussian(const std::array<int, 3> &patch_size,
                                                              float sigma_scale = 0.125f,
                                                              float value_scaling_factor = 10.0f);

    static QString getModelsPath() { return model_dir; }

public:

    InferenceEngine();
   
    /* @brief Run inference on the specified input image using the loaded ONNX model and return the output as a NiftiVolume.
     * @param modelPath The full path to the ONNX model file to use for inference (including the .onnx extension).
     * @param imagePath The full path to the input image file (e.g., a NIfTI file) to run inference on.
     * @param destinationPath The full path where the output NIfTI file should be saved (including the .nii or .nii.gz extension).
     * @param inputName The name of the input tensor in the ONNX model that corresponds to the input image data.
     * @param outputName The name of the output tensor in the ONNX model that contains the inference results.
     * @return A NiftiVolume containing the inference results, which can also be saved to disk at the specified destinationPath.
     * @throws std::runtime_error if there is an error loading the model, reading the input image, running inference, or saving the output.
     * @note This method assumes that loadModel() has already been called to load and initialize the ONNX Runtime session with the specified model. 
     *       The method will read the input image, preprocess it as needed, run it through the ONNX model using the specified input and output tensor names, 
     *       postprocess the output as needed, and return it as a NiftiVolume. The output
     * will also be saved to disk at destinationPath.
    */
    NiftiVolume run(const QString &modelPath, const QString &imagePath,
                    const QString &destinationPath, const QString &inputName,
                    const QString &outputName);

};