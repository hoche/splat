find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(GDAL REQUIRED)
find_package(Threads REQUIRED)

include_directories(${PNG_INCLUDE_DIR})
include_directories(${GDAL_INCLUDE_DIR})

add_executable(splat
    anf.cpp
    antenna_pattern.cpp
    boundary_file.cpp
    city_file.cpp
    dem.cpp
    elevation_map.cpp
    gnuplot.cpp
    itwom3.0.cpp
    kml.cpp
    json.cpp
    lrp.cpp
    main.cpp
    path.cpp
    imagewriter.cpp
    image.cpp
    region.cpp
    report.cpp
    sdf.cpp
    sdf_bz.cpp
    site.cpp
    splat_run.cpp
    udt.cpp
    utilities.cpp
    workqueue.cpp)

target_include_directories(splat PRIVATE ${cli11_SOURCE_DIR}/include)    
target_link_libraries(splat bz2 ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${GDAL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS splat DESTINATION bin)
