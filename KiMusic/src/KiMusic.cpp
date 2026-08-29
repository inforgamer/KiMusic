#include <Arduino.h> 
#include "Screen.hpp"

using namespace std;

class Encoder
{
    private:
        uint8_t pinA;
        uint8_t pinB;
        int lastState;

    public:
        Encoder(uint8_t pA, uint8_t pB) {
            pinA = pA;
            pinB = pB;
            
            pinMode(pinA, INPUT_PULLUP);
            pinMode(pinB, INPUT_PULLUP);

            lastState = digitalRead(pinA);
        }

        int readRotate() {
            int stateA = digitalRead(pinA);
            int stateB = digitalRead(pinB);
            int dir = 0;
            
            if(stateA != lastState)
            {
                if (stateA == LOW)
                {
                    if(stateB != stateA) {
                        dir = 1;
                    } else {
                        dir = -1;
                    }
                }
                lastState = stateA;
                return dir;
            }
            return 0;
        }
};

class Button
{
    private:
        uint8_t pinNumber;
        bool isPressed;

    public:
        Button(uint8_t pinNumber)
        {
            this->pinNumber = pinNumber;
            isPressed = false;
            pinMode(this->pinNumber, INPUT_PULLUP);
        };

        bool checkClick()
        {
            if(digitalRead(this->pinNumber) == LOW && isPressed == false)
            {
                isPressed = true;
                delay(100);
                return true;
            };
            if(digitalRead(this->pinNumber) == HIGH && isPressed == true)
            {
                isPressed = false;
                delay(100);
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
        
            encoder(6, 7),       
            centerButton(5),     
            previewButton(18),    
            nextButton(8),        
            volumeButton(16),     
            positionButton(17)   
            {
                this->dif = 0;
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
            if(isPlay == true) {
                 Serial.println("Play!"); 
            } else {
                 Serial.println("Pause!");
            }
        }
        else if (positionButton.checkClick() && volumeMode == false)
        {
            positionMode = !positionMode;
            if (positionMode == true) {
                Serial.println("Modo Posicao!");
                idle = false;
            } else {
                Serial.println("Modo Normal!");
                idle = true;
            }
        }
        else if(volumeButton.checkClick() && positionMode == false)
        {
            volumeMode = !volumeMode;
            if(volumeMode == true) {
                Serial.println("Modo Volume!");
                idle = false;
            } else {
                Serial.println("Modo Normal!");
                idle = true;
            }
        }
        else if(previewButton.checkClick() && idle == true)
        {
            idle = !idle;
            Serial.println("Musica Anterior!");
            idle = !idle;
        }
        else if(nextButton.checkClick() && idle == true)
        {
            idle = !idle;
            Serial.println("Proxima musica!");
            idle = !idle;
        }
    };
    
    void setVolume(int delta)
    {
        this->volume += delta;
        if(this->volume > 100) {
            this->volume = 100;
        } else if (this->volume <= 0) {
             this->volume = 0;
        }
        Serial.printf("Volume atual: %d\r\n", this->volume);
    };  
    
    void setPosition(int position)
    {
     // avançar tempo da musica referente a rotação
    };      
};


Player kiMusic;
Screen screen;

void setup() {
    Serial.begin(115200);
    delay(1000); 

    screen.init();
    Serial.println("KiMusic Iniciado no ESP32...");
}

void loop() {
    kiMusic.control();    
    delay(1);
}
