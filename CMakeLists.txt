cmake_minimum_required(VERSION 3.19)
project(Switch_Board_qt LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts PrintSupport)
# 查找 PkgConfig 模块
find_package(PkgConfig REQUIRED)

# 使用 pkg_check_modules 查找 libusb
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

qt_standard_project_setup()

qt_add_executable(Switch_Board_qt
    WIN32 MACOSX_BUNDLE
    main.cpp
    switch_board.cpp
    switch_board.h
    switch_board.ui
    app_libusb.h app_libusb.cpp
    qcustomplot.cpp qcustomplot.h
)

target_link_libraries(Switch_Board_qt
    PRIVATE
        Qt::Core
        Qt::Widgets
	Qt::Charts
        Qt::PrintSupport
	${LIBUSB_LIBRARIES}
)

include(GNUInstallDirs)

install(TARGETS Switch_Board_qt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Switch_Board_qt
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
