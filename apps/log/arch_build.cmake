###########################################################
#
# LOG platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the LOG configuration
set(LOG_PLATFORM_CONFIG_FILE_LIST
  log_internal_cfg_values.h
  log_platform_cfg.h
  log_perfids.h
  log_msgids.h
  log_msgid_values.h
)

generate_configfile_set(${LOG_PLATFORM_CONFIG_FILE_LIST})

