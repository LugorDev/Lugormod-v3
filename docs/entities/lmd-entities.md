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

# lmd_ Entities

## lmd_chance * (L)
When used, gets a random number between 1 and "count", and will go from "chance" to "chance6" checking if its number is less than or equal to it.
Will fire the first target found then stop.

### Keys:

```
chance    - Number to compare
chance2   - Number to compare
chance3   - Number to compare
chance4   - Number to compare
chance5   - Number to compare
chance6   - Number to compare
count     - The maxmum random number to generate
target    - Corresponding target to fire.
target2   - Corresponding target to fire.
target3   - Corresponding target to fire.
target4   - Corresponding target to fire.
target5   - Corresponding target to fire.
target6   - Corresponding target to fire.
```

### Example Code:

```
/place lmd_chance * count,7,chance,1,chance2,2,chance3,3,chance4,4,target,hit_1,target2,hit_2,target3,hit_3,target4_hit4,
```

- This randomly genrates a number from 1-7 and fire the target if the number is the same. If it goes over 4, it doesn't fire anything. 4/7 of the time it will do nothing.

## lmd_restrict *(L)
Restricts the person inside of it by whatever spawnflags are set. Use maxs/mins to set the bounding box.

### Spawnflags:

```
1    - Players in this area will not take damage.
2    - Players in this area will not be able to use forcepowers.
4    - Players in this area will not be able to use their jetpack.
8    - Players in this area will not be able to duel.  Existing duels will be broken if a player enters it.
16   - Players in this area will not be able to fire weapons.  Players may still see the weapon fire animation.
32   - Players in this area will not be able to use force jump.
128  - Start disabled.  Must be used by a target_activate to have any effect.
256  - Allow Desann Stance.
512  - Allow Tavion Stance.
1024 - Disallow special saber moves.
```

### Keys:

```
maxs/mins
```

### Example code:

```
/place lmd_restrict 0 mins,-500 -500 0,maxs,500 500 500,spawnflags,3,
```

- Restricts damage and forcepowers in the defined area.

## lmd_stashzone ***(L)
A zone to define an area for stashes. Also controls how many stashes can be spawned per zone. A stash may be set to delete itself when removed from its zone.

### Spawnflags:

```
1 - start inactive (stashes with no active zone can be taken anywhere).
2 - master zone. Any stash spawns that are outside a zone by their zone key will be affected by the count key of a master zone. 
    Having multiple zones under a single key with this spawnflag is stupid.
```

### Keys:

```
targetname  - name of this zone.
count       - number of stashes that can be spawned at once within this zone. This overrides any settings of individual stash spawns. 
              This is per zone-entity.
              if a stash is brought into another zone of the same name but out of this one it will no longer count for this zone.
```

### Example code:

```
/place lmd_stashzone 0 mins,-1200 -1200 -1200,maxs,1200 1200 1200,spawnflags,2,targetname,main_stashzone,count,1,
```

## lmd_stashspawnpoint ***(L)
Point where stashes spawn. Can be triggered to manually spawn a stash. Multiple stashes can be spawned at once. It is possible to make a stash with 0 credits for the purpose of making it use a target when deposited (see the deposittarget key). A new stash will not spawn if the last one has not been picked up. stash spawn points that have been deactivated will not spawn stashes, although they will still run their spawn timers (they will just not spawn the stash when the timer fires). Important: if you want to use the traditional method of one stash spawning randomly, make sure all stash spawns have a random key, else only one point will spawn stashes. Or just set the spawn time to 0 and use a target_random targeting all the spawn points.

### Spawnflags:

```
1       - spawnmessage global: send the spawnmessage to everyone in the server (rather than in the stash zone), if the spawnmessage key is set.
2       - spawnmessage screen print: send the spawnmessage as a screen print, as well as in the console.
4       - pickupmessage global: send the pickup message to everyone in the server (rather than in the stash zone), if the pickupmessage key is set.
8       - pickupmessage screen print: send the pickupmessage as a screen print, as well as in the console.
16      - dropmessage global.
32      - dropmessage screen print.
64      - startdepomessage global.
128     - startdepomessage screen print.
256     - depositmessage global.
512     - depositmessage screen print.
1024    - resetmessage global.
2048    - resetmessage screen print.
4096    - display stash amount: include the credits amount on all stash messages.
8192    - stashes can spawn here even if a stash has been spawned by another spawn within the zone.
16384   - start disabled. Needs to be used by a target_activate to start spawning.
32768   - reset credits shift when dropped.
65536   - Stashes in this zone will respawn at this spawn. If no spawns have this set, stashes will not reset and just delete themselves.
131072  - Delete the stash if it leaves the zone. See zoneexittime and zoneexitmessage keys.
```

### Keys:

```
customskill         - The name of the skill to check (see lmd_customskill for details)
customskillcompare  - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                      (default 0)
customskillvalue    - The value to compare to.
zone            - the lmd_stashzone to connect this stash to. Zones control how many stashes can be spawned at once. 
                  Also, this is the area to not let the stash be removed from, if specified.
zoneexittime    - If enabled by spawnflag, how many seconds to wait before deleting the stash after it leaves the zone. 
                  (default 4)
zoneexitmessage - If a player has a stash and exits the zone, this message will be displayed along with a countdown timer. 
                  (default "You have left the stash zone.")
profession      - profession bitmask of who can use this.
minlevel        - min level needed to pick this up.
maxlevel        - any players above this level cannot pick this up.
property        - this entity will only work for people with access to the property set here
playerflags     - only players with a flag(s) specified here can pick this up.
minstashes      - players cannot pick this stash up until they have picked up this many stashes (from other spawns).
maxstashes      - players cannot pick this stash up if they have picked up more than this amount of stashes.
name            - the name to use for this stash, defaults to "money stash"
count           - number of stashes that this spawn can have spawned at once. 
                  If this is set to 0, the stash can be picked up by unregistered players and will not count as a stash deposit 
                  (will not count for the minstashes/maxstashes counter)
                  (default 1)
basedeposittime   - time in msecs needed to deposit (lmd_stashdepo can modify this)
basedepositrandom - random amount to add to the base deposit time on pickup. Amount added is random between 0 and this number.
credits           - base credit amount
creditsrandom     - random amount to add for core credits value.
sound             - sound to make on pickup.
creditshift       - credit amount to shift per second. Can be positive (increase worth longer its picked up) 
                    or negative (decrease worth longer its picked up). If the stash amount reaches 0, the stash is reset.
creditshiftrandom - random amount to add to offset each credit shift by.
creditshifttime   - how many seconds to run the shift for.
creditshiftdelay  - how long to wait before starting the shift.
spawntime         - time between spawns. Set to 0 to only spawn on use. Defaults to the contents of the cvar lmd_stashrate
spawnrandom       - random amount between 0 and this to offset the spawn time by.
stashtime         - after this many seconds after being picked up, make the stash disappear.
stashtimerandom   - A number between 0 and this will be added to the stash time.
spawnmessage      - message to send on spawn
pickupmessage     - message to send to players in the zone when picked up (can be sent to all players by spawnflag)
dropmessage       - message to send to players in the zone when dropped (can be sent to all players by spawnflag)
startdepomessage  - message to send to players in the zone when starting deposit (can be sent to all players by spawnflag)
depositmessage    - message to send to players in the zone when deposited (can be sent to all players by spawnflag)
resetmessage      - message to send when the stash is deleted due to leaving the zone or timing out.
color             - color glow to emit when not picked up. 
                    Recommend making all stashes in a zone use the same color.
light             - light intensity to emit when not picked up
pickupcolor       - color glow to emit when picked up. Recommend making all stashes in a zone use the same color.
pickuplight       - light intensity to emit when picked up.
model             - model to use. 
                    Recommend making all stashes in a zone use the same model. 
spawntarget       - target to use on spawn
pickuptarget      - target to use on pickup
droptarget        - target to use on drop
startdepotarget   - target to use on deposit start
deposittarget     - target to use on deposit.
```

### Example code:

```
/place lmd_stashspawnpoint 4 spawnflags,169301,zone,main_stashzone,zoneexitmessage,You're leaving the stashzone.,credits,80,creditsrandom,50,creditshift,-1,creditshiftdelay,60,
```

- As the size of this spawnstring is too large to fit into console, you may have to manually /spawnstring edit the keys in or you will have to place it using a script.

## lmd_stashdepo ***(L)
"Bank" You can set deposittime to 0 and have a trigger use this, if you don't want to require the player to use the entity directly.

### Spawnflags:

```
1   - no deposit time. Useful if you want to make another non-player entity trigger this
2   - don't display stash status. If set, the terminal will not tell the user if a stash is out when it is used.
4   - don't show up in /stash command.
128 - start deactivated. Must be used by a target_activate to be usable.
```

### Keys:

```
zone                  - only deposit stashes belonging to this zone.
stashname             - name to refer to the stashes with. Only used for displaying the count of stashes 
                        (not used if spawnflag 2 is set).
stashnameplural       - same as stashname, but if there are more than 1 stashes
message               - message to print when used without a stash.
profession            - profession bitmask of who can use this.
minlevel              - min level needed to pick this up.
maxlevel              - any players above this level cannot pick this up.
minstashes            - players cannot pick this stash up until they have picked up this many stashes (from other spawns).
maxstashes            - players cannot pick this stash up if they have picked up more than this amount of stashes.
depotimeoffset        - offset to change the stash base deposit time by. Can be negative. Stash deposit time will not go below 0.
depotimeoffsetrandom  - random shift to change the deposit time by.
bonuscredits          - get extra credits for depositing here. Can be negitive to remove credits from the stash. 
                        Will not remove more credits than the stash has.
bonusrandom           - random amount to add to bonus credits.
startdepomessage      - message to add to the bottom of the startdepomessage of the stash.
depositmessage        - message to add to the bottom of the depositmessage of the stash.
startdepotarget       - target to use on deposit start
deposittarget         - target to use on deposit.
model                 - md3 model to draw, uses max/mins too.
```

### Example code:

```
/place lmd_stashdepo 0 zone,master_stashzone,model,models/map_objects/imperial/control_station,model,models\map_objects\imperial\control_panel.md3,
```

## money_dispenser *(L)
Spawns a money depo where you can depo stashs at.

### Example code:

```
/place money_dispenser 0 angle,90,
```

## random_spot ***(L)
when placed, it spawns a spawn point for a stash, one random_spot is used randomly each time a money stash is spawned.

### Example code:

```
/place random_spot 0
```

## lmd_scale **(L)
Scales a player when they trigger this to the set count

### Keys:

```
scale       - The scale amount you want the player to be
time        - The time it takes for the players scale to be set back to 100.
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_scale * targetname,scaler,scale,50,time,20,
```

## lmd_playercheck ** (L)
Entity will fire target if the activator meets the usability keys

### Keys:

```
level
adminlevel
profession
customskill
customskillcompare
customskillvalue
playerflags
targetname

target    - fire if they meet requirements
target2   - fire if they do not meet requirements
```

### Example code:

```
/place lmd_playercheck * targetname,let_me_in,level,5,target,teleport_me_in,target2,deny_msg,
```

## lmd_playereffect ** (L)
When triggered, this entity will use its target when enabled, and use its target2 when disabled.

### Keys:

```
effect      - 1: Invincible, 2: Electrocution, 3: Fall to death, 4: Jail, 5: Godmode, 6: Shield, 7: Notarget, 8: Invisible, 9. Undying., 10: Freeze
wait        - Time to play the effect for.  Default 30.
```

### Example code:

```
/place lmd_playereffect * targetname,apply_effect,effect,2,wait,5
```

## LMD_spawner **(L)(If no targetname)
This entity was implemented by Lugor, it should be able to use any of the keys provided for npc_spawner and one more, it also allows the NPC to respawn after it has been destroyed.

### Keys:

```
customscale   - scale this npc/vehicle when it spawns.
count         - ammount of NPCs it is allowed to be on the map at once from this spawn
```

### Example code:

```
/place LMD_spawner 40 NPC_type,swoop,count,1,targetname,Sweet_Ride
```

## lmd_gravity **(L)
changes an individual players gravity

### Spawnflags:

```
1   - remove the changed gravity flag, so the player uses standard gravity again 
      (use instead of setting it to 800, as gravity will be changed by vehicles if this is the case)
2   - makes the gravity change global, so everyone has the change
```

### Keys:

```
count       - gravity to change to, note 800 is default for g_gravity
time        - the time it takes to reset gravity back to normal
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_gravity * targetname,fatty,count,2000,
```

## lmd_pwterminal *(L)
The player stands in front of it and types /interact <password> if the password is correct it fires its target, if not, then it fires target2

### Keys:

```
customskill         - The name of the skill to check (see lmd_customskill for details)
customskillcompare  - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than.
                      (default 0)
customskillvalue    - The value to compare to.
profession          - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level               - Minimum player level.
maxlevel            - Maximum player level.
adminlevel          - Minimum admin level.
property            - this entity will only work for people with access to the property set here
playerflags         - A bitmask of flags set by the lmd_playerflag entity.
model               - The model. If the model is a glm model, you need to include the '.glm' extention. 
                      You do not need the 'map_objects/' folder in the path.
mins/maxs           - Hitbox data. Both values are vectors containing the x, y, and z sizes from the model's origin. 
                      If no mins or maxs are specified, then a hitbox is automatically calculated to cover the entire model.
message             - Text to show when used.
password            - The correct password.
target              - Target to fire when correct password is entered.
target2             - Target to fire when incorrect password is entered.
target4             - Target to fire when any password is given.
target5             - Target to fire when used.
```

### Example code:

```
/place lmd_pwterminal 0 model,models/map_objects/factory/f_con1,target,enter_bar,target2,kill,target3,banana,message,Enter Bar,
```

## lmd_toggle **(L)
Uses its targets in order until it hits its count, then resets.

### Keys:

```
target      - the first target to fire
target2     - the second target to fire
target3     - the third target to fire
target4     - the fourth target to fire
target5     - the fifth target to fire
target6     - the sixth target to fire
count       - the last target to use before resetting to its first target, should be set so if you have three targets the count would be three
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_toggle * target,red_light,target2,blue_light,target3,green_light,count,3,targetname,light_sequence,
```

## lmd_light **(L)
Spawns a light on the map, recommended you add a number greater than 0 to the z-axis to see the whole light

### Keys:

```
color       - the RGBcolors of the light in decimal percent (0 to 1, .5 would be 50%, EX: '0 .5 0' would be a faint green)
light       - the radius that the light can travel
offcolor    - same as color but when toggled off. 
              (default '0 0 0')
offlight    - radius when toggled off 
              (default 0)
targetname  - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_light 0 color,1 0 1,light,500,offcolor,0 1 0,offlight,500,targetname,light,
```

## lmd_door **(L)
Spawns a model that can be used as a door. If you don't set a targetname the door will have a trigger area and will fire to its open state when you get near it.

### Spawnflags:

```
1       - the door to moves to its destination when spawned, and operate in reverse. 
          It is used to temporarily or permanently close off an area when triggered (not useful for touch or take damage doors).
2       - Can be activated by force push/pull but also it can have a targetname
4       - kills the player if the player is stuck between it and a wall, and its moving (it won't reverse it movement when hits the player)
8       - wait in both the start and end states for a trigger event (stay opened until you use it again)
16      - Starts locked, with the shader animmap at the first frame and inactive. 
          Once used, the animmap changes to the second frame and the door operates normally. Note that you cannot use the door again after this.
64      - Player can use it with the use button (you can press it like a button)
128     - must be used by a target_activate before it can be used
```

### Keys:

```
model           - the md3 model to draw
target          - Door fires this when it starts moving from it's closed position to its open position.
opentarget      - Door fires this after reaching its "open" position
target2         - Door fires this when it starts moving from it's open position to its closed position.
closetarget     - Door fires this after reaching its "closed" position
targetname      - if set, no touch field will be spawned and a remote button or trigger field activates the door.
speed           - movement speed 
                  (default 100)
wait            - wait before returning (-1 = never return)
                  (default 3)
delay           - when used, how many seconds to wait before moving - default is none
dmg             - damage to inflict when blocked (set to negative for no damage)
                  (default 2)
health          - if set, the door must be shot open
linear          - set to 1 and it will move linearly rather than with acceleration 
                  (default 0)
team            - set it to the same name on two or more doors so they'll open if one of them is used, don't use too long names
vehopen         - if non-0, vehicles/players riding vehicles can open
movement        - x y z value, movement direction. movement,0 0 100, will make it move 100 units up.
```

### Example code:

```
/place lmd_door 0 model,map_objects/factory/catw2_b,speed,90,targetname,door_1,target,deact_button,closetarget,act_button,
```

- Spawns a lmd_door that moves at speed: 90, it allows vehicles to go threw it, it deacts a button once used, and then once its in its closed state it will activate the button.

## lmd_propertyterminal *(L)
What players can use to fire a target that only that set group of players can fire. The command, "/propadmin", will allow you to set who can use it.

### Spawnflags:

```
1 - If 'count' greater than zero, allow the renter to pay more money after the first payment to increase their time.
```

### Keys:

```
customskill             - The name of the skill to check. (see lmd_customskill for details)
customskillcompare      - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                          (default 0)
customskillvalue        - The value to compare to.
property                - The property to check to fire this entity.
profession              - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level                   - Minimum player level.
maxlevel                - Maximum player level.
adminlevel              - Minimum admin level.
playerflags             - A bitmask of flags set by the lmd_playerflag entity.
model                   - The model. If the model is a glm model, you need to include the '.glm' extention. 
                          You do not need the 'map_objects/' folder in the path.
mins/maxs               - Hitbox data. Both values are vectors containing the x, y, and z sizes from the model's origin. 
                          Normally, the Mins data is negitive. If no mins or maxs are specified, then a hitbox is automatically 
                          calculated to cover the entire model.
property                - The name of the property to check access to.
target                  - Target to fire if the user is a member of the property.
target2                 - Target to fire if the user is not a member of the property.
count                   - Number of credits per day this terminal costs to rent. 
                          If not set, then this terminal will not be rentable. Players can pay money to rent the terminal with '/pay <credits>'
mindays                 - If 'count', the minimum number of days this player can rent the property for.
maxdays                 - If 'count', the maximum number of days this player can rent the property for.
maxrenters              - If 'count', the maximum number of renters that this terminal will allow. Defaults to 1.
rank                    - If 'count', the property rank to give to new renters. Values are: 1 - owner, 2 - caretaker, 3 - guest
```

### Example code:

```
/place lmd_propertyterminal 0 model,models/map_objects/factory/f_con2,spawnflags,2,target,my_door,property,flips_door,
```

- This is what you would do for the first group of people you want, if you wanted to have another terminal for another group you would change the count, or property set:

```
/place lmd_propertyterminal 0 model,models/map_objects/factory/f_con2,spawnflags,2,target,his_door,property,somedudes_door,
```

## lmd_remap **(L)
This will change the shader on the map to another shader you specify. If no targetname is set, it will change shaders for you on map start

### Spawnflags

```
1 - Toggable, switches from new to old when targeted.
```

### Keys:

```
old - the shader directory of what you want to change.
new - the shader directory of what your changing the shader into.
targetname - the name of the entity, when someone fires at this it will change the shader you are referring to.
```

### Example code:

```
/place lmd_remap * old,textures/quicktrip/desert_0,new,textures/yavin/ground
```

## lmd_gate **(L)
If this is active, it uses its target when used. If it is inactive (set by a target_deactivate), then it fires its target2 when used.

### Spawnflags:

```
1 - Makes it start off
```

### Keys:

```
target - what to fire at when used, when the entity is active
target2 - what to fire at when used, when the entity is deactivated
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_gate * targetname,gate,target,act_spawn,target2,act_spawn2,spawnflags,1,
```

## lmd_drop **(L)
drops a model from the origin of this entity that can be picked up and fires a target
Create and launch a model that can be picked up by players.

### Spawnflags:

```
1: Player must use the item to pick it up rather than touching it.
```

### Keys:

```
customskill             - The name of the skill to check. (see lmd_customskill for details)
customskillcompare      - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                          (default 0)
customskillvalue        - The value to compare to.
profession              - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level                   - Minimum player level.
maxlevel                - Maximum player level.
adminlevel              - Minimum admin level.
property                - this entity will only work for people with access to the property set here
playerflags             - A bitmask of flags set by the lmd_playerflag entity.
model                   - The model. If the model is a glm model, you need to include the '.glm' extention. 
                          You do not need the 'map_objects/' folder in the path.
mins/maxs               - Hitbox data. Both values are vectors containing the x, y, and z sizes from the model's origin. 
                          Normally, the Mins data is negitive. If no mins or maxs are specified, then a hitbox is automatically 
                          calculated to cover the entire model.
target                  - Target to fire when picked up.
target2                 - Target to fire when time runs out.
noise                   - Sound to make when being picked up
count                   - Number of credits to give the player. 
                          If this is set and no noise key is specified, then the noise key defaults to sound/interface/secret_area.wav
velocity                - Speed at which to toss this item. Default 50.
angles                  - Angles to launch this item at
time                    - Number of seconds to stay for. Set this to -1 for no time limit. Default 30.
wait                    - Number of seconds to wait between triggerings.
```

### Example code:

```
/place lmd_drop 5 model,map_objects/desert/3po_head,count,50,targetname,drophead,velocity,0,wait,30
```

## lmd_terminal *(L)
When used it echoes different commands a player can use (/interact <1 to 6>)

### Spawnflags:

```
1: Send the output to the player's screen.
2: Do not send the output to the player's console.
```

### Keys:

```
customskill             - The name of the skill to check. (see lmd_customskill for details)
customskillcompare      - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                          (default 0)
customskillvalue        - The value to compare to.
profession              - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level                   - Minimum player level.
maxlevel                - Maximum player level.
adminlevel              - Minimum admin level.
property                - this entity will only work for people with access to the property set here
playerflags             - A bitmask of flags set by the lmd_playerflag entity.
model                   - The model. If the model is a glm model, you need to include the '.glm' extention. 
                          You do not need the 'map_objects/' folder in the path.
mins/maxs               - Hitbox data. Both values are vectors containing the x, y, and z sizes from the model's origin. 
                          Normally, the Mins data is negitive. If no mins or maxs are specified, then a hitbox is automatically 
                          calculated to cover the entire model.
message                 - Message to display when used.
usetarget               - Target to fire when the player presses the use key on this.
globaltarget            - Targe to fire when any command is used.
cmd-cmd6                - Name of each command.
target-target6          - Target to use for each command.
```

### Example code:

```
/place lmd_terminal 0 model,map_objects/factory/f_con1,spawnflags,2,message,I have some powerups,cmd,Bodyshield,cmd2,Speed,cmd3,Red Light,target,bodyshield_pu,target2,speed_pu,target3,rlight_pu,
```
- Spawns a terminal that displays the message "I have some powerups" at the top of the screen and has 3 options displayed. Corresponding targets fire.

## lmd_iobject **(L)
When used it grants the client an item specified here that can be used with the /inventory command.

### Keys:

```
name: The custom name you want this item to have.
noCombine: If this is set to 1, this item will not combine with others.
noDelete: If this is set to 1, the user will not be able to delete this item.
type: type of iobject to use
      The current iObject types are:

      ```
      medpack (combinable)
      weaponpack (not combinable)
      ammopack (combinable)
      upcount (combinable) Inverse keycard, for collection quests.
      downcount (combinable) Keycard 2.0
      ```

health: Amount of health to give when used.
shield: Amount of shield to give when used.
uses: Number of uses.

blaster: ammo count.
powercell: ammo count.
metalbolt: ammo count.
rocket: ammo count.
thermal: ammo count.
tripmine: ammo count.
detpack: ammo count.
```

### Weapons: bitmask of weapons

written as: weapon,<bitmask>,
        
```
2 WP_STUN_BATON
4 WP_MELEE
8 WP_SABER
16 WP_BRYAR_PISTOL
32 WP_BLASTER
64 WP_DISRUPTOR
128 WP_BOWCASTER
256 WP_REPEATER
512 WP_DEMP2
1024 WP_FLECHETTE
2048 WP_ROCKET_LAUNCHER
4096 WP_THERMAL
8192 WP_TRIP_MINE
13684 WP_DET_PACK
32768 WP_CONCUSSION
65536 WP_BRYAR_OLD
```
        
NOTE: this does NOT give ammo. The user must pick up ammo to use, or in some cases see, the weapon.

### iObject type keys

```
* upcount (combinable) Inverse keycard, for collection quests.
        property: Name of the property to grant access to when enough of these are picked up.
        max: Grant access to the property when "current" is greater than or equal to this.
        count: Current number of item type picked up. When another upcount item with the same name and property key is picked up, 
               this number is increased by the new item's current value. Defaults to 1.

* downcount (combinable) Keycard 2.0
        property: The property to grant access to while "current" is less than "count".
        max: Maximum number of times to allow access to the property.
        count: Current number of uses left.
        noAutoDelete: Set this to 1 to disable automatic deletion this item when it runs out of uses.

        While "keycard" still will spawn using a conversion placeholder for backwards compatibility, its item no longer officially exists. 
        Use "downcount" instead.
```

### Example code: 

```
<NEED EXAMPLE>
```

## lmd_customskill
An entity that, when used, creates a new custom skill for the person who targets it.

### Spawnflags:

```
1 - (Use with Value key) rather than adding value to player's current skill, entity will set player's value for this skill to the amount set in the value key.
```

### Keys:

```
skill   - Name of the skill (choose your own)
value   - Essentially the "level" of the skill
```

## lmd_mover **(L)
A moving model can be used to do movements for pendulum/rotating/bobbing/door for any of the ontype,onatype,offtype,offatype keys use the list of available movements and specify it to that number. Ex: ontype,0, for don't move at all. The entity is toggleable so if you use its targetname it will switch to its off state. by default its off state movement type is 0.

### Spawnflags:

```
1 - The set model will be solid
```

### Keys:

```
model           - the md3 model to draw
targetname      - make the trigger target this value for the entity to be used
```

Types - Use the movements list, an 'ontype/offtype' is for movement, while an 'onatype/offatype' is a rotation.

```
ontype          - Movement type when in the on state (See types of movement list)
offtype         - Movement type when in the off state (See types of movement list)
onatype         - Rotation type when in the on state (See types of movement list)
offatype        - Rotation type when in the off state (See types of movement list)
```

Durations - The time it takes for the model to complete its action in milliseconds. An 'onduration/offduration' is for the time it takes to complete a movement, while an 'onaduration/offaduration' is the time it takes to complete a rotation.

```
onduration      - The time it takes for the model to move to the set delta when in the on state (milliseconds)
offduration     - The time it takes for the model to move to the set delta when in the off state (milliseconds)
onaduration     - The time it takes for the model to rotate to the set delta when in the on state (milliseconds)
offaduration    - The time it takes for the model to rotate to the set delta when in the off state (milliseconds)
```

Deltas - How far something will move/rotate to. An 'ondelta/offdelta' is how far the model will move. An 'onadelta/offadelta' how far the model will rotate to.

```
ondelta         - How many units to move to when in the on state. (X Y Z)
offdelta        - How many units to move to when in the off state. (X Y Z)
onadelta        - How far the model will rotate to from the base of the model in the on state. (X Y Z)
offadelta       - How far the model will rotate to from the base off the model in the off state. (X Y Z)
```

### Types Of Movement

```
0       - Don't move at all.
1       - Teleport between starting and ending point.
2       - Move towards its goal, after its goal is reached it will keep moving forever. Use this movement type for models that rotate.
3       - Move towards its goal, and stop when the goal is reached.
4       - Move towards its goal, near the end it will start to decelerate then stop.
5       - Slowly accelerate towards middle of goal. Accelerating fastest at the middle of its goal then slowly start to decelerate 
          after it hit the middle of its goal towards the end. It will come back down to the starting point and keep moving like that forever. 
          Use this for bobbing and pendulums.
6       - This one is kind of pointless. It will ignore any set delta keys and fall to the ground at 800 units per second (Default Gravity Setting) 
          and ignores any objects in its way. So it will keep falling through the floor.
```

### Example code:

Because this is new and not a lot of people understand it I will explain how to do the same movements t2_pendulum, t2_bobbing, and t2_rotating used to do.

```
/place lmd_mover 0 model,models/map_objects/desert/3po_head,onaduration,1000,onadelta,0 180 0,onatype,2,
```

- Spawns a sphere that will rotate 180 degrees and keep rotating for ever it takes 1 second to complete its goal

```
/place lmd_mover 0 model,models/map_objects/factory/catw2_b,spawnflags,1,ontype,5,onduration,3000,ondelta,0 0 200
```

- Spawns a catwalk that will bob up and down 200 units. It takes 3 seconds to complete its goal

```
/place lmd_mover 0 model,models/map_objects/factory/catw2_b,onaduration,5000,onatype,5,onadelta,50 0 0,angles,90 0 0,spawnflags,1,
```

- Spawns a catwalk that will swing in a pendulum motion 50 units back and forth takes 5 seconds to complete the full movement

## lmd_rentterminal *(L)
Entity is used for a player to use the /pay command on the terminal and then he can have access to it as long as he wants or until it reaches the set max time.

### Spawnflags:

```
1 - Time will expire if player dies while owning the terminal
2 - Can pay for more time while you already own the terminal
4 - Tell the renter when they are at 30/15/5 seconds left.
```

### Keys:

```
customskill             - The name of the skill to check. (see lmd_customskill for details)
customskillcompare      - Comparison type. -1 for direct compare (use this for text), 0 for greater or equal to, 1 for less than. 
                          (default 0)
customskillvalue        - The value to compare to.
profession              - A bitmask of professions. Values: 1 - Jedi, 2 - Merc, 4 - Tech.
level                   - Minimum player level.
maxlevel                - Maximum player level.
adminlevel              - Minimum admin level.
property                - this entity will only work for people with access to the property set here
playerflags             - A bitmask of flags set by the lmd_playerflag entity.
model                   - The model. If the model is a glm model, you need to include the '.glm' extention. 
                          You do not need the 'map_objects/' folder in the path.
mins/maxs               - Hitbox data. Both values are vectors containing the x, y, and z sizes from the model's origin. 
                          Normally, the Mins data is negitive. If no mins or maxs are specified, then a hitbox is automatically 
                          calculated to cover the entire model.
message                 - The message to be displayed when the terminal is used.
count                   - Cost.
minutes                 - Number of minutes to gain when payed 'count' number of credits.
wait                    - Minimum time to wait between payments, if spawnflag 2 is set.
maxtime                 - Maximum number of minutes a player can rent.
mintime                 - Minimum number of minutes a player can rent.
target                  - Target to fire when used by its current renter.
target2                 - Target to fire when the rent runs out.
target3                 - Target to fire when first rented.
target4                 - Target to fire when used by someone who is not the renter while being rented.
target5                 - Target to fire when used by anyone when not being rented.
```

### Example code:

```
/place lmd_rentterminal 0 model,models/map_objects/factory/f_con1,message,Rent House,minutes,60,count,50,target,tele_inhouse,target2,act_exit,target3,moremoney_msg,
```

## lmd_body **(L)
Leaves behind a fake player body like misc_camera does when you use it. Any damage given to the body while the entity is in use is forwarded to the client who used it. Unless the trigger that targets this, targets a delay which targets a target_teleporter .01 seconds later you will be stuck in your body. The next time this entity gets used the body gets removed.

### Keys:

```
targetname - make the trigger target this value for the entity to be used
```

### Example code:

```
/place lmd_body 0 targetname,body
```

## lmd_flagplayer **(L)
sets a flag to a player so you can give them temporary access to stuff, works kind of like a keycard. When you target this it grants you lets say flags 5, then you can have misc_model_breakable with the playerflags key set to 5 and only this person will have access to it.

### Keys:

```
flags - the amount of flags to set to the player, only players with this set flags can have access to entities with the playerflags key.
```

### Example code:

```
/place lmd_flagplayer 0 flags,5,targetname,give_accesstobar,
```