###########################################################
#
# EVENT_MAKER mission build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the EVENT_MAKER configuration
set(EVENT_MAKER_MISSION_CONFIG_FILE_LIST
  event_maker_fcncode_values.h
  event_maker_interface_cfg_values.h
  event_maker_mission_cfg.h
  event_maker_perfids.h
  event_maker_msg.h
  event_maker_msgdefs.h
  event_maker_msgstruct.h
  event_maker_tbl.h
  event_maker_tbldefs.h
  event_maker_tblstruct.h
  event_maker_topicid_values.h
)

generate_configfile_set(${EVENT_MAKER_MISSION_CONFIG_FILE_LIST})

