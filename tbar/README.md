# tbar: experimentální editor scén

Qt aplikace pro přípravu vlastních rozložení Touch Baru na Omarchy. Je to
první verze: **na fyzickém Touch Baru nebyla ověřena**. Samotná aplikace
neinstaluje ovladač, T1Bridge ani [omarchy-glance](https://github.com/andersryanc/omarchy-macbook-touchbar-custom).
Současný firmwareový režim nepodporuje její vlastní tlačítka.

## Příprava a spuštění

Z adresáře `tbar`:

```bash
bash bin/build
./build/tbar
```

Je potřeba qmake6, make, C++ kompilátor a Qt 6 včetně Qt Quick Controls.
Pro případnou lokální instalaci do nabídky aplikací je připravený Arch balíček:
`bash bin/install` zavolá `makepkg -si` a může požádat o instalaci závislostí
a heslo. Tuto instalaci jsme zatím na systému neprovedli. Projektová licence
dosud nebyla zvolena; balíček označuje vlastní kód jako nelicencovaný a není
tím uděleno oprávnění k dalšímu šíření.

## Použití

Vyber nebo vytvoř scénu, přidej příkazy či stavy a případně změň jejich
pořadí. Každá scéna podporuje nejvýše 12 položek. „Použít rozložení“ zapíše
nastavení pro renderer; samotný výběr scény ji na Touch Bar neaplikuje. Na
horní vrstvě exportu je přidané `Esc`, na vrstvě Fn `Esc` a F1–F12; jejich
skutečné fungování závisí na ovladači, rendereru a hardwaru.

Příkaz může být například `omarchy menu keybindings`,
`omarchy menu toggle appearance.theme`, `omarchy agent` nebo
`omarchy system lock`. `omarchy agent` otevře výchozího agenta Omarchy,
kterým nemusí být na jiném počítači Copilot. Příkazy se spouštějí pod tvým
účtem v `/bin/sh`: přidávej jen ty, kterým důvěřuješ.

Stav internetu potřebuje NetworkManager a `nmcli`, teplota čte senzor
`coretemp` a stav aplikace používá `pgrep -x` a přesný název procesu
(nejvýše 15 znaků). Neznámý stav se zobrazuje jako `?`. Stavové texty se
mohou zobrazovat i při zamčené obrazovce. Spouštění příkazů se pokouší
zamčení ověřit dvěma dotazy a při chybě příkaz odmítá, ale mezi kontrolou
a spuštěním existuje časové okno: **nejde o bezpečnostní záruku**.

| Zkratka | Akce |
| --- | --- |
| `?` | Nápověda |
| `Ctrl + Enter` | Potvrzení použití scény |
| `Esc` | Zavření dialogu |

Scény jsou v `~/.config/tbar/profiles.json` (respektuje se `XDG_CONFIG_HOME`).
Při prvním použití editor zaznamená původní konfiguraci rendereru v
`~/.config/tbar/original-renderer.json` a před každým přepsáním existujícího
`~/.config/omarchy-glance/touchbar.json` vytvoří další zálohu vedle něj.
„Obnovit původní“ vrací stav před prvním použitím a odmítne přepsat změny
provedené mezitím mimo aplikaci. Zálohy nemaž, dokud je nepotřebuješ.
Export obsahuje absolutní cestu ke spuštěnému `tbar`; po přesunu aplikace
nebo instalaci balíčku je nutné scénu znovu použít.

Pro kontrolu bez grafického prostředí:

```bash
./build/tbar check
./build/tbar export
bash bin/test
```

`export` vypíše JSON, ale nezapíše konfiguraci. Export byl předběžně ověřen
náhledem `omarchy-glance` revize `9c75e5e1d71cf583fe9b02c124cffa272fd7fcb1`,
nikoli na hardwaru. Renderer je samostatný projekt pod svou licencí.

## Omezení a další vývoj

Na MacBookPro14,2 T1Bridge uvádí omezení uspávání/probuzení. Přechod vyžaduje
kontrolu kompatibility, zálohu Apple EFI, plán návratu, možnou aktualizaci
jádra a restart. **Nepřepínej konkurenční ovladače za běhu.** Na tomto stroji
jsou současné ovladače Touch Baru a zvuku mimo správu balíčků; aktualizace je
může rozbít. Postup instalace experimentálního stacku není v tomto projektu
zatím ověřen ani automatizován.

Výběr scén přímo na Touch Baru, automatické přepínání podle aktivní aplikace,
GIFy, libovolný počet prvků a grafické ikony zatím nejsou implementovány.
Budoucí úpravy OpenCore jsou samostatný úkol; tento projekt bootloader nemění.
