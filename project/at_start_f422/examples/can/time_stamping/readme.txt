/**
  **************************************************************************
  * @file     readme.txt
  * @brief    readme
  **************************************************************************
  */

  this demo is based on the at-start board and at32-comm-ev, in this demo, 
  shows how to use the can communication mode. every 1s transmit one message
  and the led4 blink, if receive a message, led2 blink(message id is standard) 
  or led3 blink(message id is extended).
  it is print timestamp when transmit or receive frame is complete.
  set-up
  - can_tx(pa12)
  - can_rx(pa11)
  - can_stb(pb7)

  for more detailed information. please refer to the application note document AN0264.
