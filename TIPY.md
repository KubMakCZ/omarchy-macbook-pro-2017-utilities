# MacBook Pro 2017 s Omarchy: praktický průvodce

Tyto poznámky se týkají **MacBookPro14,2 (13", 2017, Touch Bar, T1)**.
Jiné modely a budoucí verze Omarchy se mohou chovat jinak. Německý potisk
kláves se při českém rozložení nezmění.

## Klávesnice: přechod z Windows

| Na MacBooku | Přibližný protějšek na PC | V Omarchy |
| --- | --- | --- |
| `Command` / `⌘` | klávesa Windows | obvykle `Super`: používá se pro většinu zkratek prostředí |
| `Option` / `⌥` | `Alt` | `Alt`; pravý Option může při českém rozložení fungovat jako `AltGr` |
| `Control` / `⌃` | `Ctrl` | `Ctrl`, například pro zkratky v terminálu |
| `Delete` | `Backspace` | maže znak vlevo; `Fn + Delete` maže znak vpravo |
| `Fn` | přepínač funkcí | mění ovládání Touch Baru a některých kláves |

`Command` není ve všech aplikacích totéž co macOS zkratky: Omarchy používá
vlastní mapování. Aktuální zkratky zobrazíš pomocí **Super + K** nebo
`omarchy menu keybindings`.

### České QWERTZ

Tento projekt nastavuje základní české `cz`: horní písmenová řada obsahuje
`Z`, spodní `Y`. Klávesy s českou diakritikou ani speciální znaky nemusí
odpovídat německému potisku. Na českém rozložení bývá `@` **pravý Alt + V**,
čísla jsou nad českými znaky a píší se se Shiftem. Nejdřív si vyzkoušej
diakritiku, závorky a znaky potřebné k heslům.

Přihlašovací obrazovka, textová konzole a odemykání disku mohou mít **jiné
rozložení** než desktop. Neměň heslo, dokud neumíš potřebné znaky bezpečně
napsat v příslušné obrazovce. Rozložení desktopu změníš přes
`omarchy setup keyboard`; projektové `bash setup-keyboard.sh` nastaví české
QWERTZ pouze pro aktuálního uživatele.

### Užitečné klávesy Fn

- `Fn + ←` a `Fn + →`: Home a End.
- `Fn + ↑` a `Fn + ↓`: Page Up a Page Down.
- `Fn + Delete`: Delete směrem vpravo.

Chování závisí na ovladači a aplikaci; vyzkoušej je v textovém editoru.

## Touch Bar

V současném režimu jej vykresluje firmware. Vlevo je `Esc`; ostatní tlačítka
obvykle ovládají jas, přehrávání a hlasitost. Podrž `Fn` pro řadu **F1–F12**.
Některá místa nebo akce mohou zůstat prázdná. Když panel zhasne nečinností,
zkus stisknout klávesu nebo použít touchpad; pokud nesvítí ani potom:

```bash
systemctl status omarchy-mbp-touchbar.service --no-pager
bash diagnose.sh
```

Projektová služba pomáhá pouze s již nainstalovaným ovladačem. Neinstaluje ho
a zatím není ověřená po restartu. Současný firmware neumí nakreslit libovolné
vlastní ikony: k tomu je nutný odlišný, experimentální režim. U
[T1Bridge](https://github.com/standardagents/t1bridge) je na tomto modelu
hlášené nefunkční uspání/probuzení; nepřepínej ovladače bez zálohy a plánu
návratu. Neprováděj ruční reset T1 ani neinstaluj dva konkurenční ovladače
naslepo.

### Vlastní scény

Projekt obsahuje připravovaný editor [tbar](tbar/README.md): v něm lze
vytvořit několik scén s vlastními příkazy a stavem internetu, CPU nebo procesu.
Scény se zatím přepínají v aplikaci, nikoli přímo na Touch Baru. Výstup je
určen pro renderer `omarchy-glance` a T1Bridge, které nejsou součástí tohoto
projektu a nebyly na tomto počítači přepnuty ani vyzkoušeny. Editor tedy
sám o sobě nezmění současný firmwareový Touch Bar. Automatické přepínání podle
aplikace, výběr scén na panelu a GIFy jsou zatím pouze nápady.

## Nejčastější ovládání Omarchy

`Super` znamená zpravidla klávesu `⌘`:

| Zkratka | Akce |
| --- | --- |
| `Super + Space` | hlavní nabídka |
| `Super + Enter` | terminál |
| `Super + Shift + Enter` | prohlížeč |
| `Super + Shift + F` | správce souborů |
| `Super + K` | seznam klávesových zkratek |
| `Super + F` | celá obrazovka |
| `Super + W` nebo `Super + Q` | zavřít okno |
| `Super + Ctrl + L` | zamknout obrazovku |
| `Alt + Tab` | přepnout okno |
| `Super + Tab` | další pracovní plocha |

Zkratky jsou z aktuální instalace a mohou se změnit podle konfigurace a verze.
Na českém rozložení se čísla pracovních ploch mohou v seznamu zkratek
zobrazovat jako české znaky.

Snímek obrazovky lze pořídit bez klávesy Print Screen příkazem
`omarchy capture screenshot region`. Jas klávesnice upravíš příkazy
`omarchy brightness keyboard up` a `omarchy brightness keyboard down`.

## Touchpad a připojení

Posouvání dvěma prsty je v aktuální konfiguraci nastavené bez přirozeného
směru; klik dvěma prsty slouží jako pravé tlačítko. Toto nastavení můžeš
později změnit v Omarchy/Hyprlandu. Klávesnice a touchpad používají `applespi`.

Stav Wi-Fi ukáže `omarchy network status`. Na tomto počítači fungovalo
připojení přes `brcmfmac`; 5 GHz ani návrat po uspání zatím nebyly ověřeny.
Neinstaluj `broadcom-wl` jen proto, že máš Broadcom. Bluetooth, kameru,
sluchátka, externí monitor a USB-C zařízení si ověř jednotlivě.

## Co instalovat a kdy aktualizovat

Nejdřív vyzkoušej, co už funguje. Omarchy už obsahuje mnoho běžných nástrojů;
balíčky zbytečně neduplikuj. V nabídce můžeš přidávat aplikace, v terminálu
slouží například `omarchy pkg add <balíček>`. Před použitím AUR nebo cizího
skriptu zkontroluj jeho zdroj a změny.

**Pozor na jádro:** současné moduly Touch Baru a zvuku byly nainstalované
ručně a jejich automatické sestavování pro další jádro není vyřešené. Před
`omarchy update` měj zálohu, obnovovací USB a připravený postup pro ovladače;
po aktualizaci ověř sestavení DKMS a až pak restartuj.

## Před přeinstalací

1. Zálohuj osobní data mimo počítač a ověř obnovu.
2. Zachovej původní **Apple EFI oddíl** a zálohuj jeho
   `EFI/APPLE/EMBEDDEDOS` včetně `FDRData` mimo počítač. Data jsou specifická
   pro tento Mac: nedávej je do repozitáře ani na GitHub.
3. Zkontroluj skutečný cíl instalace a nenech původní Apple EFI oddíl
   přeformátovat. Smazání může vyřadit Touch Bar a kameru. Postup a další
   omezení popisuje [ArchWiki](https://wiki.archlinux.org/title/MacBookPro14,2).
4. Po instalaci nejprve ověř klávesnici, síť a ovladače. Tento repozitář
   **zatím není kompletní automatický instalátor** všech funkcí.

Pro Touch ID existuje experimentální podpora v T1Bridge, ale vyžaduje
odpovídající zachovaná data a další nastavení. Nezapínej přihlášení otiskem
bez ověření funkčního hesla a možnosti obnovy.
