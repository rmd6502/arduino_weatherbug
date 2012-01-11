# Arduino Weatherbug Client
This is a simple demonstration of using [Mashery](http://developer.mashery.com)'s APIs with a simple device.  I used an Arduino Diecimila, a 
[Sparkfun WiFly shield](http://www.sparkfun.com/products/9954), and a [Sparkfun LED strip](http://www.sparkfun.com/products/10312) with individually addressable LEDs.

# Usage
You'll need to create a file Credentials.h with the appropriate definitions:
* passphrase

  The passphrase with which to associate to your wireless lan, if needed.

* ssid

  Self explanatory

* apiKey

  The XML REST key you use for Weatherbug.  The Geo key won't work, and you'll tear your hair out wondering why.

Once you've done that, plug your Arduino into an external power supply (the regulator won't drive the LED strip).  Upload the sketch, set your lat and long (or
zip code) and go!
