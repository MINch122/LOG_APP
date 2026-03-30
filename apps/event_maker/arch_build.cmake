###########################################################
#
# EVENT_MAKER platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the EVENT_MAKER configuration
set(EVENT_MAKER_PLATFORM_CONFIG_FILE_LIST
  event_maker_internal_cfg_values.h
  event_maker_platform_cfg.h
  event_maker_perfids.h
  event_maker_msgids.h
  event_maker_msgid_values.h
)

generate_configfile_set(${EVENT_MAKER_PLATFORM_CONFIG_FILE_LIST})

