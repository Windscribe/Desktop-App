set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

# Making a universal binary for Mac with lipo util
if(VCPKG_TARGET_IS_OSX)
   set(CTRLD_FILE_NAME_ARM64 "ctrld_${VERSION}_darwin_arm64")
   set(CTRLD_FILE_NAME_AMD64 "ctrld_${VERSION}_darwin_amd64")

   vcpkg_download_distfile(ARCHIVE_ARM64
      URLS "https://github.com/Control-D-Inc/ctrld/releases/download/v${VERSION}/${CTRLD_FILE_NAME_ARM64}.tar.gz"
      FILENAME "${CTRLD_FILE_NAME_ARM64}.tar.gz"
      SKIP_SHA512
   )
   vcpkg_download_distfile(ARCHIVE_AMD64
      URLS "https://github.com/Control-D-Inc/ctrld/releases/download/v${VERSION}/${CTRLD_FILE_NAME_AMD64}.tar.gz"
      FILENAME "${CTRLD_FILE_NAME_AMD64}.tar.gz"
      SKIP_SHA512
   )

   vcpkg_extract_source_archive(SOURCE_PATH_ARM64
      ARCHIVE "${ARCHIVE_ARM64}"
   )
   vcpkg_extract_source_archive(SOURCE_PATH_AMD64
      ARCHIVE "${ARCHIVE_AMD64}"
   )

   vcpkg_execute_build_process(
      COMMAND "lipo" "${SOURCE_PATH_AMD64}/${CTRLD_FILE_NAME_AMD64}/ctrld" "${SOURCE_PATH_ARM64}/${CTRLD_FILE_NAME_ARM64}/ctrld" -create -output ctrld
      WORKING_DIRECTORY "${SOURCE_PATH_AMD64}"
      LOGNAME "ctrld_${TARGET_TRIPLET}.log"
   )
   vcpkg_copy_tools(TOOL_NAMES ctrld${EXECUTABLE_EXTENSION}
                    SEARCH_DIR ${SOURCE_PATH_AMD64}
                    AUTO_CLEAN)

else()
   if(VCPKG_TARGET_IS_WINDOWS)
      set(ARCHIVE_EXTENSION "zip")
      if(VCPKG_TARGET_ARCHITECTURE MATCHES "x64")
         set(CTRLD_FILE_NAME "ctrld_${VERSION}_windows_amd64")
      elseif(VCPKG_TARGET_ARCHITECTURE MATCHES "arm64")
         set(CTRLD_FILE_NAME "ctrld_${VERSION}_windows_arm64")
      else()
         message(FATAL_ERROR "Unsupported Windows architecture.")
      endif()
   elseif(VCPKG_TARGET_IS_LINUX)
      set(ARCHIVE_EXTENSION "tar.gz")
      if(VCPKG_TARGET_ARCHITECTURE MATCHES "x64")
         set(CTRLD_FILE_NAME "ctrld_${VERSION}_linux_amd64")
      elseif(VCPKG_TARGET_ARCHITECTURE MATCHES "arm64")
         set(CTRLD_FILE_NAME "ctrld_${VERSION}_linux_arm64")
      else()
         message(FATAL_ERROR "Unsupported Linux architecture.")
      endif()
   else()
      message(FATAL_ERROR "Unsupported platform.")
   endif()

   vcpkg_download_distfile(ARCHIVE
      URLS "https://github.com/Control-D-Inc/ctrld/releases/download/v${VERSION}/${CTRLD_FILE_NAME}.${ARCHIVE_EXTENSION}"
      FILENAME "${CTRLD_FILE_NAME}.${ARCHIVE_EXTENSION}"
      SKIP_SHA512
   )
   vcpkg_extract_source_archive(SOURCE_PATH
      ARCHIVE "${ARCHIVE}"
   )

   vcpkg_copy_tools(TOOL_NAMES ctrld
                    SEARCH_DIR ${SOURCE_PATH}/${CTRLD_FILE_NAME}
                    AUTO_CLEAN)
endif()


vcpkg_download_distfile(LICENSE_PATH
    URLS "https://raw.githubusercontent.com/Control-D-Inc/ctrld/main/LICENSE"
    FILENAME "CTRLD_LICENSE"
    SKIP_SHA512
)

file(INSTALL "${LICENSE_PATH}" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)