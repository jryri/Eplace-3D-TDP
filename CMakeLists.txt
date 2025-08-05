PROJECT (ePlace)
#SET(CMAKE_C_FLAGS "-O3 -w  -funroll-loops -ffast-math -funit-at-a-time -fPIC")
# SET(CMAKE_C_FLAGS "-m64 -lX11 -O2 -fPIC -DNDEBUG -ffast-math -Dcimg_display=1 ")
SET(CMAKE_CXX_FLAGS "-w -m64 -O3 -fpermissive -std=c++1y -fPIC -fopenmp -funroll-loops -ffast-math -Dcimg_display=1")
#SET(CMAKE_CXX_FLAGS "-O3 -w -funroll-loops -ffast-math -funit-at-a-time -fPIC -DNDEBUG")
#SET(CMAKE_EXE_LINKER_FLAGS "-static")
ADD_SUBDIRECTORY (QPlace)
ADD_SUBDIRECTORY (Parser)
ADD_SUBDIRECTORY (EPlace)
ADD_SUBDIRECTORY (Optimization)
ADD_SUBDIRECTORY (Legalization)
ADD_SUBDIRECTORY (main)
ADD_EXECUTABLE(
    main3D
    ${PROJECT_SOURCE_DIR}/main/ePlace_main.cpp
    ${PROJECT_SOURCE_DIR}/EPlace/eplace.cpp
    ${PROJECT_SOURCE_DIR}/EPlace/eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/Parser/parser.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/Plot/plot.cpp
    ${PROJECT_SOURCE_DIR}/QPlace/qplace.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg3d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
    ${PROJECT_SOURCE_DIR}/DetailedPlacement/detailed.cpp
    ${PROJECT_SOURCE_DIR}/OpenroadInterface/interface.cpp
    ${PROJECT_SOURCE_DIR}/Legalization/legalizer.cpp
)

# Include directories for main3D
target_include_directories(main3D PRIVATE
    ${PROJECT_SOURCE_DIR}/PlaceCommon
    ${PROJECT_SOURCE_DIR}/PlaceDB
    ${PROJECT_SOURCE_DIR}/EPlace
    ${PROJECT_SOURCE_DIR}/Parser
    ${PROJECT_SOURCE_DIR}/Plot
    ${PROJECT_SOURCE_DIR}/QPlace
    ${PROJECT_SOURCE_DIR}/FFT
    ${PROJECT_SOURCE_DIR}/DetailedPlacement
    ${PROJECT_SOURCE_DIR}/OpenroadInterface
    ${PROJECT_SOURCE_DIR}/Legalization
    ${PROJECT_SOURCE_DIR}/Optimization
    ${PROJECT_SOURCE_DIR}/Library/eigen-git-mirror
    ${PROJECT_SOURCE_DIR}/Library/CImg
)

target_link_libraries(main3D "-lgomp -lpthread -lm -ldl -lX11")