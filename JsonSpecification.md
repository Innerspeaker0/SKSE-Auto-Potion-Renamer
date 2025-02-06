# JSON config specification
The mod supports multiple JSON files along with an optional UserSettings.json. Each JSON file can define **potions**, **descriptors**, and **effect potencies**.
There is a hard limit of 31 potions, 31 effect potencies, and 15 descriptor definitions in each file. 
There is a potential performance impact the more files that are loaded, but this should not be noticable with under 255 potions.
Do not attempt to use this plugin to rename every potion, use the wonderful [Alchemy Plus](https://www.nexusmods.com/skyrimspecialedition/mods/80882) instead!

## Names 
- The format specifier, `{}`, should be placed without spaces, e.g. `"Dralval's{}Sapping Poison"`. 
- Put the format specifier where it makes most sense for a describing word to go. 
This is usually at the start (e.g. "Potent" Poison of...) or before the adjective (e.g. Potion of "Brief" ...). Sometimes it can replace the word "Potion" entirely with words like "Draught" or "Elixir".

## Effects
- Each potion can be defined by between 2 and 4 (inclusive) effects.
- An effect is specified by its File and FormID, e.g. `Skyrim.esm|10DE5E`. This means that effect FormIDs from mods can be used. Make sure to use **the file name, not the mod name**

## Descriptors
Descriptors tell the mod what to name potions as their effectiveness increase. These can be defined in either the user settings file or individual potion files, but definitions in user settings are given priority.
Inside a "descriptor" field, descriptors are defined as arrays of strings from least powerful (e.g. "Weak") to most powerful (e.g. "Potent").
- Descriptors should be first-letter-capitalized and not have any leading or trailing whitespace.
- An empty descriptor should be defined as `""`. Any required spaces are inserted by the code.
- Descriptors can be defined in `UserSettings.json` and/or potion files.
- Descriptors in `UserSettings.json` take priority over those defined elsewhere
- Descriptors will not be used if UseRomanNumerals is set to True

## Effect Potencies
Effect potencies are specified in a potion file by dictionaries in the form: 
```
"EffectName": {
	"min": 5
	"max": 125.0
}
```
Where effect name is the case sensitive editor ID (e.g. AlchDamageSpeed) or the case sensitive name (e.g. Slow). When potencies are defined more than once, the editor ID definition is given priority.

- The min and max values are interpreted as either duration (seconds) or magnitude (health points, stamina, etc) depending on which aspect of that effect scales with alchemy skill.
- Only the magnitude of the most expensive effect is used when determining what descriptor to give the potion.
- Potions with potencies outside the range of min and max will use the lowest or highest descriptor respectively.