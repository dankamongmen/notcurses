#ifndef __NCPP_NCKEY_HH
#define __NCPP_NCKEY_HH

#include <cstdint>
#include <notcurses/notcurses.h>

namespace ncpp
{
	struct NCKey
	{
		static constexpr char32_t Invalid   = NCKEY_INVALID;
		static constexpr char32_t Resize    = NCKEY_RESIZE;
		static constexpr char32_t Up        = NCKEY_UP;
		static constexpr char32_t Right     = NCKEY_RIGHT;
		static constexpr char32_t Down      = NCKEY_DOWN;
		static constexpr char32_t Left      = NCKEY_LEFT;
		static constexpr char32_t Ins       = NCKEY_INS;
		static constexpr char32_t Del       = NCKEY_DEL;
		static constexpr char32_t Backspace = NCKEY_BACKSPACE;
		static constexpr char32_t PgDown    = NCKEY_PGDOWN;
		static constexpr char32_t PgUp      = NCKEY_PGUP;
		static constexpr char32_t Home      = NCKEY_HOME;
		static constexpr char32_t End       = NCKEY_END;
		static constexpr char32_t F00       = NCKEY_F00;
		static constexpr char32_t F01       = NCKEY_F01;
		static constexpr char32_t F02       = NCKEY_F02;
		static constexpr char32_t F03       = NCKEY_F03;
		static constexpr char32_t F04       = NCKEY_F04;
		static constexpr char32_t F05       = NCKEY_F05;
		static constexpr char32_t F06       = NCKEY_F06;
		static constexpr char32_t F07       = NCKEY_F07;
		static constexpr char32_t F08       = NCKEY_F08;
		static constexpr char32_t F09       = NCKEY_F09;
		static constexpr char32_t F10       = NCKEY_F10;
		static constexpr char32_t F11       = NCKEY_F11;
		static constexpr char32_t F12       = NCKEY_F12;
		static constexpr char32_t F13       = NCKEY_F13;
		static constexpr char32_t F14       = NCKEY_F14;
		static constexpr char32_t F15       = NCKEY_F15;
		static constexpr char32_t F16       = NCKEY_F16;
		static constexpr char32_t F17       = NCKEY_F17;
		static constexpr char32_t F18       = NCKEY_F18;
		static constexpr char32_t F19       = NCKEY_F19;
		static constexpr char32_t F20       = NCKEY_F20;
		static constexpr char32_t F21       = NCKEY_F21;
		static constexpr char32_t F22       = NCKEY_F22;
		static constexpr char32_t F23       = NCKEY_F23;
		static constexpr char32_t F24       = NCKEY_F24;
		static constexpr char32_t F25       = NCKEY_F25;
		static constexpr char32_t F26       = NCKEY_F26;
		static constexpr char32_t F27       = NCKEY_F27;
		static constexpr char32_t F28       = NCKEY_F28;
		static constexpr char32_t F29       = NCKEY_F29;
		static constexpr char32_t F30       = NCKEY_F30;
		static constexpr char32_t F31       = NCKEY_F31;
		static constexpr char32_t F32       = NCKEY_F32;
		static constexpr char32_t F33       = NCKEY_F33;
		static constexpr char32_t F34       = NCKEY_F34;
		static constexpr char32_t F35       = NCKEY_F35;
		static constexpr char32_t F36       = NCKEY_F36;
		static constexpr char32_t F37       = NCKEY_F37;
		static constexpr char32_t F38       = NCKEY_F38;
		static constexpr char32_t F39       = NCKEY_F39;
		static constexpr char32_t F40       = NCKEY_F40;
		static constexpr char32_t F41       = NCKEY_F41;
		static constexpr char32_t F42       = NCKEY_F42;
		static constexpr char32_t F43       = NCKEY_F43;
		static constexpr char32_t F44       = NCKEY_F44;
		static constexpr char32_t F45       = NCKEY_F45;
		static constexpr char32_t F46       = NCKEY_F46;
		static constexpr char32_t F47       = NCKEY_F47;
		static constexpr char32_t F48       = NCKEY_F48;
		static constexpr char32_t F49       = NCKEY_F49;
		static constexpr char32_t F50       = NCKEY_F50;
		static constexpr char32_t F51       = NCKEY_F51;
		static constexpr char32_t F52       = NCKEY_F52;
		static constexpr char32_t F53       = NCKEY_F53;
		static constexpr char32_t F54       = NCKEY_F54;
		static constexpr char32_t F55       = NCKEY_F55;
		static constexpr char32_t F56       = NCKEY_F56;
		static constexpr char32_t F57       = NCKEY_F57;
		static constexpr char32_t F58       = NCKEY_F58;
		static constexpr char32_t F59       = NCKEY_F59;
		static constexpr char32_t F60       = NCKEY_F60;
		static constexpr char32_t Enter     = NCKEY_ENTER;
		static constexpr char32_t CLS       = NCKEY_CLS;
		static constexpr char32_t DLeft     = NCKEY_DLEFT;
		static constexpr char32_t DRight    = NCKEY_DRIGHT;
		static constexpr char32_t ULeft     = NCKEY_ULEFT;
		static constexpr char32_t URight    = NCKEY_URIGHT;
		static constexpr char32_t Center    = NCKEY_CENTER;
		static constexpr char32_t Begin     = NCKEY_BEGIN;
		static constexpr char32_t Cancel    = NCKEY_CANCEL;
		static constexpr char32_t Close     = NCKEY_CLOSE;
		static constexpr char32_t Command   = NCKEY_COMMAND;
		static constexpr char32_t Copy      = NCKEY_COPY;
		static constexpr char32_t Exit      = NCKEY_EXIT;
		static constexpr char32_t Print     = NCKEY_PRINT;
		static constexpr char32_t CapsLock  = NCKEY_CAPS_LOCK;
		static constexpr char32_t ScrollLock= NCKEY_SCROLL_LOCK;
		static constexpr char32_t NumLock   = NCKEY_NUM_LOCK;
		static constexpr char32_t PrintScreen= NCKEY_PRINT_SCREEN;
		static constexpr char32_t Pause     = NCKEY_PAUSE;
		static constexpr char32_t Menu      = NCKEY_MENU;
		static constexpr char32_t Refresh   = NCKEY_REFRESH;
		static constexpr char32_t Button1   = NCKEY_BUTTON1;
		static constexpr char32_t Button2   = NCKEY_BUTTON2;
		static constexpr char32_t Button3   = NCKEY_BUTTON3;
		static constexpr char32_t Button4   = NCKEY_BUTTON4;
		static constexpr char32_t Button5   = NCKEY_BUTTON5;
		static constexpr char32_t Button6   = NCKEY_BUTTON6;
		static constexpr char32_t Button7   = NCKEY_BUTTON7;
		static constexpr char32_t Button8   = NCKEY_BUTTON8;
		static constexpr char32_t Button9   = NCKEY_BUTTON9;
		static constexpr char32_t Button10  = NCKEY_BUTTON10;
		static constexpr char32_t Button11  = NCKEY_BUTTON11;
		static constexpr char32_t ScrollUp  = NCKEY_SCROLL_UP;
		static constexpr char32_t ScrollDown = NCKEY_SCROLL_DOWN;
		static constexpr char32_t Return    = NCKEY_RETURN;

		static bool IsMouse (char32_t ch) noexcept
		{
			return nckey_mouse_p (ch);
		}

		static bool IsSupPUAa (char32_t ch) noexcept
		{
			return nckey_supppuaa_p (ch);
		}

		static bool IsSupPUAb (char32_t ch) noexcept
		{
			return nckey_supppuab_p (ch);
		}
	};

	struct EvType
	{
		static constexpr ncintype_e Unknown = NCTYPE_UNKNOWN;
		static constexpr ncintype_e Press = NCTYPE_PRESS;
		static constexpr ncintype_e Repeat = NCTYPE_REPEAT;
		static constexpr ncintype_e Release = NCTYPE_RELEASE;
	};

}
#endif
