#include <inference/inference.h>

class Inference::Impl {// defined privately here
  // ... all private data and functions: all of these
  //     can now change without recompiling callers ...
	public:
		int someValue; // example of a private data member
};

Inference::Inference() : pImpl(std::make_unique<Impl>()) {
	// ... set impl values ...

	pImpl->someValue = 42;
}

Inference::~Inference() = default;

NiftiVolume Inference::run(const QString &modelPath, const QString &imagePath,
                           const QString &destinationPath, const QString &inputName,
                           const QString &outputName) {
    return NiftiVolume(); // placeholder implementation
}

