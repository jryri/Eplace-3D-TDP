ADD_EXECUTABLE(
    ePlaceTest
    binDensity_test.cpp
    eplace.cpp
    eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/Parser/parser.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/Plot/plot.cpp
    ${PROJECT_SOURCE_DIR}/QPlace/qplace.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)

ADD_EXECUTABLE(
    ePlace3DTest
    test_3d_basic.cpp
    eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg3d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)

ADD_EXECUTABLE(
    ePlace3DDensityTest
    test_3d_density.cpp
    eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg3d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)

ADD_EXECUTABLE(
    ePlace3DIntegrationTest
    test_3d_integration_simple.cpp
    eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg3d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)

ADD_EXECUTABLE(
    ePlace2Dto3DTest
    test_2d_to_3d.cpp
    eplace_3d.cpp
    ${PROJECT_SOURCE_DIR}/Parser/parser.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/arghandler.cpp
    ${PROJECT_SOURCE_DIR}/PlaceCommon/objects.cpp
    ${PROJECT_SOURCE_DIR}/PlaceDB/placedb.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fft.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg3d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg2d.cpp
    ${PROJECT_SOURCE_DIR}/FFT/fftsg.cpp
)

INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/Parser
	${PROJECT_SOURCE_DIR}/PlaceCommon
	${PROJECT_SOURCE_DIR}/PlaceDB
    ${PROJECT_SOURCE_DIR}/Plot
    ${PROJECT_SOURCE_DIR}/Library/eigen-git-mirror
    ${PROJECT_SOURCE_DIR}/Library/CImg
    ${PROJECT_SOURCE_DIR}/QPlace
    ${PROJECT_SOURCE_DIR}/FFT
    ${PROJECT_SOURCE_DIR}/Optimization
    )

target_link_libraries(ePlaceTest "-lgomp -lpthread -lm -ldl -lX11")
target_link_libraries(ePlace3DTest "-lgomp -lpthread -lm -ldl")
target_link_libraries(ePlace3DDensityTest "-lgomp -lpthread -lm -ldl")
target_link_libraries(ePlace3DIntegrationTest "-lgomp -lpthread -lm -ldl")
target_link_libraries(ePlace2Dto3DTest "-lgomp -lpthread -lm -ldl")