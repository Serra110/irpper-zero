
# Irper-Zero

Irper-Zero is a small handheld device ran on the Raspberry Pi Pico that can capture infrared (IR) signals and save them to a microSD card for later use. The project combines infrared tools with a small OLED display and games, being focused on the display, as everything involves it. It currently includes two games, with the possibility of adding more in the future by loading compatible game files from the microSD card.

## Features

- **IR Signal Capture:** Receive infrared signals using an IR receiver.
- **microSD Storage:** Save captured IR signals to a microSD card.
- **OLED Display:** Display menus, 2d viewer of signals and games.
- **Built-in Games:** Includes two games for entertainment, both 1v1.
- **Expandable Game Library:** Designed to support additional games loaded from the microSD card, provided they use a compatible format, WIP.
- **IR Transmission:** Is able to send the signals that are saved in the microSD.

## Hardware Design

The device is built around the Raspberry Pi Pico, which handles the display, buttons, infrared signal capture and microSD storage.

### 3D Model

A 3D representation of the project designed in KiCad.
<img width="817" height="553" alt="image" src="https://github.com/user-attachments/assets/09cb39ac-a799-4fbb-99c7-19a23d537364" />


### Schematic
<img width="1567" height="1084" alt="image" src="https://github.com/user-attachments/assets/7abe7ffe-2ff4-4d9d-9d13-5a26e7e2e217" />


## Controls

SW1- Select/Confirm
SW2- Back

SW3- Player 1 up/Menu up
SW4- Player 1 down/Menu down

SW5- Player 2 up
SW6- Player 2 down
<img width="1015" height="791" alt="image" src="https://github.com/user-attachments/assets/7bbbf2ad-379d-4c1d-90ef-188242696210" />


## Infrared Tools

The IR tool allows the device to capture infrared signals from compatible signals (not encrypted basically) and save the recorded data to a microSD card.
After, the device will be able to read the microSD and send those signals as well.

## Games

Irper-Zero includes two built-in games that can be played directly on the device using its display and controls, pong and spam game (the one to reach the top first wins)
## Code

The firmware runs on the Raspberry Pi Pico and manages the display, controls, infrared signal capture, microSD card and games.
It is also supposed to reads the signals and show on the display the frequency, basically an IR signal viewer
<img width="742" height="414" alt="image" src="https://github.com/user-attachments/assets/2089a636-05ed-411f-af97-79f70fb77980" />
## AI usage
AI was used to generate some code including the UI since I do not know how to do it myself, I defined everything and woke every pin up myself.

## About the Project
The project is the first one to be focused entirely on frequencies, not counting the game, IR signals and the display. This was the first "alone" project to be made since I didn't follow a tutorial, and just took consideration.
