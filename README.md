## waveshare-eink-writer
A simple, slightly configurable text writing library for waveshare eink displays, especially for use with arduino.

The library provided by waveshare (in my experience) failed to do anything except display a pre-coded image. (The painting library provided assumes you can fit the whole image in RAM). This project contains a Screen class that allows for the definition of a configurable screen, text to be written to it and displayed using the functinal portion of the provided library. This is more complex than it sounds due to the memory constraints of most arduino devices.
