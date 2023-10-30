
west build -b nucleo_h743zi app -DDTC_OVERLAY_FILE=app.overlay && west flash
- where is overlay defined ? lul

ok differential mode does not work with MUX in ADS1115 => need to mod channel config and reconfigure
adc if 2 channels needed

why only zephyr,user works in device tree overlay ??
implement continous mode properly
lets try out stm32 adc


### TODO
- [ ] add Modbus server to this
- [ ] implement black pill f103 display module, controlled via uart
- [ ] try it with STM32 ADC
    - respective to ground it should have not more then 3,3V => roughly 2,5 int ref => @+160A ADC clamps @3,3V
    - [ ] what happens at clamping voltage
    - [ ] use voltage divider
- [ ] design PCB for this ? 
- [ ] do some proper calibration routine
- [ ] build up somthn that can control the electronic load
- [ ] use pi pico with micropython to test it, needs to be controllable via stm32 firmware
- [ ] pseudo 'in the loop' testing ability

