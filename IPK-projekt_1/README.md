# Dokumentace

## Obsah

1. [Úvod](#úvod)
2. [Použití](#použití)
3. [Hlavní struktura](#hlavní-struktura)
4. [Testování](#testování)
5. [Bibliografie](#bibliografie)

## Úvod

Komunikační aplikace umožňující komunikaci mezi klientem a serverem pomocí TCP a UDP protokolu.

**TCP (Transmission Control Protocol)**

Transmission Control Protocol (TCP) je spojovaný, spolehlivý protokol navržený pro přenos dat v modelu klient-server. TCP poskytuje rozsáhlé funkce pro zajištění spolehlivého a řízeného přenosu dat. Hlavními rysy TCP jsou:

- **Vytvoření a Řízení Spojení**: TCP začíná vytvořením spojení mezi odesílatelem a příjemcem. Tento proces zahrnuje třícestný handshaking, který umožňuje oběma stranám ověření a synchronizaci.

- **Spolehlivé Doručení Dat**: TCP zajišťuje doručení dat v pořadí, bez zdvojení a bez chyb. Toho dosahuje pomocí potvrzování (ACK) a opakování ztracených nebo poškozených segmentů.

- **Řízení Toku a Síťová Kritičnost**: TCP dynamicky upravuje rychlost přenosu dat na základě aktuálních síťových podmínek, minimalizuje přetížení a optimalizuje využití šířky pásma.

- **Spolehlivost a Ztráta Paketů**: TCP využívá časová prodlení a opakování k zajištění spolehlivého doručení dat. Pokud není datový segment potvrzen, TCP ho automaticky opětovně odesílá.

- **Přenos dat v Režimu Toku**: TCP podporuje přenos dat v režimu toku, což umožňuje aplikacím přenášet nepřetržité proudy dat bez fragmentace.

**UDP (User Datagram Protocol)**

User Datagram Protocol (UDP) je nespojovaný, nespolehlivý protokol navržený pro rychlý přenos dat v síťových aplikacích, které nevyžadují spolehlivost. Hlavními rysy UDP jsou:

- **Nespojovaný Přenos Dat**: UDP neprovádí vytváření spojení ani potvrzování příjmu dat. To znamená, že aplikace mohou odesílat data bez předchozího nastavení spojení a bez očekávání potvrzení doručení.

- **Nespolehlivé Doručení**: UDP nezaručuje doručení dat. Pokud jsou data ztracena nebo poškozena, UDP se o to nestará a neodesílá je automaticky znovu.

- **Nižší Režie a Rychlejší Přenos**: Vzhledem k absenci mechanismů spolehlivosti a řízení toku má UDP nižší režii než TCP, což vede k rychlejšímu přenosu dat a nižší latenci.

- **Vhodné pro Reálný Čas a Streaming**: UDP je vhodný pro aplikace vyžadující rychlý a efektivní přenos dat, jako jsou hovory a videohovory, streamování médií nebo online hry, které tolerují ztrátu dat a nedokonalou spolehlivost.

Tato aplikace umožňuje uživatelům komunikovat s ostatními uživateli, měnit svůj nickname a přepojovat se do různých existujících skupin.

## Použití

1. **instalace:**
- stažení repozitáře, uvnitř zadat make, to vytvoří spustitelný soubor. Pro vymazání binárních souboru make clean.

2. **Spuštění UDP:**
./ipk24chat-client -t udp -s serverAddress
- `serverAddress`: IP adresa nebo název serveru

Další volitelné parametry:
- `-p port`: číslo portu (uint16, výchozí hodnota 4567)
- `-d timer`: časovač (uint16, výchozí hodnota 250 ms)
- `-r retries`: počet opakování (uint8, výchozí hodnota 3)
- `-h`: nápověda

**Spuštění TCP:**
./ipk24chat-client -t tcp -s serverAddress
- `serverAddress`: IP adresa nebo název serveru

Další volitelné parametry:
- `-p port`: číslo portu (uint16, výchozí hodnota 4567)
- `-h`: nápověda

3. **Autorizace:**
/auth username secret displayname
4. **Volitelně připojení do skupiny:**
/join channelID
5. **Volitelně změna nickname:**
/rename newDisplayName
6. **Psaní zpráv:**
Uživatelé mohou psát zprávy, které budou distribuovány mezi ostatními členy skupiny.

## Hlavní struktura

Program pracuje podle KSA (konečný stavový automat), se stavy START, AUTH, OPEN, ERROR, END. ve stavu START se nachází hned po spuštění programu a při pokusu o autorizaci přejde do stavu AUTH, kde na základě negativní či pozitivní odpovědi od serveru přechází do stavu OPEN nebo setrvává ve stavu AUTH. Jestliže přijde negativní zprávu od serveru, je uživatel vyzván pro opětovný pokus o autorizaci. Jestliže ale server odpoví pozitivně, autorizace proběhla v pořádku. Ve stavu OPEN se klient nachází v hlavní defautlní skupině a otevírají se mu možnosti psát na server zprávy a číst zprávy ostatních klientů. Klient má rozvněž v tomto stavu možnost přepojit se do jiné, již existující, skupiny (použitím příkazu /join) a nebo také měnit jméno, kterým se ukazuje ostatním uživatelům (příkazem /rename). Ukončí-li klient aplikaci/komunikaci mezi ním a serverem, serveru se zašle zpráva BYE. Do stavu ERROR se dostaneme, přijmeme-li od serveru zprávu, kterou jsme od něho nečekali, následně na to mu automatický odpovíme error zprávou a zašleme zprávu BYE.

Hlavní smyčka while je v souboru main.cpp, kde se cyklí a kontroluje `poll()`. Jestli funkce `poll()` zachytí vstup od serveru nebo klienta reaguje na to. 

Na vstup od serveru reaguje závoláním funkce `nextState`. V této funkci na základě stavu, ve kterém se program nachází pomocí switche, očekává od serveru různé typy zpráv a na tyto zprávy příslušným způsobem reaguje.

Na vstup od klienta reaguje zavoláním funkce `sendingFromClient`. Zde se klient nachází ve stavu OPEN a očekávají se od něho /join, /rename, /help a nebo msg zprávy. Podle toho, který typ klient zadá se příslušným způsobem reaguje. Jestli-že klient zadá /join, přejde se do funkce `sendingJoin`, kde se čeká na serverovou odpověď REPLY OK, a zprávy od klienta se do té doby neposílají.

## Testování
Pro testování TCP byl využit nástroj netcat, který umožňuje simulaci komunikace mezi klientem a serverem. Na základě vstupů ze serverové strany byla pomocí výpisů stavů ověřována korektnost reakcí. 

Pro testování UDP byl použit program Packet Sender, umožňující odesílání zpráv pomocí paketů. Předem byla nastavena struktura zpráv, které může server odeslat, a stejně jako u TCP byly kontrolovány reakce na vstupy ze serveru prostřednictvím výpisů stavů. V Packet Senderu jsem si rovněž mohl ověřit zprávy, které server od klienta dostane. Tyto zprávy jsem poté byl schopen porovnat s formátem ze zadání.

Při testování bylo experimentováno s různými příkazy před autorizací, špatnými parametry pro /auth, opakovaným použitím /auth po negativním potvrzení od serveru, a také s příkazy /join a /rename. Zkoušel jsem například hned po spuštění zadat jiný příkaz než je /auth nebo /help, kde na to odpovídám "ERR: Invalid command. Expected '/auth' or '/help'.". Rovněž po uspěšné autorizaci znovu zkoušet příkaz /auth. Testoval jsem, jestli po pokus o autorizaci dostane klient správnou zpětnou vazbu `Success: ` nebo `failure: `. Dále jsem testoval zda po zadání příkazu /join channelID a poté posílání zpráv, chodí serveru tyto zprávy, nebo musí první odpovědět REPLY OK. Při UDP jsem zjišťoval, zda po zaslání zprávy serveru a žádné zpětné vazbě, odesilá program zprávu po určitém čase znovu a zda po po CONFIRM a nebo REPLY OK toto znovuodesílání přestane. Zkoušel jsem v určitých fázích ctrl+c a očekával jsem na serverové straně zprávu BYE od klienta a následně klient čeká na CONFIRM od serveru. 

Pro memory leak jsem vyzkoušel valgrind, který nezjistil memory leak.

Příklad konkrétního testování TCP:
- klient: /auth m m m
- příchod na server: AUTH m AS m USING m
- odpověď serveru: REPLY NOK IS smůla
- příchod klientovi: Failure: smůla
- klient: auth m m k
- příchod na server: AUTH m AS k USING m
- odpověď serveru: REPLY OK IS ahoj
- příchod klientovi: Success: ahoj
- klient: ahoj
- příchod na server: MSG FROM k IS ahoj
- klient: /join channel
- příchod na server: JOIN channel AS k
- klient: ahoj
- příchod na server: 
- klient: jak se máš
- příchod na server: 
- odpověď serveru: REPLY OK IS přijat
- příchod klientovi: Success: přijat
- příchod na server: MSG FROM k IS ahoj \n MSG FROM k IS jak se máš
- server: MSG FROM rebeka IS dobře
- příchod klientovi: rebeka: dobře
- klient: /rename Jeník
- příchod na server: 
- klient: mam nove jmeno
- příchod na server: MSG FROM Jeník IS mam nove jmeno
- klient: "ctrl+c"
- příchod na server: BYE

Příklad konkrétního testování UDP:
- klient: /auth m m m
- příchod na server: 02 00 00 6D 00 6D 00 6D 00 
- odpověd serveru: 00 00 00 + 01 00 01 01 00 00 48 65 6C 6C 6F 20 77 6F 72 6C 64 00
- příchod klientovi: Success: Hello world
- příchod na server: 00 00 01 
- klent: ahoj
- příchod na server: 04 00 01 6D 00 61 68 6F 6A 00 
- odpověď serveru: 00 00 01
- klent: /join channel
- příchod na server: 03 00 02 63 68 61 6E 6E 65 6C 00 6D 00 
- odpověď serveru: 01 00 02 01 00 02 48 65 6c 6c 6f 20 77 6f 72 6c 64 00  + 00 00 02
- příchod klientovi: Success: Hello world
- příchod na server: 00 00 02
- klient: "ctrl+c"
- příchod na server: FF 00 03 
- příchod klientovi: 00 00 03

## Bibliografie

[RFC9293] Eddy, W. Transmission Control Protocol (TCP) [online]. August 2022. [cited 2024-04-01]. DOI: [10.17487/RFC9293](https://datatracker.ietf.org/doc/html/rfc9293). Available at: [https://datatracker.ietf.org/doc/html/rfc9293](https://datatracker.ietf.org/doc/html/rfc9293)  
[RFC768] Postel, J. User Datagram Protocol [online]. March 1997. [cited 2024-04-01]. DOI: [10.17487/RFC0768](https://datatracker.ietf.org/doc/html/rfc768). Available at: [https://datatracker.ietf.org/doc/html/rfc768](https://datatracker.ietf.org/doc/html/rfc768)
