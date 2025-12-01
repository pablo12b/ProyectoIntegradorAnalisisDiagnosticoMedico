#include "DicomHandler.h"
#include <itkRescaleIntensityImageFilter.h>
#include <itkCastImageFilter.h>

DicomHandler::DicomHandler() {}

cv::Mat DicomHandler::cargarImagenDicom(const std::string& rutaArchivo) {
    // 1. Configurar el lector ITK
    using ReaderType = itk::ImageFileReader<ImageType>;
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(rutaArchivo);

    try {
        reader->Update(); // Leer el archivo
    } catch (itk::ExceptionObject& err) {
        std::cerr << "Error leyendo DICOM: " << err << std::endl;
        return cv::Mat();
    }

    // 2. Los datos médicos tienen rangos raros (ej. -1000 a 3000).
    // Debemos escalarlos a 0-255 para que OpenCV los pueda mostrar.
    using OutputPixelType = unsigned char;
    using OutputImageType = itk::Image<OutputPixelType, 2>;
    
    using RescaleFilterType = itk::RescaleIntensityImageFilter<ImageType, OutputImageType>;
    RescaleFilterType::Pointer rescaler = RescaleFilterType::New();
    rescaler->SetInput(reader->GetOutput());
    rescaler->SetOutputMinimum(0);
    rescaler->SetOutputMaximum(255);
    rescaler->Update();

    // 3. Convertir de ITK SmartPointer a cv::Mat
    OutputImageType::Pointer itkImg = rescaler->GetOutput();
    OutputImageType::SizeType size = itkImg->GetLargestPossibleRegion().GetSize();
    
    // Crear matriz de OpenCV con el mismo tamaño
    cv::Mat opencvImg(size[1], size[0], CV_8UC1);

    // Copiar memoria buffer de ITK a OpenCV
    std::memcpy(opencvImg.data, itkImg->GetBufferPointer(), size[0] * size[1]);

    return opencvImg.clone(); // Devolver copia segura
}