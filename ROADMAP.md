# Plán a předání projektu

Požadavky a plán zkontrolovány 2026-11-20; pozorovaný stav instalace níže.
Tento dokument shrnuje zadání a nápady z vývoje, aby na ně mohl navázat člověk
nebo jiný AI agent. Nejde o slib podpory jiných modelů ani o důkaz, že je vše
hotové. Novější skutečný stav ověř v kódu a Gitu.

## Cíl

Veřejný projekt pro **MacBookPro14,2 (13" 2017, Touch Bar, T1) s Omarchy**:
bezpečný, opakovatelný post-install postup pro čistý systém, české tipy a
užitečné aplikace včetně přizpůsobitelného Touch Baru. Uživatel chce nástroje,
které pomohou i dalším lidem, ne pouze jednorázovou opravu jeho počítače.
Rozšíření podpory na jiné Macy se musí samostatně ověřit.

## Aktuální kontrola stroje (2026-11-20)

- Repozitář `main` byl čistý; lokální větev odpovídala `origin/main` na
  commitu `9461ffe` při kontrole.
- Nainstalované balíčky: `tbar 0.1.0-5`, `t1bridge 0.1.12-1`,
  `t1bridge-dkms 0.1.12-1`, `t1bridge-omarchy 0.2.2-1`.
- Zdroj `tbar` v repozitáři je `0.1.0-8` (přepínání scén, ovládací tlačítka, automatika podle aplikace); nainstalován
  2026-11-21.
- Uživatelské služby `t1-touchbar.service` a `omarchy-glance.service` a
  systémová služba `omarchy-mbp-usb-power.service` byly při kontrole aktivní.
- Jde jen o stavové pozorování v daném okamžiku. Aktivní služba sama o sobě
  nepotvrzuje funkčnost všech funkcí ani stav po budoucím restartu.
- Starší renderer byl samostatně sestaven a náhled ověřen v izolované
  konfiguraci. Nový bezpečný aktivační skript byl testován proti připnutému
  upstream rendereru a v simulovaných konfliktech; na čisté instalaci nebyl
  spuštěn. Na skutečném stroji už renderer aktivní je, proto jej test nesmí
  znovu přepínat ani restartovat.

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
- [x] `setup.sh --check` vypisuje stav kroků instalace a další krok;
  `setup.sh --deps` instaluje balíčky pro sestavení.
- [ ] Postupně a bezpečně zjednodušit instalaci T1Bridge: momentálně je ruční
  a je to hlavní překážka pro cizího uživatele.
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
- [ ] Přidat CI (kontrola skriptů, `tests/installer_test.py`, sestavení a
  testy `tbar` v kontejneru Arch), aby příspěvky od dalších lidí šly ověřit.
- [ ] Zvážit přehlednější diagnostiku, kontrolu závislostí a instrukce pro
  případ, kdy existuje odlišný renderer nebo lokálně upravená konfigurace.
  Nikdy ji bez souhlasu nepřepsat.

## Touch Bar: požadované nápady

- [x] Přepínání scén přímo na Touch Baru tlačítkem (`scene`, `scene-next`,
  `tbar next`), implementováno 2026-11-21 v `tbar 0.1.0-8`; uživatel tentýž
  den potvrdil funkci tlačítek scény „Ovladani“ na panelu. Zbývá: výběr ze seznamu a automatické tlačítko
  zpět.
- [x] Volitelné automatické přepínání podle aktivní aplikace (`apps`,
  `tbar-watch.service`), implementováno 2026-11-21. Tentýž den zapnuto na
  testovaném stroji: po přepnutí do Firefoxu se použila scéna `Web` a po
  návratu předchozí scéna (ověřeno stavem `tbar list`, Hyprland 0.56.2);
  **uživatel to na panelu ještě nepotvrdil**. Zbývá ověřit chování po restartu
  a při zamčené obrazovce.
- [x] Ovládání hlasitosti, jasu displeje, podsvícení klávesnice, médií a
  mikrofonu jako hotové skupiny tlačítek (`control`), 2026-11-21; hlasitost,
  jas, podsvícení klávesnice a média uživatel potvrdil na panelu, mikrofon
  ve scéně nebyl. Posuvník jako v macOS renderer
  neumí; vyžadoval by změnu rendereru.
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
- [ ] Uspání do RAM se na tomto stroji neprobudí (log 2026-11-11, uživatel
  znovu 2026-11-20; tehdy ho vyžádal UPower při vybité baterii). Existuje
  `setup.sh --no-suspend`, od 2026-11-21 pokrývá i vybitou baterii; uživatel
  ho spustil 2026-11-21. Hibernace se tentýž den při zkoušce neobnovila
  (první start se zasekl), na víko proto nastavena není; víko teď zamyká a
  zhasíná displej přes vazbu v Hyprlandu (zamknutí uživatel potvrdil, zhasnutí
  je doloženo jen logem). Omarchy samo nereaguje, protože Touch Bar pokládá
  za externí monitor; zvážit hlášení upstreamu a instalační krok pro vazbu.
  Zbývá: zjistit příčinu neobnovení hibernace, rozhodnout chování UPower při
  vybité baterii (hibernace, nebo vypnutí) a případně zkusit `s2idle`. Hibernaci a `s2idle` zkoušet pouze po
  domluvě a záloze; vyřešit i vypnutí displeje při zavřeném víku.
- [ ] Zdvojování klávesy `n`: nechat uživatele spustit
  `setup.sh --keyboard-check`. Podle výsledku buď prodloužit prodlevu
  opakování, nebo zvážit softwarový filtr odskoků (zachytává klávesnici,
  proto vyžaduje bezpečný návrat při chybě).
- [ ] Prozkoumat vzhled OpenCore bootloaderu s logy Macu a Omarchy jako
  **samostatný budoucí úkol**. Žádné změny bootloaderu či EFI bez výslovného
  souhlasu, ověřeného plánu návratu a záloh.
- [ ] Sbírat hlášení z dalších strojů (šablona v `.github/ISSUE_TEMPLATE/`)
  a podle nich doplňovat [docs/HARDWARE.md](docs/HARDWARE.md). Zvážit anglický
  překlad hlavních návodů.
- [x] Licence vlastního kódu: MIT, zvolena 2026-11-20 ([LICENSE](LICENSE)).
  Cizí komponenty mají vlastní podmínky.

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
| [README.md](README.md), [TIPY.md](TIPY.md) | Úvod, přehled stavu a český uživatelský průvodce |
| [docs/INSTALL.md](docs/INSTALL.md) | Postup instalace a současná omezení |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Stav hardwaru, známé potíže a workaroundy |
| [CONTRIBUTING.md](CONTRIBUTING.md), `.github/ISSUE_TEMPLATE/` | Hlášení z jiných strojů a pravidla příspěvků |
| [docs/T1BRIDGE-PLAN.md](docs/T1BRIDGE-PLAN.md) | Historie, záloha a neověřený návrat |
| `setup.sh`, `install-glance.sh`, `enable-glance.sh` | Jednotlivé instalační kroky |
| `tbar/src/backend.cpp`, `main.cpp`, `Main.qml` | Logika scén, CLI a GUI |
| [tbar/README.md](tbar/README.md) | Možnosti, bezpečnost a CLI editoru |
| `tbar/src/agent-skill.md` | Návod pro AI agenty, instaluje `tbar skill install` |
| `tbar/tests/`, `tests/installer_test.py` | Testy editoru a instalace |

Ověření změn editoru: `bash tbar/bin/build && bash tbar/bin/test`. Ověření
instalačních ochran: `python3 tests/installer_test.py`. Tyto testy nenahrazují
ověření na skutečném zařízení ani čisté instalaci.
