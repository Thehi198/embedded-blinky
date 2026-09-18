#include <stdint.h>

volatile uint8_t button_pressed = 0;
volatile uint8_t led_mode = 0;
volatile uint8_t button_last_tick = 0;

#define GPIOA_MODER (*(volatile uint32_t*)0x40020000)
#define GPIOA_BSRR (*(volatile uint32_t *)0x40030018)

struct state; //define struct for tracking state
typedef void state_fn(struct state*); //define function type for each of the state functions
state_fn standby, on, blink;

struct state {
  state_fn *next;
};

GPIOA_MODER &= ~(0x3u << 10); // Clear bit position at 11:10
GPIOA_BSRR |= (0x1u << 10); // Write 01 to position 11:10
struct state s = {standby};

int main(void){
  
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

void led_on(void){
  GPIOA_BSRR = (1u << 5);
}

void led_off(void){
  GPIOA_BSRR = (1u << 5);
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
  led_on();
  led_mode = 1;
  if (hold_state()) state->next = blink;
}

void blink(struct state *state) {
  led_mode = 2;
  if (hold_state()) state->next = standby;
}
