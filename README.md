## PolarBowler-HD

PolarBowler-HD is a DLL-based mod for updating the supported resolutions for Polar Bowler Classic (2004).

### Setup

1. Install a DLL-proxy plugin loader such as [bl-sdk/PluginLoader](https://github.com/bl-sdk/pluginloader/releases)
    * In this case, download one of the `msvc-x86-release` for Windows. Extract the `winmm.dll` file out into your game installation folder, right next to your exe; For example: `C:\Games\Polar Bowler Classic`
2. Go to the [Releases](https://github.com/FromDarkHell/PolarBowler-HD/releases) page, and download `PolarBowler-HD.zip`. Extract the zip into your game install folder. It will make a `Plugins` folder, with a `PolarBowler-HD.dll` file inside of it.
3. Launch your game, and it will boot up in 960x720 (rather than 640x480).
    * If you wish to configure a larger resolution, make a `PolarBowler-HD.ini` file next to it.
    ```ini
    [Display]
    Width=1400
    Height=1050
    ```


### License

This project is licensed using [MIT](/LICENSE.md)
- This project uses [HoShiMin/HookLib](https://github.com/HoShiMin/HookLib/), which is MIT licensed
- This project also uses [benhoyt/inih](https://github.com/benhoyt/inih/), which is licensed under New BSD.

### Support

If you for some reason want to support me financially for this project (or others I make), you can donate to me via [ko-fi](https://ko-fi.com/fromdarkhell) or [Patreon](https://patreon.com/fromdarkhell).

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/O4O44GLCD) [![Support me on Patreon](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Fshieldsio-patreon.vercel.app%2Fapi%3Fusername%3Dfromdarkhell%26type%3Dpatrons&style=for-the-badge)](https://patreon.com/fromdarkhell)