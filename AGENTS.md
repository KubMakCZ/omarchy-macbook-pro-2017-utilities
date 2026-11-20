# Pokyny pro AI agenty

Tento soubor je přenosný kontext pro práci v tomto repozitáři. Podrobná
historie požadavků, nápady a nehotové úkoly jsou v [ROADMAP.md](ROADMAP.md).
Neber starší pozorování jako důkaz současného stavu systému.

## Rychlá orientace

| Kde | Co tam je |
| --- | --- |
| `setup.sh` | rozcestník kroků; `--check` jen čte, ostatní volby viz `--help` |
| `diagnose.sh` | diagnostika bez změn a bez názvů sítí či MAC adres |
| `setup-keyboard.sh`, `install-*.sh`, `enable-glance.sh` | jednotlivé instalační kroky |
| `*.service`, `touchbar-rebind.sh`, `usb-controllers-active.sh` | systémové služby a jejich skripty |
| `tbar/src/` | editor: `backend.cpp` (scény, export, zálohy), `main.cpp` (CLI), `Main.qml` (GUI) |
| `tbar/src/profile-schema.json`, `tbar/src/agent-skill.md` | schéma scény a návod pro agenty, obojí vložené do binárky |
| `tests/`, `tbar/tests/` | testy instalačních ochran a editoru |
| `docs/`, `TIPY.md`, `ROADMAP.md` | návody, stav hardwaru, plán |

Příkazy, které lze spouštět bez ptaní (nic nemění mimo `tbar/build`):

```bash
bash setup.sh --check
python3 tests/installer_test.py
bash tbar/bin/build && bash tbar/bin/test
```

Vše ostatní v `setup.sh`, `sudo`, `systemctl` se změnou stavu, `tbar apply`,
`tbar restore`, `tbar select` a `omarchy-glance on/off` mění skutečný systém
nebo Touch Bar: nejdřív se zeptej.

## Záměr a spolupráce

- Cílem je bezpečně a opakovaně zprovoznit **MacBookPro14,2 (13" 2017, T1,
  Touch Bar)** na Omarchy, včetně české QWERTZ, hardwaru a editoru `tbar`.
  Podpora jiných modelů musí být samostatně ověřena.
- Komunikuj česky. Před činností stručně řekni, co budeš dělat, a před
  rizikovými změnami vysvětli riziko i možnost návratu. Požádej o potvrzení
  před restartem, přeinstalací, změnou bootloaderu, disků nebo EFI.
- Uživatel chce pokud možno, aby agent příkazy provedl sám; pokud je nutné
  heslo nebo interaktivní potvrzení, vysvětli, co má uživatel udělat.
  Neobcházej oprávnění ani bezpečnostní kontroly.
- Průběžně aktualizuj dokumentaci. U tohoto projektu si uživatel přeje změny
  commitovat a pushovat; před rizikovými zásahy ověř, že je bezpečný stav
  odeslán. Nepřidávej do Gitu cizí rozpracované změny ani citlivá data.

## Nový nebo přeinstalovaný systém

1. Nejdřív si přečti [README.md](README.md), [docs/INSTALL.md](docs/INSTALL.md)
   a relevantní část [ROADMAP.md](ROADMAP.md). Zkontroluj `git status` a
   aktuální dokumentaci upstream projektů; verze a postupy se mění.
2. Začni čtením stavu: ověř model, jádro, nainstalované balíčky a služby,
   funkci sítě, klávesnice, zvuku, USB a Touch Baru. Použij
   `bash setup.sh --check` a přiměřenou další diagnostiku. Neinstaluj
   preventivně konkurenční ovladače jen proto, že něco má značku Broadcom.
3. Před instalací či změnou T1 stacku ověř **aktuálně dostupnou externí
   zálohu Apple EFI a dat tohoto Macu**, funkční heslo a možnost obnovy.
   Historicky byla záloha vytvořena a ověřena, ale její pozdější dostupnost
   se nesmí předpokládat. Nikdy nezveřejňuj `FDRData`, obraz EFI, osobní
   soubory ani detailní logy bez kontroly.
4. Postupuj po samostatných krocích podle [docs/INSTALL.md](docs/INSTALL.md).
   T1Bridge v současné verzi projektu vyžaduje ruční kroky; kompletní
   instalace od začátku na čistém systému ještě nebyla ověřena. Ověř původ
   a primární otisk podpisového klíče, kompatibilní kernel headers, DKMS a
   vytvoření initramfs/UKI. Na Omarchy používej aktualizaci přes `omarchy
   update`; neřeš ji samostatným `pacman -Sy`.
5. Nedovol souběh konkurenčních T1 ovladačů či služeb, zejména staré
   `omarchy-mbp-touchbar.service` a `tiny-dfr` s T1Bridge. Nepřepínej
   ovladače za běhu. Nevypínej funkční služby ani nepřepisuj existující
   konfiguraci bez ověření a plánu návratu.
6. Po restartu ověř skutečné zobrazení, Esc, dotyk a ovládání; zkontroluj
   zvuk, mikrofon, Wi-Fi, USB a stav T1Bridge. Uspání/probuzení má známá
   omezení a zkouší se pouze po domluvě. Neoznačuj Touch ID, kameru nebo jiné
   zařízení za funkční bez odpovídajícího ověření.

## Vývoj a ochrana dat

- `tbar` podporuje scény, vlastní příkazy, grafy, náhled a JSON CLI. Stav
  implementace i budoucí požadavky jsou v [ROADMAP.md](ROADMAP.md); použití
  a hranice bezpečnosti v [tbar/README.md](tbar/README.md).
- Uživatelské příkazy mohou být nebezpečné. Kontrola uzamčení při spouštění
  není bezpečnostní zárukou; stavové texty mohou zůstat viditelné na zamčeném
  panelu. Před použitím kontroluj neznámé příkazy.
- Zachovej zálohy, atomické zápisy a kontrolu souběžných změn. Testuj s
  izolovanými `HOME` a `XDG_CONFIG_HOME`; neměň skutečnou aktivní scénu,
  motiv ani výběr rendereru jen kvůli testu.
- Testy instalačních kroků: `python3 tests/installer_test.py`. Po změně
  editoru: `bash tbar/bin/build && bash tbar/bin/test`. Testy nenahrazují
  potvrzení na skutečném hardwaru.
- Vlastní kód je pod licencí MIT ([LICENSE](LICENSE)); cizí projekty mají vlastní
  licence a jejich kód se do repozitáře nekopíruje bez kontroly kompatibility.
  OpenCore je samostatný budoucí nápad, nikoli součást současné instalace.

## Konvence při úpravách

- **Skripty:** `set -euo pipefail`, české hlášky, chyby na stderr. Systémové
  kroky nejdřív ověří model `MacBookPro14,2`. Existující odlišný soubor ani
  symbolický odkaz se nepřepisuje: skript skončí chybou a nic nezmění.
  Opakované spuštění musí být bezpečné. Novou ochranu pokryj v
  `tests/installer_test.py`.
- **Editor:** změna formátu scény znamená upravit společně validaci v
  `backend.cpp`, `profile-schema.json`, `agent-skill.md`, příklad a tabulku v
  [tbar/README.md](tbar/README.md) a testy. Texty GUI jdou přes překlady v
  `tbar/src/i18n/`. Při změně obsahu balíčku zvyš `pkgrel` v `tbar/PKGBUILD`.
- **Dokumentace:** každý fakt má jedno místo. Stav hardwaru a workaroundy
  patří do [docs/HARDWARE.md](docs/HARDWARE.md), postup do
  [docs/INSTALL.md](docs/INSTALL.md), plány do [ROADMAP.md](ROADMAP.md),
  každodenní tipy do [TIPY.md](TIPY.md); [README.md](README.md) je jen přehled
  s odkazy. [docs/T1BRIDGE-PLAN.md](docs/T1BRIDGE-PLAN.md) je historický
  deník: nepřepisuj ho, jen případně přidej datovaný záznam.
- **Tvrzení o funkčnosti:** rozlišuj „potvrzeno uživatelem na panelu“,
  „prošly testy“ a „neověřeno“. K ověřenému stavu piš datum a verze. Když se
  změní verze jádra, T1Bridge nebo Omarchy, aktualizuj je v README i
  HARDWARE.md společně.
- **Hotovo znamená:** testy prošly, dokumentace odpovídá změně, `git status`
  neobsahuje cizí ani citlivé soubory, změna je commitnutá a odeslaná.
  Co nešlo ověřit na skutečném hardwaru, výslovně uveď.
