parsec_addtest_cmd(${parsec/}apps/haar_tree ${SHM_TEST_CMD_LIST} apps/haar_tree/project -x)
if( MPI_C_FOUND )
  parsec_addtest_cmd(${parsec/}apps/haar_tree:mp ${MPI_TEST_CMD_LIST} 4 apps/haar_tree/project -x)
  if(TEST apps/haar_tree:mp)
    set_tests_properties(${parsec/}apps/haar_tree:mp PROPERTIES DEPENDS launch:mp)
  endif()
endif( MPI_C_FOUND )
