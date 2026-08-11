#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
using namespace std;

//class Encoder{}

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
    uint volume;

    public:
        Player():
            centerButton(5),
            previewButton(8),
            nextButton(9),
            volumeButton(6),
            positionButton(7)
            {
                isPlay = false;
                volumeMode = false;
                positionMode = false;
                idle = true;
                volume = 50;
            };
    void control()
    {
    //self.dif = self.encoder.rotation()
    //if self.is_volume_mode and self.dif != 0:
    //self.set_volume()
    if(centerButton.checkClick())
        {
            isPlay = !isPlay;
            if(isPlay == true)
            {
                 printf("Play!\n");
            }
            else
            {
                 printf("Puase!\n");
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
            if(previewButton.checkClick() && idle == true)
            {
                idle = !idle;
                printf("Musica Anterior!\n");
                idle = !idle;
            }
            if(nextButton.checkClick() && idle == true)
            {
                idle = !idle;
                printf("Proxima misica!\n");
                idle = !idle;
            }
    };
  void setVolume()
    {
        volume = volume;
        if(volume >= 100)
        {
            volume = 100;
        }
        else if (volume <= 0)
        {
             volume = 0;
        }
    };  
    void setPosition()
    {
    
    };      
};


int main() {
    // 1. Liga o terminal USB
    stdio_init_all();

    // 2. Cria o Player (Acorda os botões)
    Player kiMusic;

    // 3. O Loop Principal
    while (true) {
        kiMusic.control();
        sleep_ms(10);
    }
}
