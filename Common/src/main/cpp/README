For more information see: https://www.juggluco.nl/Juggluco/cmdline/index.html

Compile for debugging with:

cmake -DDEBUG=ON $srcdir
make juggluco

Replace $srcdir with the directory cpp source of Juggluco. The directory that contains this README and CMakeLists.txt (https://github.com/j-kaltes/Juggluco/tree/primary/Common/src/main/cpp).
To compile without logging and debug information (in a fresh directory):

cmake $srcdir
make juggluco

Juggluco server can also function as simplified Nightscout/xDrip web server.

SSL doesn't work with the precompiled version, because statically linked programs can't use dlopen.

To use the SSL version, you need an authenticated ssl key for the hostname that is used to contact with the juggluco server.

You can get such a key for your domain name for free using certbot (https://certbot.eff.org/instructions). 

See Left menu->Settings->Web server->Help in Juggluco or https://www.juggluco.nl/Jugglucohelp/Nightscouthelp.html

Put fullchain.pem privkey.pem in the jugglucodata directory where also the other data is saved. 

It does not work with Nsclient and scout.
It does work with Garmin Nightscout watches, xDrip and Diabox as followers, Nightwatch, G-Watch as follower
Glimp doesn't connect at all and complaints about invalid nightscout web address also when I enter a webaddress of another nightscout server doesn't it work.
For more information see in Juggluco Left menu->Settings->Web Server->Help.
