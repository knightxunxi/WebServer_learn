add_executable(csl_smoke_test smoke_test.cpp)
target_link_libraries(csl_smoke_test PRIVATE csl_project_options)

add_test(NAME csl_smoke_test COMMAND csl_smoke_test)

