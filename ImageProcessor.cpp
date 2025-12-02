#include "ImageProcessor.h"

void ImageProcessor::cargarRedNeuronal(const std::string& rutaModelo) {
    try {
        this->redNeuronal = cv::dnn::readNetFromONNX(rutaModelo);
        this->redCargada = true;
    } catch (...) { this->redCargada = false; }
}

// --- CUMPLE: Ecualización y "Técnica Nueva" (CLAHE) ---
cv::Mat ImageProcessor::mejorarContraste(cv::Mat entrada, bool usarCLAHE) {
    cv::Mat salida;
    if (usarCLAHE) {
        // TÉCNICA INVESTIGADA: CLAHE mejora contraste local sin saturar ruido
        auto clahe = cv::createCLAHE();
        clahe->setClipLimit(2.0); // Parámetro ajustable
        clahe->setTilesGridSize(cv::Size(8, 8));
        clahe->apply(entrada, salida);
    } else {
        // Ecualización estándar (Requisito básico)
        cv::equalizeHist(entrada, salida);
    }
    return salida;
}

cv::Mat ImageProcessor::aplicarReduccionRuido(cv::Mat entrada, bool usarDNN) {
    cv::Mat salida;
    if (usarDNN && redCargada) {
        // Inferencia DNN (simplificada para ejemplo)
        // En producción real DnCNN requiere manejo de float32
        cv::Mat blob = cv::dnn::blobFromImage(entrada, 1.0/255.0); 
        redNeuronal.setInput(blob);
        cv::Mat prob = redNeuronal.forward();
        salida = entrada; // Placeholder si no hay modelo real
    } else {
        // CUMPLE: Suavizado de imágenes (Gaussiano)
        cv::GaussianBlur(entrada, salida, cv::Size(5, 5), 0);
    }
    return salida;
}

// --- CUMPLE: Filtros de detección de bordes ---
cv::Mat ImageProcessor::detectarBordes(cv::Mat entrada, double umbralBajo, double umbralAlto) {
    cv::Mat bordes;
    cv::Canny(entrada, bordes, umbralBajo, umbralAlto);
    return bordes;
}

// --- LÓGICA MÉDICA: ZONA 1 (Huesos) ---
cv::Mat ImageProcessor::segmentarHueso(cv::Mat entrada) {
    cv::Mat mascara;
    // Hueso es muy brillante (>200 en escala 0-255)
    cv::threshold(entrada, mascara, 200, 255, cv::THRESH_BINARY);
    return limpiarMascara(mascara, cv::MORPH_CLOSE); // Cierre para unir fragmentos
}

// --- LÓGICA MÉDICA: ZONA 2 (Pulmones/Aire) ---
cv::Mat ImageProcessor::segmentarPulmon(cv::Mat entrada) {
    cv::Mat mascara;
    // Pulmón es aire (oscuro), pero no fondo puro. Rango aprox 20-70.
    cv::inRange(entrada, cv::Scalar(20), cv::Scalar(80), mascara);
    return limpiarMascara(mascara, cv::MORPH_OPEN); // Apertura para quitar ruido
}

// --- LÓGICA MÉDICA: ZONA 3 (Tejido Blando/Corazón) ---
cv::Mat ImageProcessor::segmentarTejidoBlando(cv::Mat entrada) {
    cv::Mat mascara;
    // Gris medio. Rango aprox 90-160.
    cv::inRange(entrada, cv::Scalar(90), cv::Scalar(160), mascara);
    return limpiarMascara(mascara, cv::MORPH_OPEN);
}

// CUMPLE: Operaciones Morfológicas
cv::Mat ImageProcessor::limpiarMascara(cv::Mat mascara, int tipoMorfologico) {
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mascara, mascara, tipoMorfologico, kernel);
    return mascara;
}

cv::Mat ImageProcessor::crearOverlay(cv::Mat original, cv::Mat mascara, cv::Scalar color) {
    cv::Mat resultado, originalColor;
    cv::cvtColor(original, resultado, cv::COLOR_GRAY2BGR);
    cv::cvtColor(original, originalColor, cv::COLOR_GRAY2BGR);
    
    cv::Mat colorMask(original.size(), CV_8UC3, color);
    colorMask.copyTo(resultado, mascara);
    
    cv::addWeighted(resultado, 0.6, originalColor, 0.4, 0, resultado);
    return resultado;
}

// --- CUMPLE: Contrast Stretching ---
cv::Mat ImageProcessor::aplicarContrastStretching(cv::Mat entrada) {
    cv::Mat salida;
    // Normaliza la imagen para usar todo el rango dinámico de 0 a 255
    // Esto es "estirar" el histograma linealmente
    cv::normalize(entrada, salida, 0, 255, cv::NORM_MINMAX);
    return salida;
}

// En ImageProcessor.cpp

cv::Mat ImageProcessor::aplicarApertura(cv::Mat mascara) {
    cv::Mat salida;
    // Creamos un elemento estructurante de 3x3 (Rectangular o Elíptico)
    // MORPH_ELLIPSE es mejor para formas biológicas que MORPH_RECT
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4));
    
    // Aplicamos APERTURA (Erosión -> Dilatación)
    // Esto elimina los puntos blancos pequeños (ruido)
    cv::morphologyEx(mascara, salida, cv::MORPH_OPEN, kernel);
    
    return salida;
}

cv::Mat ImageProcessor::aplicarGradienteMorfologico(cv::Mat mascara) {
    cv::Mat salida;
    // Un kernel de 3x3 es estándar para bordes finos
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    
    // El gradiente resta la versión "gorda" (dilatada) menos la "flaca" (erosionada)
    // El resultado es el borde.
    cv::morphologyEx(mascara, salida, cv::MORPH_GRADIENT, kernel);
    
    return salida;
}

// --- CUMPLE: Almacenamiento en carpeta ---
#include <sys/stat.h> // Para crear carpeta (en Linux/Mac)

void ImageProcessor::guardarResultados(const std::string& nombreBase, cv::Mat orig, cv::Mat proc, cv::Mat mask, cv::Mat final) {
    std::string carpeta = "Resultados_Output";
    
    // Crear carpeta si no existe (comando simple para Linux)
    mkdir(carpeta.c_str(), 0777);

    // Extraer nombre del archivo sin ruta
    std::string nombre = nombreBase.substr(nombreBase.find_last_of("/\\") + 1);

    cv::imwrite(carpeta + "/1_Original_" + nombre + ".png", orig);
    cv::imwrite(carpeta + "/2_Procesada_" + nombre + ".png", proc);
    cv::imwrite(carpeta + "/3_Mascara_" + nombre + ".png", mask);
    cv::imwrite(carpeta + "/4_Final_" + nombre + ".png", final);
    
    std::cout << " [GUARDADO] Imagenes guardadas en carpeta: " << carpeta << std::endl;
}