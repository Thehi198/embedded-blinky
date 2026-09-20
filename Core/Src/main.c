#include <stdint.h>

volatile uint8_t button_pressed = 0;
volatile uint8_t led_mode = 0;
volatile uint32_t button_last_tick = 0;
volatile uint32_t ticks = 0;

// RCC
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)
#define RCC_APB2ENR (*(volatile uint32_t *)0x40023844)

//LED
#define GPIOA_MODER (*(volatile uint32_t*)0x40020000)
#define GPIOA_ODR (*(volatile uint32_t *)0x40020014)
#define GPIOA_BSRR (*(volatile uint32_t *)0x40020018)

//Button
#define GPIOC_MODER (*(volatile uint32_t *)0x40020800)

// EXTI
#define SYSCFG_EXTICR4 (*(volatile uint32_t *)0x40013814)
#define EXTI_IMR (*(volatile uint32_t *)0x40013C00)
#define EXTI_FTSR (*(volatile uint32_t *)0x40013C0C)
#define EXTI_PR (*(volatile uint32_t *)0x40013C14)

// NVIC
#define NVIC_ISER1 (*(volatile uint32_t *)0xE000E104)
#define SYSTIK_CTRL (*(volatile uint32_t *)0xE000E010)
#define SYSTIK_LOAD (*(volatile uint32_t *)0xE000E014)
#define SYSTIK_VAL (*(volatile uint32_t *)0xE000E018)


struct state; //define struct for tracking state
typedef void state_fn(struct state*); //define function type for each of the state functions
state_fn standby, on, blink;

struct state {
  state_fn *next;
};


struct state s = {standby};

int main(void){

	RCC_AHB1ENR |= (1u << 0) | (1u << 2); // Enable clock for GPIOA and GPIOC
	RCC_APB2ENR |= (1u <<14); // Set EXTI to listen to port c

	// LED Output
	GPIOA_MODER &= ~(0x3u << 10); // Clear bit position at 11:10
	GPIOA_MODER |= (0x1u << 10); // Set bit position at 11:10

	//Button Input
	GPIOC_MODER &= ~(0x3u << 26);

	// EXTI13 on Port C
	SYSCFG_EXTICR4 = (SYSCFG_EXTICR4 & ~(0xFu << 4)) | (0x2u << 4);
	EXTI_FTSR |= (1u << 13);
	EXTI_PR = (1u << 13);
	EXTI_IMR |= (1u << 13);
	NVIC_ISER1 = (1u << 8);

	// SysTick
	SYSTIK_LOAD = 16000u - 1u;
	SYSTIK_VAL = 0;
	SYSTIK_CTRL = (1u << 2) | (1u << 1) | (1u << 0);
  
  while (1){
    s.next(&s);

  }
}

// callback
// 	void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
// if (GPIO_Pin == GPIO_PIN_13) {
//			uint32_t now = HAL_GetTick();
//			if (now - btn_last_tick >= 50) {
//				btn_last_tick = now;
//				button_pressed = 1;
//			}
//		}
//	}

void SysTick_Handler(void){
  ticks++;
}


void EXTI15_10_IRQHandler(void){
  if (EXTI_PR & (1u << 13)) {
    EXTI_PR = (1u << 13);
    uint32_t now = ticks;
    if (now - button_last_tick >= 50u) {
      button_last_tick = now;
      button_pressed = 1;
    }
  }
}

void led_toggle(void){
	GPIOA_ODR ^= (1u << 5);
}

void led_off(void){
  GPIOA_BSRR = (1u << 21);
}

int hold_state(void){
  if (!button_pressed) return 0;
  button_pressed = 0;
  return 1;
  
}

void standby(struct state *state) {
  led_off();
  led_mode = 0;
  if (hold_state()) state->next = on;
}

void on(struct state *state) {
  led_toggle();
  led_mode = 1;
  if (hold_state()) state->next = blink;
}

void blink(struct state *state) {
  static uint32_t last_on = 0;
  led_mode = 2;
  // Tie with timer
  if (ticks - last_on >= 100u) {
    last_on = ticks;
    led_toggle();
  }

  if (hold_state()) state->next = standby;
}
