# Get direcotry name.
GET_FILENAME_COMPONENT(MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Get all cpp files from src/.
FILE(GLOB LIB${MODULE_NAME}_SRC "./src/*.cpp")

# Set include directory.
INCLUDE_DIRECTORIES("./inc")

# Set the path to save library.
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Add util as the target.
ADD_LIBRARY(${MODULE_NAME} STATIC ${LIB${MODULE_NAME}_SRC})
