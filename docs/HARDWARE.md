# Hardware: stav, známé potíže a workaroundy

Platí pro jeden **MacBookPro14,2 (13" 2017, Touch Bar, T1)**. Naposledy
zkontrolováno 2026-11-20 s Omarchy 4.5.0, jádrem `7.2.15-1-omarchy`,
`t1bridge` a `t1bridge-dkms` 0.1.12-1 a `t1bridge-omarchy` 0.2.2-1. Starší
pozorování nejsou důkazem současného stavu; vlastní stroj si ověř pomocí
`bash setup.sh --check`. Podrobnosti se mění s verzí jádra, proto před zásahem
zkontroluj i aktuální
[ArchWiki](https://wiki.archlinux.org/title/MacBookPro14,2).

## Přehled

| Součást | Ovladač nebo balíček | Stav | Neověřeno |
| --- | --- | --- | --- |
| Klávesnice, touchpad | `applespi` (v jádře) | funguje | |
| Wi-Fi BCM43602 | `brcmfmac`, `linux-firmware-broadcom` | připojení funguje | 5 GHz, stav po uspání |
| Reproduktory, mikrofon | `snd_hda_codec_cs8409` (mimo balíčky) | potvrzeno uživatelem | sluchátka |
| Touch Bar | `t1bridge`, `t1bridge-dkms`, `omarchy-glance` | zobrazení, Esc, dotyk, popupy i po restartu | uspání |
| USB porty | workaround `omarchy-mbp-usb-power.service` | funguje po restartu | spotřeba, stav po uspání |
| iBridge `05ac:8600`, kamera | detekováno | | skutečná funkce kamery |
| Bluetooth, externí monitor, USB-C zařízení | | | vše |
| Touch ID | experimentální podpora v T1Bridge | nenastaveno | vše |
| Uspání a probuzení | | známý problém upstreamu | nezkoušeno |

## Wi-Fi

BCM43602 používá `brcmfmac` a balíček `linux-firmware-broadcom`. Funkční
ovladač není důvod preventivně instalovat `broadcom-wl` jen proto, že čip má
značku Broadcom. Stav ukáže `omarchy network status`.

## Klávesnice a touchpad

Obojí obsluhuje vestavěný `applespi`. V DKMS zůstaly staré položky
`applespi` a `macbook12-spi-driver` ve stavu `added`: pro současné jádro nejsou
sestavené a nepoužívají se. Rozložení a zkratky popisuje [TIPY.md](../TIPY.md).

## Zvuk

Reproduktory i mikrofon podle uživatele fungují. Modul
`snd_hda_codec_cs8409` byl nainstalován ručně mimo správu balíčků
(`/lib/modules/<jádro>/updates/codecs/`), takže ho **nové jádro nemusí
načíst**. Dlouhodobé řešení je otevřený úkol v [ROADMAP.md](../ROADMAP.md).

## Touch Bar

Na testovaném stroji běží **T1Bridge a omarchy-glance**. Při potížích:

```bash
systemctl --user status t1-touchbar.service omarchy-glance.service --no-pager
```

```bash
sudo t1bridge status
```

Instalaci a návrat na vestavěný renderer popisuje [INSTALL.md](INSTALL.md),
vlastní scény [tbar/README.md](../tbar/README.md). `keybag: not-enrolled`
ve výpisu není potvrzení funkčního Touch ID.

Alternativní [apple-touchbar-driver](https://github.com/Phennova/apple-touchbar-driver)
se pro tehdejší jádro sestavil, ale **nebyl nainstalován ani vyzkoušen**. Jeho
instalační skript mění DKMS a systémové služby; nejprve ověř kompatibilitu
s existujícím nastavením.

### Starý firmwareový ovladač (historie)

Před přechodem na T1Bridge převzal jedno rozhraní iBridge `hid-sensor-hub`.
Repozitář proto obsahuje starší službu, která rozhraní znovu připojí k již
nainstalovanému ovladači `apple-ibridge-hid`:

```bash
sudo bash install-touchbar-service.sh
```

**Nepoužívat společně s T1Bridge, `tiny-dfr` ani jiným rendererem.**
Instalátor odmítne běžet, když najde nainstalovaný nebo aktivní T1Bridge.
Služba sama ovladač neinstaluje a po aktualizaci jádra nemusí fungovat. Restart
byl v tehdejším režimu ověřen, uspání nikoli. Vlastní tlačítka z `tbar` tento
režim nepodporuje.

## USB zařízení se po odpojení znovu neobjeví

Po odpojení flešek se nová USB zařízení přestala hlásit jádru, přestože
`udiskie` běžel a staré přípojné body zmizely. Pomohlo vypnutí runtime
uspávání obou externích USB řadičů. Jde o **předběžně ověřený workaround,
nikoli potvrzenou příčinu**.

Trvalé nastavení při každém startu (jen MacBookPro14,2 s odpovídajícími PCI
řadiči `8086:15d4` na adresách `06:00.0` a `7c:00.0`):

```bash
sudo bash install-usb-power-service.sh
```

```bash
systemctl status omarchy-mbp-usb-power.service --no-pager
```

Služba nastaví `power/control=on` u obou řadičů. Může mírně zvýšit spotřebu;
po restartu byla ověřena, po uspání nikoli. Pokud nové zařízení nevidí ani
`lsblk`, přeinstalace `udiskie` to nevyřeší.

Návrat k automatickému uspávání řadičů:

```bash
sudo systemctl disable --now omarchy-mbp-usb-power.service
```

```bash
sudo sh -c 'echo auto > /sys/bus/pci/devices/0000:06:00.0/power/control; echo auto > /sys/bus/pci/devices/0000:7c:00.0/power/control'
```

Disky před odpojením vždy vysuň. Při hlášení nečistého FAT/exFAT zkontroluj
souborový systém až po odpojení oddílu a před opravou zazálohuj důležitá data.

## Uspání a probuzení

Upstream T1Bridge uvádí systémové suspend/resume pro T1 jako nepodporované
a na tomto modelu byl hlášen černý displej po probuzení. Na testovaném stroji
se uspání záměrně nezkoušelo. Zkoušej ho jen s uloženou prací, zálohou
a připraveným způsobem obnovy.

## Po aktualizaci jádra

Kompatibilita s budoucími jádry není zaručená. Aktualizuj přes
`omarchy update` a **před restartem** ověř:

1. `dkms status` ukazuje `t1bridge-dkms` jako `installed` pro nové jádro.
2. Instalace proběhla bez chyb včetně tvorby initramfs/UKI.
3. Máš po ruce obnovovací USB; ve výběru Limine jde zvolit starší snapshot.

Po restartu zkontroluj Touch Bar, zvuk, Wi-Fi a USB pomocí
`bash setup.sh --check` a skutečným vyzkoušením.

## Diagnostika a sdílení výstupu

```bash
bash diagnose.sh
```

Výstup ukazuje model, jádro, verze balíčků, stav ovladačů, DKMS, služeb
a síťových rozhraní. Záměrně neobsahuje názvy Wi-Fi sítí ani MAC adresy,
přesto jej před zveřejněním vždy zkontroluj.
