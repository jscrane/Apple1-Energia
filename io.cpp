#include <Stream.h>
#include <stdint.h>
#include <memory.h>
#include <tftdisplay.h>
#include <serialio.h>
#include <filer.h>
#include <keyboard.h>
#include <timed.h>

#include "pia.h"
#include "io.h"
#include "hardware.h"
#include "config.h"

#define ROWS	24
#define COLS	40
static unsigned r, c;
static char screen[ROWS][COLS];

void io::reset() {
	TFTDisplay::begin(TFT_BG, TFT_FG, TFT_ORIENT);
	clear();
	_cy += 2;

	r = c = 0;
	for (int j = 0; j < ROWS; j++)
		for (int i = 0; i < COLS; i++)
			screen[j][i] = ' ';

	_loading = false;
	pia::reset();
}

void io::load() {
	if (files.more()) {
		_loading = true;
		enter(files.read());
	}
}

// ascii map for scan-codes
static const uint8_t scanmap[] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 	// 0x00
	0xff, 0xff, 0xff, 0xff, 0xff, 0x09, 0x60, 0xff, 	// 0x08
	0xff, 0xff, 0xff, 0xff, 0xff, 0x51, 0x31, 0xff, 	// 0x10
	0xff, 0xff, 0x5a, 0x53, 0x41, 0x57, 0x32, 0xff, 	// 0x18
	0xff, 0x43, 0x58, 0x44, 0x45, 0x34, 0x33, 0xff, 	// 0x20
	0xff, 0x20, 0x56, 0x46, 0x54, 0x52, 0x35, 0xff, 	// 0x28
	0xff, 0x4e, 0x42, 0x48, 0x47, 0x59, 0x36, 0xff, 	// 0x30
	0xff, 0xff, 0x4d, 0x4a, 0x55, 0x37, 0x38, 0xff, 	// 0x38
	0xff, 0x2c, 0x4b, 0x49, 0x4f, 0x30, 0x39, 0xff, 	// 0x40
	0xff, 0x2e, 0x2f, 0x4c, 0x3b, 0x50, 0x2d, 0xff, 	// 0x48
	0xff, 0xff, 0x27, 0xff, 0x5b, 0x3d, 0xff, 0xff, 	// 0x50
	0xff, 0xff, 0x0d, 0x5d, 0xff, 0x23, 0xff, 0xff, 	// 0x58
	0xff, 0x5c, 0xff, 0xff, 0xff, 0xff, 0x5f, 0xff, 	// 0x60
	0xff, 0x31, 0xff, 0x34, 0x37, 0xff, 0xff, 0xff, 	// 0x68
	0x30, 0x08, 0x32, 0x35, 0x36, 0x38, 0x1b, 0xff, 	// 0x70
	0xff, 0x2b, 0x33, 0x2d, 0x2a, 0x39, 0xff, 0xff, 	// 0x78
};

static const uint8_t shiftmap[] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x00
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x08
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x21, 0xff,		// 0x10
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x22, 0xff,		// 0x18
	0xff, 0xff, 0xff, 0xff, 0xff, 0x24, 0x23, 0xff,		// 0x20
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x25, 0xff,		// 0x28
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x5e, 0xff,		// 0x30
	0xff, 0xff, 0xff, 0xff, 0xff, 0x26, 0x2a, 0xff,		// 0x38
	0xff, 0x3c, 0xff, 0xff, 0xff, 0x29, 0x28, 0xff,		// 0x40
	0xff, 0x3e, 0x3f, 0xff, 0x3a, 0xff, 0x5f, 0xff,		// 0x48
	0xff, 0xff, 0x40, 0xff, 0x7b, 0x2b, 0xff, 0xff,		// 0x50
	0xff, 0xff, 0xff, 0x7d, 0xff, 0x7e, 0xff, 0xff,		// 0x58
	0xff, 0x7c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x60
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x68
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x70
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,		// 0x78
};

void io::down(uint8_t scan) {
	set_porta(0);
	if (isshift(scan))
		_shift = true;
}

void io::enter(uint8_t key) {
	set_porta(key + 0x80);
}

void io::up(uint8_t scan) {
	if (isshift(scan)) {
		_shift = false;
		return;
	}
	enter(_shift? pgm_read_byte(shiftmap + scan): pgm_read_byte(scanmap + scan));
}

void io::draw(char ch, int i, int j) {
	if (screen[j][i] != ch) {
		screen[j][i] = ch;
		char c[2] = { ch, 0 };
		drawString(c, i*_cx, j*_cy);
	}
}

void io::display(uint8_t b) {
	char ch = (char)b;
	switch(ch) {
	case 0x5f:
		draw(' ', c, r);
		if (c-- == 0) {
			r--;
			c = COLS-1;
		}
		break;
	case 0x0d:
		draw(' ', c, r);
		c = 0;
		r++;
		break;
	default:
		if (ch >= 0x20 && ch < 0x7f) {
			draw(ch, c, r);
			if (++c == COLS) {
				c = 0;
				r++;
			}
		}
	}
	if (r == ROWS) {
		// scroll
		r--;
		for (int j = 0; j < (ROWS-1); j++)
			for (int i = 0; i < COLS; i++)
				draw(screen[j+1][i], i, j);
		for (int i = 0; i < COLS; i++)
			draw(' ', i, ROWS-1);
	}
	draw('_', c, r);
}

void io::write_portb(uint8_t b) {
	b &= 0x7f;
	display(b);
	pia::write_portb(b);
}

uint8_t io::read_porta_cr() {
	uint8_t b = pia::read_porta_cr();
	if (b != 0xa7)
		return b;

	if (_loading) {
		if (files.more())
			enter(files.read());
		else
			_loading = false;
	}
	return b;
}

void io::checkpoint(Stream &s) {
	pia::checkpoint(s);
	s.write(r);
	s.write(c);
	for (int j = 0; j < ROWS; j++)
		for (int i = 0; i < COLS; i++)
			s.write(screen[j][i]);
}

void io::restore(Stream &s) {
	pia::restore(s);
	r = s.read();
	c = s.read();
	for (int j = 0; j < ROWS; j++)
		for (int i = 0; i < COLS; i++) {
			char c = s.read();
			screen[j][i] = c;
			draw(c, i, j);
		}
	draw('_', c, r);
}
