These are just some notes to self after a session of figuring out how to get a rotary encoder do what I want. Thanks for the help, @james147
# Working with rotary encoders

For reading rotary encoders for input, use the [ClickEncoder](https://github.com/0xPIT/encoder/tree/arduino) library.

This library relies on [TimerOne](https://code.google.com/p/arduino-timerone/).

Each (click) rotary encoder requires 3 pins on the uC, two for the rotary encoder, one for the button. These pins can be any ol' digital input.

In `void loop()`, poll check encoder->getValue() to get the integer increase/decrease since last time. Base the integration on the example that comes with the library.

The input pins get pulled up on the arduino board, but it might need external pull-up resistors w/o the arduino board.

Also, and remember this: **the common is the middle pin**.



