# PLUGIN

pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-controller-1.0)

option(WITH_GST "GST Shmdata" ${GST_FOUND})
add_feature_info("gst" WITH_GST "GST Shmdata")

if (WITH_GST)

    pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-controller-1.0)

    add_compile_options(${GST_CFLAGS})

    # Replaces config.h
    add_definitions(
        -DPACKAGE="${PACKAGE_NAME}" # For gstplugin.h
        -DPACKAGE_NAME="${PACKAGE_NAME}"
        -DPACKAGE_URL="${PACKAGE_URL}"
    )

    include_directories(
        ${SHMDATA_DIR}
        ${GST_INCLUDE_DIRS}
    )

    link_directories(
        ${GST_LIBRARY_DIRS}
    )

    link_libraries(
        ${SHMDATA_LIBRARY}
        ${GST_LIBRARIES}
    )

    # Using MODULE as this needs to be an *.so file on mac, not a *.dylib
    add_library(gstshmdata MODULE
        gstshmdata.c
        gstshmdatasink.c
        gstshmdatasrc.c
        )

    # TEST

    add_executable(check-shmdatasrc check-shmdatasrc.c)
    add_test(check-shmdatasrc check-shmdatasrc)

    add_executable(check-shmdatasink check-shmdatasink.c)
    add_test(check-shmdatasink check-shmdatasink)

    # INSTALL

    install(TARGETS gstshmdata LIBRARY DESTINATION lib/gstreamer-1.0)

endif ()
