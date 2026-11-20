# Jak přispět

Projekt je ověřený na jediném MacBooku Pro 14,2, takže nejcennější jsou
zkušenosti z dalších strojů a verzí. Dokumentace je česky; hlášení můžeš psát
česky i anglicky.

## Licence

Projekt je pod [licencí MIT](LICENSE). Odesláním příspěvku souhlasíš s jeho
zveřejněním pod stejnou licencí. Nevkládej cizí kód, jehož licence to
neumožňuje; větší změny raději předem domluv v issue.

## Hlášení o hardwaru

Otevři issue a použij šablonu „Hlášení o hardwaru“. Pomůže:

- přesný model (`cat /sys/class/dmi/id/product_name`), verze Omarchy a jádra,
- výstup `bash setup.sh --check`,
- co funguje, co ne a co jsi nezkoušel; u Touch Baru zvlášť zobrazení, Esc,
  dotyk a stav po restartu.

**Před odesláním výstup zkontroluj.** Nikdy nepřikládej `FDRData`, obraz
Apple EFI oddílu, hesla, klíče ani osobní soubory. Diagnostika záměrně
nevypisuje názvy Wi-Fi sítí ani MAC adresy, ale logy jiných nástrojů je
obsahovat mohou.

## Změny v repozitáři

1. Přečti [AGENTS.md](AGENTS.md): pravidla tam platí pro lidi i pro AI agenty.
2. Jedna změna na pull request, s popisem toho, co bylo ověřeno a na čem.
3. Před odesláním spusť testy:

   ```bash
   python3 tests/installer_test.py
   ```

   ```bash
   bash tbar/bin/build && bash tbar/bin/test
   ```

4. Skripty nesmí přepisovat existující odlišnou konfiguraci, formátovat disky,
   měnit bootloader ani přepínat T1 ovladače za běhu. Systémové kroky ověřují
   model a jdou bezpečně spustit opakovaně.
5. Netvrď, že něco funguje, pokud to bylo ověřeno jen testy. Testy
   nenahrazují vyzkoušení na skutečném hardwaru.
6. Screenshoty a příklady scén smějí obsahovat jen fiktivní data.

## Jiné modely

Podpora dalších MacBooků se přidává až po ověření na daném stroji. Skripty
měnící systém proto zatím odmítají jiný model než `MacBookPro14,2`; tuto
kontrolu neodstraňuj bez skutečného testu.
