## This is my custom fork of the Generals Game Code project

I've been modding *Command & Conquer: Generals* and its expansion *Zero Hour* on and off for over two decades. I am primarily a Web developer by trade (using higher-level programming languages like PHP and JavaScript) but, although my knowledge of C++ is limited, I do bring a unique modder's perspective to the table and this repository exists for me to try out my own changes to the source code (on top of those made by TheSuperHackers).

So far I've done the following:
* Re-enabled the original campaign mission intro features which were disabled in *Zero Hour* (my personal mod already included both the original and expansion singleplayer campaigns, but this change means the original campaign mission intros can finally work exactly the same as they did in *Generals*)
  * This requires opting in using `CampaignUsesIntroBriefings yes` with applicable Campaign definitions in Campaign.ini
  * Also, switching the load screen background (i.e. after the intro video finishes playing, for campaigns **without** `CampaignUsesIntroBriefings yes`) to a faction-specific one now depends on setting the value of `PlayerFaction` to one of `FactionAmerica`, `FactionGLA` or `FactionChina` (before this was hardcoded based on the campaign name)
* Changed the default viewport height scale from 0.8 to 0.9 (this allows my mod to use a commandbar UI that takes up less of the screen, while still looking the same as the original UI)
* Increased the maximum commands per set to 20 and the maximum garrison slots to 16 (along with moving the hardcoded positions of the Stop and Evacuate commands); the maximum sciences for rank 1 have also been increased from 4 to 5 because the existing UI already clearly allowed for this
* Altered the `ALWAYS_VISIBLE` kind of to only affect an object's visibility within the fog of war - now, objects with this kind of are no longer visible within the shroud (in my mod I use this kind of on General's Power aircraft to prevent them from awkwardly disappearing and reappearing as they pass through the fog of war on their way to the target)
  * In the unmodified game, this kind of is only used for the MultiplayerBeacon object and, since multiplayer matches don't have the shroud anyway, I figured it was pretty safe to make this change
* Fixed the `CLEARED_BY_BUILD` kind of in multiplayer matches so that objects with this kind of can actually, you know, be cleared by building (lol)
  * Without this fix, the workaround for most prop-type objects was to instead use the `SHRUBBERY` kind of
* Wonky math used for `W3DCommandBarGridDraw` (this is what adds the grid overlay to an object's right-side HUD) has been fixed - it's obviously supposed to be in thirds but never quiiite used to be (mostly due to sloppy rounding errors)

What follows is the original project's README content...

# Welcome to the Generals Game Code project

GeneralsGameCode is a community-driven project aimed at fixing and improving the classic RTS game, *Command &
Conquer: Generals* and its expansion *Zero Hour*. This repository contains the source code for both games, with a
primary focus on *Zero Hour*.

Additionally, there is a complementary project repository for fixing and improving game data and assets such as
INI scripts, GUI, AI, maps, models, textures, audio, localization. You can find it
[here](https://github.com/TheSuperHackers/GeneralsGamePatch/) and contribute to it as well.

## Project Overview

The game was originally developed using Visual Studio 6 and C++98. We've updated the code to be compatible with Visual
Studio 2022 and C++20.

The initial goal of this project is to fix critical bugs and implement improvements while maintaining compatibility with
the original *Generals* version 1.08 and *Zero Hour* version 1.04. Once we can break retail compatibility, more fixes
and features will be possible to implement.

## Current Focus and Future Plans

Here's an overview of our current focus and future plans

- **Modernizing the Codebase**: Transitioning to modern C++ standards and refactoring old code.
- **Critical Bug Fixes**: Fixing game-breaking issues (e.g., fullscreen crash).
- **Minor Bug Fixes**: Addressing minor bugs (e.g., UI issues, graphical glitches).
- **Cross-Platform Support**: Adding support for more platforms (e.g., Linux, macOS).
- **Engine Improvements**: Enhancing the game engine to improve performance and stability.
- **Client-Side Features**: Enhancing the game's client with features such as an improved replay viewer and UI updates.
- **Multiplayer Improvements**: Implementing a new game server and an upgraded matchmaking lobby.
- **Tooling Improvements**: Developing new or improving existing tools for modding and game development.
- **Community-Driven Improvements**: Once the community grows, we plan to incorporate more features, updates, and
  changes based on player feedback.

## Running the Game

To run *Generals* or *Zero Hour* using this project, you need to have the original *Command & Conquer: Generals and Zero Hour* game
installed. The easiest way to get it is through *Command & Conquer The Ultimate Collection*
on [Steam](https://store.steampowered.com/bundle/39394). Once the game is ready, download the latest version of the
project from [GitHub Releases](https://github.com/TheSuperHackers/GeneralsGameCode/releases), extract the necessary 
files, and follow the instructions in the [Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki).


## Joining the Community

You can chat and discuss the development of the project on our [Discord channel](https://www.community-outpost.com/discord) to get the latest updates,
report bugs, and contribute to the project!

## Building the Game Yourself

We provide support for building the project on Windows and Linux. For detailed build instructions, check the
[Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki/build_guides), which includes guides for VS6, VS2022,
Docker, CLion, and links to forks supporting additional versions.

### Quick Start

**Windows (Visual Studio 2022)**
```bash
cmake --preset win32
cmake --build build/win32 --config Release
```

**Linux (via Docker)**
```bash
./scripts/docker-build.sh              # Build using Docker
./scripts/docker-install.sh --detect # Install to your game
```

### Dependency management

The repository uses a vcpkg manifest (`vcpkg.json`). Dependency versions come from the `builtin-baseline` commit
recorded there, with per-port `overrides` when a specific version is required. Update the baseline to pick up new
versions. GitHub Actions consumes these ports through a vcpkg binary cache backed by a NuGet feed on GitHub
Packages, keyed by vcpkg's own ABI hashes, so the first CI build warms the feed and subsequent builds pull prebuilt
binaries instead of re-compiling everything. Pull requests from forks restore from the feed but cannot write to it.

### Profiling

Tracy profiling is supported in the CMake preset `win32-profile`.
Use `tracy-profiler.exe` from [Tracy v0.13.1](https://github.com/wolfpld/tracy/releases/tag/v0.13.1).
If you get an error when using Tracy, try removing `dbghelp.dll` from the game binary directory.

## Contributing

We welcome contributions to the project! If you’re interested in contributing, you need to have knowledge of C++. Join
the developer chat on Discord for more information on how to get started. Please make sure to read our
[Contributing Guidelines](CONTRIBUTING.md) before submitting a pull request. You can also check out 
the [Wiki](https://github.com/TheSuperHackers/GeneralsGameCode/wiki) for more detailed documentation.


## License & Legal Disclaimer

EA has not endorsed and does not support this product. All trademarks are the property of their respective owners.

This project is licensed under the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html), which allows you to
freely modify and distribute the source code under the terms of this license. Please see [LICENSE.md](LICENSE.md) 
for details.
