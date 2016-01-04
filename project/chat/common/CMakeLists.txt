# Set depend modules.
SET(DEPEND_BASE_MODULE util sys)
SET(DEPEND_COMPONENT_MODULE json websocket servergear)
SET(DEPEND_PRJ_MODULE)
SET(DEPEND_THIRD_PARTY RapidJson OpenSSL) # OpenSSL is case sensitive.

# Get direcotry name.
GET_FILENAME_COMPONENT(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get all cpp files from src/.
FILE(GLOB LIB${MODULE_NAME}_SRC "./src/*.cpp")

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
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/${THIS_PRJ_NAME})

# Add library.
ADD_LIBRARY(${MODULE_NAME} STATIC ${LIB${MODULE_NAME}_SRC})

# Add dependencies.
ADD_DEPENDENCIES(${MODULE_NAME} ${DEPEND_BASE_MODULE} ${DEPEND_COMPONENT_MODULE} ${DEPEND_PRJ_MODULE})
