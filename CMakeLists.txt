cmake_minimum_required(VERSION 3.1)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS true)

project(gsage)

include(cmake/Functions.cmake)
configure()

include(${CMAKE_SOURCE_DIR}/conanbuildinfo.cmake OPTIONAL)

if(CONAN_PACKAGE_NAME)
  set(CMAKE_INSTALL_PATH "./")
  include(cmake/ConanDependencies.cmake)
else(CONAN_PACKAGE_NAME)
  set(CMAKE_INSTALL_PATH "/usr/local")
  include(cmake/Dependencies.cmake)
endif(CONAN_PACKAGE_NAME)

process_templates()

add_subdirectory(Core)
add_subdirectory(PlugIns)
add_subdirectory(Tests)
add_subdirectory(Vendor)
if(OGRE_FOUND)
  add_subdirectory(Editor)
endif(OGRE_FOUND)

install(DIRECTORY cmake DESTINATION "${CMAKE_INSTALL_PATH}/share/gsage")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bundle/"
        DESTINATION "${CMAKE_INSTALL_PATH}"
        FILES_MATCHING REGEX ".+")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/"
        DESTINATION "${CMAKE_INSTALL_PATH}/resources"
        COMPONENT "resources")

set(CPACK_PACKAGE_NAME "GSAGE Engine")
set(CPACK_PACKAGE_VERSION_MAJOR ${GSAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${GSAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${GSAGE_VERSION_PATCH})

set(CPACK_COMPONENT_resources_REQUIRED true)

set(CPACK_CREATE_DESKTOP_LINKS editor)
set(CPACK_PACKAGE_EXECUTABLES editor "GSAGE Editor")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
  # pack
  set (A_PACK_DESCRIPTION_SUMMARY "GSAGE engine")
  set (A_INSTALL_PREFIX Consolas)

  set(CPACK_WIX_PRODUCT_GUID "1A33E980-D505-41A4-B4E1-B70D96E2DC4E")
  set(CPACK_WIX_UPGRADE_GUID "1A33E980-D505-41A4-B4E1-B70D96E2DC4F")
  set(CPACK_WIX_LICENSE_RTF "${CMAKE_SOURCE_DIR}/resources/editor/license.rtf")

  set(CPACK_GENERATOR "WIX")
endif(WIN32)

include(CPack)
