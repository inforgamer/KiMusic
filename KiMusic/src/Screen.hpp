#ifndef SCREEN_HPP
#define SCREEN_HPP

#include "conceito02.h"
#include "conceito01.h"
#include "p3.h"
#include "p5.h"

#include <TFT_eSPI.h>

class Screen{
    private:
    TFT_eSPI tft = TFT_eSPI();

    public:
        Screen() {}
        
        void init() 
        {
            tft.init();
            tft.setRotation(1);
            tft.setSwapBytes(true);
        tft.pushImage(0, 0, 320, 240, Conceito02);
        /*     
        tft.fillScreen(TFT_WHITE);

              //marcações
            tft.drawLine(10,0,10,30,TFT_BLACK);
            tft.drawFastHLine(0,120,320,TFT_BLACK);
            tft.drawFastVLine(160,0,240,TFT_BLACK);
            tft.drawFastVLine(10,140,18,TFT_GREEN);
            tft.drawFastVLine(10,158,10,TFT_BLUE);
            tft.drawFastVLine(10,168,35,TFT_RED);
            
            



            tft.fillCircle(110,84.5,50,TFT_BLUE);
            tft.fillRect(10,30,110,110,TFT_RED);


            tft.setCursor(10,150);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
            tft.setTextSize(1);
            tft.print("Nome da Musica");

            tft.setCursor(10,165);
            tft.setTextColor(TFT_BLACK, TFT_WHITE);
            tft.setTextSize(1);
            tft.print("Cantor");
    */
        }

};

#endif