# Anleitung: QuinLED Dig-Next-2 mit WLED + KNX

Diese Anleitung beschreibt, wie ein **QuinLED Dig-Next-2** mit der KNX-Firmware aus diesem Repository
eingerichtet wird – vom Flashen bis zur Bedienung per KNX-Taster (z. B. MDT Glastaster II).

> **Stand:** Firmware-Branch `knx-16.0.1` (WLED 16.0.1 + KNX_IP-Usermod).
> Die Firmware ist auf GitHub kompiliert, aber **noch nicht an echter Hardware getestet**.
> Punkte, die noch nicht verifiziert sind, sind mit **[ungeprüft]** markiert.

---

## 1. Wie das Ganze funktioniert

```
KNX-Taster (TP) ──► KNX-IP-Router ──► LAN/WLAN (VLAN, Multicast 224.0.23.12:3671)
                                                   │
                                                   ▼
                                    Dig-Next-2 (WLED + KNX_IP) ──► WS2805-Streifen (RGB + CCT)
```

* Der Controller spricht **KNXnet/IP Routing** (Multicast). **Tunneling und IP Secure werden nicht unterstützt.**
  Ein reines IP-*Interface* (nur Tunneling) reicht deshalb nicht – es wird ein **KNX-IP-Router** gebraucht,
  auf dem Routing **unverschlüsselt** läuft.
* Der Controller ist in der ETS **kein Gerät** (keine Produktdatenbank). Die Verbindung entsteht nur über
  **gleiche Gruppenadressen**: in der ETS mit den Taster-Objekten verknüpft, in WLED im KNX-Usermod eingetragen.
* Der Controller arbeitet **ohne Home Assistant**. Ohne WLAN geht er aber nicht.

## 2. Was du brauchst

| Teil | Hinweis |
|---|---|
| QuinLED **Dig-Next-2** | 2 Datenausgänge, 3 schaltbare Versorgungsausgänge (2× 5 A, 1× 10 A), 5–48 V |
| LED-Streifen mit **WS2805** (RGB + CCT), 24 V | z. B. SuperLightingLED FCOB-24V-ARGBCCT-W15: 15 mm, 14 Pixel/m, kürzbar alle 71,42 mm, max. 26 W/m |
| 24-V-Netzteil | Leistung nach Streifenlänge (bis 26 W/m bei voller Last) |
| KNX-**IP-Router** mit Routing | z. B. Enertex KNXnet/IP Secure Router oder MDT SCN-IP100.03 |
| WLAN im KNX-VLAN | Multicast muss durchkommen (siehe Abschnitt 8) |
| ETS | zum Anlegen der Gruppenadressen und Verknüpfen mit den Tastern |

## 3. Firmware herunterladen

Alle Builds liegen hier: **Releases → `knx-latest`**
<https://github.com/Someone2025/WLED-KNX/releases/tag/knx-latest>

Für den Dig-Next-2: **`dig-next-2-knx.bin`** (identisch mit `WLED_16.0.1_Dig-Next-2-KNX.bin`).

Jede Änderung am Branch `knx-16.0.1` baut GitHub automatisch neu; der Download-Link bleibt gleich.

Was in diesem Build schon voreingestellt ist:

| Einstellung | Wert |
|---|---|
| LED-Ausgang 1 / 2 | GPIO2 / GPIO4, Typ **WS2805**, je 14 Pixel (in WLED anpassen) |
| Tasten | GPIO34, GPIO35 |
| Schaltausgänge (Multi-Relay) | GPIO21 = 10A_1, GPIO20 = 5A_1, GPIO22 = 5A_2, GPIO5 = externes Relais |
| Strombegrenzer (ABL) | 6000 mA (passend zu 160 W / 24 V) |
| KNX-Gruppenadressen | **alle leer** – nichts ist aktiv, bevor du es einträgst |

Quelle der Pinbelegung: <https://quinled.info/dig-next-2-pinout-guide/>

## 4. Erstes Flashen

Der Dig-Next-2 wird mit WLED von QuinLED ausgeliefert.

1. Controller mit Strom versorgen (24 V am Haupteingang oder 5 V per USB-C).
2. Mit dem WLAN-Hotspot des Controllers verbinden. Bei WLED heißt er standardmäßig `<Marke>-AP`
   (hier vermutlich `QuinLED-AP`), Passwort `wled1234` [ungeprüft, ob QuinLED das ab Werk ändert].
3. In der WLED-Oberfläche dein WLAN eintragen (**Config → WiFi Setup**) – das WLAN, das im KNX-VLAN liegt.
4. Neue IP des Controllers im Router nachsehen und **im UniFi-DHCP fest reservieren**.
5. **Config → Security & Updates → „Update WLED“** → `dig-next-2-knx.bin` auswählen → hochladen.
   Der Controller startet neu.
6. Falls das Update abgelehnt wird (z. B. wegen sehr alter Werks-Firmware) [ungeprüft]:
   einmal per USB-C über <https://install.quinled.info> eine aktuelle WLED-Version installieren
   und Schritt 5 wiederholen.

Künftige Updates laufen immer über Schritt 5 – ohne Kabel.

## 5. Anschließen

1. **24 V** vom Netzteil auf den Haupteingang des Dig-Next-2.
2. **Streifen-Versorgung** auf den **10A_1**-Ausgang (bei einem Raum mit bis zu ca. 6,7 A = 160 W).
   Längere Läufe auf zwei Ausgänge aufteilen (5A_1, 5A_2) und ggf. an beiden Enden einspeisen.
3. **Daten** des Streifens auf **LED-Ausgang 1** (GPIO2).
4. WS2805 ist ein Streifen mit **Dual Signal** (4 Pole: +, DI, BI, −). Wie die Backup-Datenleitung (BI)
   am Anfang des Streifens angeschlossen wird, steht im WS2805-Datenblatt des Streifenherstellers
   **[offen – vor der Montage prüfen]**.
5. **Antenne** anschrauben und so legen, dass sie nicht direkt auf Metallprofilen liegt.

> **Wichtig (laut QuinLED):** Ohne aktivierte Relais liefert der Dig-Next-2 **keinen Strom** an die Ausgänge.
> In diesem Build sind die Relais voreingestellt. Prüfen unter **Config → Usermods → MultiRelay**:
> Relais aktiviert, *external* **aus** (dann folgen sie dem Ein/Aus des Lichts).

Damit ist der Streifen bei „Licht aus“ **stromlos** (WS2805-Chips ziehen sonst auch im Aus-Zustand Strom).

## 6. WLED einrichten

### 6.1 LED-Einstellungen (Config → LED Preferences)

* **Länge** je Ausgang = Anzahl **Pixel** (nicht LEDs!). Beim WS2805-COB: **14 Pixel pro Meter**
  (1 Pixel = 71,42 mm = 60 LEDs). Beispiel: 5,5 m → 77 Pixel.
* Typ ist bereits **WS2805 RGBCW**.
* Farbreihenfolge testen: Weiß/Rot/Grün/Blau einzeln einstellen und prüfen, ob die richtige Farbe leuchtet.
* **Strombegrenzer** (ABL) auf das Netzteil abstimmen (Voreinstellung 6000 mA).

### 6.2 Verhalten beim Einschalten (Rückfallebene)

Wenn das Netzteil über einen KNX-Schaltaktor-Kanal (z. B. MDT AKS) als **Rückfallebene** geschaltet wird,
soll der Controller nach dem Einschalten selbst Licht machen:

* **Config → LED Preferences → „Turn LEDs on after power up/reset“** aktivieren.
* Optional: **„Apply preset … at boot“** auf ein Preset mit z. B. 100 % Warmweiß setzen.

Nachteil: Nach einem Stromausfall geht das Licht dann ebenfalls an.

### 6.3 Abschnitte (Segmente)

Abschnitte, die einzeln geschaltet werden sollen (z. B. „Schreibtisch“), werden in WLED als **Segmente**
angelegt (Hauptseite → Segmente → Start-/Endpixel). Die Reihenfolge der Segmente bestimmt ihre
Nummer (0, 1, 2 …) und damit ihre Gruppenadressen (Abschnitt 7.2).

## 7. KNX im Controller einrichten (Config → Usermods → KNX_IP)

### 7.1 Grundeinstellungen

| Feld | Empfehlung |
|---|---|
| `enabled` | an |
| `individual_address` | Adresse auf der IP-Seite des Routers, z. B. `1.0.11` (eindeutig je Controller) |
| `cct_kelvin_min` / `cct_kelvin_max` | **echte** Farbtemperaturen des Streifens, beim WS2805-COB **2700 / 6500** |
| `segment_offset_L` / `M` / `N` | Abstand der Abschnitts-Adressen (siehe 7.2) |
| `segment_ga_start_at_1` | **an** – auch Abschnitt 0 bekommt eigene Adressen |
| `segment_color_objects` | an |
| `answer_read_requests` | an – Taster/HA bekommen beim Start den aktuellen Zustand |
| `dim_time_ms` / `cct_dim_time_ms` | Dauer der Dimmrampe 0→100 % bzw. warm→kalt (Standard 5000 ms) |
| `dim_min_pct` | kleinste Helligkeit beim Runterdimmen (Standard 1 %) |
| `dim_down_switch_off` | aus = Runterdimmen stoppt bei `dim_min_pct` |
| `master_on_all_segments` | an = „Raum EIN“ schaltet alle Abschnitte ein |

> **Alle Gruppenadressen sind ab Werk leer.** Trage nur die ein, die du wirklich nutzt.
> Jede eingetragene Adresse wird gehört bzw. beschrieben.

### 7.2 Gruppenadressen-Schema

Für jeden Raum gibt es **zentrale** Adressen (ganzer Raum). Für jeden Abschnitt *n* errechnet die Firmware
die Adressen automatisch:

```
Adresse(Abschnitt n) = zentrale Adresse + (n + 1) × Versatz (L / M / N)
```

**Vorschlag** (eine Hauptgruppe nur für WLED, eine Mittelgruppe je Raum, Versatz `N = 20`):

| Zweck | DPT | Raum (zentral) | Abschnitt 0 | Abschnitt 1 |
|---|---|---|---|---|
| Schalten | 1.001 | 8/5/0 | 8/5/20 | 8/5/40 |
| Helligkeit absolut | 5.001 | 8/5/1 | 8/5/21 | 8/5/41 |
| Dimmen relativ (Start/Stopp) | 3.007 | 8/5/2 | 8/5/22 | 8/5/42 |
| Farbtemperatur absolut | 7.600 | 8/5/3 | 8/5/23 | 8/5/43 |
| Farbtemperatur relativ (Start/Stopp) | 3.007 | 8/5/4 | 8/5/24 | 8/5/44 |
| Preset (WLED-Szene) | 5.010 | 8/5/5 | – | – |
| **Status** Schalten | 1.001 | 8/5/10 | 8/5/30 | 8/5/50 |
| **Status** Helligkeit | 5.001 | 8/5/11 | 8/5/31 | 8/5/51 |
| **Status** Farbtemperatur | 7.600 | 8/5/13 | 8/5/33 | 8/5/53 |

In WLED trägst du nur die **zentralen** Adressen ein (`GA in: power = 8/5/0`, `bri = 8/5/1`,
`bri_rel = 8/5/2`, `cct = 8/5/3`, `cct_rel = 8/5/4`, `preset = 8/5/5`; `GA out: power = 8/5/10`,
`bri = 8/5/11`, `cct = 8/5/13`) und `segment_offset_N = 20`, `L = 0`, `M = 0`.
Die Abschnitts-Adressen entstehen dann von selbst. Prüfen kannst du sie unter **Info → KNX GA Table**.
Bei Überschneidungen zeigt WLED eine Warnung „KNX GA Conflicts“.

> Das Schema ist ein Vorschlag. Hauptgruppe 8 ist in der bestehenden ETS-Struktur noch frei (0–7 belegt).
> Mit 8 Mittelgruppen passen genau 8 Räume (z. B. 0 Schlafen, 1 Ankleide, 2 Bad, 3 Dusche,
> 4 Flur OG, 5 Kind 1, 6 Kind 2, 7 Kind 3).

**Dusche + Duschfach (zwei Controller, gemeinsam geschaltet):** Beide Controller bekommen dieselben
**Eingangs**-Adressen. Status-Adressen (`GA out`) nur bei **einem** der beiden eintragen,
sonst senden zwei Geräte auf dieselbe Rückmeldung.

### 7.3 Verhalten

* **Dimmen per Taster:** langer Druck = Rampe startet, Loslassen = Stopp (so arbeitet der MDT Glastaster II).
* **Abschnitt ein, Raum aus:** Raum geht an, nur dieser Abschnitt leuchtet.
* **Raum ein:** alle Abschnitte gehen an (`master_on_all_segments`).
* **Status „Raum ein“** ist nur dann „ein“, wenn wirklich ein Abschnitt leuchtet.
* **Farbtemperatur zentral** gilt für alle Abschnitte; pro Abschnitt gibt es eigene Objekte.

## 8. Netzwerk (UniFi)

* WLAN-SSID für die Controller ins **KNX-VLAN** (bei dir VLAN 20) legen – Multicast geht nicht über VLAN-Grenzen.
* Feste IP per DHCP-Reservierung.
* Multicast/IGMP: Im KNX-User-Forum betreibt ein Nutzer Enertex- und Gira-Router im UniFi-Netz
  **mit ausgeschaltetem IGMP Snooping** ohne Probleme. Ob WLAN-Multicast bei dir zuverlässig läuft,
  muss getestet werden [ungeprüft].

## 9. KNX-IP-Router und ETS

1. Router als **Linienkoppler** einsetzen (z. B. physikalische Adresse **1.1.0**). Die Controller liegen
   dann auf der IP-Seite (**1.0.x**).
2. **Routing unverschlüsselt** betreiben (die Firmware kann kein IP Secure).
3. **Filtertabelle:** Eine WLED-Gruppenadresse wird durchgelassen, wenn sie in der ETS mit mindestens einem
   Objekt auf der TP-Linie verknüpft ist (z. B. mit dem Glastaster). Adressen, die nur WLED und HA nutzen,
   werden sonst blockiert. „Alles weiterleiten“ ist nur die Notlösung.
4. In der ETS die Adressen aus 7.2 anlegen und mit den Taster-Objekten verknüpfen:
   * GT2 **Dimmen**: „Schalten“ → Schalten, „Dimmen relativ“ → Dimmen relativ,
     Status Schalten/Helligkeit → Status-Adressen.
   * GT2 **Tunable White**: „Farbtemperatur relativ ändern“ → Farbtemperatur relativ,
     „Status Farbtemperatur“ (2 Byte) → Status Farbtemperatur.
   * Taste für einen **Abschnitt** (z. B. Schreibtisch aus): Schalten des Abschnitts + dessen Status.

## 10. Test-Checkliste

- [ ] Streifen leuchtet über die WLED-Oberfläche; Farben stimmen (R, G, B, Warm-, Kaltweiß)
- [ ] Licht aus → Ausgang stromlos (Relais klicken / Messung)
- [ ] **Info → KNX Status: Connected**, keine „KNX GA Conflicts“
- [ ] Taster: Ein/Aus, Status im Display stimmt
- [ ] Taster: langes Drücken dimmt gleichmäßig, Loslassen stoppt
- [ ] Tunable White am Taster ändert die Farbtemperatur, Status-Kelvin stimmt
- [ ] Abschnitt einzeln aus/an, Rest bleibt unverändert
- [ ] HA bzw. ETS „Lesen“ auf eine Status-Adresse → Antwort kommt
- [ ] Netzteil per Schaltaktor aus/an → Controller startet und macht Licht (Rückfallebene)

## 11. Fehlersuche

| Problem | Mögliche Ursache |
|---|---|
| Kein Licht, WLED zeigt „an“ | Relais (MultiRelay) nicht aktiviert / *external* an; falscher Ausgang |
| Falsche Farben | Farbreihenfolge in den LED-Einstellungen |
| KNX Status „Disconnected“ | kein WLAN / keine IP |
| Taster wirkt nicht | Router filtert (Adresse in ETS nicht mit TP-Objekt verknüpft); Routing aus oder verschlüsselt; Controller nicht im KNX-VLAN; Multicast im WLAN blockiert |
| Display zeigt keinen Status | Status-Adresse (`GA out`) fehlt; `answer_read_requests` aus |
| Abschnitt reagiert nicht | Segment-Nummer/Versatz prüfen (Info → KNX GA Table) |

## 12. Bekannte Einschränkungen

* Nur **KNX/IP Routing** (kein Tunneling, kein KNX Secure).
* **RGB relativ** (Farbton/Sättigung/Helligkeit per 4-Bit) springt noch in Einzelschritten statt zu rampen.
* Ändert man die Anzahl der Segmente, werden alte Abschnitts-Adressen nicht abgemeldet (harmlos).
* Noch nicht an echter Hardware getestet.
