/*
  LED_Matrix.cpp
  2013 Copyright (c) Seeed Technology Inc.  All right reserved.

  Author:Loovee
  2013-10-21
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <Streaming.h>
#include <MsTimer2.h>

#include "LED_Matrix.h"
#include "font.h"


// 2-dimensional array of row pin numbers:
const int row[8] = {
2,7,19,5,13,18,12,16 };

// 2-dimensional array of column pin numbers:
const int col[8] = {
6,11,10,3,17,4,8,9  };


#define __BIT(n)    (0x01<<n)


#if _USE_TIMER_
void timerIsr()
{
    matrix.timer_();
}
#endif

// if no use timer, this function should be used in main() per 1ms
void LED_Matrix::timer_()
{
    for(int i=0; i<8; i++)
    {
        digitalWrite(row[i], LOW);
    }
    
    matrix.set_n(~matrix.disp_dta[7-matrix.ctrl_bit]);
    matrix.set_p(BIT(matrix.ctrl_bit++));
    matrix.ctrl_bit = matrix.ctrl_bit>7 ? 0 : matrix.ctrl_bit;
}

void LED_Matrix::begin()
{
    ctrl_bit = 0;
    cmd_get  = 0;
    dirDisp  = DIR_NORMAL;

    clear();
    
    io_init();
    
#if _USE_TIMER_
    //Timer1.initialize(1000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
    //Timer1.attachInterrupt( timerIsr ); // attach the service routine here
    
    MsTimer2::set(1, timerIsr); // 500ms period
    MsTimer2::start();
#endif  
}

void LED_Matrix::setDispDta(uchar *dta)
{

    if(NULL == dta)return;
    
    for(int i=0; i<8; i++)
    {
        disp_dta[i] = dta[i];
    }
}

void LED_Matrix::io_init()
{
    for(int i=0; i<8; i++)
    {
        pinMode(row[i], OUTPUT);
        pinMode(col[i], OUTPUT);
    }
}

void LED_Matrix::set_n(uchar dta)
{
    for(int i=0; i<8; i++)
    {
        if(dta & __BIT(i))
        {
            digitalWrite(col[i], HIGH);
        }
        else
        {
            digitalWrite(col[i], LOW);
        }
    }
    
}

void LED_Matrix::set_p(uchar dta)
{

    for(int i=0; i<8; i++)
    {
        if(dta & __BIT(i))
        {
            digitalWrite(row[i], HIGH);
        }
        else
        {
            digitalWrite(row[i], LOW);
        }
    }
}

void LED_Matrix::getMatrix(uchar *matrix, char asc)
{
    if(NULL == matrix)return;
    
    for(int i=0; i<7; i++)
    {
        int tmp = (asc-32)*7+i;
        matrix[i] = pgm_read_byte(&Font5x7[tmp]); 
        matrix[i] = matrix[i] << 1;
    }
    
    matrix[7] = 0x00;
}

void LED_Matrix::putIntMatrix(unsigned int *matrix)
{
    unsigned char mat[8];
    for(int i=0; i<8; i++)
    {
        mat[i] = matrix[i]>>8;
    }
    setDispDta(mat);  
    if(DIR_DOWN != dirDisp)return ;
    matrixRev();

}

void LED_Matrix::dispChar(char c)
{
    if(DIR_NORMAL != dirDisp && DIR_DOWN != dirDisp)return ;
    getMatrix(disp_dta, c);
    setDispDta(disp_dta);
    if(DIR_DOWN != dirDisp)return ;
    matrixRev();
}

void LED_Matrix::dispStringSlide(uchar cycle, int ts, int len_, char *str)
{
    if(DIR_NORMAL != dirDisp && DIR_DOWN != dirDisp)return ;
    
    cmd_get = 0;
    while(1)
    {

        int len = len_;
        
        if(len > 20)return ;
        
        unsigned char matrix[154];
        unsigned int matrix_i[7];
        
        memset(matrix, 0, 147);
        
        for(int i=0; i<len; i++)
        {
            getMatrix(&matrix[7*(i+1)], str[i]);
        }
        
        len++;
        
        for(int i=0; i<len; i++)
        {
            for(int j=0; j<7; j++)
            {
                matrix_i[j] = matrix[7*i+j];
                matrix_i[j] <<= 8;
                matrix_i[j] += matrix[7*(i+1)+j]<<2;
            }
            
            for(int k=0; k<6; k++)
            {
                putIntMatrix(matrix_i);
                
                if(cmd_get)
                {
                    cmd_get = 0;
                    return ;
                }
                delay(ts/5);
                
                for(int m=0; m<7; m++)
                {
                    matrix_i[m] <<= 1;
                }
            }
        } 
        
        if (STR_ONCE == cycle)return;
    }
}

uchar LED_Matrix::byteRev(uchar dta)
{
    uchar tmp = 0;  

    for(int i=0; i<5; i++)
    {
        if(dta & BIT(i))
        {
            tmp += BIT(4-i);
        }
    }
    return tmp;
}

void LED_Matrix::matrixRev()
{

    uchar mat_tmp[8];
    
    for(int i=0; i<8; i++)
    {
        mat_tmp[i] = disp_dta[i];
    }

    for(int i=0; i<8; i++)
    {
        disp_dta[i] = byteRev(mat_tmp[7-i]);
    }
}

void LED_Matrix::dispMatrix(uchar *mat)
{

    if(DIR_DOWN == dirDisp)
    {
        for(int i=0; i<8; i++)
        {
            disp_dta[i] = mat[i];
        }
        
        matrixRev();
    }
    
    else if(DIR_NORMAL == dirDisp)
    {
        for(int i=0; i<8; i++)
        {
            disp_dta[i] = mat[i];
        }
    }
    else if(DIR_RIGHT == dirDisp)
    {
        for(int i=0; i<8; i++)
        {
            for(int j=0; j<8; j++)
            {
                if(mat[j] & BIT(i))
                {
                    disp_dta[7-i] += BIT(j);
                }
            }
        }
    }
    else if(DIR_LEFT == dirDisp)
    {
        for(int i=0; i<8; i++)
        {
            for(int j=0; j<8; j++)
            {
                if(mat[j] & BIT(i))
                {
                    disp_dta[7-i] += BIT(j);
                }
            }
        }
        
        matrixRev();
    } 
    
}

void LED_Matrix::setPoint(uchar x, uchar y, uchar state)
{

    if(DIR_DOWN == dirDisp)
    {
        disp_dta[7-y] &= ~BIT(x);
        disp_dta[7-y] |= state ? BIT(x) : 0;
    }
    else if(DIR_NORMAL == dirDisp)
    {
        disp_dta[y] &= ~BIT(4-x);
        disp_dta[y] |= state ? BIT(4-x) : 0;
    }
    else if(DIR_RIGHT == dirDisp)
    {
        //disp_dta[x] &= ~BIT(7-y);
        //disp_dta[x] |= state ? BIT(7-y) : 0;
    }
    else if(DIR_LEFT == dirDisp)
    {
        //disp_dta[7-x] &= ~BIT(y);
        //disp_dta[7-x] |= state ? BIT(y) : 0;
    } 
}

LED_Matrix matrix;


/*********************************************************************************************************
  END FILE
*********************************************************************************************************/