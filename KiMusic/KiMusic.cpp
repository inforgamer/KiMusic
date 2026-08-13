#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "../libs/quadrature.pio.h"

using namespace std;

/*class Encoder
{
    private:
     uint pinClk;
     uint pinDt;
     PIO pio;
     uint sm;
     int valOld;


    public:
    Encoder()
    {
        this-> valOld = 0;
        this->pinClk = 2;
        this->pinDt = 3;
        this->pio = pio0;
        
        uint offset = pio_add_program(this->pio, &quadratureA_program);
        this->sm = pio_claim_unused_sm(this->pio, true);
        quadratureA_program_init(this->pio,this->sm, offset, this->pinClk, this->pinDt);
        pio_sm_set_enabled(this->pio, this->sm, true);
    };

   int readRotate()
    {
        pio_sm_clear_fifos(this->pio, this->sm);
        
        pio_sm_exec(this->pio, this->sm, pio_encode_in(pio_x, 32));
        pio_sm_exec(this->pio, this->sm, pio_encode_push(false, false));
        
        uint x = pio_sm_get_blocking(this->pio, this->sm); 
        
        int valNew = (int)x;
        
        if (this->valOld != valNew)
        {
            int dif = valNew - this->valOld;
            this->valOld = valNew;
            printf("Giro do Encoder: %d (Diferenca: %d)\n", valNew, dif);
            
            return dif;
        }
        
        return 0;
    };
};*/

class Button
{
    private:
    uint pinNumber;
    bool isPressed;
    bool isDo;

    public:
        Button(uint pinNumber)
        {
            this->pinNumber = pinNumber;
            isPressed = false;
            isDo = false;

            gpio_init(this->pinNumber);
            gpio_set_dir(this->pinNumber, GPIO_IN);
            gpio_pull_up(this->pinNumber);
        };

   bool checkClick()
   {
    if(gpio_get(this->pinNumber) == 0 && isPressed == false)
    {
        isPressed = true;
        isDo = !isDo;
        sleep_ms(100);
        return true;
    };
    if(gpio_get(this->pinNumber) == 1 && isPressed == true)
    {
        isPressed = false;
        sleep_ms(100);
    };
    return false;
   };
};

class Player
{
    private:
    Button centerButton;
    Button previewButton;
    Button nextButton;
    Button volumeButton;
    Button positionButton;
    bool isPlay;
    bool volumeMode;
    bool positionMode;
    bool idle;
    int volume; 
   // Encoder encoder;
    int dif;

    public:
        Player():
          //  encoder(),
            centerButton(5),
            previewButton(8),
            nextButton(9),
            volumeButton(6),
            positionButton(7)
            {
                this-> dif = 0;
                isPlay = false;
                volumeMode = false;
                positionMode = false;
                idle = true;
                volume = 50;
            };
            
    void control()
    {
       // this->dif = this->encoder.readRotate();
        
        if(volumeMode == true && dif != 0 )
        {
            setVolume(this->dif);
        }
        if(positionMode == true && dif != 0)
        {
            setPosition(this->dif);
        }
        if(centerButton.checkClick())
        {
            isPlay = !isPlay;
            if(isPlay == true)
            {
                 printf("Play!\n");
            }
            else
            {
                 printf("Pause!\n");
            }
        }
        else if (positionButton.checkClick() && volumeMode == false)
        {
            positionMode = !positionMode;
            if (positionMode == true)
            {
                printf("Modo Posicao!\n");
                idle = false;
            }
            else
            {
                printf("Modo Normal!\n");
                idle = true;
            }
        }
        else if(volumeButton.checkClick() && positionMode == false)
        {
            volumeMode = !volumeMode;
            if(volumeMode == true)
            {
                printf("Modo Volume!\n");
                idle = false;
            }
            else
            {
                printf("Modo Normal!\n");
                idle = true;
            }
        }
        else if(previewButton.checkClick() && idle == true)
        {
            idle = !idle;
            printf("Musica Anterior!\n");
            idle = !idle;
        }
        else if(nextButton.checkClick() && idle == true)
        {
            idle = !idle;
            printf("Proxima musica!\n");
            idle = !idle;
        }
    };
    
    void setVolume(int delta)
    {
        this->volume += delta;
        if(this->volume > 100)
        {
            this->volume = 100;
        }
        else if (this->volume <= 0)
        {
             this->volume = 0;
        }
        printf("Volume atual: %d\n", this->volume);
    };  
    
    void setPosition(int position)
    {
     // avançar tempo da musica referente a rotação
    };      
};

int main() {
    stdio_init_all();
    sleep_ms(1000); 

    Player kiMusic;
    printf("KiMusic Iniciado...\n");

    while (true) {
        kiMusic.control();    
        sleep_ms(10);
    }
}