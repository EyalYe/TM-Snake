# TaskMaster-C3 app project — top-level ESP-IDF project file.
cmake_minimum_required(VERSION 3.16)

# Regenerate the build manifest from apps.yaml BEFORE the component manager (invoked
# by project.cmake) resolves dependencies. apps.yaml is the only file you edit;
# main/idf_component.yml is a generated, git-ignored artifact. Runs on every configure.
# Re-run configure (→ recompose) whenever apps.yaml changes, so editing it + rebuilding
# actually picks up the change.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/apps.yaml")
find_package(Python3 QUIET COMPONENTS Interpreter)
if(NOT Python3_EXECUTABLE)
    set(Python3_EXECUTABLE python3)
endif()
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/tools/compose_apps.py"
    RESULT_VARIABLE _compose_rc)
if(NOT _compose_rc EQUAL 0)
    message(FATAL_ERROR "compose_apps.py failed — check apps.yaml")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(taskmaster_c3_app)
