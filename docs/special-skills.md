# Level 40 starts and special skills

New characters start at level 40 when first selected. Returning Jedi, Mercenary
and unassigned characters below the configured starting level are raised to it
when selected or logged in. Existing credits and regular skill allocations are
retained. Choosing a profession also uses the configured starting level, so it
does not restart the regular level grind. Admin and bot professions are excluded
from the returning-character upgrade.

```cfg
seta lmd_startingLevel "40"
seta lmd_specialSkills "1"
seta lmd_skillCreditCost "100000"
```

`lmd_startingLevel` is clamped to 1–40. `lmd_specialSkills` enables UFO's KEBAB
skills in FFA. `lmd_skillCreditCost` sets credits per original UFO skill point;
the default gives prices of 100,000–600,000 credits. This is a new configurable
price policy: the original plugin used purchasable skill points. Invalid or
overflowing prices cannot be purchased. There is no remaining skill-point grind
or point budget for special skills.

Place a vendor with `/place lmd_skillshop`. It comes with the stock factory
console model. It can also be placed with the usual distance, model, angle,
hitbox and player requirement keys. `/saveit <map entity set>` persists it using
the existing entity saving system.

Press Use at the vendor to open a paged on-screen menu of your profession's
skills, prices and ownership status, with your current credit balance at the top.
Forward/Back selects a skill; Use or Attack opens its description and purchase
confirmation. Select Buy or Cancel, or press Alt Attack to go back or close the
menu. Holding a button does not repeat purchases. Each player has an independent
menu, and leaving the vendor or changing character closes it. A changed price
must be confirmed again. No console commands are needed for menu purchases;
`/interact <short name> buy` remains available as an optional shortcut.
Purchases require a level 40 character, the matching
profession, enough credits, and proximity to the vendor. Duplicate purchases do
not charge credits. A failed account-file open refunds the purchase.

Use `/sskills` to list your profession's skills and `/sskills <short name>` to
activate an owned ability. Original command shortcuts such as `/forcedash` and
`/summonbacta` also work. Passives apply automatically; spawn-related benefits
apply on the next spawn. Active skills retain UFO's cooldowns and Force/fuel
costs. Skills cannot be cast while dead, spectating, dueling, jailed, frozen,
using a vehicle or otherwise restricted. Cooldowns survive death and character
changes. Unlocks are per character and are retained across profession changes;
only the matching profession can use them. Legacy UFO ownership bitmasks are
also recognized when present in that character's custom data.

The implementation is ported from UFO's `KEBAB/skills.cpp` and the corresponding
passive/timer hooks in `KEBAB/main.cpp` and `KEBAB/hookfuncs.cpp`. It calls native
game functions and does not load the old binary-hook plugin. Run it without the
old special-skill plugin to avoid applying effects twice.

## Validation

Build with `-DLMD_REGRESSION_TESTS=ON`, then run
`ctest --test-dir build -C Release --output-on-failure`. The mock-engine tests
exercise account migration and round trips, character isolation, purchase
requirements, duplicate purchases, failed-save refunds, ability resource costs,
shield cleanup, and cooldowns surviving respawns. Menu input tests cover
confirmation, held buttons, cancellation, paging, changing prices, simultaneous
users, character changes, distance checks and godmode restoration on closure.

Before deployment, use a separate test server and account directory to check
terminal placement and collision, the full skill catalogue, ranged targets,
projectiles, status-effect expiry, death/disconnect cleanup, and regular duels.
The automated tests do not emulate the client renderer, navigation, physics or
the server's full combat loop.

## Catalogue

Prices below use the default credit multiplier.

| Profession | Short name | Skill | Type | Credits |
| --- | --- | --- | --- | ---: |
| Jedi / Sith | `battlemed` | Battle Meditation | Active | 200,000 |
| Jedi / Sith | `combustion` | Force Combustion | Active | 500,000 |
| Jedi / Sith | `destruction` | Force Destruction | Active | 200,000 |
| Jedi / Sith | `disablejp` | Jetpack Disabling | Active | 300,000 |
| Jedi / Sith | `energyball` | Energy Ball | Active | 400,000 |
| Jedi / Sith | `firemagic` | Fire Magic | Passive | 600,000 |
| Jedi / Sith | `forcebody` | Force Body | Active | 300,000 |
| Jedi / Sith | `forcebreach` | Force Breach | Active | 400,000 |
| Jedi / Sith | `forcecloak` | Force Cloak | Active | 100,000 |
| Jedi / Sith | `forcedash` | Force Dash | Active | 100,000 |
| Jedi / Sith | `forcereflex` | Force Reflex | Passive | 400,000 |
| Jedi / Sith | `forcescream` | Force Scream | Active | 300,000 |
| Jedi / Sith | `forcesever` | Force Sever | Active | 400,000 |
| Jedi / Sith | `forceshield` | Force Shield | Active | 300,000 |
| Jedi / Sith | `forceslow` | Force Slow | Active | 300,000 |
| Jedi / Sith | `forcestomp` | Force Stomping | Passive | 400,000 |
| Jedi / Sith | `highresistance` | High Resistance | Passive | 600,000 |
| Jedi / Sith | `incinerate` | Force Incineration | Active | 500,000 |
| Jedi / Sith | `insanity` | Force Insanity | Active | 500,000 |
| Jedi / Sith | `repulse` | Force Repulse | Active | 500,000 |
| Mercenary | `adrenaline` | Adrenaline Stimulation | Active | 200,000 |
| Mercenary | `blastarmor` | Blast Armor | Passive | 600,000 |
| Mercenary | `cortosisfist` | Cortosis Fist | Passive | 200,000 |
| Mercenary | `critical` | Critical Hit | Passive | 600,000 |
| Mercenary | `demolition` | Demolition | Passive | 400,000 |
| Mercenary | `drug` | Drug Injection | Active | 300,000 |
| Mercenary | `firewall` | Fire Wall | Active | 300,000 |
| Mercenary | `forceresist` | Force Resistance | Passive | 500,000 |
| Mercenary | `fueljump` | Fuel Jump | Active | 100,000 |
| Mercenary | `grapple` | Grapple Choke | Passive | 500,000 |
| Mercenary | `implant` | Implant Regeneration | Passive | 400,000 |
| Mercenary | `lshield` | Lightning Shield | Passive | 300,000 |
| Mercenary | `mobility` | Mobility | Passive | 500,000 |
| Mercenary | `poisondart` | Poison Dart | Active | 400,000 |
| Mercenary | `pourfuel` | Pour Fuel | Active | 400,000 |
| Mercenary | `seeing` | Seeing | Active | 300,000 |
| Mercenary | `slowregen` | Slow Regeneration | Active | 300,000 |
| Mercenary | `summonbacta` | Summoning Bactas | Active | 100,000 |
| Mercenary | `timebomb` | Time Bomb | Active | 500,000 |
| Mercenary | `weaponprof` | Weapon Proficiency | Passive | 400,000 |
