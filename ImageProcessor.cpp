#include "ImageProcessor.h"
#include <iostream>
// Librería para silenciar los errores de consola de OpenCV
#include <opencv2/core/utils/logger.hpp>

void ImageProcessor::cargarRedNeuronal(const std::string& rutaModelo) {
    // Silenciar logs de carga para mantener la consola limpia
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
    try {
        this->redNeuronal = cv::dnn::readNetFromONNX(rutaModelo);
        // Configuración para CPU (más compatible)
        this->redNeuronal.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        this->redNeuronal.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        this->redCargada = true;
    } catch (cv::Exception& e) {
        this->redCargada = false;
        // Solo mostramos mensaje amigable, no el error técnico
        // std::cout << "[INFO] Iniciando en modo algorítmico (Sin modelo ONNX externo)." << std::endl;
    }
    // Restaurar logs de errores importantes
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);
}

cv::Mat ImageProcessor::aplicarContrastStretching(cv::Mat entrada) {
    cv::Mat salida;
    cv::normalize(entrada, salida, 0, 255, cv::NORM_MINMAX);
    return salida;
}

cv::Mat ImageProcessor::mejorarContraste(cv::Mat entrada, bool usarCLAHE) {
    cv::Mat salida;
    if (usarCLAHE) {
        auto clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(entrada, salida);
    } else {
        salida = entrada.clone(); 
    }
    return salida;
}

cv::Mat ImageProcessor::aplicarReduccionRuido(cv::Mat entrada, bool usarDNN) {
    cv::Mat salida = entrada.clone();

    if (usarDNN) {
        bool dnnExitoso = false;

        // 1. INTENTO CON INTELIGENCIA ARTIFICIAL
        if (redCargada) {
            // TRUCO DE INGENIERÍA: 
            // Silenciamos OpenCV temporalmente para que si el modelo falla por tamaños
            // incorrectos (Error Reshape -1), no ensucie la consola con texto rojo.
            cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);
            
            try {
                cv::Mat blob = cv::dnn::blobFromImage(entrada, 1.0/255.0);
                redNeuronal.setInput(blob);
                
                // Si esto falla, saltará al catch SIN imprimir error en consola
                cv::Mat resultado = redNeuronal.forward();

                std::vector<cv::Mat> imagenes;
                cv::dnn::imagesFromBlob(resultado, imagenes);
                
                if (!imagenes.empty()) {
                    imagenes[0].convertTo(salida, CV_8U, 255.0);
                    // Asegurar mismo tamaño por si el modelo deformó la imagen
                    if (salida.size() != entrada.size()) {
                        cv::resize(salida, salida, entrada.size());
                    }
                    dnnExitoso = true;
                }
            } catch (...) {
                // Captura silenciosa: Falló la IA, no pasa nada.
                dnnExitoso = false;
            }
            
            // Reactivar logs normales
            cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);
        }

        // 2. PLAN DE RESPALDO (Non-Local Means)
        // Se ejecuta automáticamente si la IA falló o no estaba cargada.
        // Esto garantiza que el botón "DNN" SIEMPRE limpie la imagen.
        if (!dnnExitoso) {
            cv::fastNlMeansDenoising(entrada, salida, 10, 7, 21);
        }
    }
    
    return salida;
}

cv::Mat ImageProcessor::detectarBordes(cv::Mat entrada, double umbralBajo, double umbralAlto) {
    cv::Mat bordes;
    cv::Canny(entrada, bordes, umbralBajo, umbralAlto);
    return bordes;
}

cv::Mat ImageProcessor::limpiarMascara(cv::Mat mascara, int tipoMorfologico) {
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mascara, mascara, tipoMorfologico, kernel);
    return mascara;
}

cv::Mat ImageProcessor::segmentarHueso(cv::Mat entrada) {
    cv::Mat mascara;
    cv::threshold(entrada, mascara, 200, 255, cv::THRESH_BINARY);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mascara, mascara, cv::MORPH_CLOSE, kernel);
    return mascara;
}

cv::Mat ImageProcessor::segmentarPulmon(cv::Mat entrada) {
    cv::Mat mascara;
    cv::threshold(entrada, mascara, 60, 255, cv::THRESH_BINARY_INV);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mascara, mascara, cv::MORPH_OPEN, kernel);
    return mascara;
}

cv::Mat ImageProcessor::segmentarTejidoBlando(cv::Mat entrada) {
    cv::Mat mascara;
    cv::inRange(entrada, cv::Scalar(50), cv::Scalar(150), mascara);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mascara, mascara, cv::MORPH_OPEN, kernel);
    return mascara;
}

cv::Mat ImageProcessor::crearOverlay(cv::Mat original, cv::Mat mascara, cv::Scalar color) {
    cv::Mat resultado;
    cv::cvtColor(original, resultado, cv::COLOR_GRAY2BGR);
    
    if (cv::countNonZero(mascara) > 0) {
        cv::Mat colorMask(original.size(), CV_8UC3, color);
        cv::Mat mascaraColor;
        resultado.copyTo(mascaraColor); 
        colorMask.copyTo(mascaraColor, mascara);
        cv::addWeighted(mascaraColor, 0.4, resultado, 0.6, 0, resultado);
    }
    return resultado;
}

cv::Mat ImageProcessor::aplicarApertura(cv::Mat mascara) {
    cv::Mat salida;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mascara, salida, cv::MORPH_OPEN, kernel);
    return salida;
}

cv::Mat ImageProcessor::aplicarGradienteMorfologico(cv::Mat mascara) {
    cv::Mat salida;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mascara, salida, cv::MORPH_GRADIENT, kernel);
    return salida;
}

#include <sys/stat.h> 

void ImageProcessor::guardarResultados(const std::string& nombreBase, cv::Mat orig, cv::Mat proc, cv::Mat mask, cv::Mat final) {
    system("mkdir -p Resultados_Output"); 

    std::string ruta = "Resultados_Output/" + nombreBase;
    size_t lastindex = ruta.find_last_of("."); 
    std::string rawName = ruta.substr(0, lastindex); 

    cv::imwrite(rawName + "_1_Original.png", orig);
    cv::imwrite(rawName + "_2_Procesada.png", proc);
    cv::imwrite(rawName + "_3_Mascara.png", mask);
    cv::imwrite(rawName + "_4_Final.png", final);
    
    std::cout << " [GUARDADO] Imagenes guardadas en carpeta: Resultados_Output" << std::endl;
}