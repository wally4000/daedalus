add_library(AudioPlugin OBJECT 
                            AudioPluginW32.cpp
            )

list(APPEND daed_libs AudioPlugin)
set(daed_libs ${daed_libs} PARENT_SCOPE)

