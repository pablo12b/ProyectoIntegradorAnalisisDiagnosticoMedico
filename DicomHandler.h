#ifndef DICOMHANDLER_H
#define DICOMHANDLER_H

#include <string>
#include <opencv2/opencv.hpp>
#include <itkImage.h>
#include <itkImageFileReader.h>

// Definimos el tipo de imagen médica (2 dimensiones, pixeles short con signo)
using PixelType = signed short;
using ImageType = itk::Image<PixelType, 2>;

class DicomHandler {
public:
    DicomHandler();
    // Función que recibe la ruta del archivo .IMA y devuelve una cv::Mat
    cv::Mat cargarImagenDicom(const std::string& rutaArchivo);
};

#endif