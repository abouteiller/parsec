#ifndef PINS_PAPI_EXEC_H
#define PINS_PAPI_EXEC_H

#include "dague_config.h"
#include "dague/mca/mca.h"
#include "dague/mca/pins/pins.h"
#include "dague.h"

#define NUM_EXEC_EVENTS 4
#define PAPI_EXEC_NATIVE_EVENT_NAMES {PAPI_L1_DCM, PAPI_L2_DCH, PAPI_L2_DCM, PAPI_L2_DCA}

typedef struct papi_exec_info_s {
	int kernel_type;
	long long values[NUM_EXEC_EVENTS];
} papi_exec_info_t;

BEGIN_C_DECLS

/**
 * Globally exported variable
 */
DAGUE_DECLSPEC extern const dague_pins_base_component_t dague_pins_papi_exec_component;
DAGUE_DECLSPEC extern const dague_pins_module_t dague_pins_papi_exec_module;
/* static accessor */
mca_base_component_t * pins_papi_exec_static_component(void);

END_C_DECLS

#endif // PINS_PAPI_EXEC_H