# NSMBU-Checkpoint
A more robust checkpoint flag for NSMBU

## Overview
This patch for the checkpoint flag allows you to modify flag angles, powerups, and behaviours for granting those powerups via additional spritedata settings.


### For Modders
- Download the **`bundle`** from the [latest release](https://github.com/Swiftshine/NSMBU-Checkpoint/releases/latest) and extract it to your mod folder, merging the `content` and `code` folders into your project. The actors will now be available in-game.
    - The `rules.txt` doesn't matter as long as the `version = 8` in your own.
- Running on console: Use the [Telkin](https://github.com/Zenith-Team/Telkin) aroma plugin to load your whole mod.
    - Place the `code`/`content` folders in `sd:/wiiu/telkin/TITLEID/` where `TITLEID` is the [title ID](https://wiiubrew.org/wiki/Title_database#00050000:_Game_Application_Titles) of your game's region (without dashes).
- Running on Cemu: Load and distribute your mod as a GraphicPack by placing it in Cemu's `graphicPacks` folder and activating it in the game's settings.

> [!WARNING]
> You must use a 2.7+ version of Cemu, Downloads can be found [here](https://cemu.info/ActionBuilds.php).

> [!IMPORTANT]
> Make sure to also install the [editor patch](https://github.com/Swiftshine/NSMBU-Checkpoint/tree/main/editor) so that you can place the actors in your levels!
