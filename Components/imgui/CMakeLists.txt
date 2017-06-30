set(LIB_NAME "imgui")

include_directories(./)
FILE(GLOB sources ./*.cpp)

include_directories(
  ${OGRE_INCLUDE_DIRS}
  ${OIS_INCLUDE_DIRS}
)

add_library(${LIB_NAME} STATIC ${sources})

set_target_properties(${LIB_NAME} PROPERTIES DEBUG_POSTFIX _d)

target_link_libraries(${LIB_NAME} ${OGRE_LIBRARIES})
