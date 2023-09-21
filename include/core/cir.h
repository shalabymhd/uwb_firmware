#include "common.h"
#include "deca_regs.h"
#include "deca_device_api.h"

int read_cir(uint8_t initiator_id, uint8_t target_id);
void output_cir(uint8_t initiator_id, uint8_t target_id, uint16_t first_path_idx);