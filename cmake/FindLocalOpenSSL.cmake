SET(OPENSSL_DIR ${PROJECT_SOURCE_DIR}/third-party/${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}/openssl)

IF(EXISTS ${OPENSSL_DIR}/include)
  SET(OPENSSL_INCLUDE_DIR ${OPENSSL_DIR}/include)
ELSE(EXISTS ${OPENSSL_DIR}/include)
  MESSAGE(FATAL_ERROR "Failed to find rapidjson direcotry.")
ENDIF(EXISTS ${OPENSSL_DIR}/include)

FIND_LIBRARY(CRYPTO_LIBRARY NAMES libcrypto.a PATHS ${OPENSSL_DIR}/lib)
FIND_LIBRARY(SSL_LIBRARY NAMES libssl.a PATHS ${OPENSSL_DIR}/lib)

IF(CRYPTO_LIBRARY AND SSL_LIBRARY)
  SET(OPENSSL_LIBRARIES ${CRYPTO_LIBRARY} ${SSL_LIBRARY})
  SET(OPENSSL_FOUND TRUE)
ENDIF(CRYPTO_LIBRARY AND SSL_LIBRARY)

IF(OPENSSL_FOUND)
  IF(NOT OPENSSL_FIND_QUIETLY)
    MESSAGE(STATUS "FOUND OPENSSL.")
  ENDIF (NOT OPENSSL_FIND_QUIETLY)
ELSE(OPENSSL_FOUND)
  IF(OPENSSL_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "CANNOT FIND OPENSSL.")
  ENDIF (OPENSSL_FIND_REQUIRED)
ENDIF(OPENSSL_FOUND)
