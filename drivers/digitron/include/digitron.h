#ifndef _DIGITRON_H_
#define _DIGITRON_H_


int digitron_init(uint8_t *digitron_io_list, uint8_t digitron_io_num);
int digitron_show(uint32_t data);
void digitron_show_off();

#endif
