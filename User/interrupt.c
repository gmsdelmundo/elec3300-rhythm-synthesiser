#include "interrupt.h"

#include "lcd.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"

#include "pattern.h"

////////////////////////////////////////////////////////////////////////////////
// Pattern interrupt
////////////////////////////////////////////////////////////////////////////////

void TIM3_IRQHandler(void) {	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update)) {
    readPatternButtons();

	/* Clear the pending bit */
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}