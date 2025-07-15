# Sysmenu
Sysmenu is a simple and efficient application launcher<br>
<!---
// TODO: Add screenshots
![default](https://github.com/System64fumo/sysmenu/blob/qt-rewrite/preview_default.png "default")
![fullscreen](https://github.com/System64fumo/sysmenu/blob/qt-rewrite/preview_fullscreen.png "fullscreen")
-->
[![Packaging status](https://repology.org/badge/vertical-allrepos/sysmenu.svg)](https://repology.org/project/sysmenu/versions)

> [!NOTE]  
> This project has been rewritten to use QT6 instead of GTKMM 4<br>
> This branch will replace main once the rest of my shell is ported.<br>

# Configuration
sysmenu can be configured in 3 ways<br>
1: By changing config.h and recompiling (Suckless style)<br>
2: Using a config file (~/.config/sys64/menu/config.conf)<br>
3: Using launch arguments<br>
```
arguments:
  -S	Hide the program on launch
  -s	Hide the search bar
  -i	Set launcher icon size
  -m	Set launcher margins
  -u	Show name under icon
  -b	Show scroll bars
  -n	Max name length
  -p	Items per row
  -a	Anchors ("top right bottom left")
  -W	Set window width
  -H	Set window Height
  -v	Prints version info
```

# Signals
You can send signals to show/hide the window.<br>
``pkill -USR1 sysmenu`` to show.<br>
``pkill -USR2 sysmenu`` to hide.<br>
``pkill -RTMIN sysmenu`` to toggle.<br>

# Theming
sysmenu uses your QT6 theme in addition to a custom style in `/usr/share/sys64/menu/style.qss`.<br>
Should you choose to override the theme just copy it over to ~/.config/sys64/menu/style.qss and adjust it.<br>

# Known bugs/issues
There is no slide animation, No clue if i can re-implement that.<br>
There are no custom value QSS object names.<br>
Dock is not yet implemented.<br>
Touch gestures not yet implemented.<br>
Proper async loading not yet implemented.<br>

# Credits
[wf-shell](https://github.com/WayfireWM/wf-shell) for showing how to do launcher related stuff<br>

# Also check out
[wofi](https://hg.sr.ht/~scoopta/wofi)<br>
[nwg-drawer](https://github.com/nwg-piotr/nwg-drawer)<br>
