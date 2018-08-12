
# Blues Brothers

This is a rewrite of the game [Blues Brothers](https://www.mobygames.com/game/blues-brothers) developed by [Titus](https://www.mobygames.com/company/titus-interactive-sa).

![Screenshot1](blues1.png) ![Screenshot2](blues2.png)


## Requirements

The game data files of the DOS or Amiga version are required.

```
*.BIN, *.CK1, *.CK2, *.SQL, *.SQV, *.SQZ
```

For sounds and music, the Amiga version files need to be copied.

```
SOUND, ALMOST, GUNN, EVERY, SHOT
```

The [ExoticA](https://www.exotica.org.uk/wiki/The_Blues_Brothers) modules set can also be used.


## Running

By default, the executable loads the data files from the current directory.

This can be changed by using command line switch.

```
Usage: blues [OPTIONS]...
  --datapath=PATH   Path to data files (default '.')
  --level=NUM       Start at level NUM
  --cheats=MASK     Cheats bitmask
  --startpos=XxY    Start at position (X,Y)
  --fullscreen      Enable fullscreen
  --scale           Graphics scaling factor (default 2)
  --filter          Graphics scaling filter
  --screensize=WxH  Graphics screen size (default 320x200)
  --cga             Enable CGA colors
```
