# Omarchy na MacBooku Pro 13" 2017

Poznámky k modelu **MacBookPro14,2 (A1706, Touch Bar, čip T1)**. Ostatní
modely mohou vyžadovat jiné ovladače. Tento projekt zatím není kompletní
instalátor hardwaru.

Praktické tipy ke klávesnici, Touch Baru a Omarchy jsou v [TIPY.md](TIPY.md).
Experimentální editor scén je v [tbar/README.md](tbar/README.md).

## Rychlý postup

V adresáři projektu lze jednotlivé kroky opakovat:

```bash
bash setup.sh --check       # pouze diagnostika, bez změn
bash setup.sh --keyboard    # české QWERTZ pro aktuálního uživatele
bash setup.sh --build-app   # sestavení editoru, bez instalace
```

Volitelně `bash setup.sh --touchbar-service` nainstaluje níže popsanou službu
pro **již existující** firmwareový ovladač; vyžaduje `sudo`. K sestavení
editoru je potřeba Qt 6, qmake6, make a C++ kompilátor. Skript neinstaluje
balíčky, neaktualizuje systém ani nepřepíná ovladače. Nejde zatím o kompletní
instalaci pro čistý systém: ovladač Touch Baru a jeho aktualizace pro nové
jádro musí být vyřešeny zvlášť.

Vývoj aplikace a návodů je zatím lokální. Licenci vlastního kódu jsme ještě
nezvolili; před přijetím či šířením příspěvků ji bude nutné určit. Cizí
projekty mají vlastní licence a nejsou součástí tohoto repozitáře.

## Než začneš

- Před novou instalací zálohuj na jiné médium původní Apple EFI oddíl včetně
  `EFI/APPLE/EMBEDDEDOS` a souboru `FDRData`. Je specifický pro daný počítač:
  **nezveřejňuj ho na GitHubu**. Smazání může vyřadit Touch Bar a kameru.
- Nepřeformátovávej Apple EFI oddíl. Podrobnosti a postup zálohy jsou na
  [ArchWiki pro MacBookPro14,2](https://wiki.archlinux.org/title/MacBookPro14,2).
- Před změnami ovladačů nebo bootování si připrav obnovovací USB a zálohu dat.

## Česká klávesnice QWERTZ

V běžící relaci Hyprlandu spusť:

```bash
bash setup-keyboard.sh
```

Skript nastaví české rozložení v uživatelském souboru Omarchy, případný
předchozí soubor zazálohuje a znovu načte Hyprland. Německé popisky zůstanou,
česká diakritika a další symboly tedy nebudou odpovídat potisku. Toto
nenastavuje klávesnici pro přihlašování ani pro textovou konzoli.
Jiné rozložení lze vybrat pomocí `omarchy setup keyboard`.

## Diagnostika

```bash
bash diagnose.sh
```

Výstup ukazuje model, jádro, stav vybraných ovladačů, DKMS a síťových
rozhraní. Neobsahuje záměrně názvy Wi-Fi sítí ani MAC adresy, ale před
zveřejněním jej vždy zkontroluj.

## Zhasnutý Touch Bar

Na tomto stroji byl ovladač již načtený, ale jedno rozhraní iBridge převzal
`hid-sensor-hub`. Po připojení obou rozhraní k `apple-ibridge-hid` Touch Bar
svítil a tlačítka podle uživatele fungovala. Tento projekt obsahuje službu,
která stejné připojení provádí po startu:

```bash
sudo bash install-touchbar-service.sh
systemctl status omarchy-mbp-touchbar.service
```

Služba **neinstaluje ovladač** a vyžaduje již načtený `apple-ibridge-hid`.
Není náhradou za správnou instalaci ovladače pomocí DKMS. Po aktualizaci jádra
proto nemusí Touch Bar fungovat. Chování po restartu a uspání je třeba ověřit.

## Poznatky z aktuální instalace

- BCM43602 používá `brcmfmac` a balíček `linux-firmware-broadcom`. Wi-Fi byla
  připojena; pásmo 5 GHz a chování po uspání nejsou ověřené. Funkční ovladač
  není důvod preventivně nahrazovat `broadcom-wl`.
- Vestavěný `applespi` obsluhuje klávesnici a touchpad. Systém detekuje iBridge
  `05ac:8600` a kamerové zařízení, jejich reálná funkce se musí vyzkoušet.
- Aktuálně přítomné ovladače Touch Baru a zvuku byly nainstalovány mimo správu
  balíčků. Dvě staré položky DKMS nejsou pro aktuální jádro nainstalované;
  zdroj `macbook12-spi-driver-dkms` se pro toto jádro nepodařilo sestavit.
  Aktualizace jádra tak může tyto funkce rozbít. Automatická instalace nového
  ovladače v tomto projektu zatím není.
- Alternativní [apple-touchbar-driver](https://github.com/Phennova/apple-touchbar-driver)
  se pro aktuální jádro sestavil, ale **nebyl nainstalován ani vyzkoušen na
  tomto stroji**. Jeho instalační skript mění DKMS a systémové služby; nejprve
  ověř kompatibilitu s existujícím nastavením.
- Podle uživatele fungují reproduktory i mikrofon. Kameru a probuzení z uspání
  je ještě třeba vyzkoušet.
- Touch ID nefunguje v aktuálním režimu. Alternativní T1Bridge uvádí
  experimentální podporu na tomto modelu, s dalšími omezeními.

Podrobnosti o Wi-Fi, zvuku, Touch Baru a uspávání se mohou měnit podle verze
jádra; před zásahem ověř aktuální [ArchWiki](https://wiki.archlinux.org/title/MacBookPro14,2).
