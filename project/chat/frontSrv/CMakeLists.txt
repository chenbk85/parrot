# Set depend modules.
# Put the dependencies to the back position
# E.g., sys dependents util, so, the sequence is sys util.
SET(DEPEND_BASE_MODULE sys util)
SET(DEPEND_COMPONENT_MODULE servergear websocket json)
SET(DEPEND_THIRD_PARTY RapidJson OpenSSL)
SET(DEPEND_PRJ_MODULE common)

# Get direcotry name.
GET_FILENAME_COMPONENT(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get all cpp files from src/.
FILE(GLOB ${MODULE_NAME}_SRC "./src/*.cpp")

# Set include directory.
INCLUDE_DIRECTORIES("./inc")

# Find header of base modules.
FOREACH(module ${DEPEND_BASE_MODULE})
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base/${module}/inc)
ENDFOREACH(module)

# Find header of component modules.
FOREACH(module ${DEPEND_COMPONENT_MODULE})
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/component/${module}/inc)
ENDFOREACH(module)

FOREACH(module ${DEPEND_THIRD_PARTY})
  FIND_PACKAGE(${module})
  STRING(TOUPPER ${module} MODULE)
  IF(${MODULE}_FOUND)
    INCLUDE_DIRECTORIES(${${MODULE}_INCLUDE_DIR})
  ELSE(${MODULE}_FOUND)
    MESSAGE(FATAL_ERROR "Failed to find ${module}")
  ENDIF(${MODULE}_FOUND)
ENDFOREACH(module)

# Find header of project modules.
FOREACH(module ${DEPEND_PRJ_MODULE})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../${module}/inc)
ENDFOREACH(module)

# Set the path to save library.
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${THIS_PRJ_NAME}/bin)

# Add target.
ADD_EXECUTABLE(${MODULE_NAME} ${${MODULE_NAME}_SRC})

# Add link libraries.
TARGET_LINK_LIBRARIES(${MODULE_NAME}
        ${DEPEND_PRJ_MODULE}
        ${DEPEND_COMPONENT_MODULE}
        ${DEPEND_BASE_MODULE})

FOREACH(module ${DEPEND_THIRD_PARTY})
  STRING(TOUPPER ${module} MODULE)
  TARGET_LINK_LIBRARIES(${MODULE_NAME} ${${MODULE}_LIBRARY} ${${MODULE}_LIBRARIES})
ENDFOREACH(module)

TARGET_LINK_LIBRARIES(${MODULE_NAME} pthread dl)

# Add dependencies.
ADD_DEPENDENCIES(${MODULE_NAME}
        ${DEPEND_BASE_MODULE}
        ${DEPEND_COMPONENT_MODULE}
        ${DEPEND_PRJ_MODULE})
