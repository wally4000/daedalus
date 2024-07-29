add_library(AudioPlugin OBJECT AudioPluginmacOS.cpp)

list(APPEND daed_libs AudioPlugin)
set(daed_libs ${daed_libs} PARENT_SCOPE)

