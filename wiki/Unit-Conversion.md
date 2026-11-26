# Unit Conversion

Cactulator converts between about 130 units in 15 categories, including chemistry-friendly ones such as molar energies, wavenumbers and hartrees.

## On HOME

Write a value, a unit, `to` (or `in`) and the target unit:

```text
5 km to mi                 → 3.10685596119 mi
100 F to C                 → 37.7777777778 C
1 atm to kPa               → 101.325 kPa
(3+4) ft to m              → 2.1336 m
2 h in min                 → 120 min
1 Eh to eV                 → 27.211386246 eV
1000 cm-1 to kJ/mol        → 11.9626565639 kJ/mol
```

The value can be any expression. The result is stored in `ans`. Converting between categories gives an error such as *can't convert Mass (kg) to Length (m)*.

## In expressions: `convert`

Inside larger calculations and programs, use `convert(value, "from", "to")` with the unit names in quotes:

```text
convert(3, "cup", "mL")    → 709.7647095
convert(100, "C", "F")     → 212
```

## The UNITS app

Open with **F7** on HOME (or `units`). There are four fields — category, value, *from* unit and *to* unit:

| Key | Does |
|---|---|
| **↑ ↓** (or **Tab**, **Enter**) | move between fields |
| **← →** | change the category or unit in the selected field |
| type | enter the value (expressions like `3*12` work) |
| **F1 Swap** | swap *from* and *to* |
| **F2 → Home** | send the converted value to HOME |
| **F3 Prev cat** / **F4 Next cat** | change category |

Below the fields, the value is shown in **every** unit of the category; click one to pick it as the target.

## Spelling

Unit names are matched exactly first, then ignoring case, so `KM` works when it isn't ambiguous. These aliases are also understood:

| You can write | Meaning |
|---|---|
| `°C`, `degC`, `celsius` / `°F`, `degF`, `fahrenheit` / `kelvin` | `C` / `F` / `K` |
| `µm`, `micron`, `µg`, `µs`, `µL` | `um`, `ug`, `us`, `uL` |
| `l`, `ml` | `L`, `mL` |
| `kph`, `kmh` | `km/h` |
| `A`, `angstrom`, `Ang` | `Å` |
| `°`, `degree`, `degrees` | `deg` |
| `lbs`, `hr`, `sec` | `lb`, `h`, `s` |
| `inch`, `inches`, `foot`, `feet`, `mile`, `miles` | `in`, `ft`, `mi` |
| `amu`, `u` | `Da` |
| `wavenumber`, `cm^-1` | `cm-1` |
| `m^2`, `m^3` | `m2`, `m3` |

Temperatures are converted as absolute temperatures (`0 C to K` → `273.15 K`), not as temperature differences.

Molar energy units treat `eV/particle`, `Eh/particle` and `cm-1` as energies per particle multiplied by Avogadro's number, so they convert directly to `kJ/mol` and `kcal/mol`.

## All units

### Length

| Unit | Name |
|---|---|
| `m` | metre |
| `km` | kilometre |
| `cm` | centimetre |
| `mm` | millimetre |
| `um` | micrometre (µm) |
| `nm` | nanometre |
| `pm` | picometre |
| `Å` | ångström |
| `in` | inch |
| `ft` | foot |
| `yd` | yard |
| `mi` | mile |
| `nmi` | nautical mile |
| `au` | astronomical unit |
| `ly` | light year |
| `pc` | parsec |

### Mass

| Unit | Name |
|---|---|
| `kg` | kilogram |
| `g` | gram |
| `mg` | milligram |
| `ug` | microgram (µg) |
| `t` | tonne |
| `lb` | pound |
| `oz` | ounce |
| `st` | stone |
| `ton` | short ton (US) |
| `Da` | dalton / amu |
| `ct` | carat |

### Time

| Unit | Name |
|---|---|
| `s` | second |
| `ms` | millisecond |
| `us` | microsecond (µs) |
| `ns` | nanosecond |
| `min` | minute |
| `h` | hour |
| `d` | day |
| `wk` | week |
| `yr` | year (365.25 d) |

### Temperature

| Unit | Name |
|---|---|
| `K` | kelvin |
| `C` | degree Celsius |
| `F` | degree Fahrenheit |
| `R` | degree Rankine |

### Volume

| Unit | Name |
|---|---|
| `L` | litre |
| `mL` | millilitre |
| `uL` | microlitre (µL) |
| `m3` | cubic metre |
| `dm3` | cubic decimetre |
| `cm3` | cubic centimetre |
| `gal` | US gallon |
| `qt` | US quart |
| `pt` | US pint |
| `cup` | US cup |
| `floz` | US fluid ounce |
| `tbsp` | tablespoon |
| `tsp` | teaspoon |
| `ukgal` | imperial gallon |
| `ft3` | cubic foot |
| `in3` | cubic inch |

### Area

| Unit | Name |
|---|---|
| `m2` | square metre |
| `km2` | square kilometre |
| `cm2` | square centimetre |
| `mm2` | square millimetre |
| `ha` | hectare |
| `acre` | acre |
| `ft2` | square foot |
| `in2` | square inch |
| `yd2` | square yard |
| `mi2` | square mile |

### Speed

| Unit | Name |
|---|---|
| `m/s` | metres per second |
| `km/h` | kilometres per hour |
| `mph` | miles per hour |
| `kn` | knot |
| `ft/s` | feet per second |
| `c` | speed of light |

### Energy

| Unit | Name |
|---|---|
| `J` | joule |
| `kJ` | kilojoule |
| `MJ` | megajoule |
| `cal` | calorie |
| `kcal` | kilocalorie |
| `Wh` | watt hour |
| `kWh` | kilowatt hour |
| `eV` | electronvolt |
| `BTU` | British thermal unit |
| `erg` | erg |
| `Eh` | hartree |

### Molar energy

| Unit | Name |
|---|---|
| `J/mol` | joule per mole |
| `kJ/mol` | kilojoule per mole |
| `kcal/mol` | kilocalorie per mole |
| `eV/particle` | electronvolt per particle |
| `cm-1` | wavenumber |
| `Eh/particle` | hartree per particle |

### Pressure

| Unit | Name |
|---|---|
| `Pa` | pascal |
| `kPa` | kilopascal |
| `MPa` | megapascal |
| `bar` | bar |
| `mbar` | millibar |
| `atm` | atmosphere |
| `torr` | torr |
| `mmHg` | millimetre of mercury |
| `psi` | pound per square inch |
| `inHg` | inch of mercury |

### Power

| Unit | Name |
|---|---|
| `W` | watt |
| `kW` | kilowatt |
| `MW` | megawatt |
| `hp` | horsepower |
| `BTU/h` | BTU per hour |

### Force

| Unit | Name |
|---|---|
| `N` | newton |
| `kN` | kilonewton |
| `lbf` | pound-force |
| `kgf` | kilogram-force |
| `dyn` | dyne |

### Angle

| Unit | Name |
|---|---|
| `deg` | degree |
| `rad` | radian |
| `grad` | gradian |
| `arcmin` | arc minute |
| `arcsec` | arc second |
| `turn` | full turn |

### Frequency

| Unit | Name |
|---|---|
| `Hz` | hertz |
| `kHz` | kilohertz |
| `MHz` | megahertz |
| `GHz` | gigahertz |
| `rpm` | revolutions per minute |

### Data

| Unit | Name |
|---|---|
| `bit` | bit |
| `B` | byte |
| `KB` | kilobyte (1000 B) |
| `MB` | megabyte |
| `GB` | gigabyte |
| `TB` | terabyte |
| `KiB` | kibibyte (1024 B) |
| `MiB` | mebibyte |
| `GiB` | gibibyte |
| `TiB` | tebibyte |
