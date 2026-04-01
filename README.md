# CS 1.6 E-BOT !
AI Bot for Counter-Strike based on SyPB, this bot is only for zombie plague/escape and biohazard gamemodes.

### Try bots on public Counter-Strike 1.6 servers

- `csko.cz:27017` — custom zombie based on Zombie Plague (maps are in fog or partial darkness)
- `csko.cz:27031` — Zombie Plague 4.3 (complete darkness)  
- `csko.cz:27070` — zombie escape

  
<br>

[Click HERE To Join E-BOT Discord Community](http://discord.gg/v7PesBamXt)

[Click HERE To Join E-BOT Steam Community](https://steamcommunity.com/groups/E125G)

<br>

# How to install
1. Download & install metamod if you dont have.
2. Download latest ebot release.
3. Put ebot folder to "cstrike\addons"
4. Open "cstrike\addons\metamod\plugins.ini"
5. (For Windows) Add that line "win32 addons\ebot\dlls\ebot.dll" and save it.
6. (For Linux) Add that line "linux addons/ebot/dlls/ebot.so" and save it.


**Required glibc 2.19+ on Linux**

**tested on linux dedicated server with:** rehlds and metamod-r

    ReHLDS-3.14.0.857
    Metamod-r v1.3.0.149, API (5:13)
    Metamod-r build: 11:31:17 Apr 23 2024

**tested on windows listen server with:** metamod-p

    Metamod v1.21p37  2013/05/30 (5:13)


# How to use - local listen server (client)
1. Write this to client console: `bind k "ebot menu"`
2. Write this to client console: `bind j "ebot wp menu"`

Press `j` or `k` and you can manage waypoints and bots. 

(NOTE: Editing waypoints/paths with bots may cause crashes)


# How to use - dedicated server
1. Set password in ebot.cfg, for example: `ebot_password_key "39532"`
2. Write this to client console: `setinfo ebot_pass "39532"`
3. Restart server
3. Write this to client console: `bind k "ebot menu"`
4. Write this to client console: `bind j "ebot wp menu"`
5. Write this to client console: `ebot wp on`

Now on `k` you have main menu and on `j` wp editing menu.

(NOTE: Editing waypoints/paths with bots on the server may cause crashes)

### Other usefull commands:
- `bind "KP_MINUS"             "ebot kickall"`
- `bind "KP_PLUS"              "ebot_add"`
- `bind "*"                    "ebot killbots"`



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
      9. Jump - start learn-jump waypoint placement (experimental)
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
      5. Leave (Release Wait) - toggle leave flag (release WAIT hold behavior)
      6. Wait (Until Leave) - toggle wait flag (hold until bot reaches LEAVE waypoint)
      7. Zombie Boost - toggle zombie boost (double-jump) flag
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

## E-Bot Commands (Console)
Main command entry:
- `ebot <command> [args...]`

Standalone commands:
- `ebot_about` - print version/about info.
- `ebot_add` - add one random bot.
- `ebot_add_t` / `ebot_add_tr` - add one random Terrorist bot.
- `ebot_add_ct` - add one random Counter-Terrorist bot.

Core commands (`ebot ...`):
- `help` / `?` - show help (`ebot help full` for extended output).
- `version` / `ver` - show version.
- `about` / `about_bot` - show about block.
- `time` / `ctime` - print current server time.
- `menu` / `botmenu` - open main menu.
- `cmenu` / `cmdmenu` - open command menu.
- `list` / `listbots` - list active bots.
- `fill` / `fillserver` - fill server with bots.
- `kickall` / `kickbots` - remove all bots.
- `killall` / `killbots` - kill all bots.
- `kick` / `kickone` - remove one random bot.
- `kick_t` / `kickbot_t` - remove one random T bot.
- `kick_ct` / `kickbot_ct` - remove one random CT bot.
- `kill_t` / `killbots_t` - kill all T bots.
- `kill_ct` / `killbots_ct` - kill all CT bots.
- `swap` / `swaptteams` - swap teams.
- `order` / `sendcmd` - execute command on selected bot.
- `health <value>` / `sethealth <value>` - set bot health.
- `gravity <value>` / `setgravity <value>` - set bot gravity.

`ebot fill` syntax:
- `ebot fill [team] [personality] [skill] [count]`
- `team`: `1` = T, `2` = CT, `5` = random/default.
- omitted args use default/random behavior.

`ebot order` syntax:
- `ebot order <client_index> <bot_command>`
- example: `ebot order 3 radio1`

Waypoint commands (`ebot wp`, aliases: `ebot waypoint`, `ebot wpt`):
- `ebot wp` - print waypoint mode status.
- `ebot wp on` / `ebot wp off` - enable/disable waypoint editing mode.
- `ebot wp noclip` - toggle waypoint noclip helper.
- `ebot wp mdl on` / `ebot wp mdl off` - show/hide spawn models.
- `ebot wp menu` - open waypoint menu.
- `ebot wp add` - open add-waypoint menu.
- `ebot wp flags` - open waypoint flags menu.
- `ebot wp addbasic` - create base waypoints.
- `ebot wp analyze` / `ebot wp analyzeoff` / `ebot wp analyzefix` - analyze workflow.
- `ebot wp find <index>` - set/find waypoint index.
- `ebot wp cache` - cache nearest waypoint.
- `ebot wp teleport <index>` - teleport to waypoint.
- `ebot wp setradius <value>` - set nearest waypoint radius.
- `ebot wp setmesh <value>` - set nearest waypoint mesh id.
- `ebot wp setgravity <value>` - set nearest waypoint gravity.
- `ebot wp delete` - delete nearest waypoint.
- `ebot wp save` / `ebot wp save nocheck` - save waypoint file.
- `ebot wp load` - load waypoint file.
- `ebot wp check` - validate waypoint graph.

Path commands (`ebot path`, aliases: `ebot pathwaypoint`, `ebot pwp`):
- `ebot path create` - open create-path menu.
- `ebot path delete` - delete path between selected points.
- `ebot path autodistance` - open auto-path distance menu.
- `ebot path create_out` - create outgoing path.
- `ebot path create_in` - create incoming path.
- `ebot path create_both` - create both-way path.
- `ebot path create_jump` - create jump path.
- `ebot path create_boost` - create zombie boost path.
- `ebot path create_visible` - create visible-only path.

Autowaypoint commands:
- `ebot autowp on` / `ebot autowp off` (alias: `ebot autowaypoint`) - toggle auto waypointing.

Notes:
- A few legacy names printed by old help text can be no-op in this branch.
