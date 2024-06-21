# Sysmenu
Sysmenu is a simple and efficient application launcher written in gtkmm4<br>
![default](https://github.com/System64fumo/sysmenu/blob/main/preview_default.gif "default")
![fullscreen](https://github.com/System64fumo/sysmenu/blob/main/preview_fullscreen.gif "fullscreen")

# Install
You need the following dependencies:
* gtkmm-4.0
* gtk4-layer-shell

Customize ``config.hpp`` if you wish to do so. <br>
Then to build all you need to do is run ``make``

# Why does this exist?
Mainly because i got bored lol.<br>
But also because i disliked how the current programs i used behaved.<br>
So instead of fixing them i created my own!<br>

# Configuration
sysmenu can be configured in 2 ways<br>
1: By changing config.h and recompiling (Suckless style)<br>
2: Using launch arguments<br>
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
  -W	Set window width
  -H	Set window Height
  -l	Disable use of layer shell
  -v	Prints version info
  -D	Set dock items ("Terminal,FileManager,WebBrowser,ect..")
  -f	Fullscreen
```

# Signals
You can send signals to show/hide the window.<br>
``pkill -10 sysmenu`` to show.<br>
``pkill -12 sysmenu`` to hide.<br>
``pkill -34 sysmenu`` to toggle.<br>

> [!WARNING]
> There is currently an issue where if you toggle visibility too quickly,<br>
> The program will crash with: ``Error 71  (Protocol error) dispatching to Wayland display.``<br>
> This issue seems to have been mostly resolved on Hyprland, However your luck
> with other compositors may vary, BE WARNED!

# Theming
sysmenu uses your gtk4 theme by default, However it can be also load custom css,<br>
Just copy the included menu.css file to ~/.config/sys64/menu.css<br>

# Credits
[wf-shell](https://github.com/WayfireWM/wf-shell) for showing how to do launcher related stuff<br>

# Also check out
[wofi](https://hg.sr.ht/~scoopta/wofi)<br>
[nwg-drawer](https://github.com/nwg-piotr/nwg-drawer)<br>
