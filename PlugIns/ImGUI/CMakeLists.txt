include_directories(
  ${gsage_SOURCE_DIR}/PlugIns/ImGUI/Common/include
  ${gsage_SOURCE_DIR}/Vendor/easylogging/include
  ${gsage_SOURCE_DIR}/Vendor/cpp-channel/include
  ${gsage_SOURCE_DIR}/Vendor/sole
  ${gsage_SOURCE_DIR}/Vendor/gmath/src
  ${gsage_SOURCE_DIR}/PlugIns/OgrePlugin/include
  ${OGRE_INCLUDE_DIRS}
  ${CONAN_INCLUDE_DIRS_IMVUE}
)

add_subdirectory(Common)
if(OGRE_FOUND)
  add_subdirectory(OgreInterfaces)
endif(OGRE_FOUND)

