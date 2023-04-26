#--------------------------------------------------------------------------
# server project
#--------------------------------------------------------------------------

PROJECT(server)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("server" FILES ${files_project})

ADD_EXECUTABLE(server ${files_project})
TARGET_LINK_LIBRARIES(server core render game)
ADD_DEPENDENCIES(server core render game)

IF(MSVC)
    set_property(TARGET server PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()