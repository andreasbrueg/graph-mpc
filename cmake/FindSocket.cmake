find_package(OpenSSL REQUIRED)

if(SOCKET_DIR)
  # If user-specified folders: look there
  find_library(
    SOCKET_LIBRARY 
    NAMES socket libsocket
    PATHS /usr/local/lib/
    PATH_SUFFIXES lib "${CMAKE_INSTALL_LIBDIR}"
    NO_DEFAULT_PATH
    DOC "Socket library")

  find_path(
    SOCKET_HEADERS
    NAMES Socket.h SecureSocket.h
    PATHS ${SOCKET_DIR}
    PATH_SUFFIXES include/socket
    NO_DEFAULT_PATH
    DOC "Socket headers")

else(SOCKET_DIR)
  # Else: look in default paths
  find_library(
    SOCKET_LIB
    NAMES socket libsocket
    PATH_SUFFIXES lib "${CMAKE_INSTALL_LIBDIR}"
    DOC "Socket library")

  find_path(
    SOCKET_HEADERS
    NAMES Socket.h SecureSocket.h
    PATH_SUFFIXES include/socket
    DOC "Socket headers")
endif(SOCKET_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SOCKET DEFAULT_MSG SOCKET_HEADERS SOCKET_LIB)


if(SOCKET_FOUND)
  add_library(SOCKET STATIC IMPORTED)
  set_target_properties(SOCKET PROPERTIES IMPORTED_LOCATION ${SOCKET_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${SOCKET_HEADERS}/..)
endif(SOCKET_FOUND)
