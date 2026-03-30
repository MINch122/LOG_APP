project(CFE_LOG C)

set(APP_SRC_FILES
  fsw/src/log.c
  fsw/src/log_cmds.c
  fsw/src/log_utils.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/log_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/log_dispatch.c
  )
endif()

# Create the app module
add_cfe_app(log ${APP_SRC_FILES})

target_include_directories(log PUBLIC fsw/inc)
# to call library-provided functions
# Add table
add_cfe_tables(log fsw/tables/log_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
