#include <windows.h>
#include "XAudio2_custom.h"
#include "memmem.h"

IXAudio2 *xAudio2;
IXAudio2MasteringVoice *xMasterVoice;
IXAudio2SourceVoice *xSourceVoice;

void Log(const char *msg)
{
  HANDLE stdout = GetStdHandle(STD_OUTPUT_HANDLE);
  WriteFile(stdout, msg, lstrlen(msg), (DWORD[]){0}, NULL);
}

void InitAudio()
{
  // Must call at least once.
  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // Create the main audio interface.
  if(FAILED(XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
  {
    return Log("Failed to create XAudio2 instance\n");
  }

  // Required to play any audio.
  if(FAILED(xAudio2->lpVtbl->CreateMasteringVoice(xAudio2, &xMasterVoice,
    XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL,
    AudioCategory_GameEffects)))
  {
    return Log("Failed to create XAudio2 mastering voice\n");
  }
}

void LoadTestAudio()
{
  // Load the adpcm wav file from resource.
  HANDLE resource = FindResource(NULL, "IDR_WAVE1", "WAV");
  if(resource == NULL) return Log("Resource not found\n");
  HGLOBAL loadedResource = LoadResource(NULL, resource);
  if(loadedResource == NULL) return Log("Could not load resource\n");
  LPVOID resourceData = LockResource(loadedResource);
  DWORD resourceSize = SizeofResource(NULL, resource);

  // Prepare the adpcm struct with special coefficients.
  // See https://msdn.microsoft.com/en-us/library/microsoft.directx_sdk.xaudio2.adpcmwaveformat(v=vs.85).aspx
  ADPCMWAVEFORMAT *adpcm = (ADPCMWAVEFORMAT*)HeapAlloc(
    GetProcessHeap(), 0, sizeof(ADPCMWAVEFORMAT) + sizeof(ADPCMCOEFSET) * 7);
  adpcm->wSamplesPerBlock = 512;
  adpcm->wNumCoef = 7;
  adpcm->aCoef[0].iCoef1 = 256;
  adpcm->aCoef[0].iCoef1 = 0;
  adpcm->aCoef[1].iCoef1 = 512;
  adpcm->aCoef[1].iCoef1 = -256;
  adpcm->aCoef[2].iCoef1 = 0;
  adpcm->aCoef[2].iCoef1 = 0;
  adpcm->aCoef[3].iCoef1 = 192;
  adpcm->aCoef[3].iCoef1 = 64;
  adpcm->aCoef[4].iCoef1 = 240;
  adpcm->aCoef[4].iCoef1 = 0;
  adpcm->aCoef[5].iCoef1 = 460;
  adpcm->aCoef[5].iCoef1 = -208;
  adpcm->aCoef[6].iCoef1 = 392;
  adpcm->aCoef[6].iCoef1 = -232;

  // Find the "fmt " chunk and read in the data to the above wave structure.
  // See https://docs.microsoft.com/en-us/windows/desktop/xaudio2/resource-interchange-file-format--riff-
  DWORD wfxSize = 0;
  PVOID fmt = memmem(resourceData, resourceSize, "fmt ", 4);
  if(fmt == NULL) return Log("fmt chunk not found\n");
  CopyMemory(&wfxSize, fmt + sizeof(DWORD), sizeof(DWORD));
  CopyMemory(&adpcm->wfx, fmt + sizeof(DWORD) * 2, wfxSize);

  // Next find the "data" chunk and read in all the raw audio.
  DWORD dataSize = 0;
  PVOID data = memmem(resourceData, resourceSize, "data", 4);
  if(data == NULL) return Log("data chunk not found\n");
  CopyMemory(&dataSize, data + sizeof(DWORD), sizeof(DWORD));
  BYTE *dataBuffer = HeapAlloc(GetProcessHeap(), 0, dataSize);
  CopyMemory(dataBuffer, data + sizeof(DWORD) * 2, dataSize);

  // Set the loaded wav data buffer to the xaudio2 sound buffer.
  XAUDIO2_BUFFER buffer = {0};
  buffer.AudioBytes = dataSize;
  buffer.pAudioData = dataBuffer;
  buffer.Flags = XAUDIO2_END_OF_STREAM;

  // Try to create a source buffer for playing the audio data.
  if(FAILED(xAudio2->lpVtbl->CreateSourceVoice(xAudio2, &xSourceVoice,
    (WAVEFORMATEX*)adpcm, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL)))
  {
    return Log("could not create source voice\n");
  }

  // Submit the buffered wav data to the source voice.
  if(FAILED(xSourceVoice->lpVtbl->SubmitSourceBuffer(xSourceVoice,
    &buffer, NULL)))
  {
    return Log("could not submit source buffer\n");
  }

  // Finally play the audio.
  if(FAILED(xSourceVoice->lpVtbl->Start(xSourceVoice, 0, XAUDIO2_COMMIT_NOW)))
  {
    return Log("could not start audio\n");
  }

  // Clean up the allocated heap for the audio data.
  HeapFree(GetProcessHeap(), 0, adpcm);
}

void CleanAudio()
{
  xSourceVoice->lpVtbl->DestroyVoice(xSourceVoice);
  xMasterVoice->lpVtbl->DestroyVoice(xMasterVoice);
  xAudio2->lpVtbl->Release(xAudio2);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
  InitAudio();
  LoadTestAudio();
  system("pause");
  CleanAudio();
  return 0;
}
