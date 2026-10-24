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

# NPC Entities

## NPC_spawner **(L)
Spawns a NPC that you specify, remember all keys are case sensative.(Its not npc_type, its NPC_type), if this is going to be used by a button add "count,-1," so it can spawn more than once.

### Spawnflags:

```
16      - NPC can be in air, but will spawn on the closest floor surface below it
32      - Will spawn with no default AI (BS_CINEMATIC), or the blinking that happens when it spawns
64      - NPC will not be solid (can not be shot or attacked with saber, so will not get agressive)
256     - Spawner is shy (wont spawn if a player is looking at it)
```

### Keys:

```
NPC_type        - name of NPC to spawn
targetname      - name this NPC goes by for targeting
count           - how many NPCs to spawn (only if targeted), set to -1 for it to be able to spawn after the first use
                  (default 1)
delay           - how long to wait to spawn after used
wait            - if trying to spawn and blocked, how many seconds to wait before trying again
NPC_target      - NPC's target to fire when killed
NPC_target2     - NPC's target to fire when knocked out
NPC_target4     - NPC's target to fire when killed by friendly fire
NPC_target5     - Fires this target for the player, so if the NPC_target5 is targeting credits, player will receive credits for killing the npc.
NPC_target6     - target to fire when npc kills a player
NPC_type        - type of NPC to be spawned
health          - starting health 
                  (default 100)
weaponToss      - set to 1 to let them be disarmed by force pull
delay           - after spawned or triggered, how many seconds to wait to spawn the NPC
showhealth      - set to 1 to show health bar on this entity when crosshair is over it
noBasicSounds   - set to 1 to prevent loading and usage of basic sounds (pain, death, etc)
noCombatSounds  - set to 1 to prevent loading and usage of combat sounds 
                  (anger, victory, etc.)
noExtraSounds   - set to 1 to prevent loading and usage of "extra" sounds 
                  (chasing the enemy - detecting them, flanking them... also jedi combat sounds)
forceResistLevel - <0-4> The higher, the lower the chance of getting his weapon disarmed by force pull. 4 = no chance of disarming. Default is 0.
```


For and of the following keys use the script # that is listed after the script type.

### Script Type Keys:

```
spawnscript - default script to run once spawned (none by default)
usescript - default script to run when used (none by default)
awakescript - default script to run once awoken (none by default)
angerscript - default script to run once angered (none by default)
painscript - default script to run when hit (none by default)
fleescript - default script to run when hit and below 50% health (none by default)
deathscript - default script to run when killed (none by default)
```

### Script Type Spawnflags:

```
1: Stand around, do absolutely nothing
2: Roam around, collect stuff
3: Crouch-Walk toward their goals
4: Run toward their goals
5: Stay in one spot and shoot- duck when necessary
6: Wait around for an enemy
7: Follow a path, looking for enemies
8: Track down enemies and kill them
9: Run from enemies
10: Run from enemies, shoot them if they hit you
11: Run to your goal and shoot enemy when possible
12: Defend an entity or spot?
13: Stay hidden, shoot enemy only when have perfect shot and back turned
14: Attack, evade, use cover, move about, etc. Full combat AI - id NPC code
15: Go for lowest health buddy, hide and heal him.
16: Find nearest cover from enemies
17: Go get some ammo
18: Go somewhere and fight along the way
19: turn until facing desired angles
20: do nothing
21: Maintain a formation
22: Crouch-walk toward their goals
```

-To use a script you would enter the name of the script you want and then the # key of what you want it to actually fire when it hits that mark: ",spawnscript,14" should make the npc fight its enemies when spawned.

### Example code:

```
/place NPC_spawner 30 NPC_type,desann,count,-1,spawnscript,14,NPC_target,desann_dead,
```

## NPC_Vehicle **
Spawns a NPC Vehicle, please note the entity name is case sensitive.

### Spawnflags:

```
1   - die after certain amount of time of not having a pilot
2   - Fighters: Don't drop until someone gets in it 
      (this only works as long as no one has *ever* ridden the vehicle, to simulate ships that are suspended-docked)
      note: ships inside trigger_spaces do not drop when unoccupied
16  - spawn on the floor, instead of floating around in the air
32  - Will spawn with no default AI (BS_CINEMATIC), or won't blink when spawned
64  - Starts not solid
256 - Spawner is shy (wont spawn if a player is looking at it)
```

### Keys:

```
dropTime        - use with spawnflags 2, the vehicle will drop straight down for this number of seconds before flying forward
dmg             - use with spawnflags 1, delay in milliseconds for ship to explode if no pilot 
                  (default 10000)
speed           - use with spawnflags 1, distance for pilot to get away from ship after dismounting before it starts counting down the death timer
model2          - if the vehicle can have a droid, this NPC will be spawned and placed there
showhealth      - set to 1 to show health bar on this entity when crosshair is over it
targetname      - make the trigger target this value for the entity to be used
```

### Example code:
```
/place NPC_Vehicle 30 NPC_type,tie-fighter,count,-1,targetname,ship1a,spawnflags,3,dmg,1,dropTime,2
```