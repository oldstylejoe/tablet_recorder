//Joe Snider
//1/16
//
//This structure allows decoding some of the data coming from the touch screen.
//It's only partial since I have enough information to get position, touch id, etc.
//
//to use catch the WM_INPUT event with:
//usUsagePage = 13;
//usUsage = 4;
//dwFlags = RIDEV_INPUTSINK;
//hwndTarget = m_hWnd;
//
//Then read the raw data and cast like:
//  UINT dwSize;
//  GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
//  LPBYTE lpb = new BYTE[dwSize];
//  GetRawInputData(hRawInput, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
//  RAWINPUT* raw = (RAWINPUT*)lpb;
//  for(DWORD index(0); index < raw->data.hid.dwCount; ++index) {
//    TD* result = (TD*)&raw->data.hid.bRawData[raw->data.hid.dwSizeHid * index]
//    //access result as needed
//  }
//  

#ifndef SURFACE_TOUCH_SCREEN
#define SURFACE_TOUCH_SCREEN

#include <Windows.h>

namespace SurfaceTouchScreen {

	//struct that the surface HID touchscreen returns.
	//First 14 bytes are straightforward related to the touch duration, number, and position.
	//I don't care too much about the rest. I'd expect pressure and touch area to be there somewhere.
	//The bit sizes are not necessary, as it turns out.
	//other 37 are stored in the rest variable:
	// rest[0] -- ? number varies, not sure how
	// rest[1] -- something about the number of fingers?
	// rest[2] -- ? number varies, not sure how
	// rest[3] -- something about the number of fingers?
	// rest[4] -- 255
	// rest[5] -- 255
	// rest[6] -- 255
	// rest[7] -- 255
	// rest[8] -- ? 
	// rest[9] -- ?
	// rest[10] -- ?
	// rest[11] -- ?
	// rest[12] -- ?
	// rest[13] -- 253
	// rest[14] -- 253
	// rest[15] -- 253
	// rest[16] -- 253
	// rest[17] -- 0?
	// rest[18]
	// rest[19]
	// rest[20]
	// rest[21] -- 0?
	// rest[22]
	// rest[23] -- 0?
	// rest[24]
	// rest[25]
	// rest[26]
	// rest[27]
	// rest[28]
	// rest[29]
	// rest[30]
	// rest[31]
	// rest[32]
	// rest[33]
	// rest[34]
	// rest[35]
	// rest[36]
	struct TD {

		BYTE status1 : 8;        //always 3, the type, I guess
		BYTE durationLow : 8;    //little endian duration (use function to decode)
		BYTE durationHigh : 8;
		BYTE touchType : 8;      //231 == down, 228 == up (maybe offset?)
		BYTE touchNumber : 8;    //this tracks each touch event, even if they go up and down out of order
		BYTE status2 : 8;        //always 1?
		BYTE xLow : 8;        //x position low bit
		BYTE xHigh : 8;        //x position high bit
		BYTE xLow2 : 8;        //repeated measure of x low and high?
		BYTE xHigh2 : 8;
		BYTE yLow : 8;        //y position low bit
		BYTE yHigh : 8;        //y position high bit
		BYTE yLow2 : 8;        //repeated measure of y low and high?
		BYTE yHigh2 : 8;
		BYTE rest[37];


		//Duration is tracked as sample number on a per finger basis.
		//I don't know what the sample rate is (TODO: time it)
		int GetDuration() const {
			return joinBytes(durationLow, durationHigh);
		}

		//the x position (units? maybe 0-9600, left to right)
		int X() const {
			return joinBytes(xLow, xHigh);
		}
		//I don't know why there are two x's
		int X2() const {
			return joinBytes(xLow2, xHigh2);
		}
		//the y position (units? maybe 0-7200, top to bottom)
		int Y() const {
			return joinBytes(yLow, yHigh);
		}
		//I don't know why there are two y's
		int Y2() const {
			return joinBytes(yLow2, yHigh2);
		}

	private:
		int joinBytes(const BYTE& low, const BYTE& high) const {
			int ret = high;
			ret <<= 8;
			ret |= low;
			return ret;
		}

	};

}

#endif// SURFACE_TOUCH_SCREEN
