
# This script reads version from vcpkg.json and sets it to ${MSDFGEN_VERSION} etc.

cmake_minimum_required(VERSION 3.15)

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../vcpkg.json" MSDFGEN_VCPKG_JSON)

string(REGEX MATCH "\"version\"[ \t\n\r]*:[ \t\n\r]*\"[^\"]*\"" MSDFGEN_TMP_VERSION_PAIR ${MSDFGEN_VCPKG_JSON})
string(REGEX REPLACE "\"version\"[ \t\n\r]*:[ \t\n\r]*\"([^\"]*)\"" "\\1" MSDFGEN_VERSION ${MSDFGEN_TMP_VERSION_PAIR})
string(REGEX REPLACE "^([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\1" MSDFGEN_VERSION_MAJOR ${MSDFGEN_VERSION})
string(REGEX REPLACE "^([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\2" MSDFGEN_VERSION_MINOR ${MSDFGEN_VERSION})
string(REGEX REPLACE "^([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\3" MSDFGEN_VERSION_REVISION ${MSDFGEN_VERSION})
string(LENGTH ${MSDFGEN_VERSION} MSDFGEN_VERSION_LENGTH)
string(REPEAT "-" ${MSDFGEN_VERSION_LENGTH} MSDFGEN_VERSION_UNDERLINE)
string(TIMESTAMP MSDFGEN_COPYRIGHT_YEAR "%Y")

unset(MSDFGEN_TMP_VERSION_PAIR)
unset(MSDFGEN_VERSION_LENGTH)
unset(MSDFGEN_VCPKG_JSON)
