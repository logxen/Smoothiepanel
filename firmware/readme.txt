Smoothiepanel is designed to function as a simple slave device. It can be used over USB, uart, and spi. USB and uart comms work the same, but spi is a bit different. The command list itself does not change between the three and can be found here: https://gist.github.com/logxen/8792351

USB and uart are set up for easy use from a keyboard. Each transaction consists of a command byte, represented by a decimal number from 0 to 255, followed by a space and a data byte, also represented by a decimal number from 0 to 255, followed by a '\n'. The panel will respond with a line containing two response bytes in hex format.

SPI uses the same two byte transactions. Drop the CS pin, send the command byte while receiving the previous command's first response byte, send the data byte while receiving the previous command's second response byte, then raise the CS pin. The CS pin must be raised between each 16-bit transaction to allow the panel to process the command.


To set up a build environment:

Clone https://github.com/arthurwolf/Smoothie and run the install script appropriate to your OS.

This will create a BuildShell script that you will run any time you want to use your GCC_ARM build environment.

Clone https://github.com/mbedmicro/mbed and get any basic dependencies that requires like Python installed.

Be sure to have run the BuildShell script, then from your mbed directory run: python workspace-tools/build.py -m LPC11U35_401 -t GCC_ARM -u

Finally, you need to add a section to your mbed/workspace-tools/test.py like the following, but make sure to change "/path/to/git" to match wherever you cloned Smoothiepanel (for me that's "/home/logxen/git"):

    {
        "id": "Smoothiepanel", "description": "Smoothiepanel http://smoothieware.org/smoothiepanel",
        "source_dir": join("/path/to/git", "Smoothiepanel", "firmware"),
        "dependencies": [MBED_LIBRARIES, USB_LIBRARIES],
    },


To build:

Be sure to have run the BuildShell script, then from your mbed directory run: python workspace-tools/make.py -m LPC11U35_401 -t GCC_ARM -n Smoothiepanel


To flash:

If you are running Windows you can use the usb drag and drop system built into the mcu (plug in usb or reset while holding the isp button). Otherwise, for now, follow the process for flashing the Smoothie bootloader here using an FTDI without USB plugged in: http://smoothieware.org/flashing-the-bootloader

Eventually, I plan to have a bootloader with similar functionality to the one Triffid_Hunter wrote for Smoothie.
