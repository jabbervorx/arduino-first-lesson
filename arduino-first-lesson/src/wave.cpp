#include <Arduino.h>
#include <avr/io.h>
#include "HardwareSerial.h"

typedef struct _int_ratio {
	int ante, conseq;
} int_ratio;
int tbl_sinus_scale = 255;
byte tbl_sinus[] = { 0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 49, 53, 57, 62, 66, 70, 75, 79, 83,
		87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 127, 131, 135, 139, 143, 146, 150, 153, 157,
		160, 164, 167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 198, 201, 204, 206, 209, 211,
		214, 216, 219, 221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 241, 243, 244, 245,
		246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255 };

short irsinus(int grad) {
	short ret;
	int ag;
	grad %= 360;
	if (grad < 0)
		grad += 360;
	ag = grad % 180;
	if (ag > 90)
		ag = 180 - ag;
	ret = tbl_sinus[ag];
	if (grad > 180)
		ret = -ret;
	return ret;
}

typedef struct _int_bresenham {
	int ante, conseq;
	int ante_err, conseq_err;
} int_bresenham;

static inline byte dither_bresenham(int_bresenham *br) {
	byte ret = 0;
	if (br->ante_err < br->ante && br->conseq_err < br->conseq) {
		br->conseq_err += br->ante;
		br->ante_err += br->conseq;
	}
	if (br->ante <= br->ante_err) {
		++ret;
		br->ante_err -= br->ante;
	}
	if (br->conseq <= br->conseq_err) {
		ret |= 2;
		br->conseq_err -= br->conseq;
	}
	return ret;
}

int led = 13;

// the setup routine runs once when you press reset:
void setup() {
	// initialize the digital pin as an output.
	pinMode(led, OUTPUT);
	// initialize timer1
	noInterrupts();
	// disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	OCR1A = 1;            // compare match register 16MHz/256/2Hz
	TCCR1B |= (1 << WGM12);   // CTC mode
	TCCR1B |= (1 << CS10);    // 1 prescaler
	TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
	interrupts();
	// enable all interrupts
}

int current_grad = 1;
unsigned long next_turn = 0;

byte current_state = 0;
int_bresenham current_br = { 0, 0, 0, 0 };

void loop() {
	unsigned long time = millis();
//	if (time > next_turn) {
		++current_grad;
		if (current_grad >= 360)
			current_grad -= 360;
		short cs = irsinus(current_grad);
		if (cs < 0)
			cs = -cs;
		current_br.ante = cs;
		current_br.conseq = tbl_sinus_scale - cs;
		current_br.ante_err = current_br.conseq_err = 0;
		next_turn = time + 10;
//	}
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
	byte led_level = HIGH;
	if (current_state == 0) {
		current_state = dither_bresenham(&current_br);
	}
	if (current_state & 1) {
		led_level = PORTB | (HIGH << 5);
		--current_state;
	} else if (current_state & 2) {
		led_level = PORTB & ~(HIGH << 5);
		current_state -= 2;
	}
	PORTB = led_level;
//	digitalWrite(led, led_level);   // включаем LED
}

int main(void) {
	init();
	setup();
	//endless loop
	for (;;) {
		loop();
	}
	return 0;
}

