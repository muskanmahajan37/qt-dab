
#include	"xmlfile-reader.h"
#include	"xml-descriptor.h"
#include	"xmlfile-handler.h"
#include	<sys/time.h>
#include	<stdio.h>
#include	"rawfiles.h"

static	int shift (int a) {
int r	= 1;
	while (-- a > 0) {
	   r <<= 1;
	}
	return r;
}

static inline
std::complex<float> compmul (std::complex<float> a, float b) {
	return std::complex<float> (real (a) * b, imag (a) * b);
}

static inline
uint64_t	currentTime () {
struct timeval tv;

	gettimeofday (&tv, nullptr);
	return (uint64_t)(tv. tv_sec * 1000000 +
	                  (uint64_t)tv. tv_usec);
}

	xmlfileReader::xmlfileReader (xmlfileHandler *mr,
	                              FILE	*f,
	                              xmlDescriptor *fd,
	                              uint32_t	filePointer,
	                              RingBuffer<std::complex<float>> *b) {
	this	-> parent	= mr;
	this	-> file		= f;
	this	-> fd		= fd;
	this	-> filePointer	= filePointer;
	sampleBuffer		= b;
	convBufferSize		= fd -> sampleRate / 1000;

	for (int i = 0; i < 2048; i ++) {
	   float inVal = float (fd -> sampleRate / 1000);
	   mapTable_int [i] = (int)(floor (i * (inVal / 2048.0)));
	   mapTable_float [i] = i * (inVal / 2048.0) - mapTable_int [i];
	}

	convIndex	= 0;
	convBuffer. resize (convBufferSize + 1);
	fprintf (stderr, "nu hier naar toe\n");
	nrElements	= fd -> blockList [0]. nrElements;

	connect (this, SIGNAL (setProgress (int, int)),
	         parent, SLOT (setProgress (int, int)));

	fprintf (stderr, "reader task wordt gestart\n");
	start ();
}

	xmlfileReader::~xmlfileReader () {
	running. store (false);
	while (this -> isRunning ())
	   usleep (1000);
}

void	xmlfileReader::stopReader	() {
	if (!running. load ())
	   return;
	running. store (false);
	while (isRunning ())
	   usleep (1000);
}

static	int cycleCount = 0;
void	xmlfileReader::run () {
int	samplesRead	= 0;
int	blockSize	= 2048;
uint64_t	nextStop;

	fseek (file, filePointer, SEEK_SET);

	nextStop = currentTime ();
	running. store (true);
	for (int blocks = 0; blocks < fd -> nrBlocks; blocks ++) {
	   samplesToRead	= compute_nrSamples (file, blocks);
	   fprintf (stderr, "samples to read %d\n", samplesToRead);
	   samplesRead		= 0;
	   while ((samplesRead <= samplesToRead) && running. load ()) {
	      samplesRead += readSamples (file, blockSize);
	      if (++cycleCount >= 200) {
	         setProgress (samplesRead, samplesToRead);
	         cycleCount = 0;
	      }
	      nextStop	= nextStop + ((uint64_t)blockSize * 1000) / 2048;
	      if (nextStop > currentTime ())
	         usleep ( nextStop - currentTime ());
	   }
	}
}

int	xmlfileReader::compute_nrSamples (FILE *f, int blockNumber) {
int	nrElements	= fd -> blockList. at (blockNumber). nrElements;
int	samplesToRead	= 0;

	if (fd -> blockList. at (blockNumber). typeofUnit == "Channel") {
	   if ((fd -> iqOrder == "IQ") ||
	       (fd -> iqOrder == "QI"))
	      samplesToRead = nrElements / 2;
	   else
	      samplesToRead = nrElements;
	}
	else	// typeofUnit = "sample"
	   samplesToRead = nrElements;

	fprintf (stderr, "%d samples have to be read, order is %s\n",
	                 samplesToRead, fd -> iqOrder. toLatin1 (). data ());
	return samplesToRead;
}

int	xmlfileReader::readSamples (FILE *f, int amount) {
float I_element, Q_element;
int	sampleCount	= 0;

	while (sampleCount < amount) {
	   std::complex<float> sample;

	   if (fd -> iqOrder == "IQ") {
	      I_element = readElement (f);
	      Q_element = readElement (f);
	      sample = std::complex<float> (I_element, Q_element);
	   }
	   else
	   if (fd -> iqOrder == "QI") {
	      Q_element = readElement (f);
	      I_element = readElement (f);
	      sample = std::complex<float> (I_element, Q_element);
	   }
	   else
	   if (fd -> iqOrder == "I_Only") 
	      sample = std::complex<float> (readElement (f), 0);
	   else
	   if (fd -> iqOrder == "Q_Only") 
	      sample = std::complex<float> (0, readElement (f));

	   convBuffer [convIndex ++] = sample;
	   if (convIndex >= convBufferSize + 1) {
	      std::complex<float> temp [2048];
	      for (int i = 0; i < 2048; i ++) {
	         int16_t inpBase = mapTable_int [i];
	         float   inpRatio = mapTable_float [i];
	         temp [i] = compmul (convBuffer [inpBase + 1], inpRatio) +
	                    compmul (convBuffer [inpBase], 1 - inpRatio);
	      }
	      convBuffer [0] = convBuffer [convBufferSize];
	      convIndex	= 1;
	      sampleBuffer -> putDataIntoBuffer (temp, 2048);
	      return 2048;
	   }
	}
	return sampleCount;
}

static 
float mapTable [] = {
 -128 / 128.0 , -127 / 128.0 , -126 / 128.0 , -125 / 128.0 , -124 / 128.0 , -123 / 128.0 , -122 / 128.0 , -121 / 128.0 , -120 / 128.0 , -119 / 128.0 , -118 / 128.0 , -117 / 128.0 , -116 / 128.0 , -115 / 128.0 , -114 / 128.0 , -113 / 128.0 
, -112 / 128.0 , -111 / 128.0 , -110 / 128.0 , -109 / 128.0 , -108 / 128.0 , -107 / 128.0 , -106 / 128.0 , -105 / 128.0 , -104 / 128.0 , -103 / 128.0 , -102 / 128.0 , -101 / 128.0 , -100 / 128.0 , -99 / 128.0 , -98 / 128.0 , -97 / 128.0 
, -96 / 128.0 , -95 / 128.0 , -94 / 128.0 , -93 / 128.0 , -92 / 128.0 , -91 / 128.0 , -90 / 128.0 , -89 / 128.0 , -88 / 128.0 , -87 / 128.0 , -86 / 128.0 , -85 / 128.0 , -84 / 128.0 , -83 / 128.0 , -82 / 128.0 , -81 / 128.0 
, -80 / 128.0 , -79 / 128.0 , -78 / 128.0 , -77 / 128.0 , -76 / 128.0 , -75 / 128.0 , -74 / 128.0 , -73 / 128.0 , -72 / 128.0 , -71 / 128.0 , -70 / 128.0 , -69 / 128.0 , -68 / 128.0 , -67 / 128.0 , -66 / 128.0 , -65 / 128.0 
, -64 / 128.0 , -63 / 128.0 , -62 / 128.0 , -61 / 128.0 , -60 / 128.0 , -59 / 128.0 , -58 / 128.0 , -57 / 128.0 , -56 / 128.0 , -55 / 128.0 , -54 / 128.0 , -53 / 128.0 , -52 / 128.0 , -51 / 128.0 , -50 / 128.0 , -49 / 128.0 
, -48 / 128.0 , -47 / 128.0 , -46 / 128.0 , -45 / 128.0 , -44 / 128.0 , -43 / 128.0 , -42 / 128.0 , -41 / 128.0 , -40 / 128.0 , -39 / 128.0 , -38 / 128.0 , -37 / 128.0 , -36 / 128.0 , -35 / 128.0 , -34 / 128.0 , -33 / 128.0 
, -32 / 128.0 , -31 / 128.0 , -30 / 128.0 , -29 / 128.0 , -28 / 128.0 , -27 / 128.0 , -26 / 128.0 , -25 / 128.0 , -24 / 128.0 , -23 / 128.0 , -22 / 128.0 , -21 / 128.0 , -20 / 128.0 , -19 / 128.0 , -18 / 128.0 , -17 / 128.0 
, -16 / 128.0 , -15 / 128.0 , -14 / 128.0 , -13 / 128.0 , -12 / 128.0 , -11 / 128.0 , -10 / 128.0 , -9 / 128.0 , -8 / 128.0 , -7 / 128.0 , -6 / 128.0 , -5 / 128.0 , -4 / 128.0 , -3 / 128.0 , -2 / 128.0 , -1 / 128.0 
, 0 / 128.0 , 1 / 128.0 , 2 / 128.0 , 3 / 128.0 , 4 / 128.0 , 5 / 128.0 , 6 / 128.0 , 7 / 128.0 , 8 / 128.0 , 9 / 128.0 , 10 / 128.0 , 11 / 128.0 , 12 / 128.0 , 13 / 128.0 , 14 / 128.0 , 15 / 128.0 
, 16 / 128.0 , 17 / 128.0 , 18 / 128.0 , 19 / 128.0 , 20 / 128.0 , 21 / 128.0 , 22 / 128.0 , 23 / 128.0 , 24 / 128.0 , 25 / 128.0 , 26 / 128.0 , 27 / 128.0 , 28 / 128.0 , 29 / 128.0 , 30 / 128.0 , 31 / 128.0 
, 32 / 128.0 , 33 / 128.0 , 34 / 128.0 , 35 / 128.0 , 36 / 128.0 , 37 / 128.0 , 38 / 128.0 , 39 / 128.0 , 40 / 128.0 , 41 / 128.0 , 42 / 128.0 , 43 / 128.0 , 44 / 128.0 , 45 / 128.0 , 46 / 128.0 , 47 / 128.0 
, 48 / 128.0 , 49 / 128.0 , 50 / 128.0 , 51 / 128.0 , 52 / 128.0 , 53 / 128.0 , 54 / 128.0 , 55 / 128.0 , 56 / 128.0 , 57 / 128.0 , 58 / 128.0 , 59 / 128.0 , 60 / 128.0 , 61 / 128.0 , 62 / 128.0 , 63 / 128.0 
, 64 / 128.0 , 65 / 128.0 , 66 / 128.0 , 67 / 128.0 , 68 / 128.0 , 69 / 128.0 , 70 / 128.0 , 71 / 128.0 , 72 / 128.0 , 73 / 128.0 , 74 / 128.0 , 75 / 128.0 , 76 / 128.0 , 77 / 128.0 , 78 / 128.0 , 79 / 128.0 
, 80 / 128.0 , 81 / 128.0 , 82 / 128.0 , 83 / 128.0 , 84 / 128.0 , 85 / 128.0 , 86 / 128.0 , 87 / 128.0 , 88 / 128.0 , 89 / 128.0 , 90 / 128.0 , 91 / 128.0 , 92 / 128.0 , 93 / 128.0 , 94 / 128.0 , 95 / 128.0 
, 96 / 128.0 , 97 / 128.0 , 98 / 128.0 , 99 / 128.0 , 100 / 128.0 , 101 / 128.0 , 102 / 128.0 , 103 / 128.0 , 104 / 128.0 , 105 / 128.0 , 106 / 128.0 , 107 / 128.0 , 108 / 128.0 , 109 / 128.0 , 110 / 128.0 , 111 / 128.0 
, 112 / 128.0 , 113 / 128.0 , 114 / 128.0 , 115 / 128.0 , 116 / 128.0 , 117 / 128.0 , 118 / 128.0 , 119 / 128.0 , 120 / 128.0 , 121 / 128.0 , 122 / 128.0 , 123 / 128.0 , 124 / 128.0 , 125 / 128.0 , 126 / 128.0 , 127 / 128.0 };

float	xmlfileReader::readElement (FILE *theFile) {
uint8_t s1;
uint8_t bytes_16 [2];
uint8_t	bytes_24 [3];
uint8_t bytes_32 [4];
int	nrBits	= fd -> bitsperChannel;
float	scaler	= float (shift (nrBits));
int16_t temp_16;
uint32_t temp_32;

	if (fd -> container == "int8") {
	   fread (&s1, 1, 1, theFile);
	   return (float)s1 / 128.0;
	}

	if (fd -> container == "uint8") {
	   fread (&s1, 1, 1, theFile);
	   return mapTable [s1];
	}

	if (fd -> container == "int16") {
	   if (fd -> byteOrder == "MSB") {
	      fread (bytes_16, 2, 1, theFile);
	      temp_16 = (bytes_16 [0] << 8) | bytes_16 [1];
	      return ((float)temp_16) / scaler;
	   }
	   else {
	      fread (&temp_16, 2, 1, theFile);
	      return ((float)temp_16) / scaler;
	   }
	}

	if (fd -> container == "int24") {
	   fread (bytes_24, 3, 1, theFile);
	   if (fd -> byteOrder == "MSB")
	      temp_32 = (bytes_24 [0]<< 16) |
	                    (bytes_24 [1] << 8) | bytes_24 [2];
	   else
	      temp_32 = (bytes_24 [2]<< 16) |
	                   (bytes_24 [1] << 8) | bytes_24 [0];
	   if (temp_32 & 0x800000) 
	      temp_32 |= 0xFF000000;
	   return (float)temp_32 / scaler;
	}

	if (fd -> container == "int32") {
	   fread (bytes_32, sizeof (int32_t), 1, theFile);
	   if (fd -> byteOrder == "MSB")
	      temp_32 = (bytes_32 [0]<< 24) | (bytes_32 [1] << 16) |
	                   (bytes_32 [2] << 8) | bytes_32 [3];
	   else
	      temp_32 = (bytes_32 [3] << 24) | (bytes_32 [2] << 16) |
	                   (bytes_32 [1] << 8) | bytes_32 [0];
	   return (float)temp_32 / scaler;
	}

	if (fd -> container == "float32") {
	   fread (bytes_32, sizeof (float), 1, theFile);
	   if (fd -> byteOrder == "MSB")
	      temp_32 = (bytes_32 [0]<< 24) | (bytes_32 [1] << 16) |
	                   (bytes_32 [2] << 8) | bytes_32 [3];
	   else
	      temp_32 = (bytes_32 [3] << 24) | (bytes_32 [2] << 16) |
	                   (bytes_32 [1] << 8) | bytes_32 [0];
	   return *(float*)(&temp_32);
	}
	return 0;
}
