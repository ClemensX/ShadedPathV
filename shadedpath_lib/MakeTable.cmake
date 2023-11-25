# custome build via new executable:
add_executable(MakeTable makeTable.cpp)
target_link_libraries(MakeTable PRIVATE sp_compiler_flags)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Table.h
  COMMAND MakeTable ${CMAKE_CURRENT_BINARY_DIR}/Table.h
  DEPENDS MakeTable
  )
