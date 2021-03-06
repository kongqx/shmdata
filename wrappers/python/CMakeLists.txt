pkg_check_modules(PYTHON python3)

option(WITH_PYTHON "Python Wrapper" ${PYTHON_FOUND})
add_feature_info("wrapper-python" WITH_PYTHON "Python Wrapper")

if (WITH_PYTHON)

    find_package(PythonInterp 3 REQUIRED)
    pkg_check_modules(PYTHON REQUIRED python3)

    link_directories(${PYTHON_LIBRARY_DIRS})
    add_compile_options(${PYTHON_CFLAGS})
    add_definitions(-Wno-error=missing-field-initializers)

    add_library(pyshmdata SHARED
        pyshmdata.cpp
        )
    set_target_properties(pyshmdata PROPERTIES PREFIX "")

    target_include_directories(pyshmdata
        PRIVATE ${SHMDATA_DIR}
        PRIVATE ${PYTHON_INCLUDE_DIRS}
    )

    target_link_libraries(pyshmdata
        PRIVATE ${SHMDATA_LIBRARY}
        PRIVATE ${PYTHON_LIBRARIES}
    )

    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils import sysconfig; print((sysconfig.get_python_lib(1,0)).split(sys.prefix+'/')[1])"
        OUTPUT_VARIABLE PYTHON_DIST_PACKAGES
        )

    # The result of the preceding command end with a line return, we have to get rid of it
    string(REGEX REPLACE "(.*)[\n\r]" "\\1" PYTHON_DIST_PACKAGES ${PYTHON_DIST_PACKAGES} )

    # INSTALL

    install(TARGETS pyshmdata LIBRARY DESTINATION ${PYTHON_DIST_PACKAGES})

endif ()