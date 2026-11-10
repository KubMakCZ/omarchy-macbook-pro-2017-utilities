# Pokyny pro AI agenty

Tento soubor je přenosný kontext pro práci v tomto repozitáři. Podrobná
historie požadavků, nápady a nehotové úkoly jsou v [ROADMAP.md](ROADMAP.md).
Neber starší pozorování jako důkaz současného stavu systému.

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
- Projektová licence zatím nebyla zvolena; cizí projekty mají vlastní licence.
  OpenCore je samostatný budoucí nápad, nikoli součást současné instalace.
