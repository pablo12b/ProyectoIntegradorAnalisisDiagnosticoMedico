#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>

class ImageProcessor {
public:
    void cargarRedNeuronal(const std::string& rutaModelo);

    // --- TÉCNICAS OBLIGATORIAS DE LA RÚBRICA ---
    
    // 1. Ecualización de Histograma (Estándar vs CLAHE - Técnica Nueva)
    cv::Mat mejorarContraste(cv::Mat entrada, bool usarCLAHE);

    // 2. Denoising (Suavizado e IA)
    cv::Mat aplicarReduccionRuido(cv::Mat entrada, bool usarDNN);

    // 3. Detección de Bordes (Canny)
    cv::Mat detectarBordes(cv::Mat entrada, double umbralBajo, double umbralAlto);

    // --- PRESETS MÉDICOS (Para cumplir "3 zonas de interés") ---
    cv::Mat segmentarHueso(cv::Mat entrada);       // Zona 1
    cv::Mat segmentarPulmon(cv::Mat entrada);      // Zona 2
    cv::Mat segmentarTejidoBlando(cv::Mat entrada);// Zona 3

    // Auxiliar: Overlay para visualización
    cv::Mat crearOverlay(cv::Mat original, cv::Mat mascara, cv::Scalar color);

    // CUMPLE: Contrast Stretching (Estiramiento de contraste lineal)
    cv::Mat aplicarContrastStretching(cv::Mat entrada);
    
    // CUMPLE: Guardar imágenes en disco
    void guardarResultados(const std::string& nombreBase, cv::Mat orig, cv::Mat proc, cv::Mat mask, cv::Mat final);

private:
    cv::dnn::Net redNeuronal;
    bool redCargada = false;
    
    // Función interna para limpieza morfológica
    cv::Mat limpiarMascara(cv::Mat mascara, int tipoMorfologico);
};

#endif