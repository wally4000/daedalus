

add_library(RomFile OBJECT
                    RomFile.cpp 
                    RomFileBuffer.cpp
                    RomFileCache.cpp 
                    RomFileCompressed.cpp 
                    RomFileMemory.cpp 
                    RomFileSettings.cpp
                    RomFileUncompressed.cpp
            )

            
list(APPEND daed_libs "RomFile")
set(daed_libs ${daed_libs} PARENT_SCOPE)
