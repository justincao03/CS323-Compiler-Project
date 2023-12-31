#include "msg_type.h"
#include "stddef.h"

const char *get_splc_msg_color_code(splc_msg_t type)
{
    const char *color_code = "\033[31m";
    if (SPLC_IS_MSG_WARNING(type))
    {
        color_code = "\033[1;95m";
        return color_code;
    }
    else if (SPLC_IS_MSG_ERROR(type))
    {
        color_code = "\033[1;91m";
        return color_code;
    }

    switch (type)
    {
    case SPLM_MACRO_ERROR:
        color_code = "\033[1;91m";
        break;
    case SPLM_MACRO_WARN:
        color_code = "\033[1;95m";
        break;
    case SPLM_NOTE:
        color_code = "\033[1;96m";
        break;
    case SPLM_DIAG:
        color_code = "\033[1;93m";
        break;
    default:
        color_code = "\033[1;96m";
        break;
    }
    return color_code;
}

const char *splc_get_msg_type_prefix(splc_msg_t type)
{
    const char *type_name = "undefined message";
    if (SPLC_IS_MSG_WARNING(type))
    {
        type_name = "warning";
        return type_name;
    }
    else if (type == SPLM_ERR_FATAL)
    {
        type_name = "fatal error";
        return type_name;
    }
    else if (SPLC_IS_MSG_ERROR(type))
    {
        type_name = "error";
        return type_name;
    }
    switch (type)
    {
    case SPLM_NOTE:
        type_name = "note";
        break;
    case SPLM_DIAG:
        type_name = "diagnostics";
        break;
    case SPLM_MACRO_ERROR:
        type_name = "error";
        break;
    case SPLM_MACRO_WARN:
        type_name = "warning";
        break;
    default:
        type_name = "fatal error";
        break;
    }
    return type_name;
}

const char *splc_get_msg_type_suffix(splc_msg_t type)
{
    char *type_suffix = NULL;
    switch (type)
    {
    case SPLM_ERR_FATAL:
        type_suffix = NULL;
        break;
    case SPLM_ERR_UNIV:
        type_suffix = NULL;
        break;
    case SPLM_ERR_SYN_A:
        type_suffix = "A";
        break;
    case SPLM_ERR_SYN_B:
        type_suffix = "B";
        break;

        case SPLM_ERR_SEM_1:
        type_suffix = "1";
        break;
    case SPLM_ERR_SEM_2:
        type_suffix = "2";
        break;
    case SPLM_ERR_SEM_3:
        type_suffix = "3";
        break;
    case SPLM_ERR_SEM_4:
        type_suffix = "4";
        break;
    case SPLM_ERR_SEM_5:
        type_suffix = "5";
        break;
    case SPLM_ERR_SEM_6:
        type_suffix = "6";
        break;
    case SPLM_ERR_SEM_7:
        type_suffix = "7";
        break;
    case SPLM_ERR_SEM_8:
        type_suffix = "8";
        break;
    case SPLM_ERR_SEM_9:
        type_suffix = "9";
        break;
    case SPLM_ERR_SEM_10:
        type_suffix = "10";
        break;
    case SPLM_ERR_SEM_11:
        type_suffix = "11";
        break;
    case SPLM_ERR_SEM_12:
        type_suffix = "12";
        break;
    case SPLM_ERR_SEM_13:
        type_suffix = "13";
        break;
    case SPLM_ERR_SEM_14:
        type_suffix = "14";
        break;
    case SPLM_ERR_SEM_15:
        type_suffix = "15";
        break;

    case SPLM_Wuniv:
        type_suffix = NULL;
        break;
    case SPLM_Woverflow:
        type_suffix = "-Woverflow";
        break;
    case SPLM_Wimplicit_int:
        type_suffix = "-Wimplicit-int";
        break;
    case SPLM_NOTE:
        type_suffix = NULL;
        break;
    case SPLM_DIAG:
        type_suffix = NULL;
        break;
    case SPLM_MACRO_ERROR:
        type_suffix = "-Wmacro-error";
        break;
    case SPLM_MACRO_WARN:
        type_suffix = "-Wmacro-warning";
        break;
    default:
        type_suffix = NULL;
        break;
    }
    return type_suffix;
}