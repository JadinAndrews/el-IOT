# el-IOT
A simple framework, not yet, with advanced functions, not yet, for creating things with the SIM900 family of GSM/GPRS shields and breakouts.

This is a framework I decided to work on after creating a few functions to interface with a SIM900 GSM unit. 
It started out as a water meter project, but it is going to turn into a framework as I get time to organise things.
For now, the best way to see what this library is capable of is to read the comments inside the basic water meter example.

Please note, this project is in a state of extreme infancy, I do not care if or how you use my code, just understand that
I do not gaurauntee everything works. 

I especially do not expect anyone to find this, but hope you find it useful... ;)

Basic usage, Upload the sketch to arduino, remember to change the thingspeak api key.

What it does?

All it does is count pulses. That's it. And uploads them to a thingspeak channel. I created a few functions that would make the sketch as reliable as possible for collecting telemetry data, a WDT that resets the arduino if it locks up. 

There are also a bunch of other features I was/am working on, like using the eeprom to store timestamps of up to 500 readings incase there is a communication failure. There are some time retrieval functions I was working on, they aren't very reliable at the moment. Retreiving time from the cellular network proved the most unreliable as not all networks support this. Therefore, I have resorted to retreiving time from HTTP headers, and comparing it with the RTC on the sim900. Of course, you could add an external RTC, but that would increase the part count of the project.

I have also tried to place a major focus on energy effeciency. Since the project is intended to be powered by a solar cell and either a LiPo or supercapacitor, and also considering that I originally intended to deploy thousands of these units, I wanted them to be as independant as possible.

Ideally they would be cheap, reliable water meters that can detect problems before a utility bill scares you away.
I would like to add support for the M-bus at some point to interface with more advanced meters, as well as support for gas monitoring and other things beyond that.

At some point a central server would also need to be created from scratch as relying on thingspeak is not an option for individuals that want to really customize this to their clients.
