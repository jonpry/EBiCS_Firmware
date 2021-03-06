/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "print.h"
#include "FOC.h"
#include "config.h"


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
  #include "display_kingmeter.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  #include "display_bafang.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
  #include "display_kunteng.h"
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
  #include "display_ebics.h"
#endif


#include <arm_math.h>
/* USER CODE END Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_usart3_rx;

int c_squared;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
q31_t	raw_inj1;
q31_t	raw_inj2;



uint32_t ui32_tim1_counter=0;
uint32_t ui32_tim3_counter=0;
uint8_t ui8_hall_state=0;
uint8_t ui8_hall_state_old=0;
uint8_t ui8_hall_case=0;
uint16_t ui16_tim2_recent=0;
uint16_t ui16_timertics=5000; 					//timertics between two hall events for 60° interpolation
uint16_t ui16_reg_adc_value;
uint32_t ui32_reg_adc_value_filter;
uint16_t ui16_ph1_offset=0;
uint16_t ui16_ph2_offset=0;
uint16_t ui16_ph3_offset=0;

volatile int16_t i16_ph1_current=0;
volatile int16_t i16_ph2_current=0;
volatile int16_t i16_ph2_current_filter=0;
int16_t i16_ph3_current=0;
uint16_t i=0;
uint16_t j=0;
uint16_t k=0;
uint8_t ui8_overflow_flag=0;
uint8_t ui8_slowloop_counter=0;
uint8_t ui8_adc_inj_flag=0;
uint8_t ui8_adc_regular_flag=0;
int8_t i8_direction= REVERSE;
int8_t i8_reverse_flag = 1; //for temporaribly reverse direction

uint8_t ui8_adc_offset_done_flag=0;
uint8_t ui8_print_flag=0;
uint8_t ui8_UART_flag=0;
uint8_t ui8_Push_Assist_flag=0;
uint8_t ui8_UART_TxCplt_flag=1;
uint8_t ui8_PAS_flag=0;
uint8_t ui8_SPEED_flag=0;
uint32_t uint32_PAS_counter= PAS_TIMEOUT+1;
uint32_t uint32_PAS_HIGH_counter= 0;
uint32_t uint32_PAS_HIGH_accumulated= 32000;
uint32_t uint32_PAS_fraction= 100;
uint32_t uint32_SPEED_counter=32000;
uint32_t uint32_PAS=32000;

uint8_t ui8_UART_Counter=0;
int8_t i8_recent_rotor_direction=1;
int8_t i16_hall_order=1;

uint32_t uint32_torque_cumulated=0;
uint32_t uint32_PAS_cumulated=32000;
uint16_t uint16_mapped_throttle=0;
uint16_t uint16_mapped_PAS=0;
uint16_t uint16_half_rotation_counter=0;
uint16_t uint16_full_rotation_counter=0;
int16_t int16_current_target=0;

q31_t q31_t_Battery_Current_accumulated=0;

q31_t q31_rotorposition_absolute;
q31_t q31_rotorposition_hall;
q31_t q31_rotorposition_motor_specific = SPEC_ANGLE;
q31_t q31_u_d_temp=0;
q31_t q31_u_q_temp=0;
int16_t i16_sinus=0;
int16_t i16_cosinus=0;
char buffer[256];
char char_dyn_adc_state=1;
char char_dyn_adc_state_old=1;
uint8_t assist_factor[10]={0, 51, 102, 153, 204, 255, 255, 255, 255, 255};

q31_t switchtime[3];
volatile uint16_t adcData[8]; //Buffer for ADC1 Input
//static int8_t angle[256][4];
//static int8_t angle_old;
//q31_t q31_startpoint_conversion = 2048;

//Rotor angle scaled from degree to q31 for arm_math. -180°-->-2^31, 0°-->0, +180°-->+2^31
const q31_t DEG_0 = 0;
const q31_t DEG_plus60 = 715827883;
const q31_t DEG_plus120= 1431655765;
const q31_t DEG_plus180= 2147483647;
const q31_t DEG_minus60= -715827883;
const q31_t DEG_minus120= -1431655765;

const q31_t tics_lower_limit = WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*SPEEDLIMIT*10); //tics=wheelcirc*timerfrequency/(no. of hallevents per rev*gear-ratio*speedlimit)*3600/1000000
const q31_t tics_higher_limit = WHEEL_CIRCUMFERENCE*5*3600/(6*GEAR_RATIO*(SPEEDLIMIT+2)*10);
q31_t q31_tics_filtered=128000;
//variables for display communication

#define iabs(x) (((x) >= 0)?(x):-(x))

#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
KINGMETER_t KM;
#endif

//variables for display communication
#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
BAFANG_t BF;
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
uint8_t ui8_main_LEV_Page_counter=0;
uint8_t ui8_additional_LEV_Page_counter=0;
uint8_t ui8_LEV_Page_to_send=1;
#endif



MotorState_t MS;
MotorParams_t MP;


int16_t battery_percent_fromcapacity = 50; 			//Calculation of used watthours not implemented yet
int16_t wheel_time = 1000;							//duration of one wheel rotation for speed calculation
int16_t current_display;							//pepared battery current for display

int16_t power;	

static void dyn_adc_state(q31_t angle);
static void set_inj_channel(char state);
void get_standstill_position();
int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);

#if 0
#define JSQR_PHASE_A 0b00011000000000000000 //3
#define JSQR_PHASE_B 0b00100000000000000000 //4
#define JSQR_PHASE_C 0b00101000000000000000 //5
#else
#define JSQR_PHASE_A 0b00011000000000000000 //3
#define JSQR_PHASE_B 0b00100000000000000000 //4
#define JSQR_PHASE_C 0b00101000000000000000 //5
#endif

#define ADC_VOLTAGE 0
#define ADC_THROTTLE 1
#define ADC_TEMP 2
#define ADC_CHANA 3
#define ADC_CHANB 4
#define ADC_CHANC 5


void autodetect(){
	SET_BIT(TIM1->BDTR, TIM_BDTR_MOE);
   	MS.hall_angle_detect_flag=0; //set uq to contstant value in FOC.c for open loop control
   	q31_rotorposition_absolute=11930465/2;//1<<31;
   	HAL_Delay(5);
   	for(i=0;i<1080;i++){
//        while(1){     
   		q31_rotorposition_absolute+=1*11930465; //drive motor in open loop with steps of 1°
   		HAL_Delay(1);
   		if(ui8_hall_state_old!=ui8_hall_state){
   		printf_("hallstate:  %d, hallcase %d \n", ui8_hall_state , ui8_hall_case);

   		if(ui8_hall_case==13)
   		{
   			q31_rotorposition_motor_specific=q31_rotorposition_absolute;
   		}
   		if(ui8_hall_case==23)
   		{
   			q31_rotorposition_motor_specific=q31_rotorposition_absolute;
   		}

   		ui8_hall_state_old=ui8_hall_state;
   		}
   	}
    HAL_FLASH_Unlock();
//    EE_WriteVariable(EEPROM_POS_SPEC_ANGLE, q31_rotorposition_motor_specific>>16);
    if(i8_recent_rotor_direction == 1){
//    	EE_WriteVariable(EEPROM_POS_HALL_ORDER, 1);
    	i16_hall_order = 1;
    }
    else{
 //   	EE_WriteVariable(EEPROM_POS_HALL_ORDER, -1);
    	i16_hall_order = -1;
    }
    HAL_FLASH_Lock();

   	MS.hall_angle_detect_flag=1;
#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Motor specific angle:  %d, direction %d \n ", q31_rotorposition_motor_specific, i16_hall_order);
#endif

    HAL_Delay(5);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */



  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();

  //initialize MS struct.
  MS.hall_angle_detect_flag=1;
  MS.Speed=128000;
  MS.assist_level=1;
  MS.regen_level=7;

  MX_ADC1_Init();
  /* Run the ADC calibration */
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
    /* Calibration Error */
    Error_Handler();
  }
  MX_ADC2_Init();
  /* Run the ADC calibration */
  if (HAL_ADCEx_Calibration_Start(&hadc2) != HAL_OK)
  {
    /* Calibration Error */
    Error_Handler();
  }

  /* USER CODE BEGIN 2 */
 SET_BIT(ADC1->CR2, ADC_CR2_JEXTTRIG);//external trigger enable
 __HAL_ADC_ENABLE_IT(&hadc1,ADC_IT_JEOC);
 SET_BIT(ADC2->CR2, ADC_CR2_JEXTTRIG);//external trigger enable
 __HAL_ADC_ENABLE_IT(&hadc2,ADC_IT_JEOC);


  //HAL_ADC_Start_IT(&hadc1);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*)adcData, 6);
  HAL_ADC_Start_IT(&hadc2);
  MX_TIM1_Init(); //Hier die Reihenfolge getauscht!
  MX_TIM2_Init();
  MX_TIM3_Init();

 // Start Timer 1
    if(HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
      {
        /* Counter Enable Error */
        Error_Handler();
      }

      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
      HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1); // turn on complementary channel
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
      HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
      HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);

      HAL_TIM_PWM_Start_IT(&htim1, TIM_CHANNEL_4);




    TIM1->CCR4 = TRIGGER_DEFAULT; //ADC sampling just before timer overflow (just before middle of PWM-Cycle)
//PWM Mode 1: Interrupt at counting down.

    //TIM1->BDTR |= 1L<<15;
   // TIM1->BDTR &= ~(1L<<15); //reset MOE (Main Output Enable) bit to disable PWM output
    // Start Timer 2
       if(HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
         {
           /* Counter Enable Error */
           Error_Handler();
         }

       // Start Timer 3

       if(HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
            {
              /* Counter Enable Error */
              Error_Handler();
            }


/*
      // HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_CC4);
       __HAL_LOCK(&htim1);
       SET_BIT(TIM1->EGR, TIM_EGR_CC4G);//capture compare ch 4 event
       SET_BIT(TIM1->EGR, TIM_EGR_TG);//Trigger generation
       SET_BIT(TIM1->BDTR, TIM_AUTOMATICOUTPUT_ENABLE);//Trigger generation

       __HAL_UNLOCK(&htim1);
       SET_BIT(ADC1->CR2, ADC_CR2_JEXTTRIG);//external trigger enable

*/

       //Init KingMeter Display
       //Init KingMeter Display
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
       KingMeter_Init (&KM);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
       Bafang_Init (&BF);
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
       kunteng_init();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
       ebics_init();
#endif


    TIM1->CCR1 = 1023; //set initial PWM values
    TIM1->CCR2 = 1023;
    TIM1->CCR3 = 1023;

    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

    for(i=0;i<16;i++){
    	while(!ui8_adc_regular_flag){}
    	ui16_ph1_offset+=adcData[ADC_CHANA];
    	ui16_ph2_offset+=adcData[ADC_CHANB];
    	ui16_ph3_offset+=adcData[ADC_CHANC];
    	ui8_adc_regular_flag=0;

    }
    ui16_ph1_offset=ui16_ph1_offset>>4;
    ui16_ph2_offset=ui16_ph2_offset>>4;
    ui16_ph3_offset=ui16_ph3_offset>>4;

    printf_("phase current offsets:  %d, %d, %d \n ", ui16_ph1_offset, ui16_ph2_offset, ui16_ph3_offset);

//while(1){}

    ADC1->JSQR=JSQR_PHASE_A; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
   	ADC1->JOFR1 = ui16_ph1_offset;

   	ui8_adc_offset_done_flag=1;

#if 0 //(DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)

        autodetect();
   


//        while(1){}

#else
//   	q31_rotorposition_motor_specific = SPEC_ANGLE;
        //Motor specific angle:  1908874660, direction -1 
        q31_rotorposition_motor_specific = 1175151010;

        i16_hall_order = 1;


#endif

   		 HAL_GPIO_EXTI_Callback(GPIO_PIN_4); //read in initial rotor position


   	 	 switch (ui8_hall_state)
   			{
   			//6 cases for forward direction
   			case 4:
   				q31_rotorposition_hall = DEG_0 + q31_rotorposition_motor_specific;
   				break;
   			case 5:
   				q31_rotorposition_hall = DEG_plus60 + q31_rotorposition_motor_specific;
   				break;
   			case 1:
   				q31_rotorposition_hall = DEG_plus120 + q31_rotorposition_motor_specific;
   				break;
   			case 3:
   				q31_rotorposition_hall = DEG_plus180 + q31_rotorposition_motor_specific;
   				break;
   			case 2:
   				q31_rotorposition_hall = DEG_minus120 + q31_rotorposition_motor_specific;
   				break;
   			case 6:
   				q31_rotorposition_hall = DEG_minus60 + q31_rotorposition_motor_specific;
   				break;

   			}

   		 q31_rotorposition_absolute = q31_rotorposition_hall; // set absolute position to corresponding hall pattern.



#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
    printf_("Lishui FOC v0.9 \n ");
#endif

    HAL_Delay(5);
    CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE);//Disable PWM

    get_standstill_position();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  //PI-control processing
	  if(PI_flag){


		  q31_u_q_temp =  PI_control_i_q(MS.i_q, int16_current_target);

		  q31_t_Battery_Current_accumulated -= q31_t_Battery_Current_accumulated>>8;
		  q31_t_Battery_Current_accumulated += ((MS.i_q*MS.u_abs)>>11)*(uint16_t)(CAL_I>>8);

		  MS.Battery_Current = q31_t_Battery_Current_accumulated>>8*i8_direction*i8_reverse_flag; //Battery current in mA

		  	//Control id
		  q31_u_d_temp = -PI_control_i_d(MS.i_d, 0, iabs(q31_u_q_temp)); //control direct current to zero


                  if(q31_u_d_temp > iabs(q31_u_q_temp) >> 4)
                      q31_u_d_temp = iabs(q31_u_q_temp) >> 4;
                  
                  if(q31_u_d_temp < (-iabs(q31_u_q_temp)) >> 4)
                      q31_u_d_temp = (-iabs(q31_u_q_temp)) >> 4;

		  	//limit voltage in rotating frame, refer chapter 4.10.1 of UM1052
		  MS.u_abs = (q31_t)hypot((double)q31_u_d_temp, (double)q31_u_q_temp); //absolute value of U in static frame



#if 1
			if (MS.u_abs > _U_MAX){
				MS.u_q = (q31_u_q_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_d = (q31_u_d_temp*_U_MAX)/MS.u_abs; //division!
				MS.u_abs = _U_MAX;
			}
			else
#endif
                        {
				MS.u_q=q31_u_q_temp;
  			        MS.u_d=q31_u_d_temp;

			}
		  	PI_flag=0;
	  }
	  //display message processing
	  if(ui8_UART_flag){
#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
	  kingmeter_update();
#endif


#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
	  bafang_update();
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
	  ui8_UART_Counter++;
	  if(ui8_UART_Counter>5){
	  check_message(&MS);
	  ui8_UART_Counter=0;
	  }
#endif

#if (DISPLAY_TYPE & DISPLAY_TYPE_EBiCS)
	  process_ant_page(&MS, &MP);
#endif

	  ui8_UART_flag=0;
	  }

	  //SPEED signal processing
	  if(ui8_SPEED_flag){

		  if(uint32_SPEED_counter>200){
		  MS.Speed = uint32_SPEED_counter;
		  //HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		  uint32_SPEED_counter =0;
		  ui8_SPEED_flag=0;

		  }
	  }

#if 0 //(DISPLAY_TYPE == DISPLAY_TYPE_DEBUG) // && defined(FAST_LOOP_LOG))
		if(ui8_UART_TxCplt_flag){
	        sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n", e_log[k][0], e_log[k][1], e_log[k][2],e_log[k][3],e_log[k][4],e_log[k][5]); //>>24
			i=0;
			while (buffer[i] != '\0')
			{i++;}
			ui8_UART_TxCplt_flag=0;
			HAL_UART_Transmit_DMA(&huart3, (uint8_t *)&buffer, i);
			k++;
			if (k>299){
				k=0;
				ui8_debug_state=0;
				//Obs_flag=0;
			}
		}
#endif

	  //throttle and PAS current target setting

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
	  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(assist_factor[MS.assist_level]))>>8, 0); // level in range 0...5
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KUNTENG)
	  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(MS.assist_level))/5, 0); // level in range 0...5
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KINGMETER_618U)
	  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(MS.assist_level-1))>>2, 0); // level in range 1...5
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_KINGMETER_901U)
	  uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, ((PH_CURRENT_MAX*(int32_t)(MS.assist_level)))>>8, 0); // level in range 0...255
#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG)
	  //uint32_PAS= RAMP_END;
	  //uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, (PH_CURRENT_MAX*(int32_t)(assist_factor[7]))>>8, 0); // level in range 0...9
	 if (uint32_PAS_counter < PAS_TIMEOUT) uint16_mapped_PAS = map(uint32_PAS, RAMP_END, PAS_TIMEOUT, PH_CURRENT_MAX, 0); // Full amps in debug mode
	 else uint16_mapped_PAS = 0;
#endif

#ifdef TS_MODE //torque-sensor mode

	  int16_current_target = (TS_COEF*(int16_t)(MS.assist_level)* (uint32_torque_cumulated>>5)/uint32_PAS)>>8; //>>5 aus Mittelung über eine Kurbelumdrehung, >>8 aus KM5S-Protokoll Assistlevel 0..255
	  temp1=int16_current_target;
	  if(int16_current_target>PH_CURRENT_MAX) int16_current_target = PH_CURRENT_MAX;
	  temp2=int16_current_target;
	  if(uint32_PAS_counter > PAS_TIMEOUT) int16_current_target = 0;
	  temp3=int16_current_target;
	  //int16_current_target = 0;

#else		// torque-simulation mode with throttle override

	  uint16_mapped_throttle = map(ui16_reg_adc_value, THROTTLE_OFFSET , THROTTLE_MAX, 0, PH_CURRENT_MAX);

#ifdef DIRDET
	  if (uint32_PAS_counter< PAS_TIMEOUT){
		  if ((uint32_PAS_fraction < FRAC_LOW ||uint32_PAS_fraction > FRAC_HIGH)){
			  uint16_mapped_PAS= 0;//pedals are turning backwards, stop motor
		  }
	  }
	  else uint32_PAS_HIGH_accumulated=uint32_PAS_cumulated;
#endif //end direction detection

	  if(uint16_mapped_PAS>uint16_mapped_throttle)   											//check for throttle override

	  {
		  if (uint32_PAS_counter < PAS_TIMEOUT) int16_current_target= uint16_mapped_PAS;		//set current target in torque-simulation-mode, if pedals are turning
		  else  {
			  int16_current_target= 0;//pedals are not turning, stop motor
			  uint32_PAS_cumulated=32000;
			  uint32_PAS=32000;
		  }
	  }
	  else 
                  int16_current_target = uint16_mapped_throttle;//throttle override: set recent throttle value as current target


#endif




	  int16_current_target=map(q31_tics_filtered>>3,tics_higher_limit,tics_lower_limit,0,int16_current_target); //ramp down current at speed limit

	  if(ui8_Push_Assist_flag)int16_current_target=PUSHASSIST_CURRENT;

	  if (int16_current_target>0&&uint16_mapped_throttle>0&&!READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){

                  TIM1->CCR1 = 1023; //set initial PWM values
                  TIM1->CCR2 = 1023;
                  TIM1->CCR3 = 1023;
		  uint16_half_rotation_counter=0;
		  uint16_full_rotation_counter=0;
		  __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
		  ui16_timertics=20000; //set interval between two hallevents to a large value
		  i8_recent_rotor_direction=i8_direction*i8_reverse_flag;
		  get_standstill_position();

                  printf_("startup %d\n", q31_rotorposition_absolute);

		  SET_BIT(TIM1->BDTR, TIM_BDTR_MOE); //enable PWM if power is wanted
	  }else{
#ifdef KILL_ON_ZERO
                  if(uint16_mapped_throttle==0&&READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){
			  CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning
			  get_standstill_position();
                          printf_("shutdown %d\n", q31_rotorposition_absolute);
                  }
#endif
          }

	 //slow loop procedere @16Hz, for LEV standard every 4th loop run, send page,
	  if(ui32_tim3_counter>500){
		  MS.Temperature = adcData[ADC_TEMP]*41>>8; //0.16 is calibration constant: Analog_in[10mV/°C]/ADC value. Depending on the sensor LM35)
		  MS.Voltage=adcData[ADC_VOLTAGE];
		  if(uint32_SPEED_counter>127999)MS.Speed =128000;

		  if((uint16_full_rotation_counter>7999||uint16_half_rotation_counter>7999)&&READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){
			  CLEAR_BIT(TIM1->BDTR, TIM_BDTR_MOE); //Disable PWM if motor is not turning

			  get_standstill_position();
                          printf_("shutdown %d\n", q31_rotorposition_absolute);
		  }

#if (DISPLAY_TYPE == DISPLAY_TYPE_DEBUG && !defined(FAST_LOOP_LOG))
		  //print values for debugging


	  		sprintf_(buffer, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, T: %d, Q: %d, D: %d, %d, %d,S: %d, %d, V: %d\r\n", int16_current_target, (int16_t) raw_inj1,(int16_t) raw_inj2, (int32_t) MS.char_dyn_adc_state, q31_rotorposition_hall, q31_rotorposition_absolute, (int16_t) (ui16_reg_adc_value-THROTTLE_OFFSET),adcData[ADC_CHANA],adcData[ADC_CHANB],adcData[ADC_CHANC],i16_ph1_current,i16_ph2_current, uint16_mapped_throttle, MS.i_q, MS.i_d, MS.u_q, MS.u_d,q31_tics_filtered>>3,tics_higher_limit, adcData[ADC_VOLTAGE]);//((q31_i_q_fil*q31_u_abs)>>14)*
	  	//	sprintf_(buffer, "%d, %d, %d, %d, %d, %d\r\n",(uint16_t)adcData[0],(uint16_t)adcData[1],(uint16_t)adcData[2],(uint16_t)adcData[3],(uint16_t)(adcData[4]),(uint16_t)(adcData[5])) ;

	  	  i=0;
		  while (buffer[i] != '\0')
		  {i++;}
		 HAL_UART_Transmit_DMA(&huart3, (uint8_t *)&buffer, i);


		  ui8_print_flag=0;

#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_EBiCS)
		  ui8_slowloop_counter++;
		  if(ui8_slowloop_counter>3){
			  ui8_slowloop_counter = 0;

			  switch (ui8_main_LEV_Page_counter){
			  case 1: {
				  ui8_LEV_Page_to_send = 1;
			  	  }
			  	  break;
			  case 2: {
				  ui8_LEV_Page_to_send = 2;
			  	  }
			  	  break;
			  case 3: {
				  ui8_LEV_Page_to_send = 3;
			  	  }
			  	  break;
			  case 4: {
				  //to do, define other pages
			  	  }
			  	  break;
			  }//end switch

			  send_ant_page(ui8_LEV_Page_to_send, &MS, &MP);

			  ui8_main_LEV_Page_counter++;
			  if(ui8_main_LEV_Page_counter>4)ui8_main_LEV_Page_counter=1;
		  }

#endif
		  ui32_tim3_counter=0;
	  }// end of slow loop

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 2, 0);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  ADC_MultiModeTypeDef multimode;
  ADC_InjectionConfTypeDef sConfigInjected;
  ADC_ChannelConfTypeDef sConfig;

    /**Common config 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE; //Scan muß für getriggerte Wandlung gesetzt sein
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;// Trigger regular ADC with timer 3 ADC_EXTERNALTRIGCONV_T1_CC1;// // ADC_SOFTWARE_START; //
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 6;
  hadc1.Init.NbrOfDiscConversion = 0;


  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the ADC multi-mode 
    */
  multimode.Mode = ADC_DUALMODE_REGSIMULT_INJECSIMULT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Injected Channel 
    */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_3;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedNbrOfConversion = 1;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfigInjected.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJECCONV_T1_CC4; // Hier bin ich nicht sicher ob Trigger out oder direkt CC4
  sConfigInjected.AutoInjectedConv = DISABLE; //muß aus sein
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = 0;//ui16_ph1_offset;//1900;
  HAL_ADC_Stop(&hadc1); //ADC muß gestoppt sein, damit Triggerquelle gesetzt werden kann.
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Regular Channel 
    */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }


/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_1;
sConfig.Rank = ADC_REGULAR_RANK_2;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = ADC_CHANNEL_0;
sConfig.Rank = ADC_REGULAR_RANK_3;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = JSQR_PHASE_A>>15;
sConfig.Rank = ADC_REGULAR_RANK_4;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
/**Configure Regular Channel
*/
sConfig.Channel = JSQR_PHASE_B>>15;
sConfig.Rank = ADC_REGULAR_RANK_5;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}

sConfig.Channel = JSQR_PHASE_C>>15;
sConfig.Rank = ADC_REGULAR_RANK_6;
sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;//ADC_SAMPLETIME_239CYCLES_5;
if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
{
_Error_Handler(__FILE__, __LINE__);
}
}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  ADC_InjectionConfTypeDef sConfigInjected;

    /**Common config 
    */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE; //hier auch Scan enable?!
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Injected Channel 
    */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_4;
  sConfigInjected.InjectedRank = ADC_INJECTED_RANK_1;
  sConfigInjected.InjectedNbrOfConversion = 1;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfigInjected.ExternalTrigInjecConv = ADC_INJECTED_SOFTWARE_START;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = 0;//ui16_ph2_offset;//	1860;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc2, &sConfigInjected) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

   TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = _T;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_OC_Init(&htim1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH; //TODO: depends on gate driver!
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_SET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = _T - 1;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 32;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim1);
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 128;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 64000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM3 init function 8kHz interrupt frequency for regular adc triggering */
static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 7813;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim3);

}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_HalfDuplex_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

   huart3.Instance = USART3;

#if ((DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER) ||DISPLAY_TYPE==DISPLAY_TYPE_KUNTENG||DISPLAY_TYPE==DISPLAY_TYPE_EBiCS)
  huart3.Init.BaudRate = 9600;
#elif (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
  huart3.Init.BaudRate = 1200;
#else
  huart3.Init.BaudRate = 56000;
#endif


  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();



  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);



  HAL_GPIO_WritePin(HALL_1_GPIO_Port, HALL_1_Pin, GPIO_PIN_RESET);


  /*Configure GPIO pins : HALL_1_Pin HALL_2_Pin */
  GPIO_InitStruct.Pin = HALL_1_Pin|HALL_2_Pin|HALL_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

#if 0
  GPIO_InitStruct.Pin = HALL_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(HALL_1_GPIO_Port, &GPIO_InitStruct);

  while(1){
    HAL_GPIO_WritePin(HALL_1_GPIO_Port, HALL_1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(HALL_1_GPIO_Port, HALL_1_Pin, GPIO_PIN_SET);
  }
#endif

  /*Configure peripheral I/O remapping */
  __HAL_AFIO_REMAP_PD01_ENABLE();

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}


/* USER CODE BEGIN 4 */


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim3) {
		if(ui32_tim3_counter<32000)ui32_tim3_counter++;
		if (uint32_SPEED_counter<128000){
			  uint32_SPEED_counter++;
		}
		if(uint16_full_rotation_counter<8000)uint16_full_rotation_counter++;	//full rotation counter for motor standstill detection
		if(uint16_half_rotation_counter<8000)uint16_half_rotation_counter++;	//half rotation counter for motor standstill detectio
	}
}



// regular ADC callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	ui32_reg_adc_value_filter -= ui32_reg_adc_value_filter>>4;
	ui32_reg_adc_value_filter += adcData[ADC_THROTTLE]; //HAL_ADC_GetValue(hadc);
	ui16_reg_adc_value = ui32_reg_adc_value_filter>>4;

        ui8_adc_regular_flag=1;
}

//injected ADC

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	//for oszi-check of used time in FOC procedere
	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  ui32_tim1_counter++;

	/*  else {
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	  uint32_SPEED_counter=0;
	  }*/

	if(!ui8_adc_offset_done_flag)
	{
	i16_ph1_current = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
	i16_ph2_current = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);

	ui8_adc_inj_flag=1;
	}
	else{

#ifdef DISABLE_DYNAMIC_ADC

		i16_ph1_current = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
		i16_ph2_current = HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);


#else
	switch (MS.char_dyn_adc_state) //read in according to state
		{
		case 1: //Phase C at high dutycycles, read from A+B directly
			{
				raw_inj1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = raw_inj1 ;

				raw_inj2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = raw_inj2;
			}
			break;
		case 2: //Phase A at high dutycycles, read from B+C (A = -B -C)
			{

				raw_inj2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = raw_inj2;

				raw_inj1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = -i16_ph2_current-raw_inj1;

			}
			break;
		case 3: //Phase B at high dutycycles, read from A+C (B=-A-C)
			{
				raw_inj1=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
				i16_ph1_current = raw_inj1 ;
				raw_inj2=(q31_t)HAL_ADCEx_InjectedGetValue(&hadc2, ADC_INJECTED_RANK_1);
				i16_ph2_current = -i16_ph1_current-raw_inj2;
			}
			break;

		case 0: //timeslot too small for ADC
			{
				//do nothing
			}
			break;




		} // end case
#endif




	//extrapolate recent rotor position
	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last event
    if(MS.hall_angle_detect_flag){
	   if (ui16_tim2_recent < ui16_timertics && !ui8_overflow_flag){ //prevent angle running away at standstill
		// float with division necessary!
           		   //normal forward function
                q31_rotorposition_absolute = q31_rotorposition_hall*i16_hall_order + (q31_t)(i16_hall_order * i8_recent_rotor_direction * (715827883.0*((float)ui16_tim2_recent/(float)ui16_timertics))); //interpolate angle between two hallevents by scaling timer2 tics

	   }
	   else
	   {ui8_overflow_flag=1;

	   }
    }//end if hall angle detect

#ifndef DISABLE_DYNAMIC_ADC

	//get the Phase with highest duty cycle for dynamic phase current reading
	dyn_adc_state(q31_rotorposition_absolute);
	//set the according injected channels to read current at Low-Side active time

	if (MS.char_dyn_adc_state!=char_dyn_adc_state_old){
		set_inj_channel(MS.char_dyn_adc_state);
		char_dyn_adc_state_old = MS.char_dyn_adc_state;
		}
#endif

	//int16_current_target=0;
	// call FOC procedure if PWM is enabled

	if (READ_BIT(TIM1->BDTR, TIM_BDTR_MOE)){
	FOC_calculation(i16_ph1_current, i16_ph2_current, q31_rotorposition_absolute, (((int16_t)i8_direction*i8_reverse_flag)*int16_current_target), uint16_mapped_throttle, &MS);
	}
	//temp5=__HAL_TIM_GET_COUNTER(&htim1);
	//set PWM

	TIM1->CCR1 =  (uint16_t) switchtime[0];
	TIM1->CCR2 =  (uint16_t) switchtime[1];
	TIM1->CCR3 =  (uint16_t) switchtime[2];

	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	} // end else

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//Hall sensor event processing
	if(GPIO_Pin == GPIO_PIN_4||GPIO_Pin == GPIO_PIN_5||GPIO_Pin == GPIO_PIN_0) //check for right interrupt source
	{
	ui8_hall_state = ((GPIOB->IDR & 1) << 2) | ((GPIOB->IDR >> 4) & 0b11); //Mask input register with Hall 1 - 3 bits
        
        if(ui8_hall_state == ui8_hall_state_old)
             return;

	ui8_hall_case=ui8_hall_state_old*10+ui8_hall_state;
	if(MS.hall_angle_detect_flag){ //only process, if autodetect procedere is fininshed
	ui8_hall_state_old=ui8_hall_state;
	}

	ui16_tim2_recent = __HAL_TIM_GET_COUNTER(&htim2); // read in timertics since last hall event


	if(ui16_tim2_recent>100){//debounce
		ui16_timertics = ui16_tim2_recent; //save timertics since last hall event
		q31_tics_filtered-=q31_tics_filtered>>3;
		q31_tics_filtered+=ui16_timertics;
	   __HAL_TIM_SET_COUNTER(&htim2,0); //reset tim2 counter
	   ui8_overflow_flag=0;

	}



	switch (ui8_hall_case) //12 cases for each transition from one stage to the next. 6x forward, 6x reverse
			{
		//6 cases for forward direction
		case 64:
			q31_rotorposition_hall = DEG_0 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
			uint16_full_rotation_counter=0;
			break;
		case 45:
			q31_rotorposition_hall = DEG_plus60 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
			break;
		case 51:
			q31_rotorposition_hall = DEG_plus120 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
			break;
		case 13:
			q31_rotorposition_hall = DEG_plus180 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
		        uint16_half_rotation_counter=0;
			break;
		case 32:
			q31_rotorposition_hall = DEG_minus120 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
			break;
		case 26:
			q31_rotorposition_hall = DEG_minus60 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=1;
			break;

		//6 cases for reverse direction
		case 46:
			q31_rotorposition_hall = DEG_minus60 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
			break;
		case 62:
			q31_rotorposition_hall = DEG_minus120 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
			break;
		case 23:
			q31_rotorposition_hall = DEG_plus180 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
   		        uint16_half_rotation_counter=0;
			break;
		case 31:
			q31_rotorposition_hall = DEG_plus120 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
			break;
		case 15:
			q31_rotorposition_hall = DEG_plus60 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
			break;
		case 54:
			q31_rotorposition_hall = DEG_0 + q31_rotorposition_motor_specific;
			i8_recent_rotor_direction=-1;
			uint16_full_rotation_counter=0;
			break;

		} // end case


	} //end if


#if 0
	//Speed processing
	if(GPIO_Pin == Speed_EXTI5_Pin)
	{


			ui8_SPEED_flag = 1; //with debounce

	}
#endif
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_UART_flag=1;

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	ui8_UART_TxCplt_flag=1;
}


#if (DISPLAY_TYPE & DISPLAY_TYPE_KINGMETER)
void kingmeter_update(void)
{
    /* Prepare Tx parameters */

    if(battery_percent_fromcapacity > 10)
    {
        KM.Tx.Battery = KM_BATTERY_NORMAL;
    }
    else
    {
        KM.Tx.Battery = KM_BATTERY_LOW;
    }

    if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
    {
        // Adapt wheeltime to match displayed speedo value according config.h setting
    	KM.Tx.Wheeltime_ms = ((MS.Speed>>3)*PULSES_PER_REVOLUTION); //>>3 because of 8 kHz counter frequency, so 8 tics per ms
    }
    else
    {
        KM.Tx.Wheeltime_ms = 64000;
    }


    //KM.Tx.Wheeltime_ms = 25;

    KM.Tx.Error = KM_ERROR_NONE;

    KM.Tx.Current_x10 = (uint16_t) (MS.Battery_Current/100); //MS.Battery_Current is in mA


    /* Receive Rx parameters/settings and send Tx parameters */
    KingMeter_Service(&KM);


    /* Apply Rx parameters */

    MS.assist_level = KM.Rx.AssistLevel;

    if(KM.Rx.Headlight == KM_HEADLIGHT_OFF)
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_RESET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        }
        else // KM_HEADLIGHT_ON, KM_HEADLIGHT_LOW, KM_HEADLIGHT_HIGH
        {
        	HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, GPIO_PIN_SET);
        	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }


    if(KM.Rx.PushAssist == KM_PUSHASSIST_ON)
    {
    	ui8_Push_Assist_flag=1;
    }
    else
    {
    	ui8_Push_Assist_flag=0;
    }



}

#endif

#if (DISPLAY_TYPE == DISPLAY_TYPE_BAFANG)
void bafang_update(void)
{
    /* Prepare Tx parameters */

	if(adcData[ADC_VOLTAGE]*CAL_BAT_V>BATTERY_LEVEL_5)battery_percent_fromcapacity=75;
	else if(adcData[ADC_VOLTAGE]*CAL_BAT_V>BATTERY_LEVEL_4)battery_percent_fromcapacity=50;
	else if(adcData[ADC_VOLTAGE]*CAL_BAT_V>BATTERY_LEVEL_3)battery_percent_fromcapacity=30;
	else if(adcData[ADC_VOLTAGE]*CAL_BAT_V>BATTERY_LEVEL_2)battery_percent_fromcapacity=10;
	else if(adcData[ADC_VOLTAGE]*CAL_BAT_V>BATTERY_LEVEL_1)battery_percent_fromcapacity=5;
	else battery_percent_fromcapacity=0;


    	BF.Tx.Battery = battery_percent_fromcapacity;


    if(__HAL_TIM_GET_COUNTER(&htim2) < 12000)
    {
        // Adapt wheeltime to match displayed speedo value according config.h setting
        BF.Tx.Wheeltime_ms = WHEEL_CIRCUMFERENCE*216/(MS.Speed*PULSES_PER_REVOLUTION); // Geschwindigkeit ist Weg pro Zeit Radumfang durch Dauer einer Radumdrehung --> Umfang * 8000*3600/(n*1000000) * Skalierung Bafang Display 200/26,6

    }
    else
    {
        BF.Tx.Wheeltime_ms = 0; //64000;
    }


       BF.Tx.Power = MS.i_q*MS.Voltage;


    /* Receive Rx parameters/settings and send Tx parameters */
    Bafang_Service(&BF,1);



    /* Apply Rx parameters */

//No headlight supported on my controller hardware.
    if(BF.Rx.Headlight)
    {
       // digitalWrite(lights_pin, 0);
    }
    else
    {
       // digitalWrite(lights_pin, 1);
    }


    if(BF.Rx.PushAssist) ui8_Push_Assist_flag=1;
    else ui8_Push_Assist_flag=0;

    MS.assist_level=BF.Rx.AssistLevel;
}

#endif

int32_t map (int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
  // if input is smaller/bigger than expected return the min/max out ranges value
  if (x < in_min)
    return out_min;
  else if (x > in_max)
    return out_max;

  // map the input to the output range.
  // round up if mapping bigger ranges to smaller ranges
  else  if ((in_max - in_min) > (out_max - out_min))
    return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
  // round down if mapping smaller ranges to bigger ranges
  else
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


//assuming, a proper AD conversion takes 350 timer tics, to be confirmed. DT+TR+TS deadtime + noise subsiding + sample time
void dyn_adc_state(q31_t angle){
	if (switchtime[2]>switchtime[0] && switchtime[2]>switchtime[1]){
		MS.char_dyn_adc_state = 1; // -90° .. +30°: Phase C at high dutycycles
		if(switchtime[2]>1500)TIM1->CCR4 =  switchtime[2]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[0]>switchtime[1] && switchtime[0]>switchtime[2]) {
		MS.char_dyn_adc_state = 2; // +30° .. 150° Phase A at high dutycycles
		if(switchtime[0]>1500)TIM1->CCR4 =  switchtime[0]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}

	if (switchtime[1]>switchtime[0] && switchtime[1]>switchtime[2]){
		MS.char_dyn_adc_state = 3; // +150 .. -90° Phase B at high dutycycles
		if(switchtime[1]>1500)TIM1->CCR4 =  switchtime[1]-TRIGGER_OFFSET_ADC;
		else TIM1->CCR4 = TRIGGER_DEFAULT;
	}
}

static void set_inj_channel(char state){
	switch (state)
	{
	case 1: //Phase C at high dutycycles, read current from phase A + B
		 {
			 ADC1->JSQR=JSQR_PHASE_A; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
			 ADC1->JOFR1 = ui16_ph1_offset;
			 ADC2->JSQR=JSQR_PHASE_B; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
			 ADC2->JOFR1 = ui16_ph2_offset;


		 }
			break;
	case 2: //Phase A at high dutycycles, read current from phase C + B
			 {
				 ADC1->JSQR=JSQR_PHASE_C; //ADC1 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC1->JOFR1 = ui16_ph3_offset;
				 ADC2->JSQR=JSQR_PHASE_B; //ADC2 injected reads phase B, JSQ4 = 0b00101, decimal 5
				 ADC2->JOFR1 = ui16_ph2_offset;


			 }
				break;

	case 3: //Phase B at high dutycycles, read current from phase A + C
			 {
				 ADC1->JSQR=JSQR_PHASE_A; //ADC1 injected reads phase A JL = 0b00, JSQ4 = 0b00100 (decimal 4 = channel 4)
				 ADC1->JOFR1 = ui16_ph1_offset;
				 ADC2->JSQR=JSQR_PHASE_C; //ADC2 injected reads phase C, JSQ4 = 0b00110, decimal 6
				 ADC2->JOFR1 = ui16_ph3_offset;


			 }
				break;


	}


}

void get_standstill_position(){
	  HAL_Delay(100);
	  HAL_GPIO_EXTI_Callback(GPIO_PIN_4); //read in initial rotor position
#if 0
		switch (ui8_hall_state)
			{
			//6 cases for forward direction
			case 2:
				q31_rotorposition_hall = DEG_0 + q31_rotorposition_motor_specific;
				break;
			case 6:
				q31_rotorposition_hall = DEG_plus60 + q31_rotorposition_motor_specific;
				break;
			case 4:
				q31_rotorposition_hall = DEG_plus120 + q31_rotorposition_motor_specific;
				break;
			case 5:
				q31_rotorposition_hall = DEG_plus180 + q31_rotorposition_motor_specific;
				break;
			case 1:
				q31_rotorposition_hall = DEG_minus120 + q31_rotorposition_motor_specific;
				break;
			case 3:
				q31_rotorposition_hall = DEG_minus60 + q31_rotorposition_motor_specific;
				break;

			}
#endif
		 q31_rotorposition_absolute = q31_rotorposition_hall;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
