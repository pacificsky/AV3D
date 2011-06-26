#include "waveout.h"

WaveOut::WaveOut(AudioProvider* provider, int sampleRate, int nrChannels, int bitsPerSample)
{
    _provider = provider;

    WAVEFORMATEXTENSIBLE fmt;
	fmt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE ;
	fmt.Format.nChannels = nrChannels;
	fmt.Format.nSamplesPerSec = sampleRate;
	fmt.Format.wBitsPerSample = bitsPerSample;
	fmt.Format.nBlockAlign = (fmt.Format.wBitsPerSample >> 3) * fmt.Format.nChannels;
	fmt.Format.nAvgBytesPerSec = fmt.Format.nBlockAlign * fmt.Format.nSamplesPerSec;

    switch (nrChannels)
    {
        case 1:
        fmt.dwChannelMask = KSAUDIO_SPEAKER_MONO;
        break;

        case 2:
	    fmt.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        break;

        default:
        throw new WaveOutException("%d channel audio is not supported", nrChannels);
    }

	fmt.Samples.wValidBitsPerSample = fmt.Format.wBitsPerSample;
    switch (bitsPerSample)
    {
        case 32:
        fmt.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        break;

        default:
	    fmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        break;
    }
	
    MMRESULT error = waveOutOpen(&_waveout, WAVE_MAPPER, &fmt.Format, (DWORD_PTR) Callback, (DWORD_PTR) this, CALLBACK_FUNCTION);
    if (error != MMSYSERR_NOERROR) throw new WaveOutException("Could not open the audio device (0x%08X)", error);
}

void WaveOut::Start()
{
    for (int i=0; i<4; i++)
    {
        WAVEHDR* hdr = (WAVEHDR*) calloc(1, sizeof(WAVEHDR));
        waveOutPrepareHeader(_waveout, hdr, sizeof(WAVEHDR));
        waveOutWrite(_waveout, hdr, sizeof(WAVEHDR));
    }
}

void WaveOut::Stop()
{
}

void CALLBACK WaveOut::Callback(HWAVEOUT waveout, UINT msg, DWORD_PTR userData, DWORD_PTR p1, DWORD_PTR p2)
{
    switch (msg)
    {
        case WOM_DONE:
            WaveOut* instance = (WaveOut*) userData;
            WAVEHDR* hdr = (WAVEHDR*) p1;
            waveOutUnprepareHeader(waveout, hdr, sizeof(WAVEHDR));
            
            if (hdr->lpData) free(hdr->lpData);

            int length, bufferDone = 0;
            void* buffer;
            while (!bufferDone) bufferDone = instance->_provider->NextAudioBuffer(&buffer, &length);
            if (bufferDone == -1)
            {
                free(hdr);
            }
            else
            {
                hdr->dwBufferLength = length;
                hdr->lpData = (LPSTR) buffer;
                waveOutPrepareHeader(waveout, hdr, sizeof(WAVEHDR));
                waveOutWrite(waveout, hdr, sizeof(WAVEHDR));
            }
    }
}
