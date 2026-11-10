# Omarchy na MacBooku Pro 13" 2017

Poznámky k modelu **MacBookPro14,2 (A1706, Touch Bar, čip T1)**. Ostatní
modely mohou vyžadovat jiné ovladače. Tento projekt zatím není kompletní
instalátor hardwaru.

Praktické tipy jsou v [TIPY.md](TIPY.md), postup pro čistou instalaci
v [docs/INSTALL.md](docs/INSTALL.md) a editor scén v [tbar/README.md](tbar/README.md).

## Rychlý postup

V adresáři projektu lze jednotlivé kroky opakovat:

```bash
bash setup.sh --check       # pouze diagnostika, bez změn
bash setup.sh --keyboard    # české QWERTZ pro aktuálního uživatele
bash setup.sh --build-app   # sestavení editoru, bez instalace
bash setup.sh --install-app # sestavení a instalace editoru
bash setup.sh --renderer-build # sestavení a instalace rendereru, bez aktivace
bash setup.sh --usb-power   # volitelná oprava USB; sudo
```

Jednotlivé kroky nic samy neformátují ani nepřepínají T1 ovladače. Instalace
editoru může instalovat balíčkové závislosti a vyžádat si heslo. Renderer se
sestavuje z připnuté revize externího projektu a nenahrazuje odlišný existující
soubor. T1Bridge je zatím nutné nainstalovat podle [postupu](docs/INSTALL.md);
nejde o plně automatický instalátor a čistou instalaci jsme zatím neověřili.

Licenci vlastního kódu jsme ještě
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

## Touch Bar: dva neslučitelné postupy

Na testovaném počítači nyní běží **T1Bridge a omarchy-glance**. Při problémech
nejprve zkontroluj `systemctl --user status t1-touchbar.service --no-pager`
a `sudo t1bridge status`; viz [instalace a řešení problémů](docs/INSTALL.md).
Starou službu uvedenou níže na tomto systému nezapínej.

Historicky na tomto stroji před přechodem na T1Bridge jedno rozhraní iBridge
převzal `hid-sensor-hub`. Projekt obsahuje starší službu pro jeho opětovné
připojení k již nainstalovanému firmwareovému ovladači:

```bash
sudo bash install-touchbar-service.sh
systemctl status omarchy-mbp-touchbar.service
```

**Nepoužívat společně s T1Bridge nebo konkurenčním rendererem.** Instalátor
staré služby odmítne známý aktivní či nainstalovaný T1Bridge. Služba
neinstaluje ovladač a vyžaduje `apple-ibridge-hid`; po aktualizaci jádra
nemusí fungovat. Restart byl v tehdejším režimu ověřen, uspání nikoli.

## Poznatky z aktuální instalace

- BCM43602 používá `brcmfmac` a balíček `linux-firmware-broadcom`. Wi-Fi byla
  připojena; pásmo 5 GHz a chování po uspání nejsou ověřené. Funkční ovladač
  není důvod preventivně nahrazovat `broadcom-wl`.
- Vestavěný `applespi` obsluhuje klávesnici a touchpad. Systém detekuje iBridge
  `05ac:8600` a kamerové zařízení, jejich reálná funkce se musí vyzkoušet.
- Touch Bar nyní používá balíčky `t1bridge` a `t1bridge-dkms`; zvukový modul
  byl při původní kontrole mimo správu balíčků. Staré položky DKMS pro klávesnici
  nebyly sestavené pro tehdejší jádro. Po aktualizaci ověř sestavení DKMS,
  initramfs/UKI a funkci zařízení; kompatibilita budoucích jader není zaručena.
- Alternativní [apple-touchbar-driver](https://github.com/Phennova/apple-touchbar-driver)
  se pro aktuální jádro sestavil, ale **nebyl nainstalován ani vyzkoušen na
  tomto stroji**. Jeho instalační skript mění DKMS a systémové služby; nejprve
  ověř kompatibilitu s existujícím nastavením.
- Podle uživatele fungují reproduktory i mikrofon. Kameru a probuzení z uspání
  je ještě třeba vyzkoušet.
- Touch ID na našem stroji nebylo nastaveno ani ověřeno. T1Bridge nabízí
  experimentální podporu s dalšími podmínkami.

Podrobnosti o Wi-Fi, zvuku, Touch Baru a uspávání se mohou měnit podle verze
jádra; před zásahem ověř aktuální [ArchWiki](https://wiki.archlinux.org/title/MacBookPro14,2).

## USB zařízení se po odpojení znovu neobjeví

Na tomto MacBooku se po odpojení flešek přestala nová USB zařízení hlásit
jádru. `udiskie` přitom běžel a staré přípojné body zmizely. Dočasné vypnutí
runtime uspávání obou externích USB řadičů umožnilo opětovné připojení;
jde o předběžně ověřený workaround, nikoli potvrzenou příčinu závady.
Systém nyní při vložení flešku automaticky připojuje pomocí `udiskie`.

Pro nastavení po každém startu (jen MacBookPro14,2 s odpovídajícími PCI
řadiči) lze spustit:

```bash
sudo bash install-usb-power-service.sh
systemctl status omarchy-mbp-usb-power.service --no-pager
```

Služba nastaví `power/control=on` u obou externích řadičů při startu.
Může mírně zvýšit spotřebu; po restartu byla ověřena, po uspání nikoli.
Pokud nové USB zařízení nevidí ani `lsblk`, přeinstalace `udiskie`
to sama nevyřeší.

Návrat k automatickému uspávání (změna zmizí i restartem po vypnutí služby):

```bash
sudo systemctl disable --now omarchy-mbp-usb-power.service
sudo sh -c 'echo auto > /sys/bus/pci/devices/0000:06:00.0/power/control; echo auto > /sys/bus/pci/devices/0000:7c:00.0/power/control'
```

Disky před odpojením vždy vysuň. Při hlášení nečistého FAT/exFAT zkontroluj
souborový systém až po odpojení oddílu a před opravou zazálohuj důležitá data.
