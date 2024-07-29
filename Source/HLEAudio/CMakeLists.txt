add_subdirectory(Platform)

add_library(HLEAudio OBJECT
                    ABI1.cpp
                    ABI2.cpp
                    ABI3mp3.cpp 
                    ABI_ADPCM.cpp 
                    AudioBuffer.cpp
                    ABI3.cpp
                    ABI_Buffers.cpp 
                    ABI_Filters.cpp 
                    ABI_MixerInterleave.cpp 
                    ABI_Resample.cpp
                    ENV_Mixer.cpp 
                    HLEAudioState.cpp
                    HLEMain.cpp 
                    )

list(APPEND daed_libs "HLEAudio")
set(daed_libs ${daed_libs} PARENT_SCOPE)

message( ${daed_libs})