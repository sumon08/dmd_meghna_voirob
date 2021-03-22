


#include "STM32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dmd.h"
#include "dmd_cnfg.h"
//#include "arial15.h"
#include "STM32f10x_gpio.h"
#include "STM32f10x_rcc.h"

void DisplayScanner();
Dmd prv_dmd;
DmdHandle InitDmdModule(uint8_t panelsHigh, uint8_t panelsWide)
{
	Dmd * dmd = (Dmd *)&prv_dmd;
	dmd->DisplaysWide=panelsWide;
	dmd->DisplaysHigh=panelsHigh;
	dmd->DisplaysTotal=dmd->DisplaysWide*dmd->DisplaysHigh;
	dmd->MaxXCoOrdinate = 32 * dmd->DisplaysWide - 1;
	dmd->MaxYCoOrdinate = 16 * dmd->DisplaysHigh - 1;

	dmd->bDMDchar = 0;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef config;
	config.GPIO_Mode = GPIO_Mode_Out_PP;
	config.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
	config.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &config);
	dmd->bDMDchar = 0;
	dmd->connector_address_a_pin 	= InitPin(CONNECTOR1_A_PORT, CONNECTOR1_A_PIN, OUTPUT);
	dmd->connector_address_b_pin 	= InitPin(CONNECTOR1_B_PORT, CONNECTOR1_B_PIN, OUTPUT);
	dmd->connector_oe_pin 			= InitPin(CONNECTOR1_OE_PORT, CONNECTOR1_OE_PIN, OUTPUT);
	dmd->connector_strobe_pin 		= InitPin(CONNECTOR1_STROBE_PORT, CONNECTOR1_STROBE_PIN, OUTPUT);
	dmd->connector_sck_pin 			= InitPin(CONNECTOR1_SCK_PORT, CONNECTOR1_SCK_PIN, OUTPUT);
	ClearScreen(dmd);
	FlushScreen(dmd);
//	printf("Dmd Initialisation finished.\r\n");
	return dmd;

}




void ClearScreen(DmdHandle dmd)
{
	Dmd * _dmd = (Dmd *) dmd;
	memset((void *)_dmd->bDMDScreenRAM, 0xFF, 1024);

}

void FlushScreen(DmdHandle dmd)
{
	Dmd *_dmd = (Dmd *) dmd;
	memcpy(_dmd->bDMDScreenRAMb, _dmd->bDMDScreenRAM,1024);


}



/*--------------------------------------------------------------------------------------
 Set or clear a pixel at the x and y location (0,0 is the top left corner)
--------------------------------------------------------------------------------------*/
void writePixel(DmdHandle handle, unsigned int bX, unsigned int bY, char bGraphicsMode, char bPixel)
{
	if (handle==0)
	{
		return;
	}

	Dmd * dmd = (Dmd *) handle;


	if (bX > dmd->MaxXCoOrdinate || bY > dmd->MaxYCoOrdinate)
	{
		return;
	}


	uint8_t bit_position = /*bX / 32 == 0? (3 + (bY / 16)):*/(bY / 16);

// 	bX = bX - (bX/32) * 32;
// 	bY = bY - (bY/16) * 16;
	int array_ind = bY % 4;

	bY = bY % 16;
	bY = 15 - bY;

// 	if (bit_position == 1)
// 	{
// 		bit_position = 3;


	int array_index = (bX / 8) * 32 + (bY / 4) * 8 + (bX % 8);

	switch (bGraphicsMode) {
		case GRAPHICS_NORMAL:
			if (bPixel == TRUE)
				dmd->bDMDScreenRAM[array_ind][array_index] &= ~(1 << bit_position);	// zero bit is pixel on
			else
				dmd->bDMDScreenRAM[array_ind][array_index] |= (1 << bit_position);	// one bit is pixel off
		break;
		case GRAPHICS_INVERSE:
			if (bPixel == FALSE)
				dmd->bDMDScreenRAM[array_ind][array_index] &= ~(1 << bit_position);	// zero bit is pixel on
			else
				dmd->bDMDScreenRAM[array_ind][array_index] |= (1 << bit_position);;	// one bit is pixel off
		break;
	}


}







void DisplayScanner()
{
	Dmd * dmd = (Dmd *) &prv_dmd;
//	for (int i = 0; i < 512; i++) {
//		//SetPinState(dmd->connector_strobe_pin, LOW);
//		SetPinState(dmd->connector_sck_pin, LOW);
//		GPIOA->ODR = dmd->prvScreen[dmd->bChars][i];
//		//__NOP();
//		//__NOP();
//		SetPinState(dmd->connector_sck_pin, HIGH);
//		//GPIOA->ODR = 0;
//		//__NOP();
//		//__NOP();
//	}



//	SetPinState(dmd->connector_oe_pin, LOW);

	SetPinState(dmd->connector_strobe_pin,HIGH );




	switch(dmd->bDMDchar)
	{
		case 0:
			dmd->bDMDchar = 1;
			SetPinState(dmd->connector_address_a_pin, LOW);
			SetPinState(dmd->connector_address_b_pin, LOW);

			break;
		case 1:
			SetPinState(dmd->connector_address_a_pin, HIGH);
			SetPinState(dmd->connector_address_b_pin, LOW);

			dmd->bDMDchar = 2;
			break;
		case 2:
			SetPinState(dmd->connector_address_a_pin, LOW);
			SetPinState(dmd->connector_address_b_pin, HIGH);
			dmd->bDMDchar = 3;
			break;
		case 3:
			dmd->bDMDchar = 0;
			SetPinState(dmd->connector_address_a_pin, HIGH);
			SetPinState(dmd->connector_address_b_pin, HIGH);
			break;
		default:
			break;
	}

//	SetPinState(dmd->connector_oe_pin, HIGH);
	SetPinState(dmd->connector_strobe_pin, LOW);
}


uint32_t GetBaseAddress()
{
	return (uint32_t)prv_dmd.bDMDScreenRAMb[prv_dmd.bDMDchar];
}

//void TIM2_IRQHandler(void)
//{
//  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
//  {
//    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
//    DisplayScanner();
//  }
//}


void drawString(DmdHandle handle, int bX, int bY, const char* bChars, char length,
		     char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    uint8_t height = FontHeight();
    if (bY+height<0) return;

    int strWidth = 0;
	//drawLine(handle,bX -1 , bY, bX -1 , bY + height, GRAPHICS_INVERSE);

    for (int i = 0; i < length; i++) {
        int charWide = drawChar(handle, bX+strWidth, bY, bChars[i], bGraphicsMode);
	    if (charWide > 0) {
	        strWidth += charWide ;
	        //drawLine(handle,bX + strWidth , bY, bX + strWidth , bY + height, GRAPHICS_NORMAL);
            strWidth++;
        } else if (charWide < 0) {
            return;
        }
        //if ((bX + strWidth) >= DMD_PIXELS_ACROSS * dmd->DisplaysWide || bY >= DMD_PIXELS_DOWN * dmd->DisplaysHigh) return;
    }
}


/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
--------------------------------------------------------------------------------------*/
void drawLine(DmdHandle handle, int x1, int y1, int x2, int y2, char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;

    if (dy < 0) {
	    dy = -dy;
	    stepy = -1;
    } else {
	    stepy = 1;
    }
    if (dx < 0) {
	    dx = -dx;
	    stepx = -1;
    } else {
	    stepx = 1;
    }
    dy <<= 1;			// dy is now 2*dy
    dx <<= 1;			// dx is now 2*dx

    writePixel(handle, x1, y1, bGraphicsMode, TRUE);
    if (dx > dy) {
	    int fraction = dy - (dx >> 1);	// same as 2*dy - dx
	    while (x1 != x2) {
	        if (fraction >= 0) {
		        y1 += stepy;
		        fraction -= dx;	// same as fraction -= 2*dx
	        }
	        x1 += stepx;
	        fraction += dy;	// same as fraction -= 2*dy
	        writePixel(handle, x1, y1, bGraphicsMode, TRUE);
	    }
    } else {
	    int fraction = dx - (dy >> 1);
	    while (y1 != y2) {
	        if (fraction >= 0) {
		        x1 += stepx;
		        fraction -= dy;
	        }
	        y1 += stepy;
	        fraction += dx;
	        writePixel(handle, x1, y1, bGraphicsMode, TRUE);
	    }
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
--------------------------------------------------------------------------------------*/
void drawCircle(DmdHandle handle, int xCenter, int yCenter, int radius,
		     char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;

    drawCircleSub(handle, xCenter, yCenter, x, y, bGraphicsMode);
    while (x < y) {
	    x++;
	    if (p < 0) {
	        p += 2 * x + 1;
	    } else {
	        y--;
	        p += 2 * (x - y) + 1;
	    }
	    drawCircleSub(handle, xCenter, yCenter, x, y, bGraphicsMode);
    }
}

void drawCircleSub(DmdHandle handle, int cx, int cy, int x, int y, char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    if (x == 0) {
	    writePixel(handle, cx, cy + y, bGraphicsMode, TRUE);
	    writePixel(handle, cx, cy - y, bGraphicsMode, TRUE);
	    writePixel(handle, cx + y, cy, bGraphicsMode, TRUE);
	    writePixel(handle, cx - y, cy, bGraphicsMode, TRUE);
    } else if (x == y) {
	    writePixel(handle, cx + x, cy + y, bGraphicsMode, TRUE);
	    writePixel(handle, cx - x, cy + y, bGraphicsMode, TRUE);
	    writePixel(handle, cx + x, cy - y, bGraphicsMode, TRUE);
	    writePixel(handle, cx - x, cy - y, bGraphicsMode, TRUE);
    } else if (x < y) {
	    writePixel(handle, cx + x, cy + y, bGraphicsMode, TRUE);
	    writePixel(handle, cx - x, cy + y, bGraphicsMode, TRUE);
	    writePixel(handle, cx + x, cy - y, bGraphicsMode, TRUE);
	    writePixel(handle, cx - x, cy - y, bGraphicsMode, TRUE);
	    writePixel(handle, cx + y, cy + x, bGraphicsMode, TRUE);
	    writePixel(handle, cx - y, cy + x, bGraphicsMode, TRUE);
	    writePixel(handle, cx + y, cy - x, bGraphicsMode, TRUE);
	    writePixel(handle, cx - y, cy - x, bGraphicsMode, TRUE);
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void drawBox(DmdHandle handle, int x1, int y1, int x2, int y2, char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    drawLine(handle, x1, y1, x2, y1, bGraphicsMode);
    drawLine(handle, x2, y1, x2, y2, bGraphicsMode);
    drawLine(handle, x2, y2, x1, y2, bGraphicsMode);
    drawLine(handle, x1, y2, x1, y1, bGraphicsMode);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void drawFilledBox(DmdHandle handle, int x1, int y1, int x2, int y2,
			char bGraphicsMode)
{
	if (handle==0)
	{
		return;
	}

    for (int b = x1; b <= x2; b++) {
	    drawLine(handle, b, y1, b, y2, bGraphicsMode);
    }
}




void SelectFont(DmdHandle handle, const uint8_t * const * font)
{
	if (handle==0)
	{
		return;
	}
	Dmd * dmd = (Dmd *)handle;
    dmd->Font = font;
}

int drawChar(DmdHandle handle, const int bX, const int bY, const char letter, char bGraphicsMode)
{
	if (handle==0)
		{
			return 0;
		}
		Dmd * dmd = (Dmd *)handle;
	    if (bX > dmd->MaxXCoOrdinate || bY > dmd->MaxYCoOrdinate ) return -1;
	    char c = letter;
	    uint8_t height = FontHeight();
	    if (c == ' ') {
		    int charWide = charWidth(handle,' ');
		    drawFilledBox(handle, bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
		    return charWide;
	    }

	    const uint8_t * char_template = dmd->Font[c-32];
	    const uint8_t width = char_template[1];

		for(int i = 0; i < width; i++)
		{
			if (height > 8)
			{
				uint16_t data = char_template[2*i + 3];
				data = data << 8;
				data |= char_template[2*i + 2];
				//i++;

				for (int j = 0; j < height; j ++)
				{
					if (data & (1 << j))
					{
						writePixel(handle, bX + i, bY + j, bGraphicsMode, TRUE);
					}
					else
					{
						writePixel(handle, bX + i, bY + j, bGraphicsMode, FALSE);
					}
				}
			}
			else
			{
				uint16_t data = char_template[i+2];
				for (int j = 0; j < height; j ++)
				{
					if (data & (1 << j))
					{
						writePixel(handle, bX + i, bY + j, bGraphicsMode, TRUE);
					}
					else
					{
						writePixel(handle, bX + i, bY + j, bGraphicsMode, FALSE);
					}
				}
			}
		}

	    return width;
}

int charWidth(DmdHandle handle, const char letter)
{
	if (handle==0)
		{
			return 0;
		}
		Dmd * dmd = (Dmd *)handle;
	    char c = letter;
	    // Space is often not included in font so use width of 'n'
	//    if (c == ' ') c = 'n';
	    uint8_t width = 0;

		if (c < 32 || c > 127)
		{
			return width;
		}
	    //Read the Font template array......
		const uint8_t * char_template = dmd->Font[c-32];
		width = char_template[1];
	    return width;
}


int StringWidth(char * str)
{
	int length = 0;

	while(*str != 0)
	{
		length += charWidth(&prv_dmd, *str);
		str++;
		length++;
	}
	return length;
}


uint8_t FontHeight()
{
	const uint8_t * char_template = prv_dmd.Font[0];
	uint8_t height = char_template[0];
	return height;
}
