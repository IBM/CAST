--SWITCH INFO
INSERT INTO csm_switch
(switch_name , discovery_time       , collection_time, comment     , description     , fw_version           , gu_id      , ip       , model  , num_modules, physical_frame_location, physical_u_location, ps_id  , role , server_operation_mode, sm_mode     , state, sw_version, system_guid, system_name, total_alarms, type      , vendor  ) VALUES
('switch_001', '1999-01-27 01:00:00', 'now'          , 'my comment', 'my_description', 'firmware version 00', 'id_abc123', '0.0.0.0', 'model', '0'        , '44,35'                , '22'               , 'ps_id', 'tor', 'mode_01'            , 'sm_mode_01', 'A'  , '13.7'    , '123456'   , 'my_name'  , '0'         , 'director', 'IBM');


