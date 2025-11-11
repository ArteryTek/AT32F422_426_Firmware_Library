/**
  **************************************************************************
  * @file     main.c
  * @brief    main program
  **************************************************************************
  *
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

#include "at32f422_426_board.h"
#include "at32f422_426_clock.h"
#include <stdio.h>


/** @addtogroup AT32F426_periph_examples
  * @{
  */

/** @addtogroup 426_ADC_internal_temperature_sensor ADC_internal_temperature_sensor
  * @{
  */


#define ADC_VREF                         (3.3)
#define ADC_TEMP_BASE                    (1.28)
#define ADC_TEMP_SLOPE                   (-0.00425)

__IO uint16_t adc1_ordinary_value = 0;
__IO uint32_t adc1_overflow_flag = 0;
__IO uint32_t adc1_conversion_fail_flag = 0;
__IO uint32_t error_times_index = 0;

/**
  * @brief  dma configuration.
  * @param  none
  * @retval none
  */
static void dma_config(void)
{
  dma_init_type dma_init_struct;
  crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
  dma_reset(DMA1_CHANNEL1);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.buffer_size = 1;
  dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
  dma_init_struct.memory_base_addr = (uint32_t)&adc1_ordinary_value;
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
  dma_init_struct.memory_inc_enable = FALSE;
  dma_init_struct.peripheral_base_addr = (uint32_t)&(ADC1->odt);
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.priority = DMA_PRIORITY_HIGH;
  dma_init_struct.loop_mode_enable = TRUE;
  dma_init(DMA1_CHANNEL1, &dma_init_struct);
}

/**
  * @brief  adc configuration.
  * @param  none
  * @retval none
  */
static void adc_config(void)
{
  adc_base_config_type adc_base_struct;
  crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);
  adc_reset(ADC1);
  nvic_irq_enable(ADC1_CMP_IRQn, 0, 0);

  /* config division,adcclk is division by hclk */
  crm_adc_clock_div_set(CRM_ADC_DIV_6);

  adc_base_default_para_init(&adc_base_struct);
  adc_base_struct.sequence_mode = FALSE;
  adc_base_struct.repeat_mode = FALSE;
  adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
  adc_base_struct.ordinary_channel_length = 1;
  adc_base_config(ADC1, &adc_base_struct);

  /* config ordinary channel */
  adc_ordinary_channel_set(ADC1, ADC_CHANNEL_16, 1, ADC_SAMPLETIME_239_5);

  /* config ordinary trigger source */
  adc_ordinary_conversion_trigger_set(ADC1, ADC_ORDINARY_TRIG_SOFTWARE, TRUE);

  /* config dma mode */
  adc_dma_mode_enable(ADC1, TRUE);

  /* internal temperature sensor and vintrv channel enable */
  adc_tempersensor_vintrv_enable(TRUE);

  /* adc enable */
  adc_enable(ADC1, TRUE);
  while(adc_flag_get(ADC1, ADC_RDY_FLAG) == RESET);

  /* adc calibration */
  adc_calibration_init(ADC1);
  while(adc_calibration_init_status_get(ADC1));
  adc_calibration_start(ADC1);
  while(adc_calibration_status_get(ADC1));
}

/**
  * @brief  this function handles adc handler.
  * @param  none
  * @retval none
  */
void ADC1_CMP_IRQHandler(void)
{
  if(adc_interrupt_flag_get(ADC1, ADC_TCF_FLAG) != RESET)
  {
    adc_flag_clear(ADC1, ADC_TCF_FLAG);
    adc1_conversion_fail_flag++;

    /* to avoid data wrong,it is recommended to add the following recovery code */
    adc_enable(ADC1, FALSE);
    dma_channel_enable(DMA1_CHANNEL1, FALSE);
    dma_flag_clear(DMA1_FDT1_FLAG);
    dma_data_number_set(DMA1_CHANNEL1, 1);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);
    adc_enable(ADC1, TRUE);
  }

	if(adc_interrupt_flag_get(ADC1, ADC_OCCO_FLAG) != RESET)
  {
    adc_flag_clear(ADC1, ADC_OCCO_FLAG);
    adc1_overflow_flag++;

    /* to avoid data wrong,it is recommended to add the following recovery code */
    adc_enable(ADC1, FALSE);
    dma_channel_enable(DMA1_CHANNEL1, FALSE);
    dma_flag_clear(DMA1_FDT1_FLAG);
    dma_data_number_set(DMA1_CHANNEL1, 1);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);
    adc_enable(ADC1, TRUE);
  }
}

/**
  * @brief  main function.
  * @param  none
  * @retval none
  */
int main(void)
{
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

  /* config the system clock */
  system_clock_config();

  /* init at start board */
  at32_board_init();
  at32_led_off(LED2);
  at32_led_off(LED3);
  at32_led_off(LED4);
  uart_print_init(115200);
  dma_config();
  adc_config();

  /* enable dma after adc activation */
  dma_channel_enable(DMA1_CHANNEL1, TRUE);

  printf("internal_temperature_sensor \r\n");
  while(1)
  {
    /* adc1 software trigger start conversion */
    adc_ordinary_software_trigger_enable(ADC1, TRUE);
    
    /* wait conversion end */
    while(dma_flag_get(DMA1_FDT1_FLAG) == RESET)
    {
    }
    dma_flag_clear(DMA1_FDT1_FLAG);
    printf("internal_temperature = %f deg C\r\n",(ADC_TEMP_BASE - (double)adc1_ordinary_value * ADC_VREF / 4095) / ADC_TEMP_SLOPE + 25);
    if(error_times_index != (adc1_overflow_flag + adc1_conversion_fail_flag))
    {
      /* printf flag when error occur */
      error_times_index = adc1_overflow_flag + adc1_conversion_fail_flag;
      at32_led_on(LED3);
      at32_led_on(LED4);
      printf("error occur\r\n");
      printf("error_times_index = %d\r\n",error_times_index);
      printf("adc1_overflow_flag = %d\r\n",adc1_overflow_flag);
      printf("adc1_conversion_fail_flag = %d\r\n",adc1_conversion_fail_flag);
    }
    at32_led_on(LED2);
    delay_sec(1);
  }
}


/**
  * @}
  */

/**
  * @}
  */

