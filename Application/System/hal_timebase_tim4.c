#include "main.h"
/* HAL millisecond tick remains available before and after scheduler start. */
HAL_StatusTypeDef HAL_InitTick(uint32_t priority) {
    RCC_ClkInitTypeDef clock;uint32_t latency,hz;
    (void)priority;__HAL_RCC_TIM4_CLK_ENABLE();
    HAL_RCC_GetClockConfig(&clock,&latency);hz=HAL_RCC_GetPCLK1Freq();if(clock.APB1CLKDivider!=RCC_HCLK_DIV1)hz*=2;
    if(hz<1000000)return HAL_ERROR;
    TIM4->CR1=0;TIM4->PSC=hz/1000000-1;TIM4->ARR=999;TIM4->EGR=TIM_EGR_UG;TIM4->SR=0;TIM4->DIER=TIM_DIER_UIE;
    HAL_NVIC_SetPriority(TIM4_IRQn,4,0);HAL_NVIC_EnableIRQ(TIM4_IRQn);TIM4->CR1=TIM_CR1_CEN;return HAL_OK;
}
void HAL_SuspendTick(void) {TIM4->DIER&=~TIM_DIER_UIE;}
void HAL_ResumeTick(void) {TIM4->DIER|=TIM_DIER_UIE;}
void TIM4_IRQHandler(void) {if(TIM4->SR&TIM_SR_UIF) {TIM4->SR&=~TIM_SR_UIF;HAL_IncTick();}}
