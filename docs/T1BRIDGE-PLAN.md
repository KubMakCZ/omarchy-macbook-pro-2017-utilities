# Plán a deník: přechod na T1Bridge (MacBookPro14,2)

Experimentální krok: nahradit ručně nainstalované ovladače Touch Baru
oficiálním [T1Bridge](https://github.com/standardagents/t1bridge), aby bylo
možné používat vlastní renderer (`omarchy-glance`). Stav k 2026-11-10;
před použitím ověř aktuální dokumentaci upstreamu.

## Výchozí stav (ověřeno před změnou)

- Model MacBookPro14,2, Omarchy 4.5.0, jádro `7.2.15-1-omarchy`,
  balíčky `linux-omarchy` a `linux-omarchy-headers` 7.2.15-1.
- Touch Bar svítí díky `touchbar-rebind.sh`/systemd službě; funguje po restartu.
- Disk `nvme0n1`: p1 300 MB vfat EFI System Partition (Apple), p2 APFS (macOS),
  p3 exFAT sdílený, p4 2 GB `/boot` (Omarchy), p5 LUKS root.
- Repozitář `[standardagents]` v `/etc/pacman.conf` zatím není.
- Zvukový modul `snd-hda-codec-cs8409` a moduly Touch Baru jsou mimo správu
  balíčků: `omarchy update` nebo nové jádro je může přestat načítat.

## Předpoklady, které musí být splněny před spuštěním

1. Na jiném médiu je záloha Apple EFI oddílu **a je ověřeno**, že obsahuje
   `EFI/APPLE/EMBEDDEDOS/FDRData`. Nikdy ji nedávej do Gitu.
2. Zálohovaná data a obnovovací USB jsou k dispozici; heslo k LUKS umíš napsat
   na klávesnici s aktuálním rozložením (viz TIPY.md).
3. Poslední commit tohoto repozitáře je odeslán na vzdálený repozitář.

## Postup podle upstreamu pro Omarchy

1. `omarchy update` (aktualizuje i jádro a headers).
2. Přidat a ověřit podpisový klíč: primární otisk musí být přesně
   `35B166F78B063B04DE1E3D913E6C4216EB03D371`.
3. Přidat blok `[standardagents]` do `/etc/pacman.conf`
   (`SigLevel = Required DatabaseRequired`,
   `Server = https://linux.standardagents.ai/arch/$repo/$arch`).
4. `sudo pacman -S --needed linux-omarchy-headers t1bridge t1bridge-dkms`
   (bez balíčků fprintd/libfprint, pokud není potřeba Touch ID). Balíček
   headers musí odpovídat jádru Omarchy; upstream uvádí `linux-headers` jen
   pro standardní jádro Arch.
5. Zkontrolovat, že DKMS a initramfs/UKI proběhly bez chyb **před** restartem.
6. Povolit služby, přidat uživatele do skupiny `t1bridge`, restart a
   `sudo t1bridge status`.
7. Stávající `omarchy-mbp-touchbar.service` je třeba vyřadit, aby nesoupeřila
   s novým ovladačem.

## Návrat zpět

Zatím neověřeno. Před krokem 4 si poznamenej `pacman -Q` výstup, nech nainstalované
původní balíčky jádra a v případě problému použij Omarchy snapshot, pokud je
dostupný, nebo obnovovací USB. Doplň sem skutečný postup po vyzkoušení.

## Deník

- 2026-11-10: přečten postup upstreamu; zjištěno, že `sudo` vyžaduje heslo,
  takže kroky s oprávněním správce musí proběhnout v terminálu uživatele.
  Nic v systému nebylo změněno.
