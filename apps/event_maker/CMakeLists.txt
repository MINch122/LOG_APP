project(CFE_EVENT_MAKER C)

set(APP_SRC_FILES
  fsw/src/event_maker.c
  fsw/src/event_maker_cmds.c
  fsw/src/event_maker_utils.c
  fsw/src/event_maker_simulate.c
)

if (CFE_EDS_ENABLED)
  list(APPEND APP_SRC_FILES
    fsw/src/event_maker_eds_dispatch.c
  )
else()
  list(APPEND APP_SRC_FILES
    fsw/src/event_maker_dispatch.c
  )
endif()

# Create the app module
add_cfe_app(event_maker ${APP_SRC_FILES})

target_include_directories(event_maker PUBLIC fsw/inc)
# to call library-provided functions
# Add table
add_cfe_tables(event_maker fsw/tables/event_maker_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
