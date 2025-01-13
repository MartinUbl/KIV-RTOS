CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

# pridame standardni knihovnu (nas minimalni stub knihovny)
FILE(GLOB_RECURSE stdlib_src "*.cpp" "*.c" "*.h" "*.hpp" "*.s")

# pridame cil - sestavujeme standardni knihovnu (minimalni)
ADD_LIBRARY(kivrtos_stdlib STATIC ${stdlib_src} ${stdutils_src})
