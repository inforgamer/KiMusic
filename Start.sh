#!/bin/bash
cd Prototipo/TelaANDLogica
python3 -m mpremote connect port:rfc2217://localhost:4000 fs mkdir lib
python3 -m mpremote connect port:rfc2217://localhost:4000 fs cp lib/ili9341.py :lib/ili9341.py
python3 -m mpremote connect port:rfc2217://localhost:4000 fs cp lib/rotary.py :lib/rotary.py
python3 -m mpremote connect port:rfc2217://localhost:4000 fs cp lib/rotary_irq_rp2.py :lib/rotary_irq_rp2.py
python3 -m mpremote connect port:rfc2217://localhost:4000 run main.py