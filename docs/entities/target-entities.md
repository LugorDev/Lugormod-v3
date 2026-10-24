**Last Update:** 13 October 2025

**Note:** This documentation relates to the latest version of Lugormod v3. Some entities may differ or not exist in prior versions of Lugormod. (https://lugormod.com/downloads/).

### Key Chart:
```
* = Entity is triggerable.
** = Entity can be targeted to work. Or has to be.
*** = Is an entity that is placed that doesn't need to be targeted/triggered to work.
(L) = Either a new entity created for Lugormod, or if an existing entity then edited for Lugormod.
```

- So if an entity has **(L) by it that means that it was created for Lugormod, or was edited, and has to be targeted to work.

# target_ Entities

## target_level_change **
changes the map to whatever you specify when this entity gets used.  This is dangerous in older lugormod versions or if not used with OpenJK. Escape sequences and the ';' can be used in the mapname key maliciously. Use with care and/or remove.

### Keys:

```
mapname - map to change to when used
```

### Example code:

```
/place target_level_change * targetname,changemap,mapname,t2_trip
```

## target_position **
Only exists for the sole purpose of having stuff aim at it, like misc_weapon_shooters, effects, tele's, trigger_push, misc_portal_camera ect.

### Keys:

```
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_position 60 targetname,blaster_fire_spot,
```

- this is an example of having a misc_weapon_shooter fire at a spot of my choosing, 60 units up from that spot.

## target_push **
Gets used by something targeting this, it will push the activator in the direction of its target, which should be a target_position or something.

### Spawnflags:

```
1 - play bounce noise
2 - will push activator in direction of 'target' at constant 'speed', speed does need to be set for this to work
```

### Keys:

```
speed - speed to push activator 
        (default 1000)
```

## target_powerup **
Sets a powerup to the player.

### Keys:

```
powerup     - use this key to set one of the powerups (listed below)
targetname  - make the trigger target this value for the entity to be used
wait        - duration
```

### Powerups:

```
0   - nothing
1   - multiply the damage by the value in the cvar g_quadfactor
2   - protection from lava/acid, and protects from all radius damage (but takes knockback). also absorbs 50% of all damage
3   - force pull effect
4   - red team flag
5   - blue team flag
6   - not used, lugor uses this for the glow-around-the-player stash effect
7   - singleplayer shield damage effect. Lugormod T2 used this for the bodyshield uitem
8   - for the dodge effect (sniper fire while you're using force sense on a higher level)
9   - despite its name, this is a force push effect (like PW_PULL, except the other way around)
10  - force speed effect
11  - cloaking effect (this does NOT make npcs ignore you by itself, notarget is set by the game code when you use the cloaking item)
12  - force enlightenment
13  - force dark enlightenment
14  - Force Boon
15  - The ysalarimi
```

### Example code:

```
/place target_powerup * targetname,Coffee,powerup,11,
```

- this is an example of what you could use to make a button that would give the player a ysalarimi blast everytime he pressed the button.

## target_fp ** (L)
Applies custom Force power levels and/or a Force regeneration multiplier to whoever activates it.

### Keys:

```
customskill                 - The name of the skill to check. (see lmd_customskill for details)
customskillcompare          - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                              (default 0)
customskillvalue            - The value to compare to.
property                    - The property to check to fire this entity.
profession                  - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level                       - Minimum player level.
maxlevel                    - Maximum player level.
adminlevel                  - Minimum admin level.
playerflags                 - A bitmask of flags set by the lmd_playerflag entity.
targetname                  - make the trigger target this value for the entity to be used.
ModifyPowers                - Dot (`.`) separated list of Force power assignments in the format powernamelevel.
                              For example `push3.lightning1` will set Force Push to level 3 and Lightning to level 1.
                              Supported power names: jump, push, pull, speed, seeing, heal, protect, absorb, mindtrick,
                              theal, grip, lightning, rage, drain, tforce, sattack, sdefend, sthrow.
ResetPowers                 - If set to 1 the user's Force powers are reset to their normal values instead of applying
                              `ModifyPowers`. (Default 0)
ForceRegenSpeedMultiplier   - Multiplier applied to the player's Force regeneration speed. Must be greater than 0 to have an effect.
```
### Example code:

```
/place target_fp * targetname,force_buff,ModifyPowers,push3.lightning2.ForceRegenSpeedMultiplier,1.5,
```

- When triggered this target gives players Force Push level 3, Force Lightning level 2 and increases their Force regeneration speed by 50%.


## target_remove_powerups **
Remove all powerups from the person who uses this. In U# It will not longer remove the stash glow effect when a player uses this.

### Keys:

```
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_remove_powerups * targetname,remove_powerup
```

- this will remove all the powerups that you have gained when used

## target_teleporter **
This teleports the player when used. Does not need a trigger_teleport, the same thing as misc_teleporter_dest, which isnt listed. Use angle or angles keys to change the direction that you teleport in.

### Spawnflags:

```
1 - Do not teleport player if they are in a duel.
```

### Keys:

```
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_teleporter 12 targetname,enter_bar,angle,90,
```

- this is the area that you come out of the tele at, i added 12 to the z-axis because without it you would spawn in the ground.

## target_delay **
This is a great entity for making an entity not fire its target right away, instead it waits the amount of time you set with the "wait" key to fire its target.

### Spawnflags:

```
1 - Keeps the delay from resetting the time if it is activated again while it is counting down to an event.
```

### Keys:

```
wait        - Seconds to pause before firing targets
random      - delay variance, total delay = delay +/- random seconds.
targetname  - make the trigger target this value for the entity to be used
target      - what to fire at when the delay time is hit
```

### Example code:

```
/place target_delay * wait,3,target,ship_spawn,targetname,ship_delay,
```

- This would be for if i pressed a button and i wanna give the player enough time to get to the general area of where the x-wing spawned, so i gave it a three second delay, that way there is a smaller chance of someone stealing that players ship that they paid for.

## target_doorstate **
Checks a set of func_door or lmd_door entities to make sure they are resting in the position described by their `wantPosition` key. Fires its normal `target` when every door matches; otherwise it fires `target2`. Each monitored door must have `wantPosition` set to either `open` or `close` (synonym `closed`).

### Keys:

```
target      - fired when all referenced doors match their wantPosition.
target2     - fired when a door is missing or does not match its wantPosition.
door1-6     - targetnames of the doors to monitor.
```

### Example code:

```
/spawnstring edit vault_door wantPosition close
/place target_doorstate * target,vault_locked,target2,vault_open,door1,vault_door
```

The example ensures the door named `vault_door` is closed before firing `vault_locked`; otherwise it fires `vault_open`.


## target_speaker **
This entity will play a sound that you specify in a certain radius.

### Spawnflags:

```
1 - Makes the sound start On, loops the sound when complete
2 - Makes the sound start Off
4 - Everybody on the entire map can hear the sound
8 - Only the activator can hear the sound
```

### Keys:

```
noise       - wav file to play
wait        - Seconds between auto triggerings, 0 = don't auto trigger
random      - Wait variance, default is 0
targetname  - make the trigger target this value for the entity to be used
```

### Example code:
```
/place target_speaker 50 spawnflags,1,noise,sound/ambience/narshaddaa/cantina_1.mp3,
```

- here is a speaker for some cantina music you may hear at a bar, it will load automatically with spawnflags 1, i added 50 to the z-axis so its louder, also if its not loud enough you can try placing more of them

## target_print **
This will print a message across the screen. Basically the same as the announce command. If no spawnflag is set, it will make the print global.

### Spawnflags:

```
1   - Red Team Only
2   - Blue Team Only
4   - Only the person who activated the entity can see the message.
8   - Message will appear at top left corner of the screen
16  - Message will appear in area where chat normally appears
```

### Keys:

```
message     - text to print
wait        - don't fire off again if triggered within this many milliseconds ago
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_print * spawnflags,4,targetname,bar_text,message,^4No Laming\n ^0In the bar!,
```

- this is the entity that does basically what "/announce" does, it prints a message that you tell it to across the screen, spawnflags 4 makes it so only the activator can see the message, without it everyone can see it. Notice in the message i used "\n" this is the key you can enter anytime during a message to tell it to enter down.

## target_score **
Adds points to the players score once fired.

### Spawnflags:

```
1 - Sets your score to this count when used instead of adding to your score
```

### Keys:

```
count       - number of points to add to the activator
              (default 1)
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_score * targetname,negative_points,count,-5,
```

- when someone uses this with a button it will give them -5 points

## target_kill **
This when targeted will kill the person/npc who fires it.

### Keys:

```
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_kill * targetname,kill
```

- Kinda self-explanatory, but when something targets this it will kill its activator.

## target_counter **(L)
You have to use this entity a specified amount of times for it to fire at its target, the default is 2 times ravensoft made it so it will deactivate after it gets hit a certain amount of times, but if you set a bouncecount at -1 the entity will keep on being able to be used.

### Spawnflags:

```
1 - Start Inactive, must be hit by a target_activate to be able to be used.
```

### Keys:

```
target        - what to first fire at when the entity as been hit a certain amount of times.
target2       - fires this every time you target the entity and it isnt at its count
count         - the amount of times to be hit for firing its target
                (default 2)
bounceCount   - set this at -1 so the entity will never stop working, otherwise after the first use it will stop working 
                if you set bouncecount to 5 then after the fifth use it will stop working.
targetname    - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_counter * targetname,double_NPC_counter,target,dead_NPCS,count,2,bounceCount,-1,
```

- I've used this for quests that i wanted to fight 2 NPC's at once, each NPC_target will be double_NPC_counter, and once the counter gets used twice, or the two npcs die, it will fire its target. The bouncecount is huge because i want it to last for a long time, without stop working.

## target_random **
Randomly fires off only one of it's targets each time used, does the exact same thing as the target_relay with the random spawnflag it doesn't have a target2, target3, target4, target5, target6 function just set one target and multiple entities with that targetname. It will only fire one of them.

### Spawnflags:

```
1 - only be usable once, then disable itself
```

### Keys:

```
targetname  - make the trigger target this value for the entity to be used
target      - should target more than one entity, it will only fire at one of its targets.
```

### Example code:
Ok lets say i wanted to press a button and get a random amount of credits back..

The random selector:

```
/place target_random * targetname,random_reward,target,random_credits,
```

My array of credits:

```
/place target_credits * targetname,random_credits,count,2,
/place target_credits * targetname,random_credits,count,-2,
/place target_credits * targetname,random_credits,count,1,
/place target_credits * targetname,random_credits,count,5,
```
- all the targetnames for the credits should be the same, and the target_random will fire off only one of its targets, so you may get 2 once, and the next time you can get -2 credits.

## target_relay **(L)
Does nothing but fire at its targets, can fire at up to 6 targets it will fire them all at the same time.

### Spawnflags:

```
4     - makes the target get fired off randomly, currently this only works for target1.
128   - makes the entity start deactivated
```

### Keys:

```
targetname  - make the trigger target this value for the entity to be used
target      - the first target to fire
target2     - the second target to fire
target3     - the third target to fire
target4     - the fourth target to fire
target5     - the fifth target to fire
target6     - the sixth target to fire
```

### Example code:

```
/place target_relay * targetname,blah,target,light1,target2,light2,
```

- This will switch light entities on or off, if one of them is set to be deactivated first.

## target_scriptrunner **
Starts a script once used.

### Spawnflags:

```
1     - Will run the script on the entity that used this or tripped the trigger that used this
128   - Will start in the off state.
```

### Keys:

```
useScript   - Script to run when used
count       - how many times to run, -1 = infinite. 
              (default 1)
wait        - can't be used again in this amount of seconds 
              (default 1 second, if it's multiple-use)
delay       - how long to wait after use to run script
```

### Example code:

```
/place target_scriptrunner * targetname,run_script,Usescript,close_door_cinematic,count,1,
```

- this is a scriptrunner that i very very rarely use, it is meant to run scripts that are already in the game, i made it target a script that would close a door, i set count to one, because it should only fire once.

## target_activate **(L)
Will set the target(s) to be usable/triggerable It accepts its target(1-6) The activate must first be fired for it to activate its target.

### Keys:

```
targetname  - make the trigger target this value for the entity to be used
target      - the entity(s) to activate when targeted
target2     - the entity(s) to activate when targeted
target3     - the entity(s) to activate when targeted
target4     - the entity(s) to activate when targeted
target5     - the entity(s) to activate when targeted
target6     - the entity(s) to activate when targeted
```

### Example code:

```
/place target_activate * target,the_door,targetname,initial_entity,
```

- this entity will activate its targets, i want it to activate the door at the start of the map

## target_deactivate **(L)
Will set the target(s) to not be usable/triggerable. It accepts its target(1-6). The activate must first be fired for it to deactivate its target.

### Keys:

```
targetname  - make the trigger target this value for the entity to be used
target      - the entity(s) to deactivate when targeted
target2     - the entity(s) to deactivate when targeted
target3     - the entity(s) to deactivate when targeted
target4     - the entity(s) to deactivate when targeted
target5     - the entity(s) to deactivate when targeted
target6     - the entity(s) to deactivate when targeted
```

### Example code:

```
/place target_deactivate * targetname,tele_area_deact,target,tele_area,
```

- this would deactivate whatever its targeting, in this case i made it so it would deactivate a teleport area once it has been used

## target_play_music **
Plays the requested music files when this target is used. You must set the music key when placing this otherwise it will crash the server.

### Keys:

```
music       - directory of to use music WAV or MP3 file
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_play_music * targetname,initial_entity,music,music/mp/duel.mp3,
```

- Will change the music of the map to duel music once the map starts...

## target_screenshake **
anyone near this entity has their screen shake around, in Lugormod T2 it is bugged and always does global

### Keys:

```
intensity     - intensity of the shake
duration      - how long the shake lasts
globalshake   - if set to anything > 0, then everyone in the server will have this effect them.
                (default 0)
targetname    - make the trigger target this value for the entity to be used
```

### Example code:

```
/place target_screenshake * intensity 2000,duration,100,globalshake,1,targetname,shakey,
```

## target_laser **
Makes an invisible line between this entity and its target that damages you. Kinda pointless i know. It starts in the off state so you have to use its targetname for it to turn on

### Keys:

```
targetname  - make the trigger target this value for the entity to be used, and turn on
target      - make this target a target_position or something, you don't need to do this you can just set angles.
```

### Example code:

```
/place target_laser 20 target,laser_pos1,targetname,the_laser,
```

## target_credits **(L)
gives the person who fires this an amount of credits

### Keys:

```
count       - the amount of credits to give the activator
random      - the amount of credits you want to randomly receive this number is added onto the count
              If your count is 20 and you set this to 10 you will receive anywhere from 20-30 credits
targetname  - make the trigger target this value for the entity to be used, the person who triggers this will receive the set amount of credits
```

### Example code:

```
/place target_credits * targetname,credits,count,5,random,20
```

- Now you will receive credits from 5-25 anywhere between there