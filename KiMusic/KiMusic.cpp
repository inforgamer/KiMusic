#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"

#include "../libs/QuadratureEncoder.hpp"
using namespace std;

class Encoder
{
    private:
    QuadratureEncoder* encoder_matteo;
    int32_t ultima_posicao;
    float tempo_amostragem = 0.01f;

public:
    Encoder() {
        encoder_matteo = new QuadratureEncoder(2, 24.0f);
        ultima_posicao = 0;
    }

    int readRotate() {
        encoder_matteo->update(tempo_amostragem);
        
        int32_t posicao_atual = encoder_matteo->get_count();
        int direcao = 0;

        if (posicao_atual > ultima_posicao) {
            direcao = 1;  
        } else if (posicao_atual < ultima_posicao) {
            direcao = -1; 
        }

        ultima_posicao = posicao_atual;
        
        return direcao; 
    }
};

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