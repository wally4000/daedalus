add_library(AudioPlugin OBJECT AudioPluginSDL.cpp)

list(APPEND daed_libs AudioPlugin)
set(daed_libs ${daed_libs} PARENT_SCOPE)

