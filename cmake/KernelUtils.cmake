# Purpose: build multiple kernels with different parameters from one source
# This function will generate .spv files in output directory from one specified source file
# Usage:
# KernelUtils_build_kernels_from_one_source(
#               SOURCE <path to source file>
#               OUTPUT_DIRECTORY <path to directory with output files>
#               LIMITS_FILE <path to optional limits file>
#               DEPENDENCY_FILES <additional files on which build commands depend>
#               PARAMETERS <optional common parameters for all output files>
#               OUTPUTS <output files with corresponding parameters>)
#
# Output description syntax: parameters for each output file is separated by commas.
# Then, after a colon, output filename is written. Each entry need to be enclosed in quotes:
# "PARAMETER_11, PARAMETER_12, ..., PARAMETER_N1: output_file_name_1"
# "PARAMETER_21, PARAMETER_22, ..., PARAMETER_N2: output_file_name_2"
# ...
# "PARAMETER_K1, PARAMETER_K2, ..., PARAMETER_NK: output_file_name_K"
function(KernelUtils_build_kernels_from_one_source)
    set(options)
    set(oneValueArgs SOURCE OUTPUT_DIRECTORY LIMITS_FILE)
    set(multiValueArgs DEPENDENCY_FILES PARAMETERS OUTPUTS)

    cmake_parse_arguments(BUILD_KERNELS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # If source or output directory are relative (or not specified), add path to current CMakeLists.txt first
    if (NOT IS_ABSOLUTE ${BUILD_KERNELS_SOURCE})
        set(BUILD_KERNELS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_KERNELS_SOURCE})
    endif ()

    if (NOT IS_ABSOLUTE ${BUILD_KERNELS_OUTPUT_DIRECTORY})
        set(BUILD_KERNELS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_KERNELS_OUTPUT_DIRECTORY})
    endif ()

    # Set glsl compiler
    set(GLSL_COMPILER $ENV{VULKAN_SDK}/bin/glslangValidator)

    # Check if glslangValidator exists
    if ((WIN32 AND NOT EXISTS "${GLSL_COMPILER}.exe") OR (UNIX AND NOT EXISTS ${GLSL_COMPILER}))
        message(FATAL_ERROR "Failed to find glslangValidator!")
    endif ()

    # Add command to build each file in output list from one source
    foreach(OUT_DESC ${BUILD_KERNELS_OUTPUTS})

        # Make a list from colon-separated string
        string(REPLACE ":" ";" OUT_DESC ${OUT_DESC})

        # Get the last element of the list
        list(LENGTH OUT_DESC LIST_LENGTH)
        math(EXPR LAST_ELEM_INDEX "${LIST_LENGTH} - 1")
        list(GET OUT_DESC ${LAST_ELEM_INDEX} OUT_FILE)
		
        # Remove whitespaces from string
        string(REGEX REPLACE "[ \t]" "" OUT_FILE ${OUT_FILE})

        # Remove whitespaces from string
        string(REGEX REPLACE "[ \t]" "" OUT_FILE ${OUT_FILE})

        # Remove filename from the list
        list(REMOVE_AT OUT_DESC ${LAST_ELEM_INDEX})

        set(FILE_LOCAL_PARAMS ${BUILD_KERNELS_PARAMETERS})

        # Add single output file parameters from
        # residual part of the string (if exist)
        if (NOT OUT_DESC STREQUAL "")
            # Make a list from comma-separated string
            string(REGEX REPLACE "[ \t,]+" ";" OUT_DESC ${OUT_DESC})
            # Append single output file parameters to common parameters
            set(FILE_LOCAL_PARAMS ${FILE_LOCAL_PARAMS} ${OUT_DESC})

        endif()

        # Append output filename to the local list
        list(APPEND KERNELS_OUTPUTS_LOCAL ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE})

        # Construct build command
        set(BUILD_CMD ${GLSL_COMPILER} ${FILE_LOCAL_PARAMS} ${BUILD_KERNELS_SOURCE}
            -o ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE} ${BUILD_KERNELS_LIMITS_FILE})


        # Add build command
        add_custom_command(
            OUTPUT ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE}
            COMMAND ${BUILD_CMD}
            DEPENDS ${BUILD_KERNELS_DEPENDENCY_FILES}
            MAIN_DEPENDENCY ${BUILD_KERNELS_SOURCE}
            COMMENT "Build kernel command: ${BUILD_CMD}"
            VERBATIM
        )

    endforeach()

    # Add local processed outputs to the global list
    set(KERNELS_OUTPUTS ${KERNELS_OUTPUTS} ${KERNELS_OUTPUTS_LOCAL} PARENT_SCOPE)

endfunction(KernelUtils_build_kernels_from_one_source)

# Purpose: build multiple kernels from corresponding multiple sources
# This function will generate .spv files in output directory from listed files in source directory
# Usage:
# KernelUtils_build_kernels(
#               SOURCE_DIRECTORY <path to directory with source files>
#               OUTPUT_DIRECTORY <path to directory with output files>
#               LIMITS_FILE <path to optional limits file>
#               DEPENDENCY_FILES <files on which build commands depend>
#               PARAMETERS <common parameters for all files>
#               SOURCES <list of source files>)
function(KernelUtils_build_kernels)
    set(options)
    set(oneValueArgs SOURCE_DIRECTORY OUTPUT_DIRECTORY LIMITS_FILE)
    set(multiValueArgs DEPENDENCY_FILES PARAMETERS SOURCES)

    cmake_parse_arguments(BUILD_KERNELS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # If source or output directory are relative (or not specified), add path to current CMakeLists.txt first
    if (NOT IS_ABSOLUTE ${BUILD_KERNELS_SOURCE_DIRECTORY})
        set(BUILD_KERNELS_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_KERNELS_SOURCE_DIRECTORY})
    endif ()

    if (NOT IS_ABSOLUTE ${BUILD_KERNELS_OUTPUT_DIRECTORY})
        set(BUILD_KERNELS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${BUILD_KERNELS_OUTPUT_DIRECTORY})
    endif ()

    # Set glsl compiler
    set(GLSL_COMPILER $ENV{VULKAN_SDK}/bin/glslangValidator)

    # Check if glslangValidator exists
    if ((WIN32 AND NOT EXISTS "${GLSL_COMPILER}.exe") OR (UNIX AND NOT EXISTS ${GLSL_COMPILER}))
        message(FATAL_ERROR "Failed to find glslangValidator!")
    endif ()

    # Add command to build each file in the source list
    foreach(IN_FILE ${BUILD_KERNELS_SOURCES})
        get_filename_component(IN_FILE ${IN_FILE} NAME)
        set(OUT_FILE "${IN_FILE}.spv")

        # Append output filename to the local list
        list(APPEND KERNELS_OUTPUTS_LOCAL ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE})

        # Construct build command
        set(BUILD_CMD ${GLSL_COMPILER} ${BUILD_KERNELS_PARAMETERS} ${BUILD_KERNELS_SOURCE_DIRECTORY}/${IN_FILE}
            -o ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE} ${BUILD_KERNELS_LIMITS_FILE})

        # Add build command
        add_custom_command(
            OUTPUT ${BUILD_KERNELS_OUTPUT_DIRECTORY}/${OUT_FILE}
            COMMAND ${BUILD_CMD}
            DEPENDS ${BUILD_KERNELS_DEPENDENCY_FILES}
            MAIN_DEPENDENCY ${BUILD_KERNELS_SOURCE_DIRECTORY}/${IN_FILE}
            COMMENT "Build kernel command: ${BUILD_CMD}"
            VERBATIM
        )

    endforeach()

    # Add local processed outputs to the global list
    set(KERNELS_OUTPUTS ${KERNELS_OUTPUTS} ${KERNELS_OUTPUTS_LOCAL} PARENT_SCOPE)

endfunction(KernelUtils_build_kernels)

# Add build target that depends on list of generated .spv files
# And add dependency of the specified project on this custom target
function(KernelUtils_add_build_kernel_target ROOT_PROJECT_NAME)
    add_custom_target(${ROOT_PROJECT_NAME}BuildKernels DEPENDS ${KERNELS_OUTPUTS})
    add_dependencies(${ROOT_PROJECT_NAME} ${ROOT_PROJECT_NAME}BuildKernels)
endfunction(KernelUtils_add_build_kernel_target)
