#define _CRT_SECURE_NO_WARNINGS

#include "fit_import.h"

DLLEXPORT mint WolframLibrary_getVersion() {
    return WolframLibraryVersion;
}

DLLEXPORT int WolframLibrary_initialize(WolframLibraryData libData) {
    return 0;
}

DLLEXPORT void WolframLibrary_uninitialize(WolframLibraryData libData) {
    return;
}

DLLEXPORT int constantzero(
    WolframLibraryData libData,
    mint Argc, 
    MArgument *Args, 
    MArgument Res
) {
    MArgument_setInteger(Res, 0);
    return LIBRARY_NO_ERROR;
}


DLLEXPORT int FITFileType (
    WolframLibraryData libData,
    mint Argc, 
    MArgument *Args, 
    MArgument Res
) {
    char *path;
    FILE *file;
    mint res;
    FIT_UINT8 buf[8];
    FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
    FIT_UINT32 buf_size;
    FIT_UINT32 mesg_index = 0;

    path = MArgument_getUTF8String(Args[0]);

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FIT_CONVERT_STATE state;
    #endif

    // printf("Testing file conversion using %s file...\n", path);

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FitConvert_Init(&state, FIT_TRUE);
    #else
        FitConvert_Init(FIT_TRUE);
    #endif

    if((file = fopen(path, "rb")) == NULL)
    {
        // printf("Error opening file %s.\n", path);
        return FIT_IMPORT_ERROR_OPEN_FILE;
    }

    while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
    {
        for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
        {
            buf[buf_size] = (FIT_UINT8)getc(file);
        }

        do
        {
            #if defined(FIT_CONVERT_MULTI_THREAD)
                convert_return = FitConvert_Read(&state, buf, buf_size);
            #else
                convert_return = FitConvert_Read(buf, buf_size);
            #endif

            switch (convert_return)
            {
                case FIT_CONVERT_MESSAGE_AVAILABLE:
                {
                    #if defined(FIT_CONVERT_MULTI_THREAD)
                        const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                        FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
                    #else
                        const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                        FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
                    #endif

                    switch(mesg_num)
                    {
                        case FIT_MESG_NUM_FILE_ID:
                        {
                            const FIT_FILE_ID_MESG *id = (FIT_FILE_ID_MESG *) mesg;
                            res = id->type;
                            convert_return = FIT_CONVERT_END_OF_FILE;
                            break;
                        }
                        default:
                        {    
                            break;
                        }
                    }

                    break;
                }

                default:
                    break;
            }
        } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
    }

    #if defined(RETURN_PARTIAL_DATA)
    fclose(file);
    // FIXME: this can be undefined:
    MArgument_setInteger(Res, res);
    return LIBRARY_NO_ERROR;
    #endif

    if (convert_return == FIT_CONVERT_ERROR)
    {
        // Error decoding file
        fclose(file);
        return FIT_IMPORT_ERROR_CONVERSION;
    }

    if (convert_return == FIT_CONVERT_CONTINUE)
    {
        // Unexpected end of file
        fclose(file);
        return FIT_IMPORT_ERROR_UNEXPECTED_EOF;
    }

    if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
    {
        // File is not FIT
        fclose(file);
        return FIT_IMPORT_ERROR_NOT_FIT_FILE;
    }

    if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
    {
        // Protocol version not supported
        fclose(file);
        return FIT_IMPORT_ERROR_UNSUPPORTED_PROTOCOL;
    }

    if (convert_return == FIT_CONVERT_END_OF_FILE)
        // printf("File converted successfully.\n");

    fclose(file);

    MArgument_setInteger(Res, res);

    return LIBRARY_NO_ERROR;
}


DLLEXPORT int FITImport(
    WolframLibraryData libData,
    mint Argc, 
    MArgument *Args, 
    MArgument Res
) {
    mint err = 0;
    char *path;
    int length;
    MTensor data;
    mint dims[2];
    FILE *file;
    FIT_UINT8 buf[8];
    FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
    FIT_UINT32 buf_size;
    FIT_UINT32 mesg_index = 0;

    path = MArgument_getUTF8String(Args[0]);
    length = count_usable_fit_messages(path, &err);
    if (err) {
        return err;
    }

    dims[0] = length;
    dims[1] = MESSAGE_TENSOR_ROW_WIDTH;
    err = libData->MTensor_new(MType_Integer, 2, dims, &data);
    if (err) {
        return FIT_IMPORT_ERROR_INTERNAL;
    }

    mint pos[2];
    int idx = 0;
    int col = 0;

    FIT_UINT16 last_hrv = FIT_UINT16_INVALID;

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FIT_CONVERT_STATE state;
    #endif

    // printf("Testing file conversion using %s file...\n", path);

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FitConvert_Init(&state, FIT_TRUE);
    #else
        FitConvert_Init(FIT_TRUE);
    #endif

    if((file = fopen(path, "rb")) == NULL)
    {
        // printf("Error opening file %s.\n", path);
        return FIT_IMPORT_ERROR_OPEN_FILE;
    }

    while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
    {
        for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
        {
            buf[buf_size] = (FIT_UINT8)getc(file);
        }

        do
        {
            #if defined(FIT_CONVERT_MULTI_THREAD)
                convert_return = FitConvert_Read(&state, buf, buf_size);
            #else
                convert_return = FitConvert_Read(buf, buf_size);
            #endif

            switch (convert_return)
            {
                case FIT_CONVERT_MESSAGE_AVAILABLE:
                {
                #if defined(FIT_CONVERT_MULTI_THREAD)
                    const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                    FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
                #else
                    const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                    FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
                #endif

                mesg_index++;
                // printf("Mesg %d (%d) - ", mesg_index++, mesg_num);

                switch(mesg_num)
                {   
                    case FIT_MESG_NUM_ACTIVITY:
                    {
                        const FIT_ACTIVITY_MESG *activity = (FIT_ACTIVITY_MESG *) mesg;
                        idx++;
                        import_activity(libData, data, idx, activity);
                        {
                            FIT_ACTIVITY_MESG old_mesg;
                            old_mesg.num_sessions = 1;
                            #if defined(FIT_CONVERT_MULTI_THREAD)
                            FitConvert_RestoreFields(&state, &old_mesg);
                            #else
                            FitConvert_RestoreFields(&old_mesg);
                            #endif
                            // printf("Restored num_sessions=1 - Activity: timestamp=%u, type=%u, event=%u, event_type=%u, num_sessions=%u\n", activity->timestamp, activity->type, activity->event, activity->event_type, activity->num_sessions);
                        }
                        break;
                    }

// --- START MESSAGE IMPORT CASES ---
// This section is auto-generated. Do not edit manually.

                    case FIT_MESG_NUM_FILE_ID:
                    {
                        const FIT_FILE_ID_MESG *new = (FIT_FILE_ID_MESG *) mesg;
                        idx++;
                        import_file_id(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_FILE_CREATOR:
                    {
                        const FIT_FILE_CREATOR_MESG *new = (FIT_FILE_CREATOR_MESG *) mesg;
                        idx++;
                        import_file_creator(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SOFTWARE:
                    {
                        const FIT_SOFTWARE_MESG *new = (FIT_SOFTWARE_MESG *) mesg;
                        idx++;
                        import_software(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SLAVE_DEVICE:
                    {
                        const FIT_SLAVE_DEVICE_MESG *new = (FIT_SLAVE_DEVICE_MESG *) mesg;
                        idx++;
                        import_slave_device(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_CAPABILITIES:
                    {
                        const FIT_CAPABILITIES_MESG *new = (FIT_CAPABILITIES_MESG *) mesg;
                        idx++;
                        import_capabilities(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_FILE_CAPABILITIES:
                    {
                        const FIT_FILE_CAPABILITIES_MESG *new = (FIT_FILE_CAPABILITIES_MESG *) mesg;
                        idx++;
                        import_file_capabilities(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_MESG_CAPABILITIES:
                    {
                        const FIT_MESG_CAPABILITIES_MESG *new = (FIT_MESG_CAPABILITIES_MESG *) mesg;
                        idx++;
                        import_mesg_capabilities(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_FIELD_CAPABILITIES:
                    {
                        const FIT_FIELD_CAPABILITIES_MESG *new = (FIT_FIELD_CAPABILITIES_MESG *) mesg;
                        idx++;
                        import_field_capabilities(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_SETTINGS:
                    {
                        const FIT_DEVICE_SETTINGS_MESG *new = (FIT_DEVICE_SETTINGS_MESG *) mesg;
                        idx++;
                        import_device_settings(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_USER_PROFILE:
                    {
                        const FIT_USER_PROFILE_MESG *new = (FIT_USER_PROFILE_MESG *) mesg;
                        idx++;
                        import_user_profile(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_HRM_PROFILE:
                    {
                        const FIT_HRM_PROFILE_MESG *new = (FIT_HRM_PROFILE_MESG *) mesg;
                        idx++;
                        import_hrm_profile(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SDM_PROFILE:
                    {
                        const FIT_SDM_PROFILE_MESG *new = (FIT_SDM_PROFILE_MESG *) mesg;
                        idx++;
                        import_sdm_profile(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_BIKE_PROFILE:
                    {
                        const FIT_BIKE_PROFILE_MESG *new = (FIT_BIKE_PROFILE_MESG *) mesg;
                        idx++;
                        import_bike_profile(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_CONNECTIVITY:
                    {
                        const FIT_CONNECTIVITY_MESG *new = (FIT_CONNECTIVITY_MESG *) mesg;
                        idx++;
                        import_connectivity(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_ZONES_TARGET:
                    {
                        const FIT_ZONES_TARGET_MESG *new = (FIT_ZONES_TARGET_MESG *) mesg;
                        idx++;
                        import_zones_target(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SPORT:
                    {
                        const FIT_SPORT_MESG *new = (FIT_SPORT_MESG *) mesg;
                        idx++;
                        import_sport(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_HR_ZONE:
                    {
                        const FIT_HR_ZONE_MESG *new = (FIT_HR_ZONE_MESG *) mesg;
                        idx++;
                        import_hr_zone(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SPEED_ZONE:
                    {
                        const FIT_SPEED_ZONE_MESG *new = (FIT_SPEED_ZONE_MESG *) mesg;
                        idx++;
                        import_speed_zone(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_CADENCE_ZONE:
                    {
                        const FIT_CADENCE_ZONE_MESG *new = (FIT_CADENCE_ZONE_MESG *) mesg;
                        idx++;
                        import_cadence_zone(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_POWER_ZONE:
                    {
                        const FIT_POWER_ZONE_MESG *new = (FIT_POWER_ZONE_MESG *) mesg;
                        idx++;
                        import_power_zone(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_MET_ZONE:
                    {
                        const FIT_MET_ZONE_MESG *new = (FIT_MET_ZONE_MESG *) mesg;
                        idx++;
                        import_met_zone(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_DIVE_SETTINGS:
                    {
                        const FIT_DIVE_SETTINGS_MESG *new = (FIT_DIVE_SETTINGS_MESG *) mesg;
                        idx++;
                        import_dive_settings(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_GOAL:
                    {
                        const FIT_GOAL_MESG *new = (FIT_GOAL_MESG *) mesg;
                        idx++;
                        import_goal(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SESSION:
                    {
                        const FIT_SESSION_MESG *new = (FIT_SESSION_MESG *) mesg;
                        idx++;
                        import_session(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_LAP:
                    {
                        const FIT_LAP_MESG *new = (FIT_LAP_MESG *) mesg;
                        idx++;
                        import_lap(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_LENGTH:
                    {
                        const FIT_LENGTH_MESG *new = (FIT_LENGTH_MESG *) mesg;
                        idx++;
                        import_length(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_RECORD:
                    {
                        const FIT_RECORD_MESG *new = (FIT_RECORD_MESG *) mesg;
                        idx++;
                        import_record(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_EVENT:
                    {
                        const FIT_EVENT_MESG *new = (FIT_EVENT_MESG *) mesg;
                        idx++;
                        import_event(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_INFO:
                    {
                        const FIT_DEVICE_INFO_MESG *new = (FIT_DEVICE_INFO_MESG *) mesg;
                        idx++;
                        import_device_info(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_AUX_BATTERY_INFO:
                    {
                        const FIT_DEVICE_AUX_BATTERY_INFO_MESG *new = (FIT_DEVICE_AUX_BATTERY_INFO_MESG *) mesg;
                        idx++;
                        import_device_aux_battery_info(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_TRAINING_FILE:
                    {
                        const FIT_TRAINING_FILE_MESG *new = (FIT_TRAINING_FILE_MESG *) mesg;
                        idx++;
                        import_training_file(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WEATHER_CONDITIONS:
                    {
                        const FIT_WEATHER_CONDITIONS_MESG *new = (FIT_WEATHER_CONDITIONS_MESG *) mesg;
                        idx++;
                        import_weather_conditions(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WEATHER_ALERT:
                    {
                        const FIT_WEATHER_ALERT_MESG *new = (FIT_WEATHER_ALERT_MESG *) mesg;
                        idx++;
                        import_weather_alert(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_NMEA_SENTENCE:
                    {
                        const FIT_NMEA_SENTENCE_MESG *new = (FIT_NMEA_SENTENCE_MESG *) mesg;
                        idx++;
                        import_nmea_sentence(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_AVIATION_ATTITUDE:
                    {
                        const FIT_AVIATION_ATTITUDE_MESG *new = (FIT_AVIATION_ATTITUDE_MESG *) mesg;
                        idx++;
                        import_aviation_attitude(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_VIDEO_TITLE:
                    {
                        const FIT_VIDEO_TITLE_MESG *new = (FIT_VIDEO_TITLE_MESG *) mesg;
                        idx++;
                        import_video_title(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_VIDEO_DESCRIPTION:
                    {
                        const FIT_VIDEO_DESCRIPTION_MESG *new = (FIT_VIDEO_DESCRIPTION_MESG *) mesg;
                        idx++;
                        import_video_description(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SET:
                    {
                        const FIT_SET_MESG *new = (FIT_SET_MESG *) mesg;
                        idx++;
                        import_set(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_FIELD_DESCRIPTION:
                    {
                        const FIT_FIELD_DESCRIPTION_MESG *new = (FIT_FIELD_DESCRIPTION_MESG *) mesg;
                        idx++;
                        import_field_description(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_DEVELOPER_DATA_ID:
                    {
                        const FIT_DEVELOPER_DATA_ID_MESG *new = (FIT_DEVELOPER_DATA_ID_MESG *) mesg;
                        idx++;
                        import_developer_data_id(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_COURSE:
                    {
                        const FIT_COURSE_MESG *new = (FIT_COURSE_MESG *) mesg;
                        idx++;
                        import_course(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_COURSE_POINT:
                    {
                        const FIT_COURSE_POINT_MESG *new = (FIT_COURSE_POINT_MESG *) mesg;
                        idx++;
                        import_course_point(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_ID:
                    {
                        const FIT_SEGMENT_ID_MESG *new = (FIT_SEGMENT_ID_MESG *) mesg;
                        idx++;
                        import_segment_id(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_LEADERBOARD_ENTRY:
                    {
                        const FIT_SEGMENT_LEADERBOARD_ENTRY_MESG *new = (FIT_SEGMENT_LEADERBOARD_ENTRY_MESG *) mesg;
                        idx++;
                        import_segment_leaderboard_entry(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_POINT:
                    {
                        const FIT_SEGMENT_POINT_MESG *new = (FIT_SEGMENT_POINT_MESG *) mesg;
                        idx++;
                        import_segment_point(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_LAP:
                    {
                        const FIT_SEGMENT_LAP_MESG *new = (FIT_SEGMENT_LAP_MESG *) mesg;
                        idx++;
                        import_segment_lap(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_FILE:
                    {
                        const FIT_SEGMENT_FILE_MESG *new = (FIT_SEGMENT_FILE_MESG *) mesg;
                        idx++;
                        import_segment_file(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT:
                    {
                        const FIT_WORKOUT_MESG *new = (FIT_WORKOUT_MESG *) mesg;
                        idx++;
                        import_workout(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT_SESSION:
                    {
                        const FIT_WORKOUT_SESSION_MESG *new = (FIT_WORKOUT_SESSION_MESG *) mesg;
                        idx++;
                        import_workout_session(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT_STEP:
                    {
                        const FIT_WORKOUT_STEP_MESG *new = (FIT_WORKOUT_STEP_MESG *) mesg;
                        idx++;
                        import_workout_step(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_EXERCISE_TITLE:
                    {
                        const FIT_EXERCISE_TITLE_MESG *new = (FIT_EXERCISE_TITLE_MESG *) mesg;
                        idx++;
                        import_exercise_title(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_SCHEDULE:
                    {
                        const FIT_SCHEDULE_MESG *new = (FIT_SCHEDULE_MESG *) mesg;
                        idx++;
                        import_schedule(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_TOTALS:
                    {
                        const FIT_TOTALS_MESG *new = (FIT_TOTALS_MESG *) mesg;
                        idx++;
                        import_totals(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_WEIGHT_SCALE:
                    {
                        const FIT_WEIGHT_SCALE_MESG *new = (FIT_WEIGHT_SCALE_MESG *) mesg;
                        idx++;
                        import_weight_scale(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_BLOOD_PRESSURE:
                    {
                        const FIT_BLOOD_PRESSURE_MESG *new = (FIT_BLOOD_PRESSURE_MESG *) mesg;
                        idx++;
                        import_blood_pressure(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_MONITORING_INFO:
                    {
                        const FIT_MONITORING_INFO_MESG *new = (FIT_MONITORING_INFO_MESG *) mesg;
                        idx++;
                        import_monitoring_info(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_MONITORING:
                    {
                        const FIT_MONITORING_MESG *new = (FIT_MONITORING_MESG *) mesg;
                        idx++;
                        import_monitoring(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_HR:
                    {
                        const FIT_HR_MESG *new = (FIT_HR_MESG *) mesg;
                        idx++;
                        import_hr(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_ANT_RX:
                    {
                        const FIT_ANT_RX_MESG *new = (FIT_ANT_RX_MESG *) mesg;
                        idx++;
                        import_ant_rx(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_ANT_TX:
                    {
                        const FIT_ANT_TX_MESG *new = (FIT_ANT_TX_MESG *) mesg;
                        idx++;
                        import_ant_tx(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_EXD_SCREEN_CONFIGURATION:
                    {
                        const FIT_EXD_SCREEN_CONFIGURATION_MESG *new = (FIT_EXD_SCREEN_CONFIGURATION_MESG *) mesg;
                        idx++;
                        import_exd_screen_configuration(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_EXD_DATA_FIELD_CONFIGURATION:
                    {
                        const FIT_EXD_DATA_FIELD_CONFIGURATION_MESG *new = (FIT_EXD_DATA_FIELD_CONFIGURATION_MESG *) mesg;
                        idx++;
                        import_exd_data_field_configuration(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_EXD_DATA_CONCEPT_CONFIGURATION:
                    {
                        const FIT_EXD_DATA_CONCEPT_CONFIGURATION_MESG *new = (FIT_EXD_DATA_CONCEPT_CONFIGURATION_MESG *) mesg;
                        idx++;
                        import_exd_data_concept_configuration(libData, data, idx, new);
                        break;
                    }

                    case FIT_MESG_NUM_HRV:
                    {
                        const FIT_HRV_MESG *new = (FIT_HRV_MESG *) mesg;
                        idx++;
                        import_hrv(libData, data, idx, new);
                        break;
                    }
// --- END MESSAGE IMPORT CASES ---
                    default:
                    {
                        break;
                    }
                }
                break;
                }

                default:
                break;
            }
        } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
    }

    #if defined(RETURN_PARTIAL_DATA)
    // TODO: append a custom message that indicates the error type
    fclose(file);
    MArgument_setMTensor(Res, data);
    return LIBRARY_NO_ERROR;
    #endif

    if (convert_return == FIT_CONVERT_ERROR)
    {
        // Error decoding file
        fclose(file);
        return FIT_IMPORT_ERROR_CONVERSION;
    }

    if (convert_return == FIT_CONVERT_CONTINUE)
    {
        // Unexpected end of file
        fclose(file);
        return FIT_IMPORT_ERROR_UNEXPECTED_EOF;
    }

    if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
    {
        // File is not FIT
        fclose(file);
        return FIT_IMPORT_ERROR_NOT_FIT_FILE;
    }

    if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
    {
        // Protocol version not supported
        fclose(file);
        return FIT_IMPORT_ERROR_UNSUPPORTED_PROTOCOL;
    }

    if (convert_return == FIT_CONVERT_END_OF_FILE)
        // File converted successfully

    fclose(file);

    MArgument_setMTensor(Res, data);
    return LIBRARY_NO_ERROR;
}

DLLEXPORT int FITMessageTypes (
    WolframLibraryData libData,
    mint Argc, 
    MArgument *Args, 
    MArgument Res
) {
    mint err = 0;
    char *path;
    int length;
    MTensor data;
    mint dims[2];
    FILE *file;
    FIT_UINT8 buf[8];
    FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
    FIT_UINT32 buf_size;
    FIT_UINT32 mesg_index = 0;

    path = MArgument_getUTF8String(Args[0]);
    length = count_fit_messages(path, &err);
    if (err) {
        return err;
    }

    dims[0] = length;
    dims[1] = 8;
    err = libData->MTensor_new(MType_Integer, 2, dims, &data);
    if (err) {
        return FIT_IMPORT_ERROR_INTERNAL;
    }

    mint pos[2];
    int idx = 0;
    int col = 0;

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FIT_CONVERT_STATE state;
    #endif

    // printf("Testing file conversion using %s file...\n", path);

    #if defined(FIT_CONVERT_MULTI_THREAD)
        FitConvert_Init(&state, FIT_TRUE);
    #else
        FitConvert_Init(FIT_TRUE);
    #endif

    if((file = fopen(path, "rb")) == NULL)
    {
        // printf("Error opening file %s.\n", path);
        return FIT_IMPORT_ERROR_OPEN_FILE;
    }

    while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
    {
        for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
        {
            buf[buf_size] = (FIT_UINT8)getc(file);
        }

        do
        {
            #if defined(FIT_CONVERT_MULTI_THREAD)
                convert_return = FitConvert_Read(&state, buf, buf_size);
            #else
                convert_return = FitConvert_Read(buf, buf_size);
            #endif

            switch (convert_return)
            {
                case FIT_CONVERT_MESSAGE_AVAILABLE:
                {
                    #if defined(FIT_CONVERT_MULTI_THREAD)
                        const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                        FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
                    #else
                        const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                        FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
                    #endif

                    mesg_index++;
                    // printf("Mesg %d (%d) - ", mesg_index++, mesg_num);
                    FIT_UINT16 size = Fit_GetMesgSize(mesg_num);
                    const FIT_MESG_DEF mesg_def = * Fit_GetMesgDef(mesg_num);
                    FIT_UINT8 arch = mesg_def.arch;
                    FIT_UINT8 fields = mesg_def.num_fields;
                    idx++;
                    pos[0] = idx;
                    pos[1] = 0;
                    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_MESSAGE_INFORMATION);
                    ImportInteger(2, libData, data, pos, mesg_num);
                    ImportInteger(3, libData, data, pos, size);
                    ImportInteger(4, libData, data, pos, arch);
                    ImportInteger(5, libData, data, pos, fields);
                    ImportInteger(6, libData, data, pos, mesg_index);
                    ImportInteger(7, libData, data, pos, DONE);

                    break;
                }

                default:
                    break;
            }
        } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
    }

    #if defined(RETURN_PARTIAL_DATA)
    fclose(file);
    MArgument_setMTensor(Res, data);
    return LIBRARY_NO_ERROR;
    #endif

    if (convert_return == FIT_CONVERT_ERROR)
    {
        // Error decoding file
        fclose(file);
        return FIT_IMPORT_ERROR_CONVERSION;
    }

    if (convert_return == FIT_CONVERT_CONTINUE)
    {
        // Unexpected end of file
        fclose(file);
        return FIT_IMPORT_ERROR_UNEXPECTED_EOF;
    }

    if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
    {
        // File is not FIT
        fclose(file);
        return FIT_IMPORT_ERROR_NOT_FIT_FILE;
    }

    if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
    {
        // Protocol version not supported
        fclose(file);
        return FIT_IMPORT_ERROR_UNSUPPORTED_PROTOCOL;
    }

    if (convert_return == FIT_CONVERT_END_OF_FILE)
        // printf("File converted successfully.\n");

    fclose(file);

    MArgument_setMTensor(Res, data);
    return LIBRARY_NO_ERROR;
}


static void import_unknown(WolframLibraryData libData, MTensor data, int idx, int mesgNum, const FIT_UINT8 *mesg)
{
    mint pos[2];
    pos[0] = idx;
    pos[1] = 0;
    pos[1]++; libData->MTensor_setInteger(data, pos, mesgNum);
    for (int i = 0; i < MESSAGE_TENSOR_ROW_WIDTH - 1; i++)
    {
        pos[1]++; 
        libData->MTensor_setInteger(data, pos, mesg[i]);
    }
    SetInteger(libData, data, pos, DONE);
}


static int count_usable_fit_messages(char* input, mint *err)
{
   FILE *file;
   FIT_UINT8 buf[8];
   FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
   FIT_UINT32 buf_size;
   int mesg_count = 0;
   #if defined(FIT_CONVERT_MULTI_THREAD)
      FIT_CONVERT_STATE state;
   #endif

   #if defined(FIT_CONVERT_MULTI_THREAD)
      FitConvert_Init(&state, FIT_TRUE);
   #else
      FitConvert_Init(FIT_TRUE);
   #endif

   if((file = fopen(input, "rb")) == NULL)
   {
      *err = FIT_IMPORT_ERROR_OPEN_FILE;
      return 0;
   }

   while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
   {
      for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
      {
         buf[buf_size] = (FIT_UINT8)getc(file);
      }

      do
      {
         #if defined(FIT_CONVERT_MULTI_THREAD)
            convert_return = FitConvert_Read(&state, buf, buf_size);
         #else
            convert_return = FitConvert_Read(buf, buf_size);
         #endif

         switch (convert_return)
         {
            case FIT_CONVERT_MESSAGE_AVAILABLE:
            {
               #if defined(FIT_CONVERT_MULTI_THREAD)
                  const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                  FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
               #else
                  const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                  FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
               #endif

               switch(mesg_num)
               {
// --- START MESSAGE COUNT CASES ---
// This section is auto-generated. Do not edit manually.

                    case FIT_MESG_NUM_FILE_ID:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_FILE_CREATOR:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SOFTWARE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SLAVE_DEVICE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_CAPABILITIES:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_FILE_CAPABILITIES:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_MESG_CAPABILITIES:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_FIELD_CAPABILITIES:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_SETTINGS:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_USER_PROFILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_HRM_PROFILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SDM_PROFILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_BIKE_PROFILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_CONNECTIVITY:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_ZONES_TARGET:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SPORT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_HR_ZONE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SPEED_ZONE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_CADENCE_ZONE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_POWER_ZONE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_MET_ZONE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_DIVE_SETTINGS:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_GOAL:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_ACTIVITY:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SESSION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_LAP:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_LENGTH:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_RECORD:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_EVENT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_INFO:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_DEVICE_AUX_BATTERY_INFO:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_TRAINING_FILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WEATHER_CONDITIONS:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WEATHER_ALERT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_NMEA_SENTENCE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_AVIATION_ATTITUDE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_VIDEO_TITLE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_VIDEO_DESCRIPTION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SET:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_FIELD_DESCRIPTION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_DEVELOPER_DATA_ID:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_COURSE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_COURSE_POINT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_ID:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_LEADERBOARD_ENTRY:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_POINT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_LAP:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SEGMENT_FILE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT_SESSION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WORKOUT_STEP:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_EXERCISE_TITLE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_SCHEDULE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_TOTALS:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_WEIGHT_SCALE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_BLOOD_PRESSURE:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_MONITORING_INFO:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_MONITORING:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_HR:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_ANT_RX:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_ANT_TX:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_EXD_SCREEN_CONFIGURATION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_EXD_DATA_FIELD_CONFIGURATION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_EXD_DATA_CONCEPT_CONFIGURATION:
                    {
                        mesg_count++;
                        break;
                    }

                    case FIT_MESG_NUM_HRV:
                    {
                        mesg_count++;
                        break;
                    }
// --- END MESSAGE COUNT CASES ---
                  default:
                    break;
               }
               break;
            }

            default:
               break;
         }
      } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
   }

   #if defined(RETURN_PARTIAL_DATA)
   fclose(file);
   return mesg_count;
   #endif

   if (convert_return == FIT_CONVERT_ERROR)
    {
        // Error decoding file
        fclose(file);
        *err = FIT_IMPORT_ERROR_CONVERSION;
        return 0;
    }

    if (convert_return == FIT_CONVERT_CONTINUE)
    {
        // Unexpected end of file
        fclose(file);
        *err = FIT_IMPORT_ERROR_UNEXPECTED_EOF;
        return 0;
    }

    if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
    {
        // File is not FIT
        fclose(file);
        *err = FIT_IMPORT_ERROR_NOT_FIT_FILE;
        return 0;
    }

    if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
    {
        // Protocol version not supported
        fclose(file);
        *err = FIT_IMPORT_ERROR_UNSUPPORTED_PROTOCOL;
        return 0;
    }

   fclose(file);

   return mesg_count;
}


static int count_fit_messages(char* input, mint* err)
{
   FILE *file;
   FIT_UINT8 buf[8];
   FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
   FIT_UINT32 buf_size;
   int mesg_count = 0;
   #if defined(FIT_CONVERT_MULTI_THREAD)
      FIT_CONVERT_STATE state;
   #endif

   #if defined(FIT_CONVERT_MULTI_THREAD)
      FitConvert_Init(&state, FIT_TRUE);
   #else
      FitConvert_Init(FIT_TRUE);
   #endif

   if((file = fopen(input, "rb")) == NULL)
   {
      *err = FIT_IMPORT_ERROR_OPEN_FILE;
      return 0;
   }

   while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
   {
      for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
      {
         buf[buf_size] = (FIT_UINT8)getc(file);
      }

      do
      {
         #if defined(FIT_CONVERT_MULTI_THREAD)
            convert_return = FitConvert_Read(&state, buf, buf_size);
         #else
            convert_return = FitConvert_Read(buf, buf_size);
         #endif

         switch (convert_return)
         {
            case FIT_CONVERT_MESSAGE_AVAILABLE:
            {
               #if defined(FIT_CONVERT_MULTI_THREAD)
                  const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                  FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
               #else
                  const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                  FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
               #endif

               mesg_count++;
               break;
            }

            default:
               break;
         }
      } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
   }

   #if defined(RETURN_PARTIAL_DATA)
   fclose(file);
   return mesg_count;
   #endif

   if (convert_return == FIT_CONVERT_ERROR)
    {
        // Error decoding file
        fclose(file);
        *err = FIT_IMPORT_ERROR_CONVERSION;
        return 0;
    }

    if (convert_return == FIT_CONVERT_CONTINUE)
    {
        // Unexpected end of file
        fclose(file);
        *err = FIT_IMPORT_ERROR_UNEXPECTED_EOF;
        return 0;
    }

    if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
    {
        // File is not FIT
        fclose(file);
        *err = FIT_IMPORT_ERROR_NOT_FIT_FILE;
        return 0;
    }

    if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
    {
        // Protocol version not supported
        fclose(file);
        *err = FIT_IMPORT_ERROR_UNSUPPORTED_PROTOCOL;
        return 0;
    }

   fclose(file);

   return mesg_count;
}



// --- START MESSAGE IMPORT FUNCTIONS ---
// This section is auto-generated. Do not edit manually.
static void import_file_id(WolframLibraryData libData, MTensor data, int idx, const FIT_FILE_ID_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_FILE_ID);
    ImportInteger(2, libData, data, pos, mesg->serial_number);
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->time_created));
    ImportString(4, libData, data, pos, mesg->product_name, FIT_FILE_ID_MESG_PRODUCT_NAME_COUNT);
    ImportInteger(24, libData, data, pos, mesg->manufacturer);
    ImportInteger(25, libData, data, pos, mesg->product);
    ImportInteger(26, libData, data, pos, mesg->number);
    ImportInteger(27, libData, data, pos, mesg->type);
    ImportFinish(28, libData, data, pos);
}

static void import_file_creator(WolframLibraryData libData, MTensor data, int idx, const FIT_FILE_CREATOR_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_FILE_CREATOR);
    ImportInteger(2, libData, data, pos, mesg->software_version);
    ImportInteger(3, libData, data, pos, mesg->hardware_version);
    ImportFinish(4, libData, data, pos);
}

static void import_software(WolframLibraryData libData, MTensor data, int idx, const FIT_SOFTWARE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SOFTWARE);
    ImportString(2, libData, data, pos, mesg->part_number, FIT_SOFTWARE_MESG_PART_NUMBER_COUNT);
    ImportInteger(18, libData, data, pos, mesg->message_index);
    ImportInteger(19, libData, data, pos, mesg->version);
    ImportFinish(20, libData, data, pos);
}

static void import_slave_device(WolframLibraryData libData, MTensor data, int idx, const FIT_SLAVE_DEVICE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SLAVE_DEVICE);
    ImportInteger(2, libData, data, pos, mesg->manufacturer);
    ImportInteger(3, libData, data, pos, mesg->product);
    ImportFinish(4, libData, data, pos);
}

static void import_capabilities(WolframLibraryData libData, MTensor data, int idx, const FIT_CAPABILITIES_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_CAPABILITIES);
    ImportIntegerSequence(2, libData, data, pos, mesg->languages, FIT_CAPABILITIES_MESG_LANGUAGES_COUNT);
    ImportInteger(6, libData, data, pos, mesg->workouts_supported);
    ImportInteger(7, libData, data, pos, mesg->connectivity_supported);
    ImportIntegerSequence(8, libData, data, pos, mesg->sports, FIT_CAPABILITIES_MESG_SPORTS_COUNT);
    ImportFinish(9, libData, data, pos);
}

static void import_file_capabilities(WolframLibraryData libData, MTensor data, int idx, const FIT_FILE_CAPABILITIES_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_FILE_CAPABILITIES);
    ImportString(2, libData, data, pos, mesg->directory, FIT_FILE_CAPABILITIES_MESG_DIRECTORY_COUNT);
    ImportInteger(18, libData, data, pos, mesg->max_size);
    ImportInteger(19, libData, data, pos, mesg->message_index);
    ImportInteger(20, libData, data, pos, mesg->max_count);
    ImportInteger(21, libData, data, pos, mesg->type);
    ImportInteger(22, libData, data, pos, mesg->flags);
    ImportFinish(23, libData, data, pos);
}

static void import_mesg_capabilities(WolframLibraryData libData, MTensor data, int idx, const FIT_MESG_CAPABILITIES_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_MESG_CAPABILITIES);
    ImportInteger(2, libData, data, pos, mesg->message_index);
    ImportInteger(3, libData, data, pos, mesg->mesg_num);
    ImportInteger(4, libData, data, pos, mesg->count);
    ImportInteger(5, libData, data, pos, mesg->file);
    ImportInteger(6, libData, data, pos, mesg->count_type);
    ImportFinish(7, libData, data, pos);
}

static void import_field_capabilities(WolframLibraryData libData, MTensor data, int idx, const FIT_FIELD_CAPABILITIES_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_FIELD_CAPABILITIES);
    ImportInteger(2, libData, data, pos, mesg->message_index);
    ImportInteger(3, libData, data, pos, mesg->mesg_num);
    ImportInteger(4, libData, data, pos, mesg->count);
    ImportInteger(5, libData, data, pos, mesg->file);
    ImportInteger(6, libData, data, pos, mesg->field_num);
    ImportFinish(7, libData, data, pos);
}

static void import_device_settings(WolframLibraryData libData, MTensor data, int idx, const FIT_DEVICE_SETTINGS_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_DEVICE_SETTINGS);
    ImportInteger(2, libData, data, pos, mesg->utc_offset);
    ImportIntegerSequence(3, libData, data, pos, mesg->time_offset, FIT_DEVICE_SETTINGS_MESG_TIME_OFFSET_COUNT);
    ImportInteger(5, libData, data, pos, WLTimestamp(mesg->clock_time));
    ImportIntegerSequence(6, libData, data, pos, mesg->pages_enabled, FIT_DEVICE_SETTINGS_MESG_PAGES_ENABLED_COUNT);
    ImportIntegerSequence(7, libData, data, pos, mesg->default_page, FIT_DEVICE_SETTINGS_MESG_DEFAULT_PAGE_COUNT);
    ImportInteger(8, libData, data, pos, mesg->autosync_min_steps);
    ImportInteger(9, libData, data, pos, mesg->autosync_min_time);
    ImportInteger(10, libData, data, pos, mesg->active_time_zone);
    ImportIntegerSequence(11, libData, data, pos, mesg->time_mode, FIT_DEVICE_SETTINGS_MESG_TIME_MODE_COUNT);
    ImportIntegerSequence(13, libData, data, pos, mesg->time_zone_offset, FIT_DEVICE_SETTINGS_MESG_TIME_ZONE_OFFSET_COUNT);
    ImportInteger(15, libData, data, pos, mesg->backlight_mode);
    ImportInteger(16, libData, data, pos, mesg->activity_tracker_enabled);
    ImportInteger(17, libData, data, pos, mesg->move_alert_enabled);
    ImportInteger(18, libData, data, pos, mesg->date_mode);
    ImportInteger(19, libData, data, pos, mesg->display_orientation);
    ImportInteger(20, libData, data, pos, mesg->mounting_side);
    ImportInteger(21, libData, data, pos, mesg->tap_sensitivity);
    ImportFinish(22, libData, data, pos);
}

static void import_user_profile(WolframLibraryData libData, MTensor data, int idx, const FIT_USER_PROFILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_USER_PROFILE);
    ImportString(2, libData, data, pos, mesg->friendly_name, FIT_USER_PROFILE_MESG_FRIENDLY_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->wake_time);
    ImportInteger(19, libData, data, pos, mesg->sleep_time);
    ImportInteger(20, libData, data, pos, mesg->dive_count);
    ImportInteger(21, libData, data, pos, mesg->message_index);
    ImportInteger(22, libData, data, pos, mesg->weight);
    ImportInteger(23, libData, data, pos, mesg->local_id);
    ImportInteger(24, libData, data, pos, mesg->user_running_step_length);
    ImportInteger(25, libData, data, pos, mesg->user_walking_step_length);
    ImportInteger(26, libData, data, pos, mesg->gender);
    ImportInteger(27, libData, data, pos, mesg->age);
    ImportInteger(28, libData, data, pos, mesg->height);
    ImportInteger(29, libData, data, pos, mesg->language);
    ImportInteger(30, libData, data, pos, mesg->elev_setting);
    ImportInteger(31, libData, data, pos, mesg->weight_setting);
    ImportInteger(32, libData, data, pos, mesg->resting_heart_rate);
    ImportInteger(33, libData, data, pos, mesg->default_max_running_heart_rate);
    ImportInteger(34, libData, data, pos, mesg->default_max_biking_heart_rate);
    ImportInteger(35, libData, data, pos, mesg->default_max_heart_rate);
    ImportInteger(36, libData, data, pos, mesg->hr_setting);
    ImportInteger(37, libData, data, pos, mesg->speed_setting);
    ImportInteger(38, libData, data, pos, mesg->dist_setting);
    ImportInteger(39, libData, data, pos, mesg->power_setting);
    ImportInteger(40, libData, data, pos, mesg->activity_class);
    ImportInteger(41, libData, data, pos, mesg->position_setting);
    ImportInteger(42, libData, data, pos, mesg->temperature_setting);
    ImportIntegerSequence(43, libData, data, pos, mesg->global_id, FIT_USER_PROFILE_MESG_GLOBAL_ID_COUNT);
    ImportInteger(49, libData, data, pos, mesg->height_setting);
    ImportInteger(50, libData, data, pos, mesg->depth_setting);
    ImportFinish(51, libData, data, pos);
}

static void import_hrm_profile(WolframLibraryData libData, MTensor data, int idx, const FIT_HRM_PROFILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_HRM_PROFILE);
    ImportInteger(2, libData, data, pos, mesg->message_index);
    ImportInteger(3, libData, data, pos, mesg->hrm_ant_id);
    ImportInteger(4, libData, data, pos, mesg->enabled);
    ImportInteger(5, libData, data, pos, mesg->log_hrv);
    ImportInteger(6, libData, data, pos, mesg->hrm_ant_id_trans_type);
    ImportFinish(7, libData, data, pos);
}

static void import_sdm_profile(WolframLibraryData libData, MTensor data, int idx, const FIT_SDM_PROFILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SDM_PROFILE);
    ImportInteger(2, libData, data, pos, mesg->odometer);
    ImportInteger(3, libData, data, pos, mesg->message_index);
    ImportInteger(4, libData, data, pos, mesg->sdm_ant_id);
    ImportInteger(5, libData, data, pos, mesg->sdm_cal_factor);
    ImportInteger(6, libData, data, pos, mesg->enabled);
    ImportInteger(7, libData, data, pos, mesg->speed_source);
    ImportInteger(8, libData, data, pos, mesg->sdm_ant_id_trans_type);
    ImportInteger(9, libData, data, pos, mesg->odometer_rollover);
    ImportFinish(10, libData, data, pos);
}

static void import_bike_profile(WolframLibraryData libData, MTensor data, int idx, const FIT_BIKE_PROFILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_BIKE_PROFILE);
    ImportString(2, libData, data, pos, mesg->name, FIT_BIKE_PROFILE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->odometer);
    ImportInteger(19, libData, data, pos, mesg->message_index);
    ImportInteger(20, libData, data, pos, mesg->bike_spd_ant_id);
    ImportInteger(21, libData, data, pos, mesg->bike_cad_ant_id);
    ImportInteger(22, libData, data, pos, mesg->bike_spdcad_ant_id);
    ImportInteger(23, libData, data, pos, mesg->bike_power_ant_id);
    ImportInteger(24, libData, data, pos, mesg->custom_wheelsize);
    ImportInteger(25, libData, data, pos, mesg->auto_wheelsize);
    ImportInteger(26, libData, data, pos, mesg->bike_weight);
    ImportInteger(27, libData, data, pos, mesg->power_cal_factor);
    ImportInteger(28, libData, data, pos, mesg->sport);
    ImportInteger(29, libData, data, pos, mesg->sub_sport);
    ImportInteger(30, libData, data, pos, mesg->auto_wheel_cal);
    ImportInteger(31, libData, data, pos, mesg->auto_power_zero);
    ImportInteger(32, libData, data, pos, mesg->id);
    ImportInteger(33, libData, data, pos, mesg->spd_enabled);
    ImportInteger(34, libData, data, pos, mesg->cad_enabled);
    ImportInteger(35, libData, data, pos, mesg->spdcad_enabled);
    ImportInteger(36, libData, data, pos, mesg->power_enabled);
    ImportInteger(37, libData, data, pos, mesg->crank_length);
    ImportInteger(38, libData, data, pos, mesg->enabled);
    ImportInteger(39, libData, data, pos, mesg->bike_spd_ant_id_trans_type);
    ImportInteger(40, libData, data, pos, mesg->bike_cad_ant_id_trans_type);
    ImportInteger(41, libData, data, pos, mesg->bike_spdcad_ant_id_trans_type);
    ImportInteger(42, libData, data, pos, mesg->bike_power_ant_id_trans_type);
    ImportInteger(43, libData, data, pos, mesg->odometer_rollover);
    ImportInteger(44, libData, data, pos, mesg->front_gear_num);
    ImportIntegerSequence(45, libData, data, pos, mesg->front_gear, FIT_BIKE_PROFILE_MESG_FRONT_GEAR_COUNT);
    ImportInteger(46, libData, data, pos, mesg->rear_gear_num);
    ImportIntegerSequence(47, libData, data, pos, mesg->rear_gear, FIT_BIKE_PROFILE_MESG_REAR_GEAR_COUNT);
    ImportInteger(48, libData, data, pos, mesg->shimano_di2_enabled);
    ImportFinish(49, libData, data, pos);
}

static void import_connectivity(WolframLibraryData libData, MTensor data, int idx, const FIT_CONNECTIVITY_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_CONNECTIVITY);
    ImportString(2, libData, data, pos, mesg->name, FIT_CONNECTIVITY_MESG_NAME_COUNT);
    ImportInteger(26, libData, data, pos, mesg->bluetooth_enabled);
    ImportInteger(27, libData, data, pos, mesg->bluetooth_le_enabled);
    ImportInteger(28, libData, data, pos, mesg->ant_enabled);
    ImportInteger(29, libData, data, pos, mesg->live_tracking_enabled);
    ImportInteger(30, libData, data, pos, mesg->weather_conditions_enabled);
    ImportInteger(31, libData, data, pos, mesg->weather_alerts_enabled);
    ImportInteger(32, libData, data, pos, mesg->auto_activity_upload_enabled);
    ImportInteger(33, libData, data, pos, mesg->course_download_enabled);
    ImportInteger(34, libData, data, pos, mesg->workout_download_enabled);
    ImportInteger(35, libData, data, pos, mesg->gps_ephemeris_download_enabled);
    ImportInteger(36, libData, data, pos, mesg->incident_detection_enabled);
    ImportInteger(37, libData, data, pos, mesg->grouptrack_enabled);
    ImportFinish(38, libData, data, pos);
}

static void import_zones_target(WolframLibraryData libData, MTensor data, int idx, const FIT_ZONES_TARGET_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_ZONES_TARGET);
    ImportInteger(2, libData, data, pos, mesg->functional_threshold_power);
    ImportInteger(3, libData, data, pos, mesg->max_heart_rate);
    ImportInteger(4, libData, data, pos, mesg->threshold_heart_rate);
    ImportInteger(5, libData, data, pos, mesg->hr_calc_type);
    ImportInteger(6, libData, data, pos, mesg->pwr_calc_type);
    ImportFinish(7, libData, data, pos);
}

static void import_sport(WolframLibraryData libData, MTensor data, int idx, const FIT_SPORT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SPORT);
    ImportString(2, libData, data, pos, mesg->name, FIT_SPORT_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->sport);
    ImportInteger(19, libData, data, pos, mesg->sub_sport);
    ImportFinish(20, libData, data, pos);
}

static void import_hr_zone(WolframLibraryData libData, MTensor data, int idx, const FIT_HR_ZONE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_HR_ZONE);
    ImportString(2, libData, data, pos, mesg->name, FIT_HR_ZONE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->message_index);
    ImportInteger(19, libData, data, pos, mesg->high_bpm);
    ImportFinish(20, libData, data, pos);
}

static void import_speed_zone(WolframLibraryData libData, MTensor data, int idx, const FIT_SPEED_ZONE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SPEED_ZONE);
    ImportString(2, libData, data, pos, mesg->name, FIT_SPEED_ZONE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->message_index);
    ImportInteger(19, libData, data, pos, mesg->high_value);
    ImportFinish(20, libData, data, pos);
}

static void import_cadence_zone(WolframLibraryData libData, MTensor data, int idx, const FIT_CADENCE_ZONE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_CADENCE_ZONE);
    ImportString(2, libData, data, pos, mesg->name, FIT_CADENCE_ZONE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->message_index);
    ImportInteger(19, libData, data, pos, mesg->high_value);
    ImportFinish(20, libData, data, pos);
}

static void import_power_zone(WolframLibraryData libData, MTensor data, int idx, const FIT_POWER_ZONE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_POWER_ZONE);
    ImportString(2, libData, data, pos, mesg->name, FIT_POWER_ZONE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->message_index);
    ImportInteger(19, libData, data, pos, mesg->high_value);
    ImportFinish(20, libData, data, pos);
}

static void import_met_zone(WolframLibraryData libData, MTensor data, int idx, const FIT_MET_ZONE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_MET_ZONE);
    ImportInteger(2, libData, data, pos, mesg->message_index);
    ImportInteger(3, libData, data, pos, mesg->calories);
    ImportInteger(4, libData, data, pos, mesg->high_bpm);
    ImportInteger(5, libData, data, pos, mesg->fat_calories);
    ImportFinish(6, libData, data, pos);
}

static void import_dive_settings(WolframLibraryData libData, MTensor data, int idx, const FIT_DIVE_SETTINGS_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_DIVE_SETTINGS);
    ImportString(2, libData, data, pos, mesg->name, FIT_DIVE_SETTINGS_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->heart_rate_source);
    ImportFinish(19, libData, data, pos);
}

static void import_goal(WolframLibraryData libData, MTensor data, int idx, const FIT_GOAL_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_GOAL);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->start_date));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->end_date));
    ImportInteger(4, libData, data, pos, mesg->value);
    ImportInteger(5, libData, data, pos, mesg->target_value);
    ImportInteger(6, libData, data, pos, mesg->message_index);
    ImportInteger(7, libData, data, pos, mesg->recurrence_value);
    ImportInteger(8, libData, data, pos, mesg->sport);
    ImportInteger(9, libData, data, pos, mesg->sub_sport);
    ImportInteger(10, libData, data, pos, mesg->type);
    ImportInteger(11, libData, data, pos, mesg->repeat);
    ImportInteger(12, libData, data, pos, mesg->recurrence);
    ImportInteger(13, libData, data, pos, mesg->enabled);
    ImportInteger(14, libData, data, pos, mesg->source);
    ImportFinish(15, libData, data, pos);
}

static void import_activity(WolframLibraryData libData, MTensor data, int idx, const FIT_ACTIVITY_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_ACTIVITY);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->total_timer_time);
    ImportInteger(4, libData, data, pos, WLTimestamp(mesg->local_timestamp));
    ImportInteger(5, libData, data, pos, mesg->num_sessions);
    ImportInteger(6, libData, data, pos, mesg->type);
    ImportInteger(7, libData, data, pos, mesg->event);
    ImportInteger(8, libData, data, pos, mesg->event_type);
    ImportInteger(9, libData, data, pos, mesg->event_group);
    ImportFinish(10, libData, data, pos);
}

static void import_session(WolframLibraryData libData, MTensor data, int idx, const FIT_SESSION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SESSION);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->start_time));
    ImportInteger(4, libData, data, pos, mesg->start_position_lat);
    ImportInteger(5, libData, data, pos, mesg->start_position_long);
    ImportInteger(6, libData, data, pos, mesg->total_elapsed_time);
    ImportInteger(7, libData, data, pos, mesg->total_timer_time);
    ImportInteger(8, libData, data, pos, mesg->total_distance);
    ImportInteger(9, libData, data, pos, mesg->total_cycles);
    ImportInteger(10, libData, data, pos, mesg->nec_lat);
    ImportInteger(11, libData, data, pos, mesg->nec_long);
    ImportInteger(12, libData, data, pos, mesg->swc_lat);
    ImportInteger(13, libData, data, pos, mesg->swc_long);
    ImportInteger(14, libData, data, pos, mesg->avg_stroke_count);
    ImportInteger(15, libData, data, pos, mesg->total_work);
    ImportInteger(16, libData, data, pos, mesg->total_moving_time);
    ImportIntegerSequence(17, libData, data, pos, mesg->time_in_hr_zone, FIT_SESSION_MESG_TIME_IN_HR_ZONE_COUNT);
    ImportIntegerSequence(18, libData, data, pos, mesg->time_in_speed_zone, FIT_SESSION_MESG_TIME_IN_SPEED_ZONE_COUNT);
    ImportIntegerSequence(19, libData, data, pos, mesg->time_in_cadence_zone, FIT_SESSION_MESG_TIME_IN_CADENCE_ZONE_COUNT);
    ImportIntegerSequence(20, libData, data, pos, mesg->time_in_power_zone, FIT_SESSION_MESG_TIME_IN_POWER_ZONE_COUNT);
    ImportInteger(21, libData, data, pos, mesg->avg_lap_time);
    ImportString(22, libData, data, pos, mesg->opponent_name, FIT_SESSION_MESG_OPPONENT_NAME_COUNT);
    ImportInteger(38, libData, data, pos, mesg->time_standing);
    ImportIntegerSequence(39, libData, data, pos, mesg->avg_power_position, FIT_SESSION_MESG_AVG_POWER_POSITION_COUNT);
    ImportIntegerSequence(41, libData, data, pos, mesg->max_power_position, FIT_SESSION_MESG_MAX_POWER_POSITION_COUNT);
    ImportInteger(43, libData, data, pos, mesg->enhanced_avg_speed);
    ImportInteger(44, libData, data, pos, mesg->enhanced_max_speed);
    ImportInteger(45, libData, data, pos, mesg->enhanced_avg_altitude);
    ImportInteger(46, libData, data, pos, mesg->enhanced_min_altitude);
    ImportInteger(47, libData, data, pos, mesg->enhanced_max_altitude);
    ImportInteger(48, libData, data, pos, mesg->training_load_peak);
    ImportFloat(49, libData, data, pos, mesg->total_grit);
    ImportFloat(50, libData, data, pos, mesg->total_flow);
    ImportFloat(51, libData, data, pos, mesg->avg_grit);
    ImportFloat(52, libData, data, pos, mesg->avg_flow);
    ImportInteger(53, libData, data, pos, mesg->message_index);
    ImportInteger(54, libData, data, pos, mesg->total_calories);
    ImportInteger(55, libData, data, pos, mesg->total_fat_calories);
    ImportInteger(56, libData, data, pos, mesg->avg_speed);
    ImportInteger(57, libData, data, pos, mesg->max_speed);
    ImportInteger(58, libData, data, pos, mesg->avg_power);
    ImportInteger(59, libData, data, pos, mesg->max_power);
    ImportInteger(60, libData, data, pos, mesg->total_ascent);
    ImportInteger(61, libData, data, pos, mesg->total_descent);
    ImportInteger(62, libData, data, pos, mesg->first_lap_index);
    ImportInteger(63, libData, data, pos, mesg->num_laps);
    ImportInteger(64, libData, data, pos, mesg->num_lengths);
    ImportInteger(65, libData, data, pos, mesg->normalized_power);
    ImportInteger(66, libData, data, pos, mesg->training_stress_score);
    ImportInteger(67, libData, data, pos, mesg->intensity_factor);
    ImportInteger(68, libData, data, pos, mesg->left_right_balance);
    ImportInteger(69, libData, data, pos, mesg->avg_stroke_distance);
    ImportInteger(70, libData, data, pos, mesg->pool_length);
    ImportInteger(71, libData, data, pos, mesg->threshold_power);
    ImportInteger(72, libData, data, pos, mesg->num_active_lengths);
    ImportInteger(73, libData, data, pos, mesg->avg_altitude);
    ImportInteger(74, libData, data, pos, mesg->max_altitude);
    ImportInteger(75, libData, data, pos, mesg->avg_grade);
    ImportInteger(76, libData, data, pos, mesg->avg_pos_grade);
    ImportInteger(77, libData, data, pos, mesg->avg_neg_grade);
    ImportInteger(78, libData, data, pos, mesg->max_pos_grade);
    ImportInteger(79, libData, data, pos, mesg->max_neg_grade);
    ImportInteger(80, libData, data, pos, mesg->avg_pos_vertical_speed);
    ImportInteger(81, libData, data, pos, mesg->avg_neg_vertical_speed);
    ImportInteger(82, libData, data, pos, mesg->max_pos_vertical_speed);
    ImportInteger(83, libData, data, pos, mesg->max_neg_vertical_speed);
    ImportInteger(84, libData, data, pos, mesg->best_lap_index);
    ImportInteger(85, libData, data, pos, mesg->min_altitude);
    ImportInteger(86, libData, data, pos, mesg->player_score);
    ImportInteger(87, libData, data, pos, mesg->opponent_score);
    ImportIntegerSequence(88, libData, data, pos, mesg->stroke_count, FIT_SESSION_MESG_STROKE_COUNT_COUNT);
    ImportIntegerSequence(89, libData, data, pos, mesg->zone_count, FIT_SESSION_MESG_ZONE_COUNT_COUNT);
    ImportInteger(90, libData, data, pos, mesg->max_ball_speed);
    ImportInteger(91, libData, data, pos, mesg->avg_ball_speed);
    ImportInteger(92, libData, data, pos, mesg->avg_vertical_oscillation);
    ImportInteger(93, libData, data, pos, mesg->avg_stance_time_percent);
    ImportInteger(94, libData, data, pos, mesg->avg_stance_time);
    ImportIntegerSequence(95, libData, data, pos, mesg->avg_total_hemoglobin_conc, FIT_SESSION_MESG_AVG_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(96, libData, data, pos, mesg->min_total_hemoglobin_conc, FIT_SESSION_MESG_MIN_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(97, libData, data, pos, mesg->max_total_hemoglobin_conc, FIT_SESSION_MESG_MAX_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(98, libData, data, pos, mesg->avg_saturated_hemoglobin_percent, FIT_SESSION_MESG_AVG_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportIntegerSequence(99, libData, data, pos, mesg->min_saturated_hemoglobin_percent, FIT_SESSION_MESG_MIN_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportIntegerSequence(100, libData, data, pos, mesg->max_saturated_hemoglobin_percent, FIT_SESSION_MESG_MAX_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportInteger(101, libData, data, pos, mesg->stand_count);
    ImportInteger(102, libData, data, pos, mesg->avg_lev_motor_power);
    ImportInteger(103, libData, data, pos, mesg->max_lev_motor_power);
    ImportInteger(104, libData, data, pos, mesg->avg_vertical_ratio);
    ImportInteger(105, libData, data, pos, mesg->avg_stance_time_balance);
    ImportInteger(106, libData, data, pos, mesg->avg_step_length);
    ImportInteger(107, libData, data, pos, mesg->avg_vam);
    ImportInteger(108, libData, data, pos, mesg->jump_count);
    ImportInteger(109, libData, data, pos, mesg->avg_core_temperature);
    ImportInteger(110, libData, data, pos, mesg->min_core_temperature);
    ImportInteger(111, libData, data, pos, mesg->max_core_temperature);
    ImportInteger(112, libData, data, pos, mesg->event);
    ImportInteger(113, libData, data, pos, mesg->event_type);
    ImportInteger(114, libData, data, pos, mesg->sport);
    ImportInteger(115, libData, data, pos, mesg->sub_sport);
    ImportInteger(116, libData, data, pos, mesg->avg_heart_rate);
    ImportInteger(117, libData, data, pos, mesg->max_heart_rate);
    ImportInteger(118, libData, data, pos, mesg->avg_cadence);
    ImportInteger(119, libData, data, pos, mesg->max_cadence);
    ImportInteger(120, libData, data, pos, mesg->total_training_effect);
    ImportInteger(121, libData, data, pos, mesg->event_group);
    ImportInteger(122, libData, data, pos, mesg->trigger);
    ImportInteger(123, libData, data, pos, mesg->swim_stroke);
    ImportInteger(124, libData, data, pos, mesg->pool_length_unit);
    ImportInteger(125, libData, data, pos, mesg->gps_accuracy);
    ImportInteger(126, libData, data, pos, mesg->avg_temperature);
    ImportInteger(127, libData, data, pos, mesg->max_temperature);
    ImportInteger(128, libData, data, pos, mesg->min_heart_rate);
    ImportInteger(129, libData, data, pos, mesg->avg_fractional_cadence);
    ImportInteger(130, libData, data, pos, mesg->max_fractional_cadence);
    ImportInteger(131, libData, data, pos, mesg->total_fractional_cycles);
    ImportInteger(132, libData, data, pos, mesg->avg_left_torque_effectiveness);
    ImportInteger(133, libData, data, pos, mesg->avg_right_torque_effectiveness);
    ImportInteger(134, libData, data, pos, mesg->avg_left_pedal_smoothness);
    ImportInteger(135, libData, data, pos, mesg->avg_right_pedal_smoothness);
    ImportInteger(136, libData, data, pos, mesg->avg_combined_pedal_smoothness);
    ImportInteger(137, libData, data, pos, mesg->sport_index);
    ImportInteger(138, libData, data, pos, mesg->avg_left_pco);
    ImportInteger(139, libData, data, pos, mesg->avg_right_pco);
    ImportIntegerSequence(140, libData, data, pos, mesg->avg_left_power_phase, FIT_SESSION_MESG_AVG_LEFT_POWER_PHASE_COUNT);
    ImportIntegerSequence(142, libData, data, pos, mesg->avg_left_power_phase_peak, FIT_SESSION_MESG_AVG_LEFT_POWER_PHASE_PEAK_COUNT);
    ImportIntegerSequence(144, libData, data, pos, mesg->avg_right_power_phase, FIT_SESSION_MESG_AVG_RIGHT_POWER_PHASE_COUNT);
    ImportIntegerSequence(146, libData, data, pos, mesg->avg_right_power_phase_peak, FIT_SESSION_MESG_AVG_RIGHT_POWER_PHASE_PEAK_COUNT);
    ImportIntegerSequence(148, libData, data, pos, mesg->avg_cadence_position, FIT_SESSION_MESG_AVG_CADENCE_POSITION_COUNT);
    ImportIntegerSequence(150, libData, data, pos, mesg->max_cadence_position, FIT_SESSION_MESG_MAX_CADENCE_POSITION_COUNT);
    ImportInteger(152, libData, data, pos, mesg->lev_battery_consumption);
    ImportInteger(153, libData, data, pos, mesg->total_anaerobic_training_effect);
    ImportInteger(154, libData, data, pos, mesg->total_fractional_ascent);
    ImportInteger(155, libData, data, pos, mesg->total_fractional_descent);
    ImportFinish(156, libData, data, pos);
}

static void import_lap(WolframLibraryData libData, MTensor data, int idx, const FIT_LAP_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_LAP);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->start_time));
    ImportInteger(4, libData, data, pos, mesg->start_position_lat);
    ImportInteger(5, libData, data, pos, mesg->start_position_long);
    ImportInteger(6, libData, data, pos, mesg->end_position_lat);
    ImportInteger(7, libData, data, pos, mesg->end_position_long);
    ImportInteger(8, libData, data, pos, mesg->total_elapsed_time);
    ImportInteger(9, libData, data, pos, mesg->total_timer_time);
    ImportInteger(10, libData, data, pos, mesg->total_distance);
    ImportInteger(11, libData, data, pos, mesg->total_cycles);
    ImportInteger(12, libData, data, pos, mesg->total_work);
    ImportInteger(13, libData, data, pos, mesg->total_moving_time);
    ImportIntegerSequence(14, libData, data, pos, mesg->time_in_hr_zone, FIT_LAP_MESG_TIME_IN_HR_ZONE_COUNT);
    ImportIntegerSequence(15, libData, data, pos, mesg->time_in_speed_zone, FIT_LAP_MESG_TIME_IN_SPEED_ZONE_COUNT);
    ImportIntegerSequence(16, libData, data, pos, mesg->time_in_cadence_zone, FIT_LAP_MESG_TIME_IN_CADENCE_ZONE_COUNT);
    ImportIntegerSequence(17, libData, data, pos, mesg->time_in_power_zone, FIT_LAP_MESG_TIME_IN_POWER_ZONE_COUNT);
    ImportInteger(18, libData, data, pos, mesg->enhanced_avg_speed);
    ImportInteger(19, libData, data, pos, mesg->enhanced_max_speed);
    ImportInteger(20, libData, data, pos, mesg->enhanced_avg_altitude);
    ImportInteger(21, libData, data, pos, mesg->enhanced_min_altitude);
    ImportInteger(22, libData, data, pos, mesg->enhanced_max_altitude);
    ImportInteger(23, libData, data, pos, mesg->message_index);
    ImportInteger(24, libData, data, pos, mesg->total_calories);
    ImportInteger(25, libData, data, pos, mesg->total_fat_calories);
    ImportInteger(26, libData, data, pos, mesg->avg_speed);
    ImportInteger(27, libData, data, pos, mesg->max_speed);
    ImportInteger(28, libData, data, pos, mesg->avg_power);
    ImportInteger(29, libData, data, pos, mesg->max_power);
    ImportInteger(30, libData, data, pos, mesg->total_ascent);
    ImportInteger(31, libData, data, pos, mesg->total_descent);
    ImportInteger(32, libData, data, pos, mesg->num_lengths);
    ImportInteger(33, libData, data, pos, mesg->normalized_power);
    ImportInteger(34, libData, data, pos, mesg->left_right_balance);
    ImportInteger(35, libData, data, pos, mesg->first_length_index);
    ImportInteger(36, libData, data, pos, mesg->avg_stroke_distance);
    ImportInteger(37, libData, data, pos, mesg->num_active_lengths);
    ImportInteger(38, libData, data, pos, mesg->avg_altitude);
    ImportInteger(39, libData, data, pos, mesg->max_altitude);
    ImportInteger(40, libData, data, pos, mesg->avg_grade);
    ImportInteger(41, libData, data, pos, mesg->avg_pos_grade);
    ImportInteger(42, libData, data, pos, mesg->avg_neg_grade);
    ImportInteger(43, libData, data, pos, mesg->max_pos_grade);
    ImportInteger(44, libData, data, pos, mesg->max_neg_grade);
    ImportInteger(45, libData, data, pos, mesg->avg_pos_vertical_speed);
    ImportInteger(46, libData, data, pos, mesg->avg_neg_vertical_speed);
    ImportInteger(47, libData, data, pos, mesg->max_pos_vertical_speed);
    ImportInteger(48, libData, data, pos, mesg->max_neg_vertical_speed);
    ImportInteger(49, libData, data, pos, mesg->repetition_num);
    ImportInteger(50, libData, data, pos, mesg->min_altitude);
    ImportInteger(51, libData, data, pos, mesg->wkt_step_index);
    ImportInteger(52, libData, data, pos, mesg->opponent_score);
    ImportIntegerSequence(53, libData, data, pos, mesg->stroke_count, FIT_LAP_MESG_STROKE_COUNT_COUNT);
    ImportIntegerSequence(54, libData, data, pos, mesg->zone_count, FIT_LAP_MESG_ZONE_COUNT_COUNT);
    ImportInteger(55, libData, data, pos, mesg->avg_vertical_oscillation);
    ImportInteger(56, libData, data, pos, mesg->avg_stance_time_percent);
    ImportInteger(57, libData, data, pos, mesg->avg_stance_time);
    ImportInteger(58, libData, data, pos, mesg->player_score);
    ImportIntegerSequence(59, libData, data, pos, mesg->avg_total_hemoglobin_conc, FIT_LAP_MESG_AVG_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(60, libData, data, pos, mesg->min_total_hemoglobin_conc, FIT_LAP_MESG_MIN_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(61, libData, data, pos, mesg->max_total_hemoglobin_conc, FIT_LAP_MESG_MAX_TOTAL_HEMOGLOBIN_CONC_COUNT);
    ImportIntegerSequence(62, libData, data, pos, mesg->avg_saturated_hemoglobin_percent, FIT_LAP_MESG_AVG_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportIntegerSequence(63, libData, data, pos, mesg->min_saturated_hemoglobin_percent, FIT_LAP_MESG_MIN_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportIntegerSequence(64, libData, data, pos, mesg->max_saturated_hemoglobin_percent, FIT_LAP_MESG_MAX_SATURATED_HEMOGLOBIN_PERCENT_COUNT);
    ImportInteger(65, libData, data, pos, mesg->avg_vam);
    ImportInteger(66, libData, data, pos, mesg->event);
    ImportInteger(67, libData, data, pos, mesg->event_type);
    ImportInteger(68, libData, data, pos, mesg->avg_heart_rate);
    ImportInteger(69, libData, data, pos, mesg->max_heart_rate);
    ImportInteger(70, libData, data, pos, mesg->avg_cadence);
    ImportInteger(71, libData, data, pos, mesg->max_cadence);
    ImportInteger(72, libData, data, pos, mesg->intensity);
    ImportInteger(73, libData, data, pos, mesg->lap_trigger);
    ImportInteger(74, libData, data, pos, mesg->sport);
    ImportInteger(75, libData, data, pos, mesg->event_group);
    ImportInteger(76, libData, data, pos, mesg->swim_stroke);
    ImportInteger(77, libData, data, pos, mesg->sub_sport);
    ImportInteger(78, libData, data, pos, mesg->gps_accuracy);
    ImportInteger(79, libData, data, pos, mesg->avg_temperature);
    ImportInteger(80, libData, data, pos, mesg->max_temperature);
    ImportInteger(81, libData, data, pos, mesg->min_heart_rate);
    ImportInteger(82, libData, data, pos, mesg->avg_fractional_cadence);
    ImportInteger(83, libData, data, pos, mesg->max_fractional_cadence);
    ImportInteger(84, libData, data, pos, mesg->total_fractional_cycles);
    ImportFinish(85, libData, data, pos);
}

static void import_length(WolframLibraryData libData, MTensor data, int idx, const FIT_LENGTH_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_LENGTH);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->start_time));
    ImportInteger(4, libData, data, pos, mesg->total_elapsed_time);
    ImportInteger(5, libData, data, pos, mesg->total_timer_time);
    ImportInteger(6, libData, data, pos, mesg->message_index);
    ImportInteger(7, libData, data, pos, mesg->total_strokes);
    ImportInteger(8, libData, data, pos, mesg->avg_speed);
    ImportInteger(9, libData, data, pos, mesg->total_calories);
    ImportInteger(10, libData, data, pos, mesg->player_score);
    ImportInteger(11, libData, data, pos, mesg->opponent_score);
    ImportIntegerSequence(12, libData, data, pos, mesg->stroke_count, FIT_LENGTH_MESG_STROKE_COUNT_COUNT);
    ImportIntegerSequence(13, libData, data, pos, mesg->zone_count, FIT_LENGTH_MESG_ZONE_COUNT_COUNT);
    ImportInteger(14, libData, data, pos, mesg->event);
    ImportInteger(15, libData, data, pos, mesg->event_type);
    ImportInteger(16, libData, data, pos, mesg->swim_stroke);
    ImportInteger(17, libData, data, pos, mesg->avg_swimming_cadence);
    ImportInteger(18, libData, data, pos, mesg->event_group);
    ImportInteger(19, libData, data, pos, mesg->length_type);
    ImportFinish(20, libData, data, pos);
}

static void import_record(WolframLibraryData libData, MTensor data, int idx, const FIT_RECORD_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_RECORD);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->position_lat);
    ImportInteger(4, libData, data, pos, mesg->position_long);
    ImportInteger(5, libData, data, pos, mesg->distance);
    ImportInteger(6, libData, data, pos, mesg->time_from_course);
    ImportInteger(7, libData, data, pos, mesg->total_cycles);
    ImportInteger(8, libData, data, pos, mesg->accumulated_power);
    ImportInteger(9, libData, data, pos, mesg->enhanced_speed);
    ImportInteger(10, libData, data, pos, mesg->enhanced_altitude);
    ImportInteger(11, libData, data, pos, mesg->absolute_pressure);
    ImportInteger(12, libData, data, pos, mesg->depth);
    ImportInteger(13, libData, data, pos, mesg->next_stop_depth);
    ImportInteger(14, libData, data, pos, mesg->next_stop_time);
    ImportInteger(15, libData, data, pos, mesg->time_to_surface);
    ImportInteger(16, libData, data, pos, mesg->ndl_time);
    ImportFloat(17, libData, data, pos, mesg->grit);
    ImportFloat(18, libData, data, pos, mesg->flow);
    ImportInteger(19, libData, data, pos, mesg->altitude);
    ImportInteger(20, libData, data, pos, mesg->speed);
    ImportInteger(21, libData, data, pos, mesg->power);
    ImportInteger(22, libData, data, pos, mesg->grade);
    ImportInteger(23, libData, data, pos, mesg->compressed_accumulated_power);
    ImportInteger(24, libData, data, pos, mesg->vertical_speed);
    ImportInteger(25, libData, data, pos, mesg->calories);
    ImportInteger(26, libData, data, pos, mesg->vertical_oscillation);
    ImportInteger(27, libData, data, pos, mesg->stance_time_percent);
    ImportInteger(28, libData, data, pos, mesg->stance_time);
    ImportInteger(29, libData, data, pos, mesg->ball_speed);
    ImportInteger(30, libData, data, pos, mesg->cadence256);
    ImportInteger(31, libData, data, pos, mesg->total_hemoglobin_conc);
    ImportInteger(32, libData, data, pos, mesg->total_hemoglobin_conc_min);
    ImportInteger(33, libData, data, pos, mesg->total_hemoglobin_conc_max);
    ImportInteger(34, libData, data, pos, mesg->saturated_hemoglobin_percent);
    ImportInteger(35, libData, data, pos, mesg->saturated_hemoglobin_percent_min);
    ImportInteger(36, libData, data, pos, mesg->saturated_hemoglobin_percent_max);
    ImportInteger(37, libData, data, pos, mesg->motor_power);
    ImportInteger(38, libData, data, pos, mesg->vertical_ratio);
    ImportInteger(39, libData, data, pos, mesg->stance_time_balance);
    ImportInteger(40, libData, data, pos, mesg->step_length);
    ImportInteger(41, libData, data, pos, mesg->n2_load);
    ImportInteger(42, libData, data, pos, mesg->ebike_travel_range);
    ImportInteger(43, libData, data, pos, mesg->core_temperature);
    ImportInteger(44, libData, data, pos, mesg->heart_rate);
    ImportInteger(45, libData, data, pos, mesg->cadence);
    ImportIntegerSequence(46, libData, data, pos, mesg->compressed_speed_distance, FIT_RECORD_MESG_COMPRESSED_SPEED_DISTANCE_COUNT);
    ImportInteger(49, libData, data, pos, mesg->resistance);
    ImportInteger(50, libData, data, pos, mesg->cycle_length);
    ImportInteger(51, libData, data, pos, mesg->temperature);
    ImportIntegerSequence(52, libData, data, pos, mesg->speed_1s, FIT_RECORD_MESG_SPEED_1S_COUNT);
    ImportInteger(57, libData, data, pos, mesg->cycles);
    ImportInteger(58, libData, data, pos, mesg->left_right_balance);
    ImportInteger(59, libData, data, pos, mesg->gps_accuracy);
    ImportInteger(60, libData, data, pos, mesg->activity_type);
    ImportInteger(61, libData, data, pos, mesg->left_torque_effectiveness);
    ImportInteger(62, libData, data, pos, mesg->right_torque_effectiveness);
    ImportInteger(63, libData, data, pos, mesg->left_pedal_smoothness);
    ImportInteger(64, libData, data, pos, mesg->right_pedal_smoothness);
    ImportInteger(65, libData, data, pos, mesg->combined_pedal_smoothness);
    ImportInteger(66, libData, data, pos, mesg->time128);
    ImportInteger(67, libData, data, pos, mesg->stroke_type);
    ImportInteger(68, libData, data, pos, mesg->zone);
    ImportInteger(69, libData, data, pos, mesg->fractional_cadence);
    ImportInteger(70, libData, data, pos, mesg->device_index);
    ImportInteger(71, libData, data, pos, mesg->left_pco);
    ImportInteger(72, libData, data, pos, mesg->right_pco);
    ImportIntegerSequence(73, libData, data, pos, mesg->left_power_phase, FIT_RECORD_MESG_LEFT_POWER_PHASE_COUNT);
    ImportIntegerSequence(75, libData, data, pos, mesg->left_power_phase_peak, FIT_RECORD_MESG_LEFT_POWER_PHASE_PEAK_COUNT);
    ImportIntegerSequence(77, libData, data, pos, mesg->right_power_phase, FIT_RECORD_MESG_RIGHT_POWER_PHASE_COUNT);
    ImportIntegerSequence(79, libData, data, pos, mesg->right_power_phase_peak, FIT_RECORD_MESG_RIGHT_POWER_PHASE_PEAK_COUNT);
    ImportInteger(81, libData, data, pos, mesg->battery_soc);
    ImportInteger(82, libData, data, pos, mesg->cns_load);
    ImportInteger(83, libData, data, pos, mesg->ebike_battery_level);
    ImportInteger(84, libData, data, pos, mesg->ebike_assist_mode);
    ImportInteger(85, libData, data, pos, mesg->ebike_assist_level_percent);
    ImportFinish(86, libData, data, pos);
}

static void import_event(WolframLibraryData libData, MTensor data, int idx, const FIT_EVENT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_EVENT);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->data);
    ImportInteger(4, libData, data, pos, mesg->data16);
    ImportInteger(5, libData, data, pos, mesg->score);
    ImportInteger(6, libData, data, pos, mesg->opponent_score);
    ImportInteger(7, libData, data, pos, mesg->event);
    ImportInteger(8, libData, data, pos, mesg->event_type);
    ImportInteger(9, libData, data, pos, mesg->event_group);
    ImportInteger(10, libData, data, pos, mesg->front_gear_num);
    ImportInteger(11, libData, data, pos, mesg->front_gear);
    ImportInteger(12, libData, data, pos, mesg->rear_gear_num);
    ImportInteger(13, libData, data, pos, mesg->rear_gear);
    ImportInteger(14, libData, data, pos, mesg->radar_threat_level_max);
    ImportInteger(15, libData, data, pos, mesg->radar_threat_count);
    ImportFinish(16, libData, data, pos);
}

static void import_device_info(WolframLibraryData libData, MTensor data, int idx, const FIT_DEVICE_INFO_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_DEVICE_INFO);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->serial_number);
    ImportInteger(4, libData, data, pos, mesg->cum_operating_time);
    ImportString(5, libData, data, pos, mesg->product_name, FIT_DEVICE_INFO_MESG_PRODUCT_NAME_COUNT);
    ImportInteger(25, libData, data, pos, mesg->manufacturer);
    ImportInteger(26, libData, data, pos, mesg->product);
    ImportInteger(27, libData, data, pos, mesg->software_version);
    ImportInteger(28, libData, data, pos, mesg->battery_voltage);
    ImportInteger(29, libData, data, pos, mesg->ant_device_number);
    ImportInteger(30, libData, data, pos, mesg->device_index);
    ImportInteger(31, libData, data, pos, mesg->device_type);
    ImportInteger(32, libData, data, pos, mesg->hardware_version);
    ImportInteger(33, libData, data, pos, mesg->battery_status);
    ImportInteger(34, libData, data, pos, mesg->sensor_position);
    ImportString(35, libData, data, pos, mesg->descriptor, FIT_DEVICE_INFO_MESG_DESCRIPTOR_COUNT);
    ImportInteger(36, libData, data, pos, mesg->ant_transmission_type);
    ImportInteger(37, libData, data, pos, mesg->ant_network);
    ImportInteger(38, libData, data, pos, mesg->source_type);
    ImportFinish(39, libData, data, pos);
}

static void import_device_aux_battery_info(WolframLibraryData libData, MTensor data, int idx, const FIT_DEVICE_AUX_BATTERY_INFO_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_DEVICE_AUX_BATTERY_INFO);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->battery_voltage);
    ImportInteger(4, libData, data, pos, mesg->device_index);
    ImportInteger(5, libData, data, pos, mesg->battery_status);
    ImportInteger(6, libData, data, pos, mesg->battery_identifier);
    ImportFinish(7, libData, data, pos);
}

static void import_training_file(WolframLibraryData libData, MTensor data, int idx, const FIT_TRAINING_FILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_TRAINING_FILE);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->serial_number);
    ImportInteger(4, libData, data, pos, WLTimestamp(mesg->time_created));
    ImportInteger(5, libData, data, pos, mesg->manufacturer);
    ImportInteger(6, libData, data, pos, mesg->product);
    ImportInteger(7, libData, data, pos, mesg->type);
    ImportFinish(8, libData, data, pos);
}

static void import_weather_conditions(WolframLibraryData libData, MTensor data, int idx, const FIT_WEATHER_CONDITIONS_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WEATHER_CONDITIONS);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportString(3, libData, data, pos, mesg->location, FIT_WEATHER_CONDITIONS_MESG_LOCATION_COUNT);
    ImportInteger(67, libData, data, pos, WLTimestamp(mesg->observed_at_time));
    ImportInteger(68, libData, data, pos, mesg->observed_location_lat);
    ImportInteger(69, libData, data, pos, mesg->observed_location_long);
    ImportInteger(70, libData, data, pos, mesg->wind_direction);
    ImportInteger(71, libData, data, pos, mesg->wind_speed);
    ImportInteger(72, libData, data, pos, mesg->weather_report);
    ImportInteger(73, libData, data, pos, mesg->temperature);
    ImportInteger(74, libData, data, pos, mesg->condition);
    ImportInteger(75, libData, data, pos, mesg->precipitation_probability);
    ImportInteger(76, libData, data, pos, mesg->temperature_feels_like);
    ImportInteger(77, libData, data, pos, mesg->relative_humidity);
    ImportInteger(78, libData, data, pos, mesg->day_of_week);
    ImportInteger(79, libData, data, pos, mesg->high_temperature);
    ImportInteger(80, libData, data, pos, mesg->low_temperature);
    ImportFinish(81, libData, data, pos);
}

static void import_weather_alert(WolframLibraryData libData, MTensor data, int idx, const FIT_WEATHER_ALERT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WEATHER_ALERT);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportString(3, libData, data, pos, mesg->report_id, FIT_WEATHER_ALERT_MESG_REPORT_ID_COUNT);
    ImportInteger(15, libData, data, pos, WLTimestamp(mesg->issue_time));
    ImportInteger(16, libData, data, pos, WLTimestamp(mesg->expire_time));
    ImportInteger(17, libData, data, pos, mesg->severity);
    ImportInteger(18, libData, data, pos, mesg->type);
    ImportFinish(19, libData, data, pos);
}

static void import_nmea_sentence(WolframLibraryData libData, MTensor data, int idx, const FIT_NMEA_SENTENCE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_NMEA_SENTENCE);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->timestamp_ms);
    ImportString(4, libData, data, pos, mesg->sentence, FIT_NMEA_SENTENCE_MESG_SENTENCE_COUNT);
    ImportFinish(87, libData, data, pos);
}

static void import_aviation_attitude(WolframLibraryData libData, MTensor data, int idx, const FIT_AVIATION_ATTITUDE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_AVIATION_ATTITUDE);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportIntegerSequence(3, libData, data, pos, mesg->system_time, FIT_AVIATION_ATTITUDE_MESG_SYSTEM_TIME_COUNT);
    ImportInteger(4, libData, data, pos, mesg->timestamp_ms);
    ImportIntegerSequence(5, libData, data, pos, mesg->pitch, FIT_AVIATION_ATTITUDE_MESG_PITCH_COUNT);
    ImportIntegerSequence(6, libData, data, pos, mesg->roll, FIT_AVIATION_ATTITUDE_MESG_ROLL_COUNT);
    ImportIntegerSequence(7, libData, data, pos, mesg->accel_lateral, FIT_AVIATION_ATTITUDE_MESG_ACCEL_LATERAL_COUNT);
    ImportIntegerSequence(8, libData, data, pos, mesg->accel_normal, FIT_AVIATION_ATTITUDE_MESG_ACCEL_NORMAL_COUNT);
    ImportIntegerSequence(9, libData, data, pos, mesg->turn_rate, FIT_AVIATION_ATTITUDE_MESG_TURN_RATE_COUNT);
    ImportIntegerSequence(10, libData, data, pos, mesg->track, FIT_AVIATION_ATTITUDE_MESG_TRACK_COUNT);
    ImportIntegerSequence(11, libData, data, pos, mesg->validity, FIT_AVIATION_ATTITUDE_MESG_VALIDITY_COUNT);
    ImportIntegerSequence(12, libData, data, pos, mesg->stage, FIT_AVIATION_ATTITUDE_MESG_STAGE_COUNT);
    ImportIntegerSequence(13, libData, data, pos, mesg->attitude_stage_complete, FIT_AVIATION_ATTITUDE_MESG_ATTITUDE_STAGE_COMPLETE_COUNT);
    ImportFinish(14, libData, data, pos);
}

static void import_video_title(WolframLibraryData libData, MTensor data, int idx, const FIT_VIDEO_TITLE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_VIDEO_TITLE);
    ImportString(2, libData, data, pos, mesg->text, FIT_VIDEO_TITLE_MESG_TEXT_COUNT);
    ImportInteger(82, libData, data, pos, mesg->message_index);
    ImportInteger(83, libData, data, pos, mesg->message_count);
    ImportFinish(84, libData, data, pos);
}

static void import_video_description(WolframLibraryData libData, MTensor data, int idx, const FIT_VIDEO_DESCRIPTION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_VIDEO_DESCRIPTION);
    ImportString(2, libData, data, pos, mesg->text, FIT_VIDEO_DESCRIPTION_MESG_TEXT_COUNT);
    ImportInteger(130, libData, data, pos, mesg->message_index);
    ImportInteger(131, libData, data, pos, mesg->message_count);
    ImportFinish(132, libData, data, pos);
}

static void import_set(WolframLibraryData libData, MTensor data, int idx, const FIT_SET_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SET);
    ImportInteger(2, libData, data, pos, mesg->weight_display_unit);
    ImportFinish(3, libData, data, pos);
}

static void import_field_description(WolframLibraryData libData, MTensor data, int idx, const FIT_FIELD_DESCRIPTION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_FIELD_DESCRIPTION);
    ImportString(2, libData, data, pos, mesg->field_name, FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT);
    ImportString(66, libData, data, pos, mesg->units, FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT);
    ImportInteger(82, libData, data, pos, mesg->fit_base_unit_id);
    ImportInteger(83, libData, data, pos, mesg->native_mesg_num);
    ImportInteger(84, libData, data, pos, mesg->developer_data_index);
    ImportInteger(85, libData, data, pos, mesg->field_definition_number);
    ImportInteger(86, libData, data, pos, mesg->fit_base_type_id);
    ImportInteger(87, libData, data, pos, mesg->scale);
    ImportInteger(88, libData, data, pos, mesg->offset);
    ImportInteger(89, libData, data, pos, mesg->native_field_num);
    ImportFinish(90, libData, data, pos);
}

static void import_developer_data_id(WolframLibraryData libData, MTensor data, int idx, const FIT_DEVELOPER_DATA_ID_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_DEVELOPER_DATA_ID);
    ImportIntegerSequence(2, libData, data, pos, mesg->developer_id, FIT_DEVELOPER_DATA_ID_MESG_DEVELOPER_ID_COUNT);
    ImportIntegerSequence(18, libData, data, pos, mesg->application_id, FIT_DEVELOPER_DATA_ID_MESG_APPLICATION_ID_COUNT);
    ImportInteger(34, libData, data, pos, mesg->application_version);
    ImportInteger(35, libData, data, pos, mesg->manufacturer_id);
    ImportInteger(36, libData, data, pos, mesg->developer_data_index);
    ImportFinish(37, libData, data, pos);
}

static void import_course(WolframLibraryData libData, MTensor data, int idx, const FIT_COURSE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_COURSE);
    ImportString(2, libData, data, pos, mesg->name, FIT_COURSE_MESG_NAME_COUNT);
    ImportInteger(18, libData, data, pos, mesg->capabilities);
    ImportInteger(19, libData, data, pos, mesg->sport);
    ImportInteger(20, libData, data, pos, mesg->sub_sport);
    ImportFinish(21, libData, data, pos);
}

static void import_course_point(WolframLibraryData libData, MTensor data, int idx, const FIT_COURSE_POINT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_COURSE_POINT);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->position_lat);
    ImportInteger(4, libData, data, pos, mesg->position_long);
    ImportInteger(5, libData, data, pos, mesg->distance);
    ImportString(6, libData, data, pos, mesg->name, FIT_COURSE_POINT_MESG_NAME_COUNT);
    ImportInteger(22, libData, data, pos, mesg->message_index);
    ImportInteger(23, libData, data, pos, mesg->type);
    ImportInteger(24, libData, data, pos, mesg->favorite);
    ImportFinish(25, libData, data, pos);
}

static void import_segment_id(WolframLibraryData libData, MTensor data, int idx, const FIT_SEGMENT_ID_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SEGMENT_ID);
    ImportInteger(2, libData, data, pos, mesg->user_profile_primary_key);
    ImportInteger(3, libData, data, pos, mesg->device_id);
    ImportString(4, libData, data, pos, mesg->name, FIT_SEGMENT_ID_MESG_NAME_COUNT);
    ImportString(5, libData, data, pos, mesg->uuid, FIT_SEGMENT_ID_MESG_UUID_COUNT);
    ImportInteger(6, libData, data, pos, mesg->sport);
    ImportInteger(7, libData, data, pos, mesg->enabled);
    ImportInteger(8, libData, data, pos, mesg->default_race_leader);
    ImportInteger(9, libData, data, pos, mesg->delete_status);
    ImportInteger(10, libData, data, pos, mesg->selection_type);
    ImportFinish(11, libData, data, pos);
}

static void import_segment_leaderboard_entry(WolframLibraryData libData, MTensor data, int idx, const FIT_SEGMENT_LEADERBOARD_ENTRY_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SEGMENT_LEADERBOARD_ENTRY);
    ImportInteger(2, libData, data, pos, mesg->group_primary_key);
    ImportInteger(3, libData, data, pos, mesg->activity_id);
    ImportInteger(4, libData, data, pos, mesg->segment_time);
    ImportInteger(5, libData, data, pos, mesg->message_index);
    ImportString(6, libData, data, pos, mesg->name, FIT_SEGMENT_LEADERBOARD_ENTRY_MESG_NAME_COUNT);
    ImportInteger(7, libData, data, pos, mesg->type);
    ImportFinish(8, libData, data, pos);
}

static void import_segment_point(WolframLibraryData libData, MTensor data, int idx, const FIT_SEGMENT_POINT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SEGMENT_POINT);
    ImportInteger(2, libData, data, pos, mesg->position_lat);
    ImportInteger(3, libData, data, pos, mesg->position_long);
    ImportInteger(4, libData, data, pos, mesg->distance);
    ImportIntegerSequence(5, libData, data, pos, mesg->leader_time, FIT_SEGMENT_POINT_MESG_LEADER_TIME_COUNT);
    ImportInteger(6, libData, data, pos, mesg->message_index);
    ImportInteger(7, libData, data, pos, mesg->altitude);
    ImportFinish(8, libData, data, pos);
}

static void import_segment_lap(WolframLibraryData libData, MTensor data, int idx, const FIT_SEGMENT_LAP_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SEGMENT_LAP);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->start_time));
    ImportInteger(4, libData, data, pos, mesg->start_position_lat);
    ImportInteger(5, libData, data, pos, mesg->start_position_long);
    ImportInteger(6, libData, data, pos, mesg->end_position_lat);
    ImportInteger(7, libData, data, pos, mesg->end_position_long);
    ImportInteger(8, libData, data, pos, mesg->total_elapsed_time);
    ImportInteger(9, libData, data, pos, mesg->total_timer_time);
    ImportInteger(10, libData, data, pos, mesg->total_distance);
    ImportInteger(11, libData, data, pos, mesg->total_cycles);
    ImportInteger(12, libData, data, pos, mesg->nec_lat);
    ImportInteger(13, libData, data, pos, mesg->nec_long);
    ImportInteger(14, libData, data, pos, mesg->swc_lat);
    ImportInteger(15, libData, data, pos, mesg->swc_long);
    ImportString(16, libData, data, pos, mesg->name, FIT_SEGMENT_LAP_MESG_NAME_COUNT);
    ImportInteger(32, libData, data, pos, mesg->total_work);
    ImportInteger(33, libData, data, pos, mesg->total_moving_time);
    ImportIntegerSequence(34, libData, data, pos, mesg->time_in_hr_zone, FIT_SEGMENT_LAP_MESG_TIME_IN_HR_ZONE_COUNT);
    ImportIntegerSequence(35, libData, data, pos, mesg->time_in_speed_zone, FIT_SEGMENT_LAP_MESG_TIME_IN_SPEED_ZONE_COUNT);
    ImportIntegerSequence(36, libData, data, pos, mesg->time_in_cadence_zone, FIT_SEGMENT_LAP_MESG_TIME_IN_CADENCE_ZONE_COUNT);
    ImportIntegerSequence(37, libData, data, pos, mesg->time_in_power_zone, FIT_SEGMENT_LAP_MESG_TIME_IN_POWER_ZONE_COUNT);
    ImportInteger(38, libData, data, pos, mesg->active_time);
    ImportInteger(39, libData, data, pos, mesg->message_index);
    ImportInteger(40, libData, data, pos, mesg->total_calories);
    ImportInteger(41, libData, data, pos, mesg->total_fat_calories);
    ImportInteger(42, libData, data, pos, mesg->avg_speed);
    ImportInteger(43, libData, data, pos, mesg->max_speed);
    ImportInteger(44, libData, data, pos, mesg->avg_power);
    ImportInteger(45, libData, data, pos, mesg->max_power);
    ImportInteger(46, libData, data, pos, mesg->total_ascent);
    ImportInteger(47, libData, data, pos, mesg->total_descent);
    ImportInteger(48, libData, data, pos, mesg->normalized_power);
    ImportInteger(49, libData, data, pos, mesg->left_right_balance);
    ImportInteger(50, libData, data, pos, mesg->avg_altitude);
    ImportInteger(51, libData, data, pos, mesg->max_altitude);
    ImportInteger(52, libData, data, pos, mesg->avg_grade);
    ImportInteger(53, libData, data, pos, mesg->avg_pos_grade);
    ImportInteger(54, libData, data, pos, mesg->avg_neg_grade);
    ImportInteger(55, libData, data, pos, mesg->max_pos_grade);
    ImportInteger(56, libData, data, pos, mesg->max_neg_grade);
    ImportInteger(57, libData, data, pos, mesg->avg_pos_vertical_speed);
    ImportInteger(58, libData, data, pos, mesg->avg_neg_vertical_speed);
    ImportInteger(59, libData, data, pos, mesg->max_pos_vertical_speed);
    ImportInteger(60, libData, data, pos, mesg->max_neg_vertical_speed);
    ImportInteger(61, libData, data, pos, mesg->repetition_num);
    ImportInteger(62, libData, data, pos, mesg->min_altitude);
    ImportInteger(63, libData, data, pos, mesg->wkt_step_index);
    ImportInteger(64, libData, data, pos, mesg->front_gear_shift_count);
    ImportInteger(65, libData, data, pos, mesg->rear_gear_shift_count);
    ImportInteger(66, libData, data, pos, mesg->event);
    ImportInteger(67, libData, data, pos, mesg->event_type);
    ImportInteger(68, libData, data, pos, mesg->avg_heart_rate);
    ImportInteger(69, libData, data, pos, mesg->max_heart_rate);
    ImportInteger(70, libData, data, pos, mesg->avg_cadence);
    ImportInteger(71, libData, data, pos, mesg->max_cadence);
    ImportInteger(72, libData, data, pos, mesg->sport);
    ImportInteger(73, libData, data, pos, mesg->event_group);
    ImportInteger(74, libData, data, pos, mesg->sub_sport);
    ImportInteger(75, libData, data, pos, mesg->gps_accuracy);
    ImportInteger(76, libData, data, pos, mesg->avg_temperature);
    ImportInteger(77, libData, data, pos, mesg->max_temperature);
    ImportInteger(78, libData, data, pos, mesg->min_heart_rate);
    ImportInteger(79, libData, data, pos, mesg->sport_event);
    ImportInteger(80, libData, data, pos, mesg->avg_left_torque_effectiveness);
    ImportInteger(81, libData, data, pos, mesg->avg_right_torque_effectiveness);
    ImportInteger(82, libData, data, pos, mesg->avg_left_pedal_smoothness);
    ImportInteger(83, libData, data, pos, mesg->avg_right_pedal_smoothness);
    ImportInteger(84, libData, data, pos, mesg->avg_combined_pedal_smoothness);
    ImportInteger(85, libData, data, pos, mesg->status);
    ImportString(86, libData, data, pos, mesg->uuid, FIT_SEGMENT_LAP_MESG_UUID_COUNT);
    ImportInteger(119, libData, data, pos, mesg->avg_fractional_cadence);
    ImportInteger(120, libData, data, pos, mesg->max_fractional_cadence);
    ImportInteger(121, libData, data, pos, mesg->total_fractional_cycles);
    ImportFinish(122, libData, data, pos);
}

static void import_segment_file(WolframLibraryData libData, MTensor data, int idx, const FIT_SEGMENT_FILE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SEGMENT_FILE);
    ImportInteger(2, libData, data, pos, mesg->user_profile_primary_key);
    ImportIntegerSequence(3, libData, data, pos, mesg->leader_group_primary_key, FIT_SEGMENT_FILE_MESG_LEADER_GROUP_PRIMARY_KEY_COUNT);
    ImportIntegerSequence(4, libData, data, pos, mesg->leader_activity_id, FIT_SEGMENT_FILE_MESG_LEADER_ACTIVITY_ID_COUNT);
    ImportInteger(5, libData, data, pos, mesg->message_index);
    ImportString(6, libData, data, pos, mesg->file_uuid, FIT_SEGMENT_FILE_MESG_FILE_UUID_COUNT);
    ImportInteger(7, libData, data, pos, mesg->enabled);
    ImportIntegerSequence(8, libData, data, pos, mesg->leader_type, FIT_SEGMENT_FILE_MESG_LEADER_TYPE_COUNT);
    ImportFinish(9, libData, data, pos);
}

static void import_workout(WolframLibraryData libData, MTensor data, int idx, const FIT_WORKOUT_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WORKOUT);
    ImportInteger(2, libData, data, pos, mesg->capabilities);
    ImportString(3, libData, data, pos, mesg->wkt_name, FIT_WORKOUT_MESG_WKT_NAME_COUNT);
    ImportInteger(103, libData, data, pos, mesg->num_valid_steps);
    ImportInteger(104, libData, data, pos, mesg->pool_length);
    ImportInteger(105, libData, data, pos, mesg->sport);
    ImportInteger(106, libData, data, pos, mesg->sub_sport);
    ImportInteger(107, libData, data, pos, mesg->pool_length_unit);
    ImportFinish(108, libData, data, pos);
}

static void import_workout_session(WolframLibraryData libData, MTensor data, int idx, const FIT_WORKOUT_SESSION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WORKOUT_SESSION);
    ImportInteger(2, libData, data, pos, mesg->message_index);
    ImportInteger(3, libData, data, pos, mesg->num_valid_steps);
    ImportInteger(4, libData, data, pos, mesg->first_step_index);
    ImportInteger(5, libData, data, pos, mesg->pool_length);
    ImportInteger(6, libData, data, pos, mesg->sport);
    ImportInteger(7, libData, data, pos, mesg->sub_sport);
    ImportInteger(8, libData, data, pos, mesg->pool_length_unit);
    ImportFinish(9, libData, data, pos);
}

static void import_workout_step(WolframLibraryData libData, MTensor data, int idx, const FIT_WORKOUT_STEP_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WORKOUT_STEP);
    ImportString(2, libData, data, pos, mesg->wkt_step_name, FIT_WORKOUT_STEP_MESG_WKT_STEP_NAME_COUNT);
    ImportInteger(50, libData, data, pos, mesg->duration_value);
    ImportInteger(51, libData, data, pos, mesg->target_value);
    ImportInteger(52, libData, data, pos, mesg->custom_target_value_low);
    ImportInteger(53, libData, data, pos, mesg->custom_target_value_high);
    ImportInteger(54, libData, data, pos, mesg->secondary_target_value);
    ImportInteger(55, libData, data, pos, mesg->secondary_custom_target_value_low);
    ImportInteger(56, libData, data, pos, mesg->secondary_custom_target_value_high);
    ImportInteger(57, libData, data, pos, mesg->message_index);
    ImportInteger(58, libData, data, pos, mesg->exercise_category);
    ImportInteger(59, libData, data, pos, mesg->duration_type);
    ImportInteger(60, libData, data, pos, mesg->target_type);
    ImportInteger(61, libData, data, pos, mesg->intensity);
    ImportString(62, libData, data, pos, mesg->notes, FIT_WORKOUT_STEP_MESG_NOTES_COUNT);
    ImportInteger(112, libData, data, pos, mesg->equipment);
    ImportInteger(113, libData, data, pos, mesg->secondary_target_type);
    ImportFinish(114, libData, data, pos);
}

static void import_exercise_title(WolframLibraryData libData, MTensor data, int idx, const FIT_EXERCISE_TITLE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_EXERCISE_TITLE);
    ImportString(2, libData, data, pos, mesg->wkt_step_name, FIT_EXERCISE_TITLE_MESG_WKT_STEP_NAME_COUNT);
    ImportInteger(50, libData, data, pos, mesg->message_index);
    ImportInteger(51, libData, data, pos, mesg->exercise_category);
    ImportInteger(52, libData, data, pos, mesg->exercise_name);
    ImportFinish(53, libData, data, pos);
}

static void import_schedule(WolframLibraryData libData, MTensor data, int idx, const FIT_SCHEDULE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_SCHEDULE);
    ImportInteger(2, libData, data, pos, mesg->serial_number);
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->time_created));
    ImportInteger(4, libData, data, pos, WLTimestamp(mesg->scheduled_time));
    ImportInteger(5, libData, data, pos, mesg->manufacturer);
    ImportInteger(6, libData, data, pos, mesg->product);
    ImportInteger(7, libData, data, pos, mesg->completed);
    ImportInteger(8, libData, data, pos, mesg->type);
    ImportFinish(9, libData, data, pos);
}

static void import_totals(WolframLibraryData libData, MTensor data, int idx, const FIT_TOTALS_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_TOTALS);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->timer_time);
    ImportInteger(4, libData, data, pos, mesg->distance);
    ImportInteger(5, libData, data, pos, mesg->calories);
    ImportInteger(6, libData, data, pos, mesg->elapsed_time);
    ImportInteger(7, libData, data, pos, mesg->active_time);
    ImportInteger(8, libData, data, pos, mesg->message_index);
    ImportInteger(9, libData, data, pos, mesg->sessions);
    ImportInteger(10, libData, data, pos, mesg->sport);
    ImportFinish(11, libData, data, pos);
}

static void import_weight_scale(WolframLibraryData libData, MTensor data, int idx, const FIT_WEIGHT_SCALE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_WEIGHT_SCALE);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->weight);
    ImportInteger(4, libData, data, pos, mesg->percent_fat);
    ImportInteger(5, libData, data, pos, mesg->percent_hydration);
    ImportInteger(6, libData, data, pos, mesg->visceral_fat_mass);
    ImportInteger(7, libData, data, pos, mesg->bone_mass);
    ImportInteger(8, libData, data, pos, mesg->muscle_mass);
    ImportInteger(9, libData, data, pos, mesg->basal_met);
    ImportInteger(10, libData, data, pos, mesg->active_met);
    ImportInteger(11, libData, data, pos, mesg->user_profile_index);
    ImportInteger(12, libData, data, pos, mesg->physique_rating);
    ImportInteger(13, libData, data, pos, mesg->metabolic_age);
    ImportInteger(14, libData, data, pos, mesg->visceral_fat_rating);
    ImportFinish(15, libData, data, pos);
}

static void import_blood_pressure(WolframLibraryData libData, MTensor data, int idx, const FIT_BLOOD_PRESSURE_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_BLOOD_PRESSURE);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->systolic_pressure);
    ImportInteger(4, libData, data, pos, mesg->diastolic_pressure);
    ImportInteger(5, libData, data, pos, mesg->mean_arterial_pressure);
    ImportInteger(6, libData, data, pos, mesg->map_3_sample_mean);
    ImportInteger(7, libData, data, pos, mesg->map_morning_values);
    ImportInteger(8, libData, data, pos, mesg->map_evening_values);
    ImportInteger(9, libData, data, pos, mesg->user_profile_index);
    ImportInteger(10, libData, data, pos, mesg->heart_rate);
    ImportInteger(11, libData, data, pos, mesg->heart_rate_type);
    ImportInteger(12, libData, data, pos, mesg->status);
    ImportFinish(13, libData, data, pos);
}

static void import_monitoring_info(WolframLibraryData libData, MTensor data, int idx, const FIT_MONITORING_INFO_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_MONITORING_INFO);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, WLTimestamp(mesg->local_timestamp));
    ImportFinish(4, libData, data, pos);
}

static void import_monitoring(WolframLibraryData libData, MTensor data, int idx, const FIT_MONITORING_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_MONITORING);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportInteger(3, libData, data, pos, mesg->distance);
    ImportInteger(4, libData, data, pos, mesg->cycles);
    ImportInteger(5, libData, data, pos, mesg->active_time);
    ImportInteger(6, libData, data, pos, WLTimestamp(mesg->local_timestamp));
    ImportInteger(7, libData, data, pos, mesg->calories);
    ImportInteger(8, libData, data, pos, mesg->distance_16);
    ImportInteger(9, libData, data, pos, mesg->cycles_16);
    ImportInteger(10, libData, data, pos, mesg->active_time_16);
    ImportInteger(11, libData, data, pos, mesg->device_index);
    ImportInteger(12, libData, data, pos, mesg->activity_type);
    ImportInteger(13, libData, data, pos, mesg->activity_subtype);
    ImportFinish(14, libData, data, pos);
}

static void import_hr(WolframLibraryData libData, MTensor data, int idx, const FIT_HR_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_HR);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportIntegerSequence(3, libData, data, pos, mesg->event_timestamp, FIT_HR_MESG_EVENT_TIMESTAMP_COUNT);
    ImportInteger(4, libData, data, pos, mesg->fractional_timestamp);
    ImportInteger(5, libData, data, pos, mesg->time256);
    ImportIntegerSequence(6, libData, data, pos, mesg->filtered_bpm, FIT_HR_MESG_FILTERED_BPM_COUNT);
    ImportIntegerSequence(7, libData, data, pos, mesg->event_timestamp_12, FIT_HR_MESG_EVENT_TIMESTAMP_12_COUNT);
    ImportFinish(8, libData, data, pos);
}

static void import_ant_rx(WolframLibraryData libData, MTensor data, int idx, const FIT_ANT_RX_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_ANT_RX);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportIntegerSequence(3, libData, data, pos, mesg->data, FIT_ANT_RX_MESG_DATA_COUNT);
    ImportInteger(11, libData, data, pos, mesg->fractional_timestamp);
    ImportInteger(12, libData, data, pos, mesg->mesg_id);
    ImportIntegerSequence(13, libData, data, pos, mesg->mesg_data, FIT_ANT_RX_MESG_MESG_DATA_COUNT);
    ImportInteger(22, libData, data, pos, mesg->channel_number);
    ImportFinish(23, libData, data, pos);
}

static void import_ant_tx(WolframLibraryData libData, MTensor data, int idx, const FIT_ANT_TX_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_ANT_TX);
    ImportInteger(2, libData, data, pos, WLTimestamp(mesg->timestamp));
    ImportIntegerSequence(3, libData, data, pos, mesg->data, FIT_ANT_TX_MESG_DATA_COUNT);
    ImportInteger(11, libData, data, pos, mesg->fractional_timestamp);
    ImportInteger(12, libData, data, pos, mesg->mesg_id);
    ImportIntegerSequence(13, libData, data, pos, mesg->mesg_data, FIT_ANT_TX_MESG_MESG_DATA_COUNT);
    ImportInteger(22, libData, data, pos, mesg->channel_number);
    ImportFinish(23, libData, data, pos);
}

static void import_exd_screen_configuration(WolframLibraryData libData, MTensor data, int idx, const FIT_EXD_SCREEN_CONFIGURATION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_EXD_SCREEN_CONFIGURATION);
    ImportInteger(2, libData, data, pos, mesg->screen_index);
    ImportInteger(3, libData, data, pos, mesg->field_count);
    ImportInteger(4, libData, data, pos, mesg->layout);
    ImportInteger(5, libData, data, pos, mesg->screen_enabled);
    ImportFinish(6, libData, data, pos);
}

static void import_exd_data_field_configuration(WolframLibraryData libData, MTensor data, int idx, const FIT_EXD_DATA_FIELD_CONFIGURATION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_EXD_DATA_FIELD_CONFIGURATION);
    ImportInteger(2, libData, data, pos, mesg->screen_index);
    ImportInteger(3, libData, data, pos, mesg->concept_field);
    ImportInteger(4, libData, data, pos, mesg->field_id);
    ImportInteger(5, libData, data, pos, mesg->concept_count);
    ImportInteger(6, libData, data, pos, mesg->display_type);
    ImportString(7, libData, data, pos, mesg->title, FIT_EXD_DATA_FIELD_CONFIGURATION_MESG_TITLE_COUNT);
    ImportFinish(8, libData, data, pos);
}

static void import_exd_data_concept_configuration(WolframLibraryData libData, MTensor data, int idx, const FIT_EXD_DATA_CONCEPT_CONFIGURATION_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_EXD_DATA_CONCEPT_CONFIGURATION);
    ImportInteger(2, libData, data, pos, mesg->screen_index);
    ImportInteger(3, libData, data, pos, mesg->concept_field);
    ImportInteger(4, libData, data, pos, mesg->field_id);
    ImportInteger(5, libData, data, pos, mesg->concept_index);
    ImportInteger(6, libData, data, pos, mesg->data_page);
    ImportInteger(7, libData, data, pos, mesg->concept_key);
    ImportInteger(8, libData, data, pos, mesg->scaling);
    ImportInteger(9, libData, data, pos, mesg->data_units);
    ImportInteger(10, libData, data, pos, mesg->qualifier);
    ImportInteger(11, libData, data, pos, mesg->descriptor);
    ImportInteger(12, libData, data, pos, mesg->is_signed);
    ImportFinish(13, libData, data, pos);
}

static void import_hrv(WolframLibraryData libData, MTensor data, int idx, const FIT_HRV_MESG *mesg)
{
    mint pos[2] = {idx, 0};
    ImportInteger(1, libData, data, pos, FIT_MESG_NUM_HRV);
    ImportIntegerSequence(2, libData, data, pos, mesg->time, FIT_HRV_MESG_TIME_COUNT);
    ImportFinish(3, libData, data, pos);
}
// --- END MESSAGE IMPORT FUNCTIONS ---