#--------------------------------------------------------------------------
# spacegame project
#--------------------------------------------------------------------------

PROJECT(spacegame)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("spacegame" FILES ${files_project})
SET(files_proto)
FLAT_COMPILE(proto.fbs)

ADD_EXECUTABLE(spacegame ${files_project} ${files_proto})
TARGET_LINK_LIBRARIES(spacegame core render game)
ADD_DEPENDENCIES(spacegame core render game)

IF(MSVC)
    set_property(TARGET spacegame PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()