###########################################################
#
# LOG mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the LOG configuration
set(LOG_MISSION_CONFIG_FILE_LIST
  log_fcncode_values.h
  log_interface_cfg_values.h
  log_mission_cfg.h
  log_perfids.h
  log_msg.h
  log_msgdefs.h
  log_msgstruct.h
  log_tbl.h
  log_tbldefs.h
  log_tblstruct.h
  log_topicid_values.h
)

generate_configfile_set(${LOG_MISSION_CONFIG_FILE_LIST})

