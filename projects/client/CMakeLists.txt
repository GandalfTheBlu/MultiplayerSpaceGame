#--------------------------------------------------------------------------
# client project
#--------------------------------------------------------------------------

PROJECT(client)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("client" FILES ${files_project})

ADD_EXECUTABLE(client ${files_project})
TARGET_LINK_LIBRARIES(client core render game)
ADD_DEPENDENCIES(client core render game)

IF(MSVC)
    set_property(TARGET client PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF()