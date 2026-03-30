#include <memory>
#include <qstring.h>
#include <utils/niftiVolume.h>

class Inference {

	public:
        Inference();
        ~Inference();

        NiftiVolume run(const QString &modelPath, const QString &imagePath,
                        const QString &destinationPath, const QString &inputName,
                        const QString &outputName);
	
	private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
};