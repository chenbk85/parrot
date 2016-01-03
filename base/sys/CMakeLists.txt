SET(DEPEND_BASE_MODULE util)

# Get direcotry name.
GET_FILENAME_COMPONENT(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get all cpp files from src/.
FILE(GLOB LIB${MODULE_NAME}_SRC "./src/*.cpp")

# Set include directory.
INCLUDE_DIRECTORIES("./inc")

FOREACH(module ${DEPEND_BASE_MODULE})
  INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/base/${module}/inc)
ENDFOREACH(module)

# Set the path to save library.
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Add util as the target.
ADD_LIBRARY(${MODULE_NAME} STATIC ${LIB${MODULE_NAME}_SRC})

# Add dependencies.
ADD_DEPENDENCIES(${MODULE_NAME} util)
