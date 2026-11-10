# Instalace na čistém Omarchy

Tento postup vychází z jednoho **MacBookPro14,2 (2017, 13", T1)**. Ostatní
modely nejsou tímto projektem ověřeny. Postup T1Bridge a jednotlivých kroků
byl použit na našem stroji, ale kompletní instalace od začátku na čistém
Omarchy zatím nebyla zopakována. Automatizace nepokrývá vše; chyby a rozdíly
verzí je nutné řešit před pokračováním.

## 1. Před instalací Linuxu

Zálohuj osobní data a připrav obnovovací USB. Zachovej původní Apple EFI oddíl
včetně `EFI/APPLE/EMBEDDEDOS/FDRData` a ověř zálohu na jiném médiu.
Záloha patří **jen tomuto Macu**: nepublikuj ji ani její obsah. Instalátor
tohoto projektu žádné oddíly nemaže ani neformátuje. Při instalaci Omarchy
ověř, že instalátor zachová Apple EFI; viz
[ArchWiki](https://wiki.archlinux.org/title/MacBookPro14,2) a
[bezpečnostní upozornění T1Bridge](https://github.com/standardagents/t1bridge#readme).

Měj funkčné heslo, napájení a cestu k obnově systému. T1Bridge aktuálně
nepodporuje spolehlivé systémové uspání/probuzení; na tomto modelu byl hlášen
černý displej po probuzení. Není zaručena kompatibilita každého dalšího jádra.

## 2. Kontrola a základní nastavení

Stáhni projekt a spusť diagnostiku:

```bash
git clone https://github.com/KubMakCZ/omarchy-macbook-pro-2017-utilities.git
cd omarchy-macbook-pro-2017-utilities
```

V adresáři projektu:

```bash
bash setup.sh --check
bash setup.sh --keyboard
```

Klávesnice se mění pouze pro přihlášeného uživatele a existující konfigurace
se zálohuje. Ověř Wi-Fi, touchpad, klávesnici, zvuk, mikrofon, kameru a USB.
Na našem počítači pomohla s opakovaným připojením USB následující volitelná
služba (může zvýšit spotřebu):

```bash
bash setup.sh --usb-power
```

Příkaz kontroluje model a nepřepisuje odlišné existující soubory služby. Před
odpojením disky bezpečně vysuň. Fungování po uspání jsme neověřili.

## 3. T1Bridge: podpis, balíčky a restart

Tato část zatím **není automatizována**: přidává důvěru cizímu podpisovému
klíči, mění systémové balíčky a ovladače. Použij
[aktuální oficiální instalační postup](https://github.com/standardagents/t1bridge#install-official-packages)
a předem zkontroluj jeho obsah. U nás platilo:

1. Stáhni klíč a vypiš jeho otisk:

   ```bash
   key_dir="$(mktemp -d)"
   curl -fSLo "$key_dir/t1bridge-signing-key.asc" \
     https://linux.standardagents.ai/arch/standardagents/x86_64/t1bridge-signing-key.asc
   gpg --show-keys --with-fingerprint "$key_dir/t1bridge-signing-key.asc"
   ```

   Primární otisk musí být `35B166F78B063B04DE1E3D913E6C4216EB03D371`;
   porovnej ho také s aktuálním oficiálním postupem. Při neshodě přestaň.
   Teprve po ověření klíč přidej a lokálně podepiš:

   ```bash
   sudo pacman-key --add "$key_dir/t1bridge-signing-key.asc"
   sudo pacman-key --lsign-key 35B166F78B063B04DE1E3D913E6C4216EB03D371
   ```

2. Nejprve vytvoř samostatnou zálohu `/etc/pacman.conf`:

   ```bash
   backup="$(sudo mktemp /etc/pacman.conf.backup.XXXXXXXX)"
   sudo cp -a /etc/pacman.conf "$backup"
   printf 'Záloha: %s\n' "$backup"
   ```

   Ověř, zda již neobsahuje sekci `[standardagents]`, a přidej ji pomocí
   `sudoedit /etc/pacman.conf` jen pokud chybí:

   ```ini
   [standardagents]
   SigLevel = Required DatabaseRequired
   Server = https://linux.standardagents.ai/arch/$repo/$arch
   ```

   `$repo` a `$arch` jsou doslovný text, nikoli hodnoty k nahrazení.
   Existující odlišnou sekci nepřepisuj naslepo a neoslabuj kontrolu podpisů.
   Potom spusť `omarchy update`, aby se databáze a systém aktualizovaly
   společně. Nespouštěj samotné `pacman -Sy`.
3. Pokud se aktualizovalo jádro, dokonči požadovaný restart a ověř, že
   `uname -r` odpovídá nainstalovanému jádru Omarchy. Potom nainstaluj
   odpovídající headers, například pro běžné jádro Omarchy:

   ```bash
   sudo pacman -S --needed linux-omarchy-headers t1bridge t1bridge-dkms t1bridge-omarchy
   ```

   Jiné jádro potřebuje své odpovídající headers. Ověř `dkms status`, že se
   sestavil `t1_cfgsel` pro správné jádro, a výstup instalace včetně tvorby
   initramfs/UKI. Při chybě nepokračuj naslepo.
4. Podle aktuálních pokynů upstreamu nastav uživatele/skupinu a systémové
   služby. Pokud existuje stará `omarchy-mbp-touchbar.service`, `tiny-dfr`
   nebo jiný T1 ovladač, prověř jej a zajisti, aby při dalším startu
   nekonkuroval T1Bridge. Neměň ovladače za běhu; k přechodu použij restart.
5. Po restartu v grafické relaci dokonči nastavení uživatelské služby podle
   upstreamu. Ověř `sudo t1bridge status`, jednotlivé stavové řádky, Esc,
   dotyk, jas, zvuk a popupy. Desktopová integrace `t1bridge-omarchy` používá
   po přihlášení vlastní autostart. `keybag: not-enrolled` není potvrzení
   funkčního Touch ID; otisk jsme na našem Macu nenastavovali.

Nastavení Touch ID, PAM a firewallu není součástí tohoto postupu. Nech vždy
funkční přihlášení heslem a řiď se zvlášť dokumentací T1Bridge. Neměň ani
nemaž Apple EFI, keybag či jiná chráněná data při řešení potíží.

## 4. Editor a vlastní renderer

Sestavení vyžaduje `git`, Rust/Cargo, `pkg-config`, vývojové soubory Cairo,
qmake6, `make`, C++ kompilátor a Qt 6 včetně Qt Quick Controls. Závislosti
nainstaluj podle aktuálního Omarchy, pokud chybí. Potom:

```bash
bash setup.sh --renderer-build
bash setup.sh --install-app
```

První příkaz stáhne
[omarchy-glance](https://github.com/andersryanc/omarchy-macbook-touchbar-custom)
do uživatelské cache, ověří připnutou revizi
`9c75e5e1d71cf583fe9b02c124cffa272fd7fcb1` a sestaví ji příkazem
`cargo build --locked --release`. Cargo může stáhnout závislosti; jejich
verze jsou v upstream `Cargo.lock`. Zdroj musí být čistý. Odlišný existující
`~/.local/bin/omarchy-glance` se nepřepíše. Připnutý commit není sám o sobě
ověřeným podpisem vydavatele. Sestavení renderer **neaktivuje**.

Druhý příkaz sestaví a nainstaluje balíček `tbar`; `makepkg` může požádat
o heslo a instalovat chybějící balíčkové závislosti. Hook pro téma se instaluje
jen pokud stejnojmenný soubor neexistuje. Zavři běžící starý editor před
aktualizací balíčku.

Pro aktivaci vlastního rendereru zkontroluj a případně zazálohuj:

- `${XDG_CONFIG_HOME:-~/.config}/t1bridge/renderer`
- `${XDG_CONFIG_HOME:-~/.config}/systemd/user/omarchy-glance.service`
- `${XDG_CONFIG_HOME:-~/.config}/systemd/user/omarchy-glance.socket`

Po funkčním T1Bridge a aktivní, povolené uživatelské službě `t1-touchbar` můžeš
renderer výslovně aktivovat:

```bash
bash setup.sh --renderer-enable
```

Příkaz vytvoří konfiguraci odpovídající připnutému rendereru jen tam, kde
neexistuje; odlišné soubory a symbolické odkazy odmítne. Zapne uživatelský
socket a restartuje službu Touch Baru. Opakování při stejné aktivní konfiguraci
službu znovu nerestartuje. Při chybě během vytváření mohou již vytvořené
shodné soubory zůstat na místě; vyřeš konflikt a spusť příkaz znovu.
Aktivaci prováděj z grafické relace se stejným `XDG_CONFIG_HOME`, jaký používá
uživatelský správce systemd.
Aktivace nebyla zatím ověřena na čisté instalaci.

Samotný upstream příkaz `~/.local/bin/omarchy-glance on` na rozdíl od našeho
kroku tyto soubory může **přepsat**; použij jej jen po kontrole a záloze.
Pak otevři `tbar`, vytvoř scénu a použij ji. Editor před přepsáním rendererové
konfigurace vytváří zálohy. `~/.local/bin/omarchy-glance off` přepne na vestavěný renderer
T1Bridge a odstraní výběrový soubor rendereru; není to odinstalace T1Bridge.

## 5. Kontrola a návrat

Ověř funkci i po novém přihlášení a restartu. Dále vyzkoušej kameru, Bluetooth,
externí monitor a další hardware, který potřebuješ. Při problémech zkontroluj:

```bash
bash setup.sh --check
systemctl --user status t1-touchbar.service omarchy-glance.service --no-pager
sudo t1bridge status
```

Výstup a logy před sdílením zkontroluj kvůli soukromým údajům. Před
aktualizacemi měj ověřenou zálohu a možnost obnovy. Úplný návrat od T1Bridge
ke starému ovladači jsme nevyzkoušeli; nepoužívej starou službu současně s ním.
