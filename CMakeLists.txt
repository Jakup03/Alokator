#
# DANTE :: System Automatycznych Testów Jednostkowych oraz Akceptacyjnych
# Tomasz Jaworski, 2018-2020
#
# Plik wygenerowany automatycznie
# Znacznik czasowy: 2022-12-20 11:24:54.552884
#

cmake_minimum_required(VERSION 3.17)
project(project1 C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)

# Ustaw opcje kompilatora (z raportu Dante)
add_compile_options(
      "-Werror=vla"
      "-D_ANSI_OUTPUT"
      "-fdiagnostics-color"
      "-Wno-error=unused-parameter"
      "-Wextra"
      "-Wall"
      "-DINSIDE_DANTE"
      "-D_GNU_SOURCE"
      "-Wno-parentheses"
      "-fmax-errors=5"
      "-pedantic"
      "-Wno-error=implicit-fallthrough"
      "-std=c11"
      "-ggdb3"
      "-xc"
      "-Wno-error=parentheses"
      "-Werror"

)

# Ustaw opcje konsolidatora (z raportu Dante)
add_link_options(
        "-Wl,-Map=main.map"
        "-ggdb3"
        "-Wl,-wrap,main"
        "-Wl,-cref"
)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(project1
        "heap.c"
        "main.c"
        "unit_helper_v2.c"
        "unit_test_v2.c"
        "rdebug.c"
        "memmanager.c"
)

# Dołącz niezbędne biblioteki
target_link_libraries(project1
        "pthread"
        "m"
)