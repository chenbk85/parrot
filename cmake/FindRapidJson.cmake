SET(RAPIDJSON_DIR ${PROJECT_SOURCE_DIR}/third-party/${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}/rapidjson)

IF(EXISTS ${RAPIDJSON_DIR}/include)
  SET(RAPIDJSON_FOUND TRUE)
  SET(RAPIDJSON_INCLUDE_DIR ${RAPIDJSON_DIR}/include)
ELSE(EXISTS ${RAPIDJSON_DIR}/include)
  MESSAGE(FATAL_ERROR "Failed to find rapidjson direcotry.")
ENDIF(EXISTS ${RAPIDJSON_DIR}/include)

IF(RAPIDJSON_FOUND)
  IF(NOT RAPIDJSON_FIND_QUIETLY)
    MESSAGE(STATUS "FOUND RAPIDJSON.")
  ENDIF (NOT RAPIDJSON_FIND_QUIETLY)
ELSE(RAPIDJSON_FOUND)
  IF(RAPIDJSON_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "CANNOT FIND RAPIDJSON.")
  ENDIF (RAPIDJSON_FIND_REQUIRED)
ENDIF(RAPIDJSON_FOUND)
