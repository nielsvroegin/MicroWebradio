#include "stm32f4xx_conf.h"
#include "esp8266.h"
#include "Audio.h"
#include "mp3dec.h"
#include <string.h>

#define BUTTON		(GPIOA->IDR & GPIO_Pin_0)

CIRCBUF_DEF(mp3Buffer, 51200);

// Private variables
MP3FrameInfo mp3FrameInfo;
HMP3Decoder hMP3Decoder;

// Private function Prototypes
void GPIOInitialize(void);
static void AudioCallback(void *context,int buffer);
void waitForButtonPush(void);

int main(void)
{
	// Initialize GPIO
	GPIOInitialize();

	// Wait for user to push button
	waitForButtonPush();

	// Initialize ESP8266
	esp8266_init();

	// Init Mp3/Audio
	hMP3Decoder = MP3InitDecoder();
	InitializeAudio(Audio48000HzSettings);
	SetAudioVolume(0xCF);

	// Delay to be sure Esp8266 is online
	static int i = 0;
	for (i = 0; i < 10000000; ++i);

	// Check if ESP8266 is started
	while(!esp8266_isOnline());

	// Join AP
	esp8266_joinAp("UPC1248023", "CHNAJRDQ");

	// Set multiple connection mode
	esp8266_setMultipleConnectionMode(false);

	// Open connection
	esp8266_openConnection("icecast.omroep.nl", "80");

	// Perform GET request
	esp8266_sendData("GET /3fm-sb-mp3 HTTP/1.1\r\nHost: icecast.omroep.nl\r\n\r\n");


	// Wait for buffer filled
	while (circBufUsedSpace(&mp3Buffer) < 40000) {
		esp8266_readData(&mp3Buffer);
	}

	// Start playing audio
	PlayAudioWithCallback(AudioCallback, 0);

	while (1) {
		// Fill buffer with new data
		while (circBufUsedSpace(&mp3Buffer) < 48000) {
			esp8266_readData(&mp3Buffer);
		}
	}
}

void GPIOInitialize(void) {
	GPIO_InitTypeDef GPIOC_InitStruct;

	/*** GPIOA initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/*** GPIOC initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIOC_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIOC_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIOC_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIOC_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIOC_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIOC_InitStruct);
}

void waitForButtonPush(void) {
	for(;;) {
		/*
		 * Check if user button is pressed
		 */
		if (BUTTON) {
			break;
		}
	}
}

/*
 * Called by the audio driver when it is time to provide data to
 * one of the audio buffers (while the other buffer is sent to the
 * CODEC using DMA). One mp3 frame is decoded at a time and
 * provided to the audio driver.
 */
static void AudioCallback(void *context, int buffer) {
	#define MP3_DATA_SIZE	4096

	static int16_t audio_buffer0[4096];
	static int16_t audio_buffer1[4096];
	static char mp3_data[MP3_DATA_SIZE]; // TODO determine appropriate value

	int offset, err;
	static const char *read_ptr = mp3_data;
	static int bytes_left = 0;
	static unsigned char errorAttempts = 0;


	// Move remaining MP3 data to front of array
	if (read_ptr != mp3_data) {
		memmove(mp3_data, read_ptr, bytes_left);
		read_ptr = mp3_data;
	}

	// Fill up with data of circbuf
	while (bytes_left != MP3_DATA_SIZE) {
		char c;

		if (!circBufPop(&mp3Buffer, &c)) {
			// No more data in buffer
			break;
		}

		// Add char to MP3 data
		mp3_data[bytes_left] = c;

		bytes_left++;
	}


	int16_t *samples;

	if (buffer) {
		samples = audio_buffer0;
		GPIO_SetBits(GPIOC, GPIO_Pin_1);
	} else {
		samples = audio_buffer1;
		GPIO_ResetBits(GPIOC, GPIO_Pin_1);
	}

	offset = MP3FindSyncWord((unsigned char*)read_ptr, bytes_left);
	if(offset == -1) {
		printf("MP3 sync word not found");
		return;
	}

	bytes_left -= offset;
	read_ptr += offset;
	err = MP3Decode(hMP3Decoder, (unsigned char**)&read_ptr, &bytes_left, samples, 0);

	// Handle error
	if (err) {
		if(errorAttempts <= 3) {
			errorAttempts ++;

			// Move forward, to find next time a correct frame
			if (err == ERR_MP3_INVALID_FRAMEHEADER) {
				if(bytes_left > 50) {
					bytes_left -= 50;
					read_ptr += 50;
				}
			}

			AudioCallback(context, buffer);
			return;
		} else {
			printf("Error decoding MP3 frame, code: %i", err);
			return;
		}

	}

	// Reset error attempts
	errorAttempts = 0;

	// Get frame info and provide audio buffers
	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	ProvideAudioBuffer(samples, mp3FrameInfo.outputSamps);
}
