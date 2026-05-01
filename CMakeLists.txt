cmake_minimum_required(VERSION 3.16...3.30)
project(bookreader VERSION 1.4.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(QTREQUIREDLIBS Core Gui Widgets Sql)
set(QTREQUIREDLIBS2)
set(QTOPTIONALLIBS)

if(WIN32)
    list(APPEND QTOPTIONALLIBS TextToSpeech)
else()
    list(APPEND QTREQUIREDLIBS TextToSpeech)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QTREQUIREDLIBS} ${QTREQUIREDLIBS2})
if(${QT_VERSION_MAJOR} GREATER 5)
    list(APPEND QTREQUIREDLIBS Core5Compat)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QTREQUIREDLIBS} ${QTREQUIREDLIBS2} OPTIONAL_COMPONENTS ${QTOPTIONALLIBS})

set(QT_DEFAULT_MAJOR_VERSION ${QT_VERSION_MAJOR})

add_subdirectory(3rdparty/QHotkey)

if(QT_FOUND)
  set(QTLINKLIBS "")
  foreach(X IN LISTS QTREQUIREDLIBS)
    list(APPEND QTLINKLIBS "Qt${QT_VERSION_MAJOR}::${X}")
  endforeach()
  message(STATUS "${QTLINKLIBS}")

  file(GLOB PROJECT_SOURCES *.cpp;*.h;*.qrc;*.ui;core/*.cpp;core/*.h;dialog/*.cpp;dialog/*.h;dialog/*.ui)
  if(Qt${QT_VERSION_MAJOR}TextToSpeech_FOUND)
      set(TEXTTOSPEECH_SOURCES)
  else()
      file(GLOB TEXTTOSPEECH_SOURCES texttospeech/*.cpp texttospeech/*.h)
  endif()
  list(APPEND PROJECT_SOURCES ${TEXTTOSPEECH_SOURCES})
  if(WIN32)
      enable_language(RC)
      list(APPEND PROJECT_SOURCES icon.rc)
  endif()

  set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

  if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(bookreader MANUAL_FINALIZATION ${PROJECT_SOURCES})
  else()
    if(ANDROID)
      add_library(bookreader SHARED ${PROJECT_SOURCES})
    else()
      add_executable(bookreader ${PROJECT_SOURCES})
    endif()
  endif()

  target_link_libraries(bookreader PRIVATE ${QTLINKLIBS} QHotkey::QHotkey)
  if(Qt${QT_VERSION_MAJOR}TextToSpeech_FOUND)
      target_link_libraries(bookreader PRIVATE Qt${QT_VERSION_MAJOR}::TextToSpeech)
      target_compile_definitions(bookreader PRIVATE BOOKREADER_USE_QTSPEECH)
      message("use qt texttospeech")
  else()
      message("do not use qt texttospeech")
  endif()
  target_compile_definitions(bookreader PRIVATE APP_VERSION=\"${PROJECT_VERSION}\")
  target_precompile_headers(bookreader PRIVATE pch.h)
  target_include_directories(bookreader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/QHotkey/QHotkey)

  if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER popkc.bookreader.gui)
  endif()
  set_target_properties(
    bookreader
    PROPERTIES ${BUNDLE_ID_OPTION} MACOSX_BUNDLE_BUNDLE_VERSION
               ${PROJECT_VERSION} MACOSX_BUNDLE_SHORT_VERSION_STRING
               ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR} MACOSX_BUNDLE
               TRUE WIN32_EXECUTABLE
               TRUE AUTOUIC
               ON AUTOMOC
               ON AUTORCC
               ON)

  include(GNUInstallDirs)
  install(
    TARGETS bookreader
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(bookreader)
  endif()
endif()
