cmake_minimum_required(VERSION 3.16)
project(IntegradorVision)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Indicar la ruta de OpenCV personalizada
set(OpenCV_DIR "/home/pablo/aplicaciones/Librerias/opencv/opencvi/lib/cmake/opencv4")
set(ITK_DIR "/home/pablo/aplicaciones/Librerias/ITK/itk-install/lib/cmake/ITK-6.0")

# 1. Encontrar Paquetes
find_package(OpenCV REQUIRED)
find_package(ITK REQUIRED)

# 2. Incluir cabeceras de ITK
include(${ITK_USE_FILE})

# 3. Definir el ejecutable
add_executable(IntegradorApp 
    main.cpp 
    DicomHandler.cpp 
    ImageProcessor.cpp
)

# 4. Vincular librerías
target_link_libraries(IntegradorApp ${OpenCV_LIBS} ${ITK_LIBRARIES})