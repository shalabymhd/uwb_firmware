#include "cir.h"
#include <math.h>

#define READ_SIZE 100
#define NUM_CIR_POINTS 1016
#define ACCUM_DATA_LEN (NUM_CIR_POINTS)
static uint32 accum_data[ACCUM_DATA_LEN];

#define NUM_SYMBOLS 50
char buf[NUM_SYMBOLS*6 + NUM_SYMBOLS + 10], *pos = buf;

int read_cir(){
	for(int i = 0;i<NUM_CIR_POINTS;i++)
	{
		uint8 *cir; cir = (uint8 *)malloc(5*sizeof(uint8));
		dwt_readaccdata(cir, 5, 0 + 4*i);
		int16 real;
		int16 imag;
		real =  (int16)cir[2] << 8 | (int16)cir[1];
		imag =  (int16)cir[4] << 8 | (int16)cir[3];
		// accum_data[i] = (uint32)sqrt(pow(real,2) + pow(imag,2));
		accum_data[i] = (uint32)(fmax(abs(real),abs(imag)) + fmin(abs(real),abs(imag))/4);
		free(cir);
	}
	osDelay(1);
	output_cir();
	return 1;
}

int output_cir(){
	char response[30];
	char* ptr = buf;

	for (int lv0=0; lv0<NUM_CIR_POINTS; lv0+=NUM_SYMBOLS){
		ptr = pos;
		for (int lv1=lv0; lv1<lv0+NUM_SYMBOLS && lv1<NUM_CIR_POINTS; ++lv1){
			if (lv1 == 0) {
				sprintf(response, "%s", "R10");
				strcpy(ptr, response);
				ptr += strlen(response);
			}

			sprintf(response, "|%lu", accum_data[lv1]);
			strcpy(ptr, response);
			ptr += strlen(response);
		}

		osDelay(1);

		usb_print(buf);

		osDelay(1);
	}

	osDelay(1);

	usb_print("\r");

	return 1;
}