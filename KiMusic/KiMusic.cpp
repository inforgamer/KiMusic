#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"

using namespace std;

class Encoder
{
    private:
    uint pinA = 2;
    uint pinB = 3;
    int lastState;

public:
    Encoder() {
    gpio_init(pinA);
    gpio_set_dir(pinA, GPIO_IN);
    gpio_pull_up(pinA);

    gpio_init(pinB);
    gpio_set_dir(pinB, GPIO_IN);
    gpio_pull_up(pinB);

    lastState = gpio_get(pinA);
    }

    int readRotate() {
        int stateA = gpio_get(this->pinA);
        int stateB = gpio_get(this->pinB);
        int dir = 0;
        if(stateA != this->lastState)
        {
            if (stateA == 0)
            {
            if(stateB != stateA)
            {
                dir = 1;
            }
           else
            {
                dir = -1;
            }
        }
            this->lastState = stateA;

            return dir;
        }
        return 0;
    }
};

class Button
{
    private:
    uint pinNumber;
    bool isPressed;

    public:
        Button(uint pinNumber)
        {
            this->pinNumber = pinNumber;
            isPressed = false;
            gpio_init(this->pinNumber);
            gpio_set_dir(this->pinNumber, GPIO_IN);
            gpio_pull_up(this->pinNumber);
        };

   bool checkClick()
   {
    if(gpio_get(this->pinNumber) == 0 && isPressed == false)
    {
        isPressed = true;
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
    Encoder encoder;
    int dif;

    public:
        Player():
            encoder(),
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
        this->dif = this->encoder.readRotate();
        if(volumeMode == true && this->dif != 0 )
        {
            setVolume(this->dif);
        }
        if(positionMode == true && this->dif != 0)
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
    sleep_ms(50);
    printf("KiMusic Iniciado...\n");
    
    while (true) 
    {
        kiMusic.control();    
    }
}