#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "DicomHandler.h"
#include "ImageProcessor.h"

using namespace cv;
using namespace std;

// --- ESTRUCTURAS DE LA INTERFAZ (UI) ---
struct Boton {
    Rect zona;
    string texto;
    bool* estadoVinculado = nullptr;
    bool esAccion = false;
    bool activo = false;
};

struct AppState {
    int sliderModo = 0;   // 0:Manual, 1:Hueso, 2:Pulmon, 3:Tejido
    bool usarCLAHE = false;
    bool usarDNN = false;
    bool usarMorf = true;
    bool verBordes = false;
    bool guardarSolicitado = false;
    
    // Navegación
    int indiceArchivo = 0;
    vector<string> archivos;
    bool necesitaActualizar = true;
};

// --- VARIABLES GLOBALES ---
AppState app;
vector<Boton> botones;

// Paleta de Colores "Dark Medical"
Scalar cFondo = Scalar(30, 30, 30);      // Gris Oscuro Fondo
Scalar cPanel = Scalar(45, 45, 45);      // Gris Panel
Scalar cTexto = Scalar(220, 220, 220);   // Blanco Texto
Scalar cResaltado = Scalar(0, 120, 215); // Azul Resaltado
Scalar cVerde = Scalar(80, 160, 80);     // Verde Activo

// --- GESTIÓN DE EVENTOS DEL MOUSE ---
void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        // 1. Verificar Clics en Botones
        for (auto& btn : botones) {
            if (btn.zona.contains(Point(x, y))) {
                if (btn.esAccion) {
                    if (btn.texto == "GUARDAR") app.guardarSolicitado = true;
                    // Navegación
                    if (btn.texto == ">" && app.indiceArchivo < app.archivos.size()-1) {
                        app.indiceArchivo++; app.necesitaActualizar = true;
                    }
                    if (btn.texto == "<" && app.indiceArchivo > 0) {
                        app.indiceArchivo--; app.necesitaActualizar = true;
                    }
                    // Cambio de Modos
                    if (btn.texto == "MANUAL") app.sliderModo = 0;
                    if (btn.texto == "HUESO") app.sliderModo = 1;
                    if (btn.texto == "PULMON") app.sliderModo = 2;
                    if (btn.texto == "TEJIDO") app.sliderModo = 3;
                } else if (btn.estadoVinculado) {
                    // Interruptores (ON/OFF)
                    *btn.estadoVinculado = !(*btn.estadoVinculado);
                }
            }
        }
        
        // 2. Verificar Clic en Lista de Archivos (Zona Izquierda)
        if (x < 220 && y > 80 && y < 650) {
            // Clic arriba = Anterior, Clic abajo = Siguiente
            if (y < 350 && app.indiceArchivo > 0) { 
                app.indiceArchivo--; app.necesitaActualizar = true; 
            } else if (y >= 350 && app.indiceArchivo < app.archivos.size()-1) { 
                app.indiceArchivo++; app.necesitaActualizar = true; 
            }
        }
    }
}

// --- HELPER: DIBUJAR UN RECUADRO CON IMAGEN ---
void ponerImagenEnGrid(Mat& lienzo, Mat& img, Rect zona, string titulo, Scalar colorBorde) {
    if (img.empty()) return;
    
    // Fondo y borde del panel
    rectangle(lienzo, zona, Scalar(20, 20, 20), -1);
    rectangle(lienzo, zona, colorBorde, 1);
    
    // Título
    rectangle(lienzo, Rect(zona.x, zona.y, zona.width, 25), colorBorde, -1);
    putText(lienzo, titulo, Point(zona.x + 10, zona.y + 18), FONT_HERSHEY_SIMPLEX, 0.45, Scalar(255,255,255), 1);

    // Ajuste de imagen (Mantener relación de aspecto)
    Mat imgColor;
    if(img.channels() == 1) cvtColor(img, imgColor, COLOR_GRAY2BGR);
    else imgColor = img.clone();

    int hDisp = zona.height - 30; 
    float scale = min((float)zona.width / imgColor.cols, (float)hDisp / imgColor.rows);
    if (scale > 1.0f) scale = 1.0f; 

    Mat imgRes;
    resize(imgColor, imgRes, Size(), scale, scale);

    int xOff = (zona.width - imgRes.cols) / 2;
    int yOff = 30 + (hDisp - imgRes.rows) / 2;

    imgRes.copyTo(lienzo(Rect(zona.x + xOff, zona.y + yOff, imgRes.cols, imgRes.rows)));
}

// --- RENDERIZADO PRINCIPAL DE LA GUI ---
void dibujarAppCompleta(Mat& lienzo, Mat& orig, Mat& proc, Mat& mask, Mat& final, string nombreArchivo) {
    lienzo = Mat(768, 1366, CV_8UC3, cFondo);

    // 1. BARRA LATERAL IZQUIERDA (Explorador)
    Rect rSidebar(0, 0, 220, 768);
    rectangle(lienzo, rSidebar, cPanel, -1);
    line(lienzo, Point(220, 0), Point(220, 768), Scalar(60,60,60), 1);
    putText(lienzo, "EXPLORADOR DICOM", Point(15, 40), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(150,150,150), 1);

    int yList = 100;
    // Mostrar 20 archivos alrededor del actual
    for (int i = max(0, app.indiceArchivo - 8); i < min((int)app.archivos.size(), app.indiceArchivo + 12); i++) {
        string fname = app.archivos[i].substr(app.archivos[i].find_last_of("/\\") + 1);
        if (fname.length() > 20) fname = fname.substr(0, 17) + "...";
        
        Scalar col = (i == app.indiceArchivo) ? cResaltado : cTexto;
        if(i == app.indiceArchivo) rectangle(lienzo, Rect(5, yList-15, 210, 25), Scalar(60,60,60), -1);
        
        putText(lienzo, fname, Point(15, yList), FONT_HERSHEY_SIMPLEX, 0.4, col, 1);
        yList += 25;
    }

    // 2. ZONA CENTRAL (GRID 2x2)
    int startX = 230;
    int endX = 1366 - 230; 
    int wTotal = endX - startX;
    int hTotal = 768;
    int wPanel = (wTotal - 30) / 2; 
    int hPanel = (hTotal - 60) / 2; 

    Rect r1(startX + 10, 20, wPanel, hPanel);              
    Rect r2(startX + 20 + wPanel, 20, wPanel, hPanel);     
    Rect r3(startX + 10, 30 + hPanel, wPanel, hPanel);     
    Rect r4(startX + 20 + wPanel, 30 + hPanel, wPanel, hPanel); 

    ponerImagenEnGrid(lienzo, orig, r1, "1. ORIGINAL (ITK Raw)", Scalar(100,100,100));
    ponerImagenEnGrid(lienzo, proc, r2, "2. PROCESAMIENTO (CLAHE+DNN)", Scalar(100,100,100));
    ponerImagenEnGrid(lienzo, mask, r3, "3. MASCARA BINARIA (ROI)", Scalar(100,100,100));
    ponerImagenEnGrid(lienzo, final, r4, "4. RESULTADO FINAL (Overlay)", cResaltado);

    // 3. BARRA LATERAL DERECHA (Panel de Control)
    Rect rProps(1366-220, 0, 220, 768);
    rectangle(lienzo, rProps, cPanel, -1);
    line(lienzo, Point(1366-220, 0), Point(1366-220, 768), Scalar(60,60,60), 1);
    putText(lienzo, "PANEL DE CONTROL", Point(1366-200, 40), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(150,150,150), 1);

    // Dibujar todos los botones configurados
    for (auto& btn : botones) {
        Scalar bg = Scalar(60,60,60); // Color base
        
        // Si está activo (Switch ON)
        if (btn.estadoVinculado && *btn.estadoVinculado) bg = cVerde;
        
        // Si es acción momentánea (Guardar)
        if (btn.esAccion && btn.texto == "GUARDAR") bg = cResaltado;
        
        // Resaltar el MODO seleccionado actualmente
        if (btn.esAccion && btn.texto == "MANUAL" && app.sliderModo==0) bg = cResaltado;
        if (btn.esAccion && btn.texto == "HUESO" && app.sliderModo==1) bg = cResaltado;
        if (btn.esAccion && btn.texto == "PULMON" && app.sliderModo==2) bg = cResaltado;
        if (btn.esAccion && btn.texto == "TEJIDO" && app.sliderModo==3) bg = cResaltado;

        rectangle(lienzo, btn.zona, bg, -1);
        rectangle(lienzo, btn.zona, Scalar(100,100,100), 1);
        
        // Centrar texto del botón
        Size ts = getTextSize(btn.texto, FONT_HERSHEY_SIMPLEX, 0.4, 1, 0);
        putText(lienzo, btn.texto, 
                Point(btn.zona.x + (btn.zona.width-ts.width)/2, btn.zona.y + (btn.zona.height+ts.height)/2), 
                FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255,255,255), 1);
    }
}

// --- CONFIGURACIÓN DE LOS BOTONES ---
void configurarBotones() {
    int x = 1366 - 200;
    int w = 180; int h = 35; int y = 80;
    
    // Grupo: Modos de Segmentación
    botones.push_back({Rect(x,y,w,h), "MANUAL", nullptr, true}); y+=45;
    botones.push_back({Rect(x,y,w,h), "HUESO", nullptr, true}); y+=45;
    botones.push_back({Rect(x,y,w,h), "PULMON", nullptr, true}); y+=45;
    botones.push_back({Rect(x,y,w,h), "TEJIDO", nullptr, true}); y+=60;
    
    // Grupo: Filtros
    botones.push_back({Rect(x,y,w,h), "CLAHE (Contraste)", &app.usarCLAHE, false}); y+=45;
    botones.push_back({Rect(x,y,w,h), "DNN (Ruido IA)", &app.usarDNN, false}); y+=45;
    botones.push_back({Rect(x,y,w,h), "MORFOLOGIA", &app.usarMorf, false}); y+=45;
    botones.push_back({Rect(x,y,w,h), "VER BORDES", &app.verBordes, false}); y+=60;
    
    // Grupo: Acciones
    botones.push_back({Rect(x,y,w/2-5,h), "<", nullptr, true});
    botones.push_back({Rect(x+w/2+5,y,w/2-5,h), ">", nullptr, true}); y+=60;
    botones.push_back({Rect(x,680,w,50), "GUARDAR", nullptr, true});
}

// --- PUNTO DE ENTRADA PRINCIPAL ---
int main(int argc, char** argv) {
    // 1. Verificar Argumentos
    if (argc < 2) { 
        cout << "ERROR: Arrastra la carpeta con imagenes al ejecutable." << endl; 
        return -1; 
    }
    
    // 2. Cargar Lista de Archivos (Soporta .IMA y .dcm)
    string p = string(argv[1]) + "/*.IMA";
    glob(p, app.archivos, false);
    if(app.archivos.empty()) glob(string(argv[1]) + "/*.dcm", app.archivos, false);
    
    if(app.archivos.empty()) {
        cout << "ERROR: No se encontraron imagenes medicas en la carpeta." << endl;
        return -1;
    }

    // 3. Inicializar Módulos
    DicomHandler dicomIO; 
    ImageProcessor proc; 
    
    // INTENTO DE CARGA DE MODELO (Try-Catch de Seguridad)
    try {
        proc.cargarRedNeuronal("dncnn.onnx"); 
        cout << "[SISTEMA] Modelo de IA cargado correctamente." << endl;
    } catch (...) {
        cout << "[AVISO] No se encontro 'dncnn.onnx'. Usando algoritmo de respaldo." << endl;
    }

    configurarBotones();
    
    string win = "MediVision Pro - Interfaz Clinica";
    namedWindow(win, WINDOW_NORMAL);
    resizeWindow(win, 1366, 768);
    setMouseCallback(win, onMouse, 0); // Activar clics

    Mat imgOrig, imgFinal, lienzo;

    // --- BUCLE PRINCIPAL DE LA APLICACIÓN ---
    while(true) {
        // Cargar imagen solo si cambió el índice
        if(app.necesitaActualizar) {
            imgOrig = dicomIO.cargarImagenDicom(app.archivos[app.indiceArchivo]);
            app.necesitaActualizar = false;
        }
        if(imgOrig.empty()) break;

        // --- PIPELINE DE PROCESAMIENTO ---
        // 1. Estiramiento de Contraste (Base)
        Mat imgProc = proc.aplicarContrastStretching(imgOrig);
        // 2. Filtros Opcionales
        imgProc = proc.mejorarContraste(imgProc, app.usarCLAHE);
        imgProc = proc.aplicarReduccionRuido(imgProc, app.usarDNN);

        // --- LÓGICA DE SEGMENTACIÓN ---
        Mat mask;
        Scalar colOverlay = Scalar(0,255,255); // Amarillo por defecto

        switch(app.sliderModo) {
            case 0: // Manual
                inRange(imgProc, Scalar(50), Scalar(200), mask); 
                break;
            case 1: // Hueso (>200) + Cierre Morfológico
                mask = proc.segmentarHueso(imgProc); 
                colOverlay=Scalar(0,0,255); // Rojo
                break;
            case 2: // Pulmon (Inverso <60)
                // Threshold Inverso para detectar aire
                threshold(imgProc, mask, 60, 255, THRESH_BINARY_INV); 
                colOverlay=Scalar(255,0,0); // Azul
                break;
            case 3: // Tejido (Rango Medio)
                mask = proc.segmentarTejidoBlando(imgProc); 
                colOverlay=Scalar(0,255,0); // Verde
                break;
        }

        // --- REFINAMIENTO MORFOLÓGICO ADICIONAL ---
        if (app.usarMorf) {
            // Aplicar APERTURA (Opening) para quitar ruido en Pulmón y Tejido
            if (app.sliderModo == 2 || app.sliderModo == 3 || app.sliderModo == 0) {
                mask = proc.aplicarApertura(mask);
            }
        } else {
            // Si desactivan morfología, ensuciamos la máscara para demostrar la diferencia
            if (app.sliderModo != 0) threshold(imgProc, mask, 150, 255, THRESH_BINARY); 
        }

        // --- DETECCIÓN DE BORDES / GRADIENTE ---
        if(app.verBordes) {
            // Técnica Nueva: Gradiente Morfológico
            Mat bordesMorf = proc.aplicarGradienteMorfologico(mask);
            
            // Técnica Clásica: Canny
            Mat bordesCanny = proc.detectarBordes(imgProc, 50, 150);
            bitwise_and(bordesCanny, mask, bordesCanny); // Filtrar solo dentro del ROI
            
            // Combinar para visualización
            cv::max(mask, bordesMorf, mask);
        }

        // Generar resultado visual
        imgFinal = proc.crearOverlay(imgProc, mask, colOverlay);

        // --- RENDERIZADO FINAL ---
        string fName = app.archivos[app.indiceArchivo].substr(app.archivos[app.indiceArchivo].find_last_of("/\\")+1);
        dibujarAppCompleta(lienzo, imgOrig, imgProc, mask, imgFinal, fName);

        // Feedback de Guardado
        if(app.guardarSolicitado) {
            proc.guardarResultados(fName, imgOrig, imgProc, mask, imgFinal);
            putText(lienzo, "GUARDADO EN DISCO!", Point(550, 380), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(0,255,0), 3);
            app.guardarSolicitado = false;
            imshow(win, lienzo); waitKey(500); // Pausa para ver el mensaje
        }

        imshow(win, lienzo);
        if(waitKey(10) == 27) break; // ESC para salir
    }
    return 0;
}