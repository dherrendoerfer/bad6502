#include <SDL.h>

uint8_t get_kbd_key(SDL_Scancode scancode)
{
	// The returned value is 0x<row><column> of the vic-20 kbd matrix
	switch (scancode) {
		case SDLK_BACKSPACE:
			return 0x07;
		case SDLK_RETURN:
			return 0x17;
		case SDLK_RIGHT:
			return 0x27;
		case SDLK_DOWN:
			return 0x37;
		case SDLK_F1:
			return 0x47;
		case SDLK_F3:
			return 0x57;
		case SDLK_F5:
			return 0x67;
		case SDLK_F7:
			return 0x77;

//		case SDLK_ASTERISK:
//			return 0x06;
		case SDLK_RIGHTBRACKET:
			return 0x16;
		case SDLK_SEMICOLON:
			return 0x26;
		case SDLK_SLASH:
			return 0x36;
		case SDLK_RSHIFT:
			return 0x46;
		case SDLK_QUOTE:
			return 0x56;
//		case SDLK_RIGHTBRACKET:
//			return 0x66;
		case SDLK_HOME:
			return 0x76;


		case SDLK_EQUALS:
			return 0x05;
		case SDLK_p:
			return 0x15;
		case SDLK_l:
			return 0x25;
		case SDLK_COMMA:
			return 0x35;
		case SDLK_PERIOD:
			return 0x45;
//		case SDLK_COLON:
//			return 0x55;
		case SDLK_LEFTBRACKET:
			return 0x65;
		case SDLK_MINUS:
			return 0x75;


		case SDLK_9:
			return 0x04;
		case SDLK_i:
			return 0x14;
		case SDLK_j:
			return 0x24;
		case SDLK_n:
			return 0x34;
		case SDLK_m:
			return 0x44;
		case SDLK_k:
			return 0x54;
		case SDLK_o:
			return 0x64;
		case SDLK_0:
			return 0x74;

		case SDLK_7:
			return 0x03;
		case SDLK_y:
			return 0x13;
		case SDLK_g:
			return 0x23;
		case SDLK_v:
			return 0x33;
		case SDLK_b:
			return 0x43;
		case SDLK_h:
			return 0x53;
		case SDLK_u:
			return 0x63;
		case SDLK_8:
			return 0x73;

		case SDLK_5:
			return 0x02;
		case SDLK_r:
			return 0x12;
		case SDLK_d:
			return 0x22;
		case SDLK_x:
			return 0x32;
		case SDLK_c:
			return 0x42;
		case SDLK_f:
			return 0x52;
		case SDLK_t:
			return 0x62;
		case SDLK_6:
			return 0x72;

		case SDLK_3:
			return 0x01;
		case SDLK_w:
			return 0x11;
		case SDLK_a:
			return 0x21;
		case SDLK_LSHIFT:
			return 0x31;
		case SDLK_z:
			return 0x41;
		case SDLK_s:
			return 0x51;
		case SDLK_e:
			return 0x61;
		case SDLK_4:
			return 0x71;

		case SDLK_1:
			return 0x00;
		case SDLK_BACKQUOTE:
			return 0x10;
		case SDLK_TAB:
			return 0x20;
		case SDLK_ESCAPE:
			return 0x30;
		case SDLK_SPACE:
			return 0x40;
		case SDLK_LCTRL:
			return 0x50;
		case SDLK_q:
			return 0x60;
		case SDLK_2:
			return 0x70;
		default:
			return 0xFF;
	}
}

