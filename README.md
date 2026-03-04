# CS 1.6 E-BOT ![GitHub all releases](https://img.shields.io/github/downloads/EfeDursun125/CS-EBOT/total)
AI Bot for Counter-Strike based on SyPB, this bot is only for zombie plague/escape and biohazard gamemodes.

[Click HERE To Join E-BOT Discord Community](http://discord.gg/v7PesBamXt)

[Click HERE To Join E-BOT Steam Community](https://steamcommunity.com/groups/E125G)

[Please Download From The Blog For Support This Project](https://ebots-for-cs.blogspot.com/)

# E-Bot Requires VCRedist (For Windows)
If you see badf load on console (when you type "meta list") install this.

[Click HERE To Download](https://aka.ms/vs/17/release/vc_redist.x86.exe)

# How to install
1. Download & install metamod if you dont have.
2. Download latest ebot release.
3. Put ebot folder to "cstrike\addons"
4. Open "cstrike\addons\metamod\plugins.ini"
5. (For Windows) Add that line "win32 addons\ebot\dlls\ebot.dll" and save it.
6. (For Linux) Add that line "linux addons/ebot/dlls/ebot.so" and save it.

### E-Bot menu navigation
Open menu commands:
- `ebot menu` (or `ebot botmenu`) -> opens `E-Bot Main Menu`
- `ebot wp menu` -> opens `Waypoint Operations (Page 1/2)`
- `ebot cmenu` (or `ebot cmdmenu`) -> opens `E-Bot Command Menu`

```text
E-Bot Main Menu
1. E-Bot Control - bot management menu
   1. Add a E-Bot, Quick - instantly add one bot with random settings
   2. Add a E-Bot, Specified - start guided add flow (skill -> personality -> team -> model)
      E-Bot Skill Level
      1. Poor (0-20) - set low skill
      2. Easy (20-40) - set easy skill
      3. Normal (40-60) - set medium skill
      4. Hard (60-80) - set hard skill
      5. Expert (80-99) - set expert skill
      6. Legendary (100) - set max skill
      0. Exit - close menu

      E-Bot Personality
      1. Random - random behavior profile
      2. Normal - balanced behavior profile
      3. Rusher - aggressive behavior profile
      4. Careful - defensive behavior profile
      0. Exit - close menu

      Select a team
      1. Terrorist Force - choose T side
      2. Counter-Terrorist Force - choose CT side
      5. Auto-select - random side (bot is added immediately)
      0. Exit - close menu

      Select an appearance (T menu)
      1. Phoenix Connexion - T model
      2. L337 Krew - T model
      3. Arctic Avengers - T model
      4. Guerilla Warfare - T model
      5. Auto-select - random T model
      0. Exit - close menu

      Select an appearance (CT menu)
      1. Seal Team 6 (DEVGRU) - CT model
      2. German GSG-9 - CT model
      3. UK SAS - CT model
      4. French GIGN - CT model
      5. Auto-select - random CT model
      0. Exit - close menu

   3. Remove Random Bot - kick one random E-Bot
   4. Remove All Bots - kick all E-Bots
   5. Remove Bot Menu - open paged remove list (up to 32 slots)
      E-BOT Remove Menu (1/4)
      1-8. Slot action - kick selected E-Bot slot (if valid)
      9. More... - next page
      0. Back - back to E-Bot Control Menu

      E-BOT Remove Menu (2/4)
      1-8. Slot action - kick selected E-Bot slot (9-16)
      9. More... - next page
      0. Back - previous page

      E-BOT Remove Menu (3/4)
      1-8. Slot action - kick selected E-Bot slot (17-24)
      9. More... - next page
      0. Back - previous page

      E-BOT Remove Menu (4/4)
      1-8. Slot action - kick selected E-Bot slot (25-32)
      0. Back - previous page
   0. Exit - close menu

2. Features - feature and waypoint menus
   1. Weapon Mode Menu - weapon restriction menu (currently visual/no-op in this branch)
      E-Bot Weapon Mode
      1. Knives only - intended knife-only mode
      2. Pistols only - intended pistols-only mode
      3. Shotguns only - intended shotguns-only mode
      4. Machine Guns only - intended SMG/MG-only mode
      5. Rifles only - intended rifles-only mode
      6. Sniper Weapons only - intended snipers-only mode
      7. All Weapons - intended default unrestricted mode
      0. Exit - close menu

   2. Waypoint Menu - open waypoint operations
      Waypoint Operations (Page 1/2)
      1. Show/Hide waypoints - toggle waypoint rendering/edit mode
      2. Cache waypoint - store nearest waypoint as cached target
      3. Create path - open path creation menu
      4. Delete path - remove path between nearest and target waypoint
      5. Add waypoint - open waypoint type menu
      6. Delete waypoint - delete nearest waypoint
      7. Set Autopath Distance - open autopath distance menu
      8. Set Radius - open waypoint radius menu
      9. Next... - open page 2
      0. Exit - close menu

      Waypoint Operations (Page 2/2)
      1. Waypoint stats - print waypoint statistics to console
      2. Autowaypoint on/off - toggle automatic waypoint placement
      3. Set flags - open waypoint flags menu (3 pages)
      4. Save waypoints - save with validity check
      5. Save without checking - force save
      6. Load waypoints - load waypoint file from disk
      7. Check waypoints - run node validity check
      8. Noclip cheat on/off - toggle noclip helper for editing
      9. Previous... - return to page 1
      0. Exit - close menu

      Waypoint Radius
      1..9. SetRadius (0..128) - set radius for nearest waypoint
      0. Exit - close menu

      Waypoint Type
      1. Normal - add default waypoint
      2. Terrorist Important - add T-specific crossing waypoint
      3. Counter-Terrorist Important - add CT-specific crossing waypoint
      4. Avoid - add avoid waypoint
      5. Rescue Zone - add rescue waypoint
      6. Camping - add camp waypoint
      7. Helicopter - add button/elevator-type waypoint
      8. Map Goal - add goal waypoint
      9. Jump - start learn-jump waypoint placement
      0. Exit - close menu

      Toggle Waypoint Flags (Page 1/3)
      1. Fall Check - toggle fall-check behavior
      2. Terrorists Specific - toggle T-only flag
      3. CTs Specific - toggle CT-only flag
      4. Use Elevator - toggle lift flag
      5. Helicopter - toggle helicopter flag
      6. Zombie Mode Camp - toggle zombie mode camp flag
      7. Delete All Flags - clear all flags on nearest waypoint
      8. Previous... - to page 3
      9. Next... - to page 2
      0. Exit - close menu

      Toggle Waypoint Flags (Page 2/3)
      1. Use Button - toggle use-button flag
      2. Human Camp Mesh - toggle human mesh camp flag
      3. Zombie Only - toggle zombie-only flag
      4. Human Only - toggle human-only flag
      5. Zombie Push - toggle zombie-push flag
      6. Fall Risk - toggle fall-risk flag
      7. Specific Gravity - toggle specific-gravity flag
      8. Previous... - to page 1
      9. Next... - to page 3
      0. Exit - close menu

      Toggle Waypoint Flags (Page 3/3)
      1. Crouch - toggle crouch flag
      2. Only One Bot - toggle one-bot-only flag
      3. Wait Until Ground - toggle wait-until-ground flag
      4. Avoid - toggle avoid flag
      8. Previous... - to page 2
      9. Next... - to page 1
      0. Exit - close menu

      AutoPath Distance
      1. Distance 0 - disable autopath links
      2..7. Distance 100..250 - set max auto-link distance
      0. Exit - close menu

      Create Path (Choose Direction)
      1. Outgoing Path - create nearest -> target path
      2. Incoming Path - create target -> nearest path
      3. Bidirectional (Both Ways) - create both directions
      4. Jump Path - create nearest -> target with jump flag
      5. Jump Path + Outgoing - jump nearest -> target + normal target -> nearest
      6. Zombie Boosting Path - create nearest -> target with boost flag
      7. Delete Path - remove existing path between selected points
      0. Exit - close menu

   3. Select Personality - shortcut to personality/team/model add flow
   4. Toggle Debug Mode - toggle internal debug cvar
   5. Command Menu - open command menu (currently visual/no-op in this branch)
      E-Bot Command Menu
      1. Make Double Jump - listed command entry
      2. Finish Double Jump - listed command entry
      3. Drop the C4 Bomb - listed command entry
      4. Drop the Weapon - listed command entry
      0. Exit - close menu
   0. Exit - close menu

3. Fill Server - fill-mode add flow (team -> skill -> personality)
   Select a team
   1. Terrorist Force - set fill target team T
   2. Counter-Terrorist Force - set fill target team CT
   5. Auto-select - set fill target team auto
   0. Exit - close menu

   E-Bot Skill Level
   1..6. Skill band - select skill for all added bots
   0. Exit - close menu

   E-Bot Personality
   1. Random - fill using random personality
   2. Normal - fill using normal personality
   3. Rusher - fill using rusher personality
   4. Careful - fill using careful personality
   0. Exit - close menu

4. End Round - force kill all bots (round cleanup helper)
0. Exit - close menu
```
