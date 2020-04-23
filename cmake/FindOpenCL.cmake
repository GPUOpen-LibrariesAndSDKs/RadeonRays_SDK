# Copyright (c) 2019 DevSH Graphics Programming Sp. z O.O.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(OpenCL_INCLUDE_DIR "${IRR_ROOT_PATH}/3rdparty")

find_library(OpenCL_LIBRARY
	NAMES OpenCL
	PATHS "${IRR_ROOT_PATH}/3rdparty/CL/lib"
)

set(OpenCL_LIBRARIES ${OpenCL_LIBRARY})
set(OpenCL_INCLUDE_DIRS ${OpenCL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	OpenCL
	FOUND_VAR OpenCL_FOUND
	REQUIRED_VARS OpenCL_LIBRARY OpenCL_INCLUDE_DIR
)

mark_as_advanced(
	OpenCL_INCLUDE_DIR
	OpenCL_LIBRARY
)

if(OpenCL_FOUND AND NOT TARGET OpenCL::OpenCL)
	add_library(OpenCL::OpenCL UNKNOWN IMPORTED)
	set_target_properties(OpenCL::OpenCL PROPERTIES
		IMPORTED_LOCATION "${OpenCL_LIBRARY}")
	set_target_properties(OpenCL::OpenCL PROPERTIES
    		INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIRS}")
endif()