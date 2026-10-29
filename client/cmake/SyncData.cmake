# Mirrors SOURCE_DIR into DEST_DIR, copying only files that changed. game.ini is the player's
# settings file the game writes at runtime, so it is only seeded when missing, never overwritten.
cmake_minimum_required(VERSION 3.21)

file(GLOB_RECURSE data_files RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/*")
foreach(data_file IN LISTS data_files)
    if(data_file STREQUAL "game.ini" AND EXISTS "${DEST_DIR}/${data_file}")
        continue()
    endif()
    cmake_path(GET data_file PARENT_PATH data_subdir)
    file(MAKE_DIRECTORY "${DEST_DIR}/${data_subdir}")
    file(COPY_FILE "${SOURCE_DIR}/${data_file}" "${DEST_DIR}/${data_file}" ONLY_IF_DIFFERENT)
endforeach()
