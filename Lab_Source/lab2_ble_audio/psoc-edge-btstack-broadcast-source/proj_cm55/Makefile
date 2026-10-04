################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Makefile for configuring the Cortex-M55 core project
#
################################################################################
# \copyright
# (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG.  SPDX-License-Identifier: Apache-2.0
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
################################################################################

include ../common.mk

################################################################################
# Basic Configuration
################################################################################

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=proj_cm55

# If set to "true" or "1", display full command-lines when building.
VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS+=FREERTOS RTOS_AWARE WICED_BLE

# Add GOOGLE_LC3 component only if LC3 encoding is enabled
ifeq ($(ENABLE_LC3_ENCODING),1)
	COMPONENTS+=GOOGLE_LC3
endif

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

CORE=CM55
CORE_NAME=CM55_0

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES+=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES+=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES+=source source/encoder source/bluetooth/unicast_source

# Add include paths for the bundled LE Audio profiles and GATT interface.
INCLUDES+=source/bluetooth/le_audio_profiles_src/include
INCLUDES+=source/bluetooth/le_audio_profiles_src/internal
INCLUDES+=source/bluetooth/gatt_interface_src/include
INCLUDES+=source/bluetooth/gatt_interface_src


# Add Google liblc3 headers, sources, and compiler flags when encoding is enabled.
ifeq ($(ENABLE_LC3_ENCODING),1)
INCLUDES+=../../mtb_shared/liblc3/v1.0.3/include
SOURCES+=$(wildcard ../../mtb_shared/liblc3/v1.0.3/src/*.c)
CFLAGS+=-ffast-math
else
CY_IGNORE+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/liblc3/v1.0.3
endif

# Add additional defines to the build process (without a leading -D).
DEFINES+=CY_RETARGET_IO_CONVERT_LF_TO_CRLF CY_RTOS_AWARE

# Add LC3 encoding enable flag
ifneq ($(ENABLE_LC3_ENCODING),0)
ifneq ($(ENABLE_LC3_ENCODING),1)
$(error ENABLE_LC3_ENCODING must be 0 or 1)
endif
endif
DEFINES+=ENABLE_LC3_ENCODING=$(ENABLE_LC3_ENCODING)
ifeq ($(AUDIO_CONFIG),MONO)
DEFINES+=AUDIO_CONFIG_MONO
else ifneq ($(AUDIO_CONFIG),STEREO)
$(error AUDIO_CONFIG must be MONO or STEREO)
endif

# Defines for BT Porting layer
DEFINES+=CYBSP_BT_PLATFORM_CFG_BAUD_DOWNLOAD=3000000
DEFINES+=CYBSP_BT_PLATFORM_CFG_BAUD_FEATURE=3000000
DEFINES+=CYBSP_BT_PLATFORM_CFG_MEM_POOL_BYTES=10240

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT+=hardfp

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS+=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS+=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS+=

# Additional / custom linker flags.
LDFLAGS+=

# Additional / custom libraries to link in to the application.
LDLIBS+=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD+=

# Custom post-build commands to run.
POSTBUILD+=

# Exclude host-only liblc3 content not needed for embedded target builds.
# Use getlibs-relative paths so this remains valid after cloning/moving workspaces.
CY_IGNORE+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/liblc3/v1.0.3/test
CY_IGNORE+=$(CY_GETLIBS_SHARED_PATH)/$(CY_GETLIBS_SHARED_NAME)/liblc3/v1.0.3/tools


################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION>
# field begins with $$ASSET_REPO$$, then the repo is deposited in the path
# specified by the CY_GETLIBS_SHARED_PATH variable. The default location is one
# directory level above the current app directory. This is used with
# CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../..

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

include $(CY_TOOLS_DIR)/make/start.mk
