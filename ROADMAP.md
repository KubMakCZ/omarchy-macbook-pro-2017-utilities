# Plán a předání projektu

Stav k 2026-11-10. Tento dokument shrnuje zadání a nápady z vývoje, aby na ně
mohl navázat člověk nebo jiný AI agent. Nejde o slib podpory jiných modelů ani
o důkaz, že je vše hotové. Novější skutečný stav ověř v kódu a Gitu.

## Cíl

Veřejný projekt pro **MacBookPro14,2 (13" 2017, Touch Bar, T1) s Omarchy**:
bezpečný, opakovatelný post-install postup pro čistý systém, české tipy a
užitečné aplikace včetně přizpůsobitelného Touch Baru. Uživatel chce nástroje,
které pomohou i dalším lidem, ne pouze jednorázovou opravu jeho počítače.
Rozšíření podpory na jiné Macy se musí samostatně ověřit.

## Co už fungovalo

- [x] Česká klávesnice QWERTZ v grafické relaci; německý potisk zůstává.
  Login, konzole a odemykání disku mohou mít jiné rozložení.
- [x] Wi-Fi přes `brcmfmac`, zvuk a mikrofon byly uživatelem potvrzeny.
  USB workaround byl potvrzen i po restartu; jeho spotřeba a chování po
  uspání nejsou změřené.
- [x] T1Bridge s `t1bridge-omarchy`: Touch Bar, Esc, ovládání a popupy byly
  potvrzeny i po restartu. Dřívější konflikt s `tiny-dfr` byl vyřešen.
- [x] `omarchy-glance` a editor `tbar`: základní scéna a její tlačítka,
  CPU/RAM grafy a grafy v editoru byly na daném Macu potvrzeny.
- [x] GUI nabízí více scén, vlastní příkazy, stav internetu/teploty/procesu,
  grafy, příklady, přeuspořádání, zálohování a návrat původní konfigurace.
  Živý náhled zobrazuje i Fn vrstvu; historie grafů je krátká.
- [x] Pozice vlevo/uprostřed/vpravo, pevné mezery, šířky, dva styly grafů a
  volitelné barvy tlačítek včetně Esc podle tématu Omarchy jsou v editoru.
  Nové zarovnání a automatickou změnu barev ještě uživatel na panelu nepotvrdil.
- [x] Editor má headless CLI s JSON a volitelný agentní skill, aplikace má
  screenshoty, český návod a Arch balíček.
- [x] Existuje [postup instalace](docs/INSTALL.md), diagnostika, samostatné
  kroky pro klávesnici, USB, editor a sestavení rendereru.

## Priorita: čistá instalace a spolehlivost

- [ ] Provést celý postup na skutečně čistém Omarchy na vhodném Macu až po
  výslovné domluvě, ověřených zálohách a plánu obnovy. Zaznamenat verze,
  jednotlivé výsledky a znovuspuštění; netvrdit, že je test hotový bez něj.
- [ ] Postupně a bezpečně zjednodušit instalaci T1Bridge: momentálně je ruční.
  Ověřovat model, zálohu Apple EFI, původ a primární otisk podpisového klíče,
  konfiguraci repozitáře, kompatibilní jádro/headers, DKMS a initramfs/UKI.
  Nevynechávat aktualizaci řízenou Omarchy ani nespouštět samostatné
  `pacman -Sy`. Promyslet přerušení, opětovné spuštění a návrat.
- [ ] Ověřit nový `setup.sh --renderer-enable` na skutečném čistém systému.
  Je záměrně oddělený od sestavení: změní výběr rendereru a restartuje službu.
- [ ] Prověřit dlouhodobou instalaci zvukového modulu a klávesnice/touchpadu
  po aktualizaci jádra. Původní zvukový modul byl mimo správu balíčků;
  staré alternativní DKMS zdroje pro tehdejší jádro nefungovaly.
- [ ] Připravit a ověřit skutečný návrat po změně T1 stacku; současný
  historický návrh návratu není ověřený. Návrat z `omarchy-glance` na
  vestavěný renderer není totéž co odstranění T1Bridge.
- [ ] Zvážit přehlednější diagnostiku, kontrolu závislostí a instrukce pro
  případ, kdy existuje odlišný renderer nebo lokálně upravená konfigurace.
  Nikdy ji bez souhlasu nepřepsat.

## Touch Bar: požadované nápady

- [ ] Přepínání scén přímo na Touch Baru; případně výběr a posouvání seznamu,
  pokud to renderer a ovládání umožní. Zajistit cestu zpět a dostupné `Esc`.
- [ ] Volitelné automatické přepínání podle aktivní aplikace, např. Firefox;
  musí jít vypnout a nesmí ničit ruční výběr. Vyřešit priority, návrat z
  aplikace a chování po restartu.
- [ ] Podpora scén pro programování, grafiku a tvorbu obsahu. Uživatel
  zmiňoval ovládání podobné Macro Deck / Touch Portal: více či méně vlastních
  tlačítek, vlastní akce a výstupní stavy, například zkratky pro Copilot a
  změnu tématu. Zachovat přehledné JSON pro AI.
- [ ] Prozkoumat více stylů grafů, jejich šířky a volby, vzhled Esc a návaznost
  na aktuální téma Omarchy. Dnes existují `dots` a `bars`; Esc má barvy scény,
  ne samostatný editor vzhledu.
- [ ] Prozkoumat skutečné rozestupy `space-between`, `space-around` a
  `space-evenly`. Současný renderer je neumí: tři nezávislé sekce se mohou
  překrýt. Podle možností upstreamu zvážit změnu rendereru, ne pouze tvrdit,
  že jsou podporované.
- [ ] Volitelně prozkoumat GIF/animace, ikony a větší či dynamický počet
  položek. Nynější limit je 12 položek plus automaticky doplněné Esc a Fn.
- [ ] Ověřit nové pozice, styly a automatické změny tématu na fyzickém panelu
  včetně restartu. Nedělat to potichu změnou uživatelovy aktivní scény.

## Hardware a vzhled systému

- [ ] Ověřit kameru, Bluetooth, sluchátka, externí monitor a USB-C zařízení;
  případně Wi-Fi 5 GHz. Touch ID zatím nebylo nastaveno ani ověřeno.
- [ ] Uspání/probuzení a USB po probuzení ověřovat pouze po domluvě a záloze:
  upstream pro T1 uvádí nepodporované systémové suspend/resume a na tomto
  modelu byl hlášen černý displej.
- [ ] Prozkoumat vzhled OpenCore bootloaderu s logy Macu a Omarchy jako
  **samostatný budoucí úkol**. Žádné změny bootloaderu či EFI bez výslovného
  souhlasu, ověřeného plánu návratu a záloh.
- [ ] Vybrat s vlastníkem projektu licenci. Zatím žádná licence vlastního kódu
  nebyla zvolena; cizí komponenty mají vlastní podmínky.

## Bezpečnost a zásady pro dalšího agenta

- Zachovat Apple EFI a strojově specifická data `FDRData`; nikdy je neposílat
  do Gitu ani jinam. Nevypisovat ani nesdílet osobní data, hesla či obsah
  uživatelských scén. Neprovádět formátování, reset T1 ani změny PAM naslepo.
- Nepřepínat konkurenční T1 ovladače za běhu; stará
  `omarchy-mbp-touchbar.service` a `tiny-dfr` nesmějí soupeřit s T1Bridge.
  Při rizikové systémové změně předem vysvětli kroky, připrav zálohu a obnovu;
  restart, přeinstalaci a změny EFI prováděj pouze po souhlasu.
- `tbar` spouští vlastní příkazy jako uživatel. Kontrola uzamčení má časové
  okno a **není bezpečnostní zárukou**; stavové texty mohou být viditelné na
  zamčené obrazovce. Neimportovat ani nespouštět cizí příkazy bez kontroly.
- Konfigurace `tbar` se ukládá atomicky se zámkem a detekcí souběžných změn.
  Zálohy a ochranu před cizími úpravami zachovat; testovat s izolovanými
  `HOME`/`XDG_CONFIG_HOME`, ne na skutečných scénách.
- Komunikovat česky, před většími činnostmi říct, co se chystá, a před
  rizikovými kroky zachovat a publikovat bezpečný stav. Nepublikovat citlivý
  obsah; příklady a screenshoty mají obsahovat pouze fiktivní data.

## Orientace v projektu

| Soubor | Účel |
| --- | --- |
| [README.md](README.md), [TIPY.md](TIPY.md) | Úvod a český uživatelský průvodce |
| [docs/INSTALL.md](docs/INSTALL.md) | Postup instalace a současná omezení |
| [docs/T1BRIDGE-PLAN.md](docs/T1BRIDGE-PLAN.md) | Historie, záloha a neověřený návrat |
| `setup.sh`, `install-glance.sh`, `enable-glance.sh` | Jednotlivé instalační kroky |
| `tbar/src/backend.cpp`, `main.cpp`, `Main.qml` | Logika scén, CLI a GUI |
| [tbar/README.md](tbar/README.md) | Možnosti, bezpečnost a CLI editoru |
| `tbar/tests/`, `tests/installer_test.py` | Testy editoru a instalace |

Ověření změn editoru: `bash tbar/bin/build && bash tbar/bin/test`. Ověření
instalačních ochran: `python3 tests/installer_test.py`. Tyto testy nenahrazují
ověření na skutečném zařízení ani čisté instalaci.
