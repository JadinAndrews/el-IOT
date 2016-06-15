# el-IOT

el-IOT is not yet anything special, it will be a collecction of code examples that work together to bring your IOT projects to life.

There will be a few basic components that make up el-IOT, but basically el-IOT will be made up of a very simpe structure, with only two roles distinguished. There is only the Server, and Clients. AnyTHING can be a client, I decided to be purposefully ambiguous, because a THING, can be anything. It doesn't matter if it's an arduino, raspberry pi, esp8266 or even a web app. I consider those to all be THINGS, in the domain of el-IOT. The server will function as the backbone that manages all the house keeping and keeps THINGS under controll.

The server, will be made up of a collection of programs, most importantly a Mosquitto MQTT broker will handle the bulk of the communication between THINGS. A node.js app will write subscribed communication into a mongodb, and will also respond to websockets requests (web app THINGS) for information in the db.

I will provide a few clients that run on either arduino's (electricity meters/ water meters etc) or esp8266's (temparature sensors). Further I will provide a simple dashboard THING, that will publish to mosquitto and subscribe for live updates on stats. I am not sure how far I wan to take this concept, it would keep everything rather simple and one dimensional if I forget about serving historical data through a websocket. It might be cool to let the mosquitto handle ALL communication? so a request for information will be a simple publish to a certain topic, and the payload would contain the request for the node.js app, which would then respond by publishing the request to another certain topic? Seems cool and simple to me.

Finally, this is an educational project, if you like it, please help out, but don't expect anything special just yet. If you're looking for something functional, look at lelylan. This is just my attempt to create something that will be useful to me.
