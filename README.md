Guillemot Core
==============

History
-------
In the beginning there was Gilligan. And Gilligan was pretty OK. Gilligan is a handmade autonomous submarine built at Utah State University (USU) way back in 2012. USU entered the AUVSI Robosub competition two consecutive years with Gilligan. I was computer vision lead the first year. I was team captain the second year.

Gilligan qualified in its first year but, due to some IMU issues, was unable to get under the starting gate again in semi-finals. Gilligan qualified again its second year and was able to reliably get under the starting gate again in semi-finals. USU hasn't participated in Robosub again since I was team captain in 2013.

I feel the true potential of Gilligan remains untapped. We only got around to seriously programming its heirarchial task state machine a few days before the end of the 2013 competition. Unfortunately the current USU Robosub team has no interest in working on "someone else's" submarine and has been building a new one from scratch, disregarding a lot of the lessons learned from Gilligan, and leaving its name somewhat tarnished.

Guillemot
---------
Guillemot is my new autonomous submarine. It's different from Gilligan, but also very much the same. It has two cameras (forward and downward), an IMU, a central computer, thrusters, a mission toggle, etc. but is a bit more on the budget side (using Android device(s) for camera/IMU, Raspberry Pi for computer) and uses a single cylindrical pressure vessel (no inter-box cabling!). Probably the only new hardware functionality I'll add is hydrophones for using pingers to track location.

Guillemot Core is a fork of https://code.google.com/p/usu-robosub/, the old Gilligan Core code, which will diverge for use on Guillemot. I expect a lot to change but also want a familiar starting point so here it is.

Setup
-----
See `SETUP.md`.

Goals
-----
This is going to be pretty vaugue since it's been awhile but...

* https://code.google.com/p/usu-robosub/ is licensed under Apache 2.0 (mostly, I'm sure there are some exceptions) so I'll probably do some stuff here to make that more apparent: Adding file headers with author info and whatnot.
* There are a lot of individual custom ROS packages that could all be combined into a single package.
* There is a lot of obsolete code that could just be deleted.
* Guillemot Sensor (not released yet) is going to handle the computer vision stuff so that will probably get deleted as well.
* A lot of the startup scripts use Bash, which is great and all... but ROS has a more useful startup config so it'd be nice to switch over to that.
* I'll probably rewrite some simple nodes in Python where it aids in maintainability going forward.
* The Brain (!) is still very young and needs a lot of help to become smarter.
* Gilligan never had a very good sense of position underwater. I'd like to build a position tracking node that combines lots of inputs (general camera movement, object movement, IMU, pinger locations) to get a probabilistic idea of where it is underwater. And know how to search for old objects when a higher degree of position awareness is needed.

Contributions
-------------
Please send pull requests or open feature requests or bugs or write wiki pages or whatever. I'm OK with doing this all on my own but will definitely not turn anyone away who's making useful contributions. Just be aware that most everything in here is Apache 2.0 (I'm sure there are some exceptions) and if you edit code add yourself to the authors for it.
