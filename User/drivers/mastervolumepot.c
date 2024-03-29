#include "stm32f10x.h"

#include "mastervolumepot.h"
#include "poll.h"

static int16_t State;

static void OnPoll(void) {
  ADC_SoftwareStartConvCmd(MasterVolumePot_ADC, ENABLE);
  while (ADC_GetSoftwareStartConvStatus(MasterVolumePot_ADC));
  State = ADC_GetConversionValue(MasterVolumePot_ADC);
  if (State < MasterVolumePot_MinValue) State = 0;
}

static void InitGPIO(void) {
  GPIO_InitTypeDef GPIO_InitStruct;
  
  RCC_APB2PeriphClockCmd(MasterVolumePot_CLK, ENABLE);
  
  GPIO_InitStruct.GPIO_Pin = MasterVolumePot_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(MasterVolumePot_PORT, &GPIO_InitStruct);
}

static void InitADC(void) {
  ADC_InitTypeDef ADC_InitStruct;
  
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
  RCC_APB2PeriphClockCmd(MasterVolumePot_ADC_CLK, ENABLE);
  
  ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_NbrOfChannel = 1;
	ADC_Init(MasterVolumePot_ADC, &ADC_InitStruct);
	
	ADC_RegularChannelConfig(MasterVolumePot_ADC, MasterVolumePot_ADC_Channel, 1, ADC_SampleTime_55Cycles5);
	ADC_Cmd(MasterVolumePot_ADC, ENABLE);
  
	ADC_ResetCalibration(MasterVolumePot_ADC);
	while (ADC_GetResetCalibrationStatus(MasterVolumePot_ADC));
	ADC_StartCalibration(MasterVolumePot_ADC);
	while (ADC_GetCalibrationStatus(MasterVolumePot_ADC));
}

extern void MasterVolumePot_Init(void) {
  static int init = 0;
  if (init) return;
  else init = 1;

  Poll_Init();

  State = MasterVolumePot_MaxValue;

  InitGPIO();
  InitADC();

  Poll_AddHandler(OnPoll);
}

extern int16_t MasterVolumePot_Read(void) {
  return State;
}
