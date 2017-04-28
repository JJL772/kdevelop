# Find the heaptrack_gui executable
#
# Defines the following variables
#  heaptrack_gui_EXECUTABLE - path of the heaptrack_gui executable

#=============================================================================
# Copyright 2017 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

find_program(heaptrack_gui_EXECUTABLE NAMES heaptrack_gui)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(heaptrack_gui DEFAULT_MSG heaptrack_gui_EXECUTABLE)

mark_as_advanced(heaptrack_gui_EXECUTABLE)
