#include "stm32f10x.h"

#include "drivers/audio.h"
#include "drivers/instrument.h"
#include "drivers/tempoencoder.h"

#include "app.h"
#include "pattern.h"
#include "player.h"
#include "volume.h"

static uint16_t Buffer[2][Player_BufferSize] = { 0 };
static int CurrBuffer = 0;
static int NextBuffer = 1;

static const int16_t* Data[Player_NumTracks] = { 0 };
static int16_t        Size[Player_NumTracks] = { 0 };
static int16_t        Index[Player_NumTracks] = { 0 };
static int16_t        Queue[Player_NumTracks] = { 0 };

static int head = 0;
static int tail = 0;

static int Period = 4000;

static void UpdatePeriod(void) {
  Period = 480000 / TempoEncoder_TempoBPM();
}

static void Push(int16_t item) {
  Queue[tail] = item;
  if (++tail == Player_NumTracks)
    tail = 0;
}

static int16_t Pop(void) {
  int16_t item = Queue[head];
  if (++head == Player_NumTracks)
    head = 0;
  return item;
}

static void ResetBuffer(void) {
  int i;
  for (i = 0; i < Player_BufferSize; ++i)
    Buffer[0][i] = Buffer[1][i] = 0x8000;
}

static void InitQueue(void) {
  int i;
  for (i = 0; i < Player_NumTracks; ++i)
    Push(i);
}

static void InitIndex(void) {
  int i;
  for (i = 0; i < Player_NumTracks; ++i)
    Index[i] = -1;
}

static void ApplyVolume(int32_t* sample, int16_t volume) {
  *sample = *sample * volume >> 12;
}

static void ApplyClamp(int32_t* sample) {
  *sample = *sample > 32767 ? 32767 : *sample;
  *sample = *sample < -32768 ? -32768 : *sample;
}

static void ApplyBias(int32_t* sample) {
  *sample ^= 0xFFFF8000;
}

static int32_t ReadData(int i) {
  int32_t sample = *Data[i];
  int32_t volume = Volume_InstrumentVolume(Index[i]);
  ApplyVolume(&sample, volume);
  return sample;
}

static void EnqueueInstruments(void) {
  const uint16_t currPattern = App_CurrPattern();
  const uint16_t nextStep = App_NextStep();
  int oh = -2;
  int i, j, k;
  for (i = 0; i < Instrument_NumInstruments; ++i) {
    if (Pattern_Data(currPattern)[i] & nextStep) {
      if (i == 6 || i == 7)
        for (k = 0; k < Player_NumTracks; ++k)
          if (Index[k] == 6 && k != oh)
            Size[k] = 0;
      j = Pop();
      Data[j] = Instrument_Data(i);
      Size[j] = Instrument_Size(i);
      Index[j] = i;
      if (i == 6) oh = j;
    }
  }
}

static void WriteBuffer(uint16_t* buffer) {
  const int16_t masterVolume = Volume_MasterVolume();
  int i, j;
  for (i = 0; i < Period; ++i) {
    int32_t sample = 0;
    for (j = 0; j < Player_NumTracks; ++j) {
      if (Index[j] == -1) {
        continue;
      } else if (Size[j] == 0) {
        Push(j);
        Index[j] = -1;
      } else {
        sample += ReadData(j);
        ++Data[j];
        --Size[j];
      }
    }
    ApplyVolume(&sample, masterVolume);
    ApplyClamp(&sample);
    ApplyBias(&sample);
    *buffer++ = sample;
  }
}

static void RenderStep() {
  UpdatePeriod();
  EnqueueInstruments();
  WriteBuffer(Buffer[NextBuffer]);
}

extern void Player_Init(void) {
  static int init = 0;
  if (init) return;
  else init = 1;
  
  Audio_Init();

  ResetBuffer();
  InitQueue();
  InitIndex();
}

extern void Player_Start(void) {
  Audio_SetBuffer(Buffer[CurrBuffer], Period);
  RenderStep();
}

extern void Player_Stop(void) {
  Audio_SetBuffer(0, 0);
  ResetBuffer();
}

extern void DMA2_Channel3_IRQHandler(void) {
  if (DMA_GetITStatus(DMA2_IT_TC3)) {
    DMA_ClearITPendingBit(DMA2_IT_TC3);
    CurrBuffer ^= 0x1;
    NextBuffer ^= 0x1;
    Audio_SetBuffer(Buffer[CurrBuffer], Period);
    App_RotateCurrStep();
    RenderStep();
  }
}
