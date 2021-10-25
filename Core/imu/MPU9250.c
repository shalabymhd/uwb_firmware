/**
 * @file   MPU-9250.c
 * @brief  9-axis sensor(InvenSense MPU-9250) driver for TZ10xx.
 *
 * Adapted from: 
 * 1) https://github.com/kriswiner/MPU9250/blob/master/STM32F401/
 * 	@author Cerevo Inc.
 * 2) https://gitlab.com/nitraced/uwb_robots
 * 	@author Justin Cano
 * 
 * TODO: 1) Mag is offline.
 * 		 2) Fix notation inconsistency; Comment; Clean up. 
 * 
 */

#include "math.h"
#include "MPU9250.h"
#include <stdint.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
#include "cmsis_os.h"

#include "i2c.h"
#include "usb.h"

// See also MPU-9250 Register Map and Descriptions, Revision 4.0, RM-MPU-9250A-00, Rev. 1.4, 9/9/2013 for registers not listed in
// above document; the MPU9250 and MPU9150 are virtually identical but the latter has a different register map
//


static uint8_t Ascale = AFS_2G;     // AFS_2G, AFS_4G, AFS_8G, AFS_16G
static uint8_t Gscale = GFS_250DPS; // GFS_250DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS
static uint8_t Mscale = MFS_16BITS; // MFS_14BITS or MFS_16BITS, 14-bit or 16-bit magnetometer resolution
static float aRes, gRes, mRes;      // scale resolutions per LSB for the sensors

static int16_t rawAcc[3];  // Stores the 16-bit signed accelerometer sensor output
static int16_t rawGyr[3];  // Stores the 16-bit signed gyro sensor output
static int16_t rawMag[3];  // Stores the 16-bit signed magnetometer sensor output

static imudata imuvals;

static float scaleAcc, scaleGyr, scaleMag; // Scale of the three

/* Private functions (adaptation to C from cpp) */
// Adaptation of wait with FREERTOS
static void wait_adapted(float time){
	time *=1000.0;
	osDelay((uint32_t) time);
}

// Get all the scales
static void getScales(){
	scaleMag = getMres();
	scaleAcc = getAres();
	scaleGyr = getGres();
}

void imu_main(){
	initializeImu();
	uint32_t diff_us;
	uint32_t previous_ticks[1]={0};

	char imuMsg[100];
	char tmpMsg[30];

	while(1){
		updateValues();
		convertValues();
		diff_us = getInterval(previous_ticks);
		imuvals.dt = (float) diff_us; // cast values in microseconds
		imuvals.dt/=1.0e3; // convert in milliseconds

		if (imuvals.acc.x != 0) {
			convertElementR3ToString(tmpMsg, imuvals.acc);
			sprintf(imuMsg,"Acc: %s",tmpMsg);
			convertElementR3ToString(tmpMsg, imuvals.gyr);
			sprintf(imuMsg,"%s; Gyr: %s",imuMsg, tmpMsg);
			convertElementR3ToString(tmpMsg, imuvals.mag);
			sprintf(imuMsg,"%s; Mag: %s",imuMsg, tmpMsg);
			convertFloatToString(tmpMsg, imuvals.dt);
			sprintf(imuMsg,"%s; dt: %s \n",imuMsg, tmpMsg);

			usb_print(imuMsg);
		}
		else if (imuvals.acc.x == 0) {
			usb_print("IMU not properly initialized \n");
		}

		osDelay(1);
	}
}

uint8_t get_imu_id(){
	uint8_t id = readByte(MPU9250_ADDRESS,WHO_AM_I_MPU9250);
	return id;
}

void initializeImu(){
	char imu_id = get_imu_id();
	usb_print(imu_id);

	if(imu_id==0x71){
		// The MPU is online. Initialize.
		usb_print("MPU online: starting initialization! \n");
		
		getScales();

		resetMPU9250();
		osDelay(2000);
		
		initMPU9250(); // Initialization of the main device
		osDelay(2000); // Delay to stabilize the device
	}
	else{
		// The MPU is offline. Return error.
		usb_print("MPU offline: ID test failed! \n");
	}
}

// Update all the values
void updateValues(){
	readAccelData(rawAcc);
	readGyroData(rawGyr);
	readMagData(rawMag);
}

void convertValues(){
	// Conversion of the accelerometer values
	imuvals.acc.x = (float)rawAcc[0]*scaleAcc;
	imuvals.acc.y = (float)rawAcc[1]*scaleAcc;
	imuvals.acc.z = (float)rawAcc[2]*scaleAcc;
	// Conversion of the gyroscope values
	imuvals.gyr.x = (float)rawGyr[0]*scaleGyr;
	imuvals.gyr.y = (float)rawGyr[1]*scaleGyr;
	imuvals.gyr.z = (float)rawGyr[2]*scaleGyr;
	// Conversion of the magn values
	imuvals.mag.x = (float)rawMag[0]*scaleMag;
	imuvals.mag.y = (float)rawMag[1]*scaleMag;
	imuvals.mag.z = (float)rawMag[2]*scaleMag;
}

void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
	uint8_t buff[1]={data};
	i2c_write((uint16_t)address,(uint16_t)subAddress,1,buff);
}

char readByte(uint8_t address, uint8_t subAddress)
{
	uint8_t buff[1];
	i2c_read((uint16_t) address, (uint16_t) subAddress,1, buff);
	return buff[0];
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{
	i2c_read((uint16_t) address, (uint16_t) subAddress,(uint16_t) count,dest);
}


float getMres() {
	switch (Mscale)
	{
	// Possible magnetometer scales (and their register bit settings) are:
	// 14 bit resolution (0) and 16 bit resolution (1)
	case MFS_14BITS:
		mRes = 10.0*4912.0/8190.0; // Proper scale to return milliGauss
		break;
	case MFS_16BITS:
		mRes = 10.0*4912.0/32760.0; // Proper scale to return milliGauss
		break;
	}
	return mRes;
}


float getGres() {
	switch (Gscale)
	{
	// Possible gyro scales (and their register bit settings) are:
	// 250 DPS (00), 500 DPS (01), 1000 DPS (10), and 2000 DPS  (11).
	// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
	case GFS_250DPS:
		gRes = 250.0/32768.0;
		break;
	case GFS_500DPS:
		gRes = 500.0/32768.0;
		break;
	case GFS_1000DPS:
		gRes = 1000.0/32768.0;
		break;
	case GFS_2000DPS:
		gRes = 2000.0/32768.0;
		break;
	}
	return gRes;
}


float getAres() {
	switch (Ascale)
	{
	// Possible accelerometer scales (and their register bit settings) are:
	// 2 Gs (00), 4 Gs (01), 8 Gs (10), and 16 Gs  (11).
	// Here's a bit of an algorith to calculate DPS/(ADC tick) based on that 2-bit value:
	case AFS_2G:
		aRes = 2.0/32768.0;
		break;
	case AFS_4G:
		aRes = 4.0/32768.0;
		break;
	case AFS_8G:
		aRes = 8.0/32768.0;
		break;
	case AFS_16G:
		aRes = 16.0/32768.0;
		break;
	}
	return aRes;
}


void readAccelData(int16_t * destination)
{
	uint8_t rawData[6];  // x/y/z accel register data stored here
	readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
	destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
	destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
	destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
}

void readGyroData(int16_t * destination)
{
	uint8_t rawData[6];  // x/y/z gyro register data stored here
	readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
	destination[0] = (int16_t)(((int16_t)rawData[0] << 8) | rawData[1]) ;  // Turn the MSB and LSB into a signed 16-bit value
	destination[1] = (int16_t)(((int16_t)rawData[2] << 8) | rawData[3]) ;
	destination[2] = (int16_t)(((int16_t)rawData[4] << 8) | rawData[5]) ;
}

void readMagData(int16_t * destination)
{
	uint8_t rawData[7];  // x/y/z gyro register data, ST2 register stored here, must read ST2 at end of data acquisition
	if(readByte(AK8963_ADDRESS, AK8963_ST1) & 0x01) { // wait for magnetometer data ready bit to be set
		readBytes(AK8963_ADDRESS, AK8963_XOUT_L, 7, &rawData[0]);  // Read the six raw data and ST2 registers sequentially into data array
		uint8_t c = rawData[6]; // End data read by reading ST2 register
		if(!(c & 0x08)) { // Check if magnetic sensor overflow set, if not then report data
			destination[0] = (int16_t)(((int16_t)rawData[1] << 8) | rawData[0]);  // Turn the MSB and LSB into a signed 16-bit value
			destination[1] = (int16_t)(((int16_t)rawData[3] << 8) | rawData[2]) ;  // Data stored as little Endian
			destination[2] = (int16_t)(((int16_t)rawData[5] << 8) | rawData[4]) ;
		}
	}
}

int16_t readTempData()
{
	uint8_t rawData[2];  // x/y/z gyro register data stored here
	readBytes(MPU9250_ADDRESS, TEMP_OUT_H, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array
	return (int16_t)(((int16_t)rawData[0]) << 8 | rawData[1]) ;  // Turn the MSB and LSB into a 16-bit value
}


void resetMPU9250() {
	// reset device
	writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
	wait_adapted(0.1);
}

void initMPU9250() {
	// Initialize MPU9250 device
	// wake up device
	writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors
	wait_adapted(0.1); // Delay 100 ms for PLL to get established on x-axis gyro; should check for PLL ready interrupt

	// get stable time source
	writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x01);  // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001

	// Configure Gyro and Accelerometer
	// Disable FSYNC and set accelerometer and gyro bandwidth to 44 and 42 Hz, respectively;
	// DLPF_CFG = bits 2:0 = 010; this sets the sample rate at 1 kHz for both
	// Maximum delay is 4.9 ms which is just over a 200 Hz maximum rate
	writeByte(MPU9250_ADDRESS, CONFIG, 0x03);

	// Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
	writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; the same rate set in CONFIG above

	// Set gyroscope full scale range
	// Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
	uint8_t c = readByte(MPU9250_ADDRESS, GYRO_CONFIG); // get current GYRO_CONFIG register value
	// c = c & ~0xE0; // Clear self-test bits [7:5]
	c = c & ~0x02; // Clear Fchoice bits [1:0]
	c = c & ~0x18; // Clear AFS bits [4:3]
	c = c | Gscale << 3; // Set full scale range for the gyro
	// c =| 0x00; // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG
	writeByte(MPU9250_ADDRESS, GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register

	// Set accelerometer full-scale range configuration
	c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
	// c = c & ~0xE0; // Clear self-test bits [7:5]
	c = c & ~0x18;  // Clear AFS bits [4:3]
	c = c | Ascale << 3; // Set full scale range for the accelerometer
	writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

	// Set accelerometer sample rate configuration
	// It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
	// accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
	c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
	c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
	c = c | 0x03;  // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz
	writeByte(MPU9250_ADDRESS, ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value

	// The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
	// but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

	// Configure Interrupts and Bypass Enable
	// Set interrupt pin active high, push-pull, and clear on read of INT_STATUS, enable I2C_BYPASS_EN so additional chips
	// can join the I2C bus and all can be controlled by the Arduino as master
	writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x22);
	writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
}

