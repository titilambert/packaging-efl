/* This include has been added to support Eo in Ecore */
#include <Eo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup Ecore_Poller_Group
 *
 * @{
 */

#include "ecore_poller.eo.h"

/**
 * @}
 */

/**
 * @ingroup Ecore_Animator_Group
 *
 * @{
 */

#include "ecore_animator.eo.h"

/**
 * @}
 */

/**
 * @ingroup Ecore_Timer_Group
 *
 * @{
 */

#include "ecore_timer.eo.h"

/**
 * @}
 */

/**
 * @ingroup Ecore_Idle_Group
 *
 * @{
 */

#include "ecore_idler.eo.h"
#include "ecore_idle_exiter.eo.h"
#include "ecore_idle_enterer.eo.h"

/**
 * @}
 */

/**
 * @ingroup Ecore_Exe_Group
 *
 * @{
 */

#include "ecore_exe.eo.h"

/**
 * @}
 */


/**
 * @ingroup Ecore_Job_Group
 *
 * @{
 */

#include "ecore_job.eo.h"

/**
 * @}
 */

/**
 * @ingroup Ecore_MainLoop_Group
 *
 * @{
 */

#include "ecore_mainloop.eo.h"

/* We ue the factory pattern here, so you shouldn't call eo_add directly. */
EAPI Eo *ecore_main_loop_get(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
