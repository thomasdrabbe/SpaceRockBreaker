# Verhoogt patch in ${CMAKE_SOURCE_DIR}/version.txt (x.y.(z+1)) en synct Inno Setup:
# installer.iss → #define AppVersion "x.y.z"
#
# Alleen semver x.y.z; optioneel "+" build-metadata op dezelfde regel wordt genegeerd bij parsen.

# Bij `cmake -P` is CMAKE_SOURCE_DIR de build-directory, niet het projectroot,
# daarom expliciet via SRB_SOURCE_DIR doorgeven.
if(NOT DEFINED SRB_SOURCE_DIR)
    message(FATAL_ERROR "bump_app_version.cmake: SRB_SOURCE_DIR not set")
endif()

set(VERSION_FILE "${SRB_SOURCE_DIR}/version.txt")
set(INNO_FILE "${SRB_SOURCE_DIR}/installer.iss")

if(NOT EXISTS "${VERSION_FILE}")
    message(FATAL_ERROR "bump_app_version.cmake: missing ${VERSION_FILE}")
endif()

if(NOT EXISTS "${INNO_FILE}")
    message(FATAL_ERROR "bump_app_version.cmake: missing ${INNO_FILE}")
endif()

file(READ "${VERSION_FILE}" VERSION_RAW)

# trim (strip CR/LF/spaces/tab)
string(REGEX REPLACE "^[ \t\r\n]+" "" LINE "${VERSION_RAW}")
string(REGEX REPLACE "[ \t\r\n]+$" "" LINE "${LINE}")

# eerste token vóór spatie/tab of '+'
string(REGEX MATCH "^[^\t \\+]+" BASE "${LINE}")
if(NOT BASE MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "bump_app_version.cmake: expected semver x.y.z in version.txt first line (got '${LINE}').")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$" _ "${BASE}")
set(VMAJ "${CMAKE_MATCH_1}")
set(VMIN "${CMAKE_MATCH_2}")
set(VPAT "${CMAKE_MATCH_3}")
math(EXPR VPAT_INC "${VPAT} + 1")

set(NEXT_SEMVER "${VMAJ}.${VMIN}.${VPAT_INC}")
set(REST "")
string(LENGTH "${BASE}" NB)
string(LENGTH "${LINE}" NL)
if(NL GREATER NB)
    string(SUBSTRING "${LINE}" ${NB} -1 REST)
endif()

set(NEXT_LINE "${NEXT_SEMVER}${REST}")

file(WRITE "${VERSION_FILE}" "${NEXT_LINE}\n")

message(STATUS "Bumped ${VERSION_FILE} → ${NEXT_LINE}")

# installer.iss: #define AppVersion   "..."
file(READ "${INNO_FILE}" INNO_TEXT)
string(REGEX REPLACE "(#define[ \t]+AppVersion[ \t]+)\"([^\"]*)\"" "\\1\"${NEXT_SEMVER}\"" INNO_NEW "${INNO_TEXT}")
file(WRITE "${INNO_FILE}" "${INNO_NEW}")

message(STATUS "Synced installer.iss AppVersion → ${NEXT_SEMVER}")
