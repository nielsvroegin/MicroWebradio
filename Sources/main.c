#include "stm32f4xx_conf.h"
#include "esp8266.h"
#include "Audio.h"
#include "mp3dec.h"

#define MP3_SIZE	687348

// Private variables
MP3FrameInfo mp3FrameInfo;
HMP3Decoder hMP3Decoder;

// Private function Prototypes
void GPIOInitialize(void);
static void AudioCallback(void *context,int buffer);

// External variables
extern const char mp3_data[];

int main(void)
{
	// Initialize GPIO
	GPIOInitialize();

	// Initialize ESP8266
	esp8266_init();


	// Play mp3
	hMP3Decoder = MP3InitDecoder();
	InitializeAudio(Audio44100HzSettings);
	SetAudioVolume(0xCF);
	PlayAudioWithCallback(AudioCallback, 0);

	while(1);


	// Delay to be sure Esp8266 is online
	/*static int i = 0;
	for (i = 0; i < 10000000; ++i);

	// Check if ESP8266 is started
	while(!esp8266_isOnline());

	// Join AP
	esp8266_joinAp("UPC1248023", "CHNAJRDQ");

	// Set multiple connection mode
	esp8266_setMultipleConnectionMode(false);

	// Open connection
	esp8266_openConnection("startsmart.nl", "80");

	// Perform GET request
	esp8266_sendData("GET / HTTP/1.1\r\nHost: startsmart.nl\r\n\r\n");

	// Enable led
	GPIO_ToggleBits(GPIOC, GPIO_Pin_1);

	while (1) {
		char data[64];
		esp8266_readData(data);
	}*/
}

void GPIOInitialize(void) {
	GPIO_InitTypeDef GPIOC_InitStruct;

	/*** GPIOC initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIOC_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIOC_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIOC_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIOC_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIOC_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIOC_InitStruct);
}

/*
 * Called by the audio driver when it is time to provide data to
 * one of the audio buffers (while the other buffer is sent to the
 * CODEC using DMA). One mp3 frame is decoded at a time and
 * provided to the audio driver.
 */
static void AudioCallback(void *context, int buffer) {
	static int16_t audio_buffer0[4096];
	static int16_t audio_buffer1[4096];

	int offset, err;
	int outOfData = 0;
	static const char *read_ptr = mp3_data;
	static int bytes_left = MP3_SIZE;

	int16_t *samples;

	if (buffer) {
		samples = audio_buffer0;
		GPIO_SetBits(GPIOD, GPIO_Pin_13);
		GPIO_ResetBits(GPIOD, GPIO_Pin_14);
	} else {
		samples = audio_buffer1;
		GPIO_SetBits(GPIOD, GPIO_Pin_14);
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
	}

	offset = MP3FindSyncWord((unsigned char*)read_ptr, bytes_left);
	bytes_left -= offset;

	if (bytes_left <= 10000) {
		read_ptr = mp3_data;
		bytes_left = MP3_SIZE;
		offset = MP3FindSyncWord((unsigned char*)read_ptr, bytes_left);
	}

	read_ptr += offset;
	err = MP3Decode(hMP3Decoder, (unsigned char**)&read_ptr, &bytes_left, samples, 0);

	if (err) {
		/* error occurred */
		switch (err) {
		case ERR_MP3_INDATA_UNDERFLOW:
			outOfData = 1;
			break;
		case ERR_MP3_MAINDATA_UNDERFLOW:
			/* do nothing - next call to decode will provide more mainData */
			break;
		case ERR_MP3_FREE_BITRATE_SYNC:
		default:
			outOfData = 1;
			break;
		}
	} else {
		/* no error */
		MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	}

	if (!outOfData) {
		ProvideAudioBuffer(samples, mp3FrameInfo.outputSamps);
	}
}
