# MediVision Pro: Sistema de Diagnóstico Médico Asistido

![C++](https://img.shields.io/badge/C++-14-blue.svg) ![OpenCV](https://img.shields.io/badge/OpenCV-4.x-green.svg) ![ITK](https://img.shields.io/badge/ITK-5.3-red.svg) ![Status](https://img.shields.io/badge/Status-Academic_Release-orange.svg)

> **Proyecto Integrador - Visión por Computador**
> Sistema de escritorio de alto rendimiento para el procesamiento, segmentación y análisis de Tomografías Computarizadas (CT) de baja dosis.

---

## 📋 Descripción del Proyecto

**MediVision Pro** es una aplicación de escritorio nativa desarrollada en C++ que implementa una arquitectura híbrida para el procesamiento de imágenes médicas. El sistema resuelve la problemática del ruido en tomografías *Low Dose* mediante técnicas de **Deep Learning** y permite la segmentación automática de órganos basándose en criterios físicos de radiodensidad (**Unidades Hounsfield**).

A diferencia de visores estándar, este proyecto integra la precisión de la librería científica **ITK** con la velocidad de procesamiento de **OpenCV**, todo controlado mediante una interfaz gráfica vectorial personalizada (GUI) que no depende de frameworks externos pesados.

---

## 🚀 Características Principales

### 🧠 Procesamiento Inteligente
* **Denoising con IA:** Implementación de la red neuronal **DnCNN** (Denoising Convolutional Neural Network) mediante el módulo `cv::dnn` para restaurar imágenes sin perder nitidez en los bordes.
* **Mejora de Contraste Local:** Uso de **CLAHE** (Contrast Limited Adaptive Histogram Equalization) para resaltar tejidos blandos en el mediastino.

### 🦴 Segmentación Médica (ROI)
Segmentación automática basada en rangos físicos (Hounsfield Units) y refinamiento morfológico:
* **Modo Hueso:** Detección de alta densidad (>200 HU) con cierre morfológico para corrección de porosidad.
* **Modo Pulmón:** Detección de cavidades aéreas (< -600 HU) mediante inversión lógica.
* **Modo Tejido:** Aislamiento de estructuras blandas con filtrado de ruido "sal y pimienta".

### 🖥️ Interfaz Gráfica (GUI) Personalizada
* **Motor de Renderizado Vectorial:** Interfaz dibujada nativamente sobre OpenCV (sin Qt ni .NET).
* **Dashboard Clínico:** Visualización simultánea 2x2 (Original, Procesada, Máscara, Resultado).
* **Explorador de Archivos:** Barra lateral para navegación rápida por datasets volumétricos.

---

## 🛠️ Arquitectura Técnica

El proyecto sigue una estructura modular:

1.  **Capa de Adquisición (Backend):**
    * Uso de `itk::ImageFileReader` para ingesta de datos DICOM/NIfTI.
    * Puente de memoria directo (Buffer Copy) entre ITK y OpenCV.
2.  **Capa de Procesamiento (Core):**
    * Normalización de histograma (Contrast Stretching).
    * Inferencia de modelos ONNX.
    * Algoritmos de visión clásica: Canny Edge Detector, Operadores Booleanos (AND/NOT).
3.  **Capa de Presentación (Frontend):**
    * Gestión de eventos de ratón (`cv::setMouseCallback`).
    * Sistema de gestión de estado (`AppState`).

---

## ⚙️ Requisitos de Instalación

Para compilar este proyecto, necesitas las siguientes librerías instaladas en tu sistema (Linux/Ubuntu recomendado):

* **Compilador C++:** GCC o Clang (Soporte C++14 mínimo).
* **CMake:** Versión 3.10 o superior.
* **OpenCV 4.x:** Debe incluir el módulo `opencv_dnn` y `opencv_highgui`.
* **Insight Toolkit (ITK) 5.3:** Compilado e instalado.

---

## 🔨 Compilación y Ejecución

Sigue estos pasos para construir el proyecto desde el código fuente:

```bash
# 1. Clonar el repositorio
git clone [https://github.com/TuUsuario/MediVision-Integrador.git](https://github.com/TuUsuario/MediVision-Integrador.git)
cd MediVision-Integrador

# 2. Crear carpeta de construcción
mkdir build && cd build

# 3. Configurar con CMake
# Nota: Si CMake no encuentra ITK, usa: cmake -DITK_DIR=/ruta/a/ITK/ ..
cmake ..

# 4. Compilar
make

# 5. Ejecutar
# Debes pasar la ruta de una carpeta con imágenes .IMA o .dcm
./IntegradorApp ../data/dataset_medico/
```
## 🎮 Manual de Uso

La aplicación ha sido diseñada para un flujo de trabajo radiológico intuitivo. A continuación se describe la interacción con la interfaz:

### 1. Inicio de la Aplicación
Ejecute el programa pasando como argumento la ruta a la carpeta que contiene las imágenes DICOM (.IMA o .dcm):

```bash
./IntegradorApp ../data/ct_low_dose/
```
### 2. Navegación (Panel Izquierdo)
* **Explorador de Archivos:** En la barra lateral izquierda se listan todos los archivos encontrados en el directorio cargado.
* **Selección:** Haga **clic izquierdo** sobre el nombre de cualquier archivo para cargarlo inmediatamente en el visor central.
* **Botones de Navegación:** Utilice los botones `<` y `>` situados en el panel derecho para avanzar o retroceder secuencialmente por el dataset.

### 3. Panel de Propiedades (Panel Derecho)
Controles interactivos para manipular el procesamiento en tiempo real:

* **Selectores de Modo (Segmentación):**
    * `MANUAL`: Desactiva la segmentación automática. Permite ver la imagen procesada base.
    * `HUESO`: Activa el algoritmo de umbralización alta (>200 HU) + Cierre morfológico.
    * `PULMON`: Activa la inversión de umbral para detectar aire + Lógica booleana.
    * `TEJIDO`: Activa la detección de rango medio para mediastino.
* **Interruptores de Filtros (Switches):**
    * `ACTIVAR CLAHE`: (On/Off) Habilita la ecualización adaptativa para mejorar el contraste local.
    * `ACTIVAR IA (DNN)`: (On/Off) Habilita la inferencia de la red neuronal para reducción de ruido.
    * `VER BORDES`: Superpone los bordes Canny sobre la máscara actual.
    * `MORFOLOGIA`: Activa/Desactiva la limpieza matemática (Cierre/Apertura).

### 4. Exportación de Evidencias
* **Botón GUARDAR:** Al hacer clic, el sistema captura el estado actual de las 4 vistas (Original, Procesada, Máscara, Resultado) y las guarda automáticamente en la carpeta de ejecución con el prefijo del nombre del archivo original.

---

## 📂 Estructura del Proyecto

El código fuente está organizado siguiendo el patrón de diseño modular para separar la lógica de interfaz, procesamiento y adquisición de datos.

```text
MediVision-Integrador/
├── CMakeLists.txt          # Script de configuración de compilación (Linkeo ITK/OpenCV)
├── README.md               # Documentación técnica del proyecto
├── .gitignore              # Exclusiones de Git (Binarios y Datasets)
├── src/
│   ├── main.cpp            # Motor de GUI y gestión de eventos Mouse.
│   ├── DicomHandler.cpp    # Lectura de datos crudos mediante ITK.
│   └── ImageProcessor.cpp  # Algoritmos (CLAHE, DNN, Morfología, Canny).
├── include/
│   ├── DicomHandler.h      # Cabecera: Clase de carga DICOM.
│   └── ImageProcessor.h    # Cabecera: Clase de procesamiento.
└── models/
    └── dncnn_model.onnx    # (Opcional) Modelo de Red Neuronal.
```
## 👨‍💻 Autores y Créditos

Este software fue desarrollado como parte del **Proyecto Integrador de Interciclo** para la asignatura de Visión por Computador.

**Integrantes del Grupo:**
* **Pablo Bravo**
* **Domenika Delgado**

**Institución:** Universidad Politécnica Salesiana
**Docente:** Ing. Vladimir Robles Bykbaev
**Período Lectivo:** Octubre 2025 – Febrero 2026

### Reconocimientos y Datasets
* **Dataset Médico:** Las imágenes utilizadas para la validación del sistema provienen del conjunto de datos público *"CT Low Dose Reconstruction"* alojado en Kaggle.
* **Librerías de Terceros:**
    * *Insight Toolkit (ITK)* por Kitware Inc.
    * *OpenCV (Open Source Computer Vision Library)*.