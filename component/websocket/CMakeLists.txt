# Set depend modules.
SET(DEPEND_BASE_MODULE util sys)
SET(DEPEND_COMPONENT_MODULE json)
SET(DEPEND_THIRD_PARTY RapidJson OpenSSL)

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

# Add third party headers.
FOREACH(module ${DEPEND_THIRD_PARTY})
  STRING(TOUPPER ${module} MODULE)
  INCLUDE_DIRECTORIES(${${MODULE}_INCLUDE_DIR})
ENDFOREACH(module)

# Set the path to save library.
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Add library.
ADD_LIBRARY(${MODULE_NAME} STATIC ${LIB${MODULE_NAME}_SRC})

# Add dependencies.
ADD_DEPENDENCIES(${MODULE_NAME} ${DEPEND_BASE_MODULE} ${DEPEND_COMPONENT_MODULE})
