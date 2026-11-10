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

Provedeno na tomto stroji (stav před restartem): balíčky `t1bridge` a
`t1bridge-dkms` 0.1.12, služby `t1-touchbar-hw.service`, `t1-touchid-auth.socket`,
`t1bridge-fingerprint.socket` zapnuty, `omarchy-mbp-touchbar.service` vypnuta,
`/etc/modules-load.d/touchbar.conf` přejmenován na `.disabled-t1bridge`.
Seznam balíčků před změnou: `~/pacman-Q-before-t1bridge.txt`.

Návrat na firmwarový Touch Bar (nevyzkoušeno, připraveno):

```bash
sudo systemctl disable t1-touchbar-hw.service t1-touchid-auth.socket t1bridge-fingerprint.socket
sudo mv /etc/modules-load.d/touchbar.conf.disabled-t1bridge /etc/modules-load.d/touchbar.conf
sudo systemctl enable omarchy-mbp-touchbar.service
sudo pacman -Rns t1bridge t1bridge-dkms
```

Pokud systém nenabootuje: v menu Limine vybrat snapshot (vytvořil ho
`omarchy update`), případně obnovovací USB. Záloha Apple EFI je mimo repozitář.

## Deník

- 2026-11-10: přečten postup upstreamu; zjištěno, že `sudo` vyžaduje heslo,
  takže kroky s oprávněním správce musí proběhnout v terminálu uživatele.
  Nic v systému nebylo změněno.
- 2026-11-10: USB oprava ověřena po restartu. Záloha Apple EFI (`/dev/nvme0n1p1`,
  300 MiB) uložena na externí FAT32 USB; sha256 oddílu i obrazu shodný
  (`89f25a2c…b65e1`), obraz po připojení read-only obsahuje `FDRData`,
  `combined.memboot`, `version.plist`. Obraz není v repozitáři.
- 2026-11-10: `omarchy update` (jádro beze změny 7.2.15-1). Přidán klíč
  `35B166F7…D371` (otisk ověřen) a repozitář `[standardagents]` (záloha
  `/etc/pacman.conf.bak-t1bridge`); databáze repozitáře stažena ručně, ne `-Sy`.
  Nainstalováno `t1bridge`, `t1bridge-dkms` + závislosti (`iio-sensor-proxy`,
  `libqmi` aj.). DKMS `t1_cfgsel` sestaven pro 7.2.15-1-omarchy, UKI přegenerováno
  bez chyb (hláška o chybějícím stromu 7.2.11-arch1 se týká nepoužívaného jádra).
  Skupina `t1bridge` pro uživatele přidána přes `gpasswd` (`usermod` byl zabit).
  Restart zatím neproběhl.
- 2026-11-10: po restartu Touch Bar ukazoval chybu `tiny-dfr crashed`. Příčina:
  ručně vytvořená `/etc/systemd/system/tiny-dfr.service` (bez balíčku) soupeřila
  s T1Bridge; vypnuta (`systemctl disable --now tiny-dfr.service`). Zapnuta
  uživatelská `t1-touchbar.service`, restart `t1-touchbar-hw.service`.
  Dále nainstalován volitelný `t1bridge-omarchy` 0.2.2 (hlasitost, jas, HUD popupy
  v Omarchy) a jednorázově spuštěno `/usr/lib/t1bridge-omarchy/session-start --autostart`.
  Výsledek: Touch Bar zobrazuje vestavěné rozložení, ovládání funguje a popup
  Omarchy se zobrazuje. Zbývá ověřit: zvuk, Esc, přežití dalšího restartu,
  uspání (upstream hlásí problém), `sudo t1bridge status`, `omarchy-glance`/`tbar`.
- 2026-11-10: po dalším restartu uživatel potvrdil zobrazení a reakci Touch Baru.
  `t1bridge status` hlásí USB konfiguraci selected, DRM, NCM, xART, broker a
  Touch Bar ready; keybag not-enrolled a xART admission diagnostics unavailable.
  Hardware i uživatelská služba jsou aktivní, tiny-dfr vypnutý a Wi-Fi připojena.
  Při startu se v logu objevil nedostupný desktop provider; jeho pozdější status
  uspěl, poté znovu spuštěn oficiální session-start helper. Automatický start
  provideru při dalším přihlášení a popupy po restartu je potřeba ověřit.
  Poté uživatel potvrdil funkční popup, reakci mikrofonu v audio panelu a zvuk
  z YouTube ve Firefoxu a funkční Esc.
- 2026-11-10: po dalším restartu uživatel potvrdil Touch Bar a popup bez zásahu
  Copilota. Hardwarová i uživatelská služba jsou aktivní, tiny-dfr vypnutý.
  V logu se znovu objevilo varování o nedostupném desktop provideru při startu;
  následný kontrolní dotaz na provider uspěl. Příčina varování není ověřena.
