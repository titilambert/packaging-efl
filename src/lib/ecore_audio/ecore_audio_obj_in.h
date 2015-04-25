#ifndef ECORE_AUDIO_OBJ_IN_H
#define ECORE_AUDIO_OBJ_IN_H

#include <Eina.h>
#include <Eo.h>

#ifdef EAPI
#undef EAPI
#endif

#ifdef __GNUC__
#if __GNUC__ >= 4
#define EAPI __attribute__ ((visibility("default")))
#else
#define EAPI
#endif
#else
#define EAPI
#endif

/**
 * @file ecore_audio_obj_in.h
 * @brief Ecore_Audio Input Object
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup ecore_audio_obj_in - Ecore_Audio input object
 * @ingroup Ecore_Audio_Group
 * @{
 */
#include "ecore_audio_in.eo.h"
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif