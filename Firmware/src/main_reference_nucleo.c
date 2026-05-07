/**
  ******************************************************************************
  * @file           : main_reference_nucleo.c
  * @brief          : Código de referencia validado con NUCLEO-WL55JC2
  * @description    : Este archivo contiene el código base probado en la placa
  *                   NUCLEO para detección de flanco con reed switch y envío
  *                   de mensaje Sigfox. Sirve como referencia para la
  *                   implementación en el LSM110A.
  *
  * @attention       : NO es el código final. Es referencia del código validado
  *                   en el canal #stm32wl de Slack (Oct-Ene 2025-2026).
  ******************************************************************************
  *
  * FUNCIONALIDAD VALIDADA:
  * - Detección de flanco de subida en PB1 (reed switch)
  * - Envío de payload Sigfox ("Franco" = 0x46,0x72,0x61,0x6E,0x63,0x6F)
  * - Control de LEDs como indicadores de estado
  * - Anti-rebote por HAL_Delay (50ms en loop principal)
  * - Contador de aperturas
  *
  * CREDENCIALES SIGFOX (NUCLEO):
  * - SIGFOX_KEY:  51,AD,0E,BA,0E,92,E9,0B,7F,A6,78,F1,BF,23,3E,5D
  * - SIGFOX_ID:   03,3E,07,FC
  * - SIGFOX_PAC:  B0,6E,A4,9C,9E,0F,11,F4
  * - Backend ID:  54396924
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_sigfox.h"
#include "gpio.h"

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* ---- Función para enviar mensaje Sigfox ---- */
void EnviarMensajeFranco(void)
{
  // "Franco" en hexadecimal: 46 72 61 6E 63 6F
  uint8_t payload[] = {0x46, 0x72, 0x61, 0x6E, 0x63, 0x6F};
  uint8_t dl_msg[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint32_t nbTxRepeatFlag = 1;  // 3 repeticiones

  // Enviar el mensaje
  SIGFOX_API_send_frame(payload, sizeof(payload), dl_msg, nbTxRepeatFlag, SFX_FALSE);

  printf("Mensaje 'Franco' enviado por Sigfox!\r\n");
}

/* ---- Función para manejar el flanco de PB_1 (Reed Switch) ---- */
void GestionarFlancoPB1(void)
{
  static uint8_t estado_anterior = 0;
  uint8_t estado_actual = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
  static uint32_t contador = 0;

  // Detectar flanco de subida (LOW -> HIGH)
  if (estado_anterior == GPIO_PIN_RESET && estado_actual == GPIO_PIN_SET)
  {
    contador++;
    printf("Flanco detectado en PB1! Contador: %lu\r\n", contador);

    // Encender LEDs (indicador visual)
    BSP_LED_On(LED_RED);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

    // Enviar mensaje Sigfox
    EnviarMensajeFranco();

    // Mantener LEDs encendidos por 3 segundos
    HAL_Delay(3000);

    // Apagar LEDs
    BSP_LED_Off(LED_RED);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
  }

  estado_anterior = estado_actual;
}

/* ---- Main ---- */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  // Inicialización Sigfox
  MX_Sigfox_Init();

  // Inicializaciones de LEDs y botones
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_PB_Init(BUTTON_SW1, BUTTON_MODE_GPIO);
  BSP_PB_Init(BUTTON_SW2, BUTTON_MODE_GPIO);
  BSP_PB_Init(BUTTON_SW3, BUTTON_MODE_GPIO);

  // Configuración GPIO para PB3 y PB5 (salidas) y PB1 (entrada reed switch)
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_5;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);

  // Entrada PB_1 (reed switch)
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  printf("Sistema iniciado - Esperando flancos en PB1...\r\n");

  while (1)
  {
    GestionarFlancoPB1();      // Detección de reed switch
    MX_Sigfox_Process();       // Procesar Sigfox
    HAL_Delay(50);             // anti-rebote
  }
}

/**
  * @brief System Clock Configuration (MSI 32 MHz)
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK3|RCC_CLOCKTYPE_HCLK
                              |RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.AHBCLK3Divider = RCC_SYSCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
