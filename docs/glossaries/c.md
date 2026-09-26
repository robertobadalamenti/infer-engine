# Glossario C — infer-engine

Concetti C spiegati man mano, per chi viene da Node.js/TypeScript e parte da zero. Ogni voce: cos'è, perché conta qui.

---

## Toolchain e compilazione

### Compilatore vs build system
Sono due cose distinte. Il **compilatore** (qui `clang`, quello di Apple installato con gli Xcode Command Line Tools) traduce un file `.c` in codice macchina. Il **build system** (qui un `Makefile`) non compila niente: automatizza le chiamate al compilatore quando i file sono più di uno, ricompilando solo ciò che è cambiato. Con un file solo basterebbe `clang -o engine src/main.c` a mano; con dieci file e flag diversi diventa impraticabile. CMake, in confronto, è un generatore: produce lui i file di build — livello in più che qui non serve. Compilare sempre con `-Wall -Wextra`: il C accetta in silenzio molto codice sbagliato, e i warning sono la prima linea di difesa. Attenzione però ai suggerimenti che accompagnano i warning (`note: initialize the variable to silence this warning` → `= NULL`): servono a **zittire** il warning, non a correggere il bug — su un puntatore non inizializzato, `= NULL` trasforma un bug casuale in un crash garantito.

### clangd e `.clang-format`
**clangd** è il *language server* del C: un processo in background (la `d` sta per *daemon*) che analizza il codice mentre scrivi e fornisce all'editor autocompletamento, "vai alla definizione", errori in tempo reale e formattazione — l'equivalente di tsserver per TypeScript. Usa lo stesso parser del compilatore clang, quindi gli errori nell'editor coincidono con quelli della compilazione. Parla con VS Code tramite il Language Server Protocol, per cui funziona uguale in qualunque editor. Qui usa `/usr/bin/clangd` (Xcode). Lo stile di formattazione sta nel file **`.clang-format`** nella root (LLVM, asterisco attaccato al nome): lo leggono clangd, l'estensione C/C++ e `clang-format` da terminale, quindi lo stile appartiene al progetto, non all'editor. In VS Code i `.h` vanno associati a C (`files.associations`), altrimenti sono trattati come C++.

### Livelli di ottimizzazione (`-O0`, `-O2`, `-g`)
`-O0` (il default se non specifichi nulla) compila in modo diretto: ogni riga di C corrisponde a codice macchina riconoscibile. Da `-O2` in su il compilatore riordina le istruzioni, elimina variabili, incorpora funzioni (inlining) e vettorializza: il programma è più veloce ma **non corrisponde più riga per riga al sorgente**, quindi il debugger mostra salti apparentemente illogici e variabili "ottimizzate via". `-g` è ortogonale: aggiunge i simboli di debug (nomi e numeri di riga) e non rallenta il codice generato.

**Scelta di questo progetto**: `-g` senza nessun `-O` durante la fase di correttezza; `-O2`/`-O3` entreranno nella fase di profiling, come decisione presa sui dati e non per abitudine. Annotato anche come commento nel `Makefile`, dove il flag andrebbe aggiunto.

### Makefile
Un elenco di **regole**: `bersaglio: prerequisiti`, seguito dalle righe della **ricetta** (comandi shell), che devono iniziare con un **TAB vero** — con gli spazi Make dà `missing separator` (su macOS si verifica con `cat -t Makefile`, il tab appare come `^I`; `cat -A` è la versione Linux). Make esegue la ricetta solo se il bersaglio non esiste o se un prerequisito ha una data **più recente**; altrimenti `Nothing to be done`. Conseguenze pratiche:

- `make` senza argomenti costruisce il **primo** bersaglio del file: per convenzione `all`, che elenca tutto ciò che il progetto produce.
- Gli header vanno tra i prerequisiti anche se non si passano a `clang`: Make non legge gli `#include`, e senza di loro modificare un `.h` non fa ricompilare.
- Il Makefile stesso **non** è un prerequisito: cambiando i `CFLAGS` serve `make clean && make`, oppure `make -B`, che ricompila tutto ignorando le date.
- `.PHONY` dichiara i bersagli che non sono file (`all`, `run`, `clean`): senza, un file con quel nome li renderebbe "già aggiornati" e la ricetta non partirebbe mai. `run: $(BIN)` non confronta niente: fa sì che il binario sia aggiornato *prima* di eseguirlo.
- Le variabili (`CC`, `CFLAGS`, `$(BIN)`) sono sostituzione di testo; `$@` vale il nome del bersaglio della regola in corso.
- Ogni riga della ricetta gira in una shell separata: un `cd` non vale per la riga dopo.
- I warning compaiono solo quando il compilatore gira: `Nothing to be done` non vuol dire "nessun warning".

### File oggetto e linker
Il compilatore elabora **un `.c` alla volta**, in isolamento: non "vede" gli altri file del progetto come fa un bundler JS. Ogni `.c` diventa un file oggetto `.o` (codice macchina con i riferimenti esterni ancora da risolvere), e solo alla fine il **linker** unisce i `.o` in un eseguibile. Corollario: se manca la definizione di una funzione, l'errore arriva in fase di link, non di compilazione.

### Un solo spazio di nomi: i prefissi
In C non esistono namespace né moduli: tutti i simboli globali del programma — le tue funzioni, quelle della libreria standard, quelle di ogni libreria linkata — vivono in **un unico spazio piatto**, e due funzioni con lo stesso nome danno errore di simbolo duplicato al link. Per questo ogni simbolo pubblico si prefissa con il nome del modulo (`gguf_map_file`, `GGUF_SUCCESS`; in ggml tutto è `ggml_*`, in llama.cpp `llama_*`). Non è decorazione, è l'unico meccanismo disponibile. Vale anche per i valori degli `enum` e per le macro.

### Header (`.h`) vs sorgente (`.c`) — dichiarazione vs definizione
Poiché il compilatore vede un `.c` alla volta, quando compila `main.c` deve sapere che `gguf_parse_header` esiste, che argomenti prende e cosa restituisce, ma non ha bisogno del suo corpo. La firma senza corpo è la **dichiarazione** (prototipo); il corpo è la **definizione**. Le dichiarazioni condivise stanno in un `.h`, le definizioni nel `.c`. In questo progetto `.h` e `.c` stanno affiancati in `src/`: la separazione `include/` vs `src/` ha senso per una libreria distribuita a terzi, non per un eseguibile singolo.

Da non confondere con l'**uso**: la dichiarazione nomina i tipi (`void *memcpy(void *dst, const void *src, size_t n);`), la chiamata passa valori (`memcpy(&h.magic, bytes, 4);`).

### `#include`
**Non** è un `import` di JS: è il **preprocessore** che incolla letteralmente il testo del file indicato dentro il `.c`, prima che il compilatore veda il codice. `#include <stdio.h>` cerca nei percorsi di sistema, `#include "gguf.h"` parte dalla cartella del file corrente. Corollario importante: in C niente è disponibile di default (non esistono globali come in JS) — ogni tipo o funzione va dichiarata prima includendo l'header giusto: `<stdio.h>` per `printf`, `<stdint.h>` per `uint32_t`, `<string.h>` per `memcpy`, `<fcntl.h>` per `open`, `<sys/mman.h>` per `mmap`, `<errno.h>` per `errno`, `<stddef.h>` per `size_t`. Gli header della libreria C finiscono in `.h`: le forme `<cstddef>`, `<cstdio>`, `<cstring>` sono i nomi **C++** degli stessi header, e in C non esistono.

### Direttive del preprocessore (niente `;`)
Le righe che iniziano con `#` non sono istruzioni C: le elabora il preprocessore, e la direttiva **finisce a fine riga** — il terminatore è il newline, non il punto e virgola. Aggiungerlo non è innocuo: `#define` è sostituzione testuale, quindi `#define MAX 100;` fa sì che `int x = MAX * 2;` diventi `int x = 100; * 2;`. `#define GGUF_H` senza valore definisce una macro **vuota**: `#ifndef` controlla se la macro esiste, non quanto vale.

### Include guard
Poiché `#include` è sostituzione testuale, lo stesso header incluso due volte nella stessa unità di compilazione farebbe comparire le sue definizioni due volte → errore di ridefinizione. Si protegge con `#ifndef GGUF_H` / `#define GGUF_H` / … / `#endif`: alla seconda inclusione la macro esiste già e il blocco viene saltato. Forma breve equivalente: `#pragma once` (non standard formalmente, ma supportata da clang/gcc/msvc e usata da `ggml.h`). Ogni header ne ha bisogno, e tutto il contenuto dell'header va messo dentro la guard, include compresi.

---

## Sintassi di base (differenze da TS che sorprendono)

### Il tipo viene prima del nome
`uint32_t magic;` si legge "magic è un uint32_t" — l'opposto di `magic: uint32_t` in TypeScript. Vale per variabili, campi di struct e parametri.

### Niente inferenza di tipo, niente ASI
Il tipo va **sempre** scritto: `const size = sizeof(x);` non compila, perché `const` è un qualificatore, non un tipo (serve `size_t size = …`). E ogni istruzione finisce con `;`: in C non esiste l'inserimento automatico del punto e virgola di JavaScript.

### Inizializzare una struct: `{0}` e designated initializer
`gguf_file_t f = {0};` azzera tutti i campi. `gguf_file_t f = { .fd = -1 };` è un *designated initializer* — sintassi con il **punto**, non `fd:` come negli oggetti JS — e imposta i campi nominati, mentre **quelli non nominati vengono azzerati automaticamente**. Serve a partire da uno stato noto invece che da valori casuali, scegliendo per ogni campo il suo valore "vuoto" corretto (per un file descriptor è `-1`, non `0`).

### Cast `(tipo)valore`
Dice al compilatore "tratta questo valore come se fosse di quel tipo": `(size_t)st.st_size`, `(const uint8_t *)base`. Serve quando i tipi non coincidono e la conversione è voluta e sicura.

### `exit()` / `return` in `main`, e `main(void)`
Dentro `main`, `return 1;` equivale a `exit(1)` ed è più idiomatico (`exit` richiede `<stdlib.h>`). Il codice di uscita si controlla con `echo $?`: 0 = successo. `int main(void)` è preferibile a `int main()`: le parentesi vuote significano storicamente "parametri non specificati", `(void)` significa esplicitamente "nessun parametro".

### Stile del progetto
`snake_case` per i nomi (convenzione C, coerente con ggml/llama.cpp), asterisco attaccato al nome (`uint8_t *p`, vedi sotto), commenti in inglese e solo dove il *perché* non è ovvio.

---

## Memoria e puntatori

### Puntatore
Una variabile che non contiene un valore, ma **l'indirizzo in memoria** dove si trova un valore. In JS/TS non esistono esplicitamente — ogni oggetto è già "un puntatore" gestito per te dal runtime. In C li scrivi e li maneggi tu: `int* p` è "un puntatore a un intero", cioè una variabile che contiene l'indirizzo dove sta un intero.

### Il `*` sta sulla variabile, non sul tipo
Non esiste "il tipo dei puntatori": ogni puntatore dichiara **a cosa punta**, e il `*` appartiene al nome della variabile. `uint8_t *bytes` si legge da destra a sinistra: "bytes è un puntatore a uint8_t". Tutti i puntatori occupano 8 byte sull'M1 (sono indirizzi a 64 bit), indipendentemente dal tipo puntato. Che il `*` appartenga alla variabile si vede qui: `int* a, b;` dichiara `a` puntatore e `b` **int normale** — per questo in C si scrive `int *a, *b;`, con l'asterisco attaccato al nome.

### `*` dereferenziazione, e `.` vs `->`
Nell'uso (non nella dichiarazione), `*p` significa "vai all'indirizzo contenuto in `p`" e restituisce la cosa puntata. Per i campi di una struct: si usa `.` quando hai la struct stessa (`gguf_file_t file;` → `file.fd`) e `->` quando hai un puntatore alla struct (`gguf_file_t *f` → `f->fd`). `f->fd` è una scorciatoia per `(*f).fd`. Tipico dentro una funzione che riceve un out-parameter: il parametro è un puntatore, quindi `->`. `&h->magic` si legge `&(h->magic)`: l'indirizzo del campo `magic` della struct puntata.

### Dichiarare un puntatore non crea l'oggetto puntato
`int n;` crea un `int`: lo spazio esiste, con contenuto casuale. `int *p;` crea solo un puntatore: 8 byte che contengono un indirizzo casuale, e **nessun `int` esiste da nessuna parte**. È come `let file: GgufFile;` in TypeScript: l'annotazione di tipo non crea l'oggetto. Passare un puntatore del genere a una funzione che ci scrive dentro significa scrivere in memoria che non appartiene a nessuna tua variabile. Lo schema giusto è quello di `struct stat st; fstat(fd, &st);`: si crea la struct (senza asterisco) e si passa il suo indirizzo.

### Aritmetica dei puntatori
Il tipo puntato decide **di quanto ti sposti** quando aggiungi 1: `p + 1` significa "un elemento più avanti", non "un byte più avanti". Con `uint8_t *` avanza di 1 byte, con `uint32_t *` di 4, con `uint64_t *` di 8, con `gguf_header_t *` di 24. Per leggere un formato binario si usa `uint8_t *` proprio perché così `bytes + 16` corrisponde esattamente all'offset 16 della specifica.

### `void *`
Puntatore "a tipo non specificato": un indirizzo e basta. Proprio perché non si sa cosa ci sia dall'altra parte, il C **non permette** né di dereferenziarlo né di farci aritmetica (non saprebbe di quanto avanzare). È il tipo restituito da `mmap` e accettato da `memcpy`; per usarlo sui byte si casta a `uint8_t *`.

### `&` — operatore "indirizzo di"
In C **tutto è passato per valore**: gli argomenti di una funzione vengono copiati, quindi una funzione non può modificare una variabile del chiamante. Per farlo le si passa l'**indirizzo** della variabile, e lei ci scrive attraverso il puntatore. È la differenza tra `printf(…, x)` — legge il valore — e `fstat(fd, &st)` o `memcpy(&h.magic, …)` — devono scrivere nella tua variabile.

### `const`: dato protetto vs puntatore bloccato
Due cose diverse, e l'intuizione da TypeScript porta alla seconda:

| Dichiarazione | Cosa è const | `p[0] = 5;` | `p = altro;` |
|---|---|---|---|
| `const uint8_t *p` | il **dato** puntato | ✗ | ✓ |
| `uint8_t *const p` | il **puntatore** | ✓ | ✗ |
| `const uint8_t *const p` | entrambi | ✗ | ✗ |

Si legge da destra a sinistra dal nome: `const` si applica a ciò che ha immediatamente a sinistra. Il `const p` di TypeScript corrisponde alla **seconda** riga. Per il parser GGUF serve la prima: il cursore deve potersi spostare nel file, i byte del file devono restare intoccabili.

### malloc / free
`malloc` chiede al sistema operativo un blocco di memoria e ti restituisce un puntatore al suo inizio. `free` lo restituisce. In JS/TS questo lo fa il garbage collector automaticamente; in C lo fai a mano, e se ti dimentichi `free` hai una memory leak, se lo chiami due volte o usi memoria già liberata hai un bug serio (spesso silenzioso).

---

## Struct e layout in memoria

### struct, tag e `typedef`
`struct esempio { … };` definisce una struct il cui **tag** è `esempio`. Il tag non è un nome di tipo utilizzabile da solo: il tipo si chiama `struct esempio` (in C++ basterebbe `esempio`, in C no). Per evitare di ripetere `struct` ovunque si usa `typedef struct { … } gguf_header_t;`, che crea un alias di tipo; il suffisso `_t` è la convenzione per "questo è un nome di tipo". Si accede ai campi con `.` su una variabile (`h.magic`) e con `->` su un puntatore a struct (`p->magic`).

### `sizeof`
Operatore (non funzione) che restituisce la dimensione **in byte** del tipo o della variabile che gli passi: `sizeof(uint64_t)` vale 8. Il risultato è di tipo `size_t`, quindi si stampa con `%zu`, ed è calcolato a tempo di compilazione.

### Allineamento e padding delle struct
Il compilatore può inserire **byte invisibili tra i campi** di una struct, quindi la struct in memoria può essere più grande e disposta diversamente da come l'hai scritta. Regola: ogni campo va a un offset multiplo del proprio allineamento (per i tipi base coincide con la dimensione), e la struct riceve padding finale perché `sizeof` sia multiplo dell'allineamento più grande tra i campi (così gli elementi di un array restano allineati).

```c
struct esempio { uint8_t a; uint32_t b; };   // sizeof == 8, non 5

offset:   0     1     2     3     4     5     6     7
        [ a ][ pad ][ pad ][ pad ][    b (4 byte)    ]
```
`b` non può stare a offset 1 (non multiplo di 4) → va a 4, con 3 byte di padding prima. (Verificato sull'M1: stampa 8.)

**Perché il compilatore lo fa**: la CPU legge la memoria a blocchi di dimensione fissa. Un valore contenuto in un blocco si legge in un accesso solo; a cavallo tra due ne servono due (su ARM64 è permesso ma più lento, su altre architetture è un crash).

**Perché conta qui**: in un file binario come il GGUF il padding non esiste — i byte sono quelli della specifica, uno dietro l'altro. Castare i byte del file a un puntatore a struct funziona solo finché per caso nessun campo richiede padding, e si rompe in silenzio se qualcuno riordina i campi. Per questo i campi si leggono uno alla volta con `memcpy`, ragionando sempre in termini di **offset + dimensione + tipo** dichiarati dalla specifica, mai di "questi byte sembrano una struct".

### Le variabili locali non sono inizializzate
In JS una variabile non assegnata vale `undefined`; in C una variabile **locale** contiene i byte già presenti in quella zona di stack — valori arbitrari — e leggerla prima di averci scritto è comportamento indefinito. (Le globali e le `static` sono invece azzerate.) `struct stat st;` senza inizializzazione va bene **se** la prima cosa che accade è che qualcuno ci scriva, come `fstat(fd, &st)`. Per azzerare esplicitamente: `struct stat st = {0};`.

### `memcpy` per leggere campi da un buffer binario
Firma: `void *memcpy(void *restrict dst, const void *restrict src, size_t n)`.

- **`void *`**: lavora sui byte grezzi, quindi accetta qualunque puntatore (la conversione da `uint32_t *` a `void *` è automatica, niente cast).
- **`const`** sulla sorgente: promessa che quella memoria non viene modificata, verificata dal compilatore; chi legge la firma capisce subito chi è sorgente e chi destinazione.
- **`restrict`**: promessa *tua*, non verificata, che le due aree non si sovrappongono, così il compilatore può copiare a blocchi e vettorializzare. Se si sovrappongono è comportamento indefinito: in quel caso si usa `memmove`.
- **`n` è sempre in byte**, mai "numero di elementi" — da cui la forma idiomatica con `sizeof`: `memcpy(&h.magic, bytes + 0, sizeof(h.magic))`, corretta anche se il tipo del campo cambia.

Leggere così invece di castare il puntatore a un tipo evita tre problemi insieme: il padding delle struct, l'accesso non allineato (dopo una stringa a lunghezza variabile i campi cadono a offset qualsiasi) e la violazione dello strict aliasing. Su poche dimensioni fisse il compilatore lo riduce a una singola istruzione: non costa nulla.

---

## Funzioni e gestione degli errori

### Restituire esito e valore: out-parameter
`return` restituisce un valore solo. Quando una funzione deve dare sia un **esito** sia un **risultato**, la convenzione principale in C è: il valore di ritorno è il codice d'esito, il risultato viene scritto in una variabile del chiamante di cui la funzione riceve l'indirizzo (*out-parameter*), es. `int gguf_parse_header(…, gguf_header_t *out)`. Il chiamante possiede la memoria (spesso in stack), quindi niente `malloc` né `free`. L'alternativa — restituire un puntatore allocato dalla funzione, `NULL` se fallisce — ha senso quando la dimensione del risultato non è nota in anticipo, ma obbliga a una funzione di rilascio e dice solo *che* è fallito, non *perché*. La firma documenta la direzione dei dati: input `const`, output no. Buona norma: se la funzione fallisce, non lasciare risorse aperte (un file descriptor aperto a metà) né scrivere nell'out-parameter valori che il chiamante potrebbe scambiare per validi.

### Funzioni di libreria: niente `printf`, niente `exit`
Una funzione di un modulo (non `main`) **segnala** l'errore con il codice di ritorno e lascia decidere al chiamante: non stampa (l'output è una scelta dell'applicazione), non termina il programma (il chiamante potrebbe voler reagire diversamente), non contiene dati di configurazione come un path (li riceve come parametri). Per gli errori di sistema si segue la convenzione POSIX: si documenta che in caso di fallimento `errno` è impostato dalla syscall fallita, e il chiamante usa `perror`/`strerror(errno)` subito. Leggere `errno` *dentro* una funzione la cui firma non lo mostra crea una dipendenza invisibile. Le funzioni si nominano con un verbo (`gguf_parse_header`: fanno qualcosa), i tipi con un sostantivo e `_t` (`gguf_header_t`: sono qualcosa).

### Centralizzare gli errori: enum + `xxx_strerror`
Lo schema standard in C: **un `enum`** con tutti i codici d'errore del modulo in un posto solo, e **una funzione** `const char *modulo_strerror(int code)` che traduce ogni codice in testo (uno `switch` con un `return "…";` per case, più un `default`). La funzione restituisce la stringa, **non la stampa**: stampare è compito del chiamante. Esempi reali con la stessa forma: `strerror` (libc), `gai_strerror` (POSIX), `curl_easy_strerror` (libcurl), `sqlite3_errstr` (SQLite), `zError` (zlib).

Per gli errori di sistema, il dettaglio viene da `errno` e si attacca nel chiamante: `fprintf(stderr, "%s: %s\n", gguf_strerror(rc), strerror(errno));` → `Cannot open file: No such file or directory`, la stessa forma di `perror`. `strerror` (`<string.h>`) è il fratello di `perror` che restituisce la stringa invece di stamparla; va chiamato subito dopo l'errore, e solo per i codici che vengono da una syscall. Quando una libreria cresce, spesso aggiunge un messaggio dettagliato salvato nel proprio oggetto di contesto (es. `sqlite3_errmsg(db)`).

### Restituire una stringa: letterali sì, array locali no
Una funzione tipo `strerror` può restituire un `const char *` che punta a un **letterale** (`return "bad magic";`): i letterali vivono per tutta la durata del programma. Restituire invece un puntatore a un array **locale** della funzione è un bug grave: quella memoria sparisce quando la funzione ritorna e il chiamante legge spazzatura (*dangling pointer*). Se ogni `case` di uno `switch` fa `return`, il fallthrough non può accadere e il `break` è superfluo.

---

## Comportamento del linguaggio

### `switch` e fallthrough
In C i `case` non sono blocchi chiusi ma **etichette**: il `switch` salta a quella giusta e da lì l'esecuzione **continua in avanti** attraverso le etichette successive, fino a un `break` o alla fine del blocco. Dimenticare `break` non è un errore di sintassi: il codice esegue silenziosamente anche i case successivi. Il comportamento esiste per permettere a più `case` di condividere codice (`case 1: case 2: comune; break;`) — utile di rado, fonte di bug spesso (Rust e Swift l'hanno eliminato). Anche `enum` non aiuta: il `switch` accetta qualunque intero, e un `default` mancante non è segnalato.

**Da sapere sui flag**: `-Wimplicit-fallthrough` **non** è incluso in `-Wall -Wextra`, va aggiunto esplicitamente ai flag di compilazione. Verificato su questo progetto: con solo `-Wall -Wextra` un fallthrough in `printError` non produceva alcun warning.

### Undefined Behavior (comportamento indefinito)
In C esistono operazioni che lo standard del linguaggio **non definisce cosa debbano fare**: il compilatore è libero di far succedere qualsiasi cosa (funzionare per caso, andare in crash, produrre risultati sbagliati solo in certe condizioni, ottimizzare in modi imprevedibili). Non è un errore che il compilatore ti segnala sempre — a volte il programma "sembra funzionare" e poi si rompe cambiando compilatore, livello di ottimizzazione, o piattaforma. Per questo va evitato attivamente, non scoperto a runtime.

---

## Tipi e rappresentazione dei dati

### Tipi a dimensione fissa (`uint32_t`, `uint64_t`, ...)
In C il tipo `int` ha una dimensione che dipende dalla piattaforma (di solito 4 byte, ma lo standard non lo garantisce), a differenza del `number` di JS che è sempre un double a 64 bit. I tipi di `<stdint.h>` hanno la dimensione **scritta nel nome**: `uint32_t` è un intero senza segno a 32 bit (4 byte), `uint64_t` a 64 bit (8 byte), `int32_t` è con segno. Qui servono perché i formati binari (GGUF) specificano campi di dimensione esatta: per leggerli servono tipi che hanno la stessa dimensione su ogni macchina. Per i byte grezzi si usa sempre la versione **unsigned** (`uint8_t`, 0-255), non `int8_t` (-128..127).

Da non confondere: `INT8_C(x)` / `UINT64_C(x)` non sono tipi ma **macro per costanti letterali**, che aggiungono al numero il suffisso giusto per la piattaforma. Servono quasi solo per i valori a 64 bit; qui non servono mai.

### Tipi alias (`size_t`, `off_t`)
Alias creati con `typedef` su un tipo intero che varia per piattaforma. `size_t` è il tipo senza segno usato per dimensioni e lunghezze (lo restituisce `sizeof`, lo vuole `mmap` come `len`), e si stampa con `%zu`. `off_t` è il tipo con segno usato per dimensioni e posizioni nei file (`st.st_size`): non avendo uno specificatore dedicato, o si casta (`(long long)` con `%lld`) o lo si converte nel tipo che serve davvero (`(size_t)`, che poi riusi per `mmap`).

### `enum`
Un insieme di **costanti intere con un nome**: `enum gguf_return { GGUF_SUCCESS = 0, GGUF_FILE_OPEN_ERROR, … };`. Senza valori espliciti partono da 0 e crescono di 1. Serve a non avere numeri "magici" nel codice: il chiamante scrive `rc == GGUF_SUCCESS` invece di `rc == 0`. Differenze dagli enum di TS: i valori sono interi qualsiasi, il compilatore non impedisce di passare un numero che non appartiene all'enum (da cui il `default` negli `switch`), e i nomi dei valori sono globali, quindi vanno prefissati.

### Endianness
L'ordine in cui i byte di un numero multi-byte sono scritti in memoria o su file. Il valore `0x12345678` è `12 34 56 78` in **big-endian** (byte più significativo per primo) e `78 56 34 12` in **little-endian** (byte meno significativo per primo). L'M1 è little-endian e il GGUF è little-endian di default (la spec v3 prevede anche file big-endian, che noi non gestiamo), quindi i byte del file si possono leggere senza conversione. Va tenuta presente quando si legge un dump esadecimale: `0300 0000` vale 3, non `0x03000000`. L'inversione è **a livello di byte** (coppie di cifre hex), non di singola cifra: `xxd` raggruppa i byte a coppie, quindi `0300` sono i due byte `03 00`, non "30". Il valore è `03·256⁰ + 00·256¹ + ...`.

---

## Sistema operativo e I/O

### Pagine man e sezioni
La documentazione di sistema è divisa in sezioni numerate e lo stesso nome può stare in più sezioni: **1** = comandi da terminale, **2** = system call, **3** = funzioni di libreria C. Quindi `man stat` apre il *comando* `stat`, mentre la system call è `man 2 stat`; allo stesso modo `man 2 open`, `man 2 mmap`, ma `man 3 printf`, `man 3 memcpy`. `man -aw <nome>` elenca tutte le pagine esistenti per quel nome. Una pagina copre tutta la famiglia: `fstat` è documentata dentro `man 2 stat`.

### File descriptor (`open` / `close`)
Un piccolo numero intero che il sistema operativo ti dà per riferirti a un file aperto. `open(percorso, flag)` lo restituisce, **`-1` in caso di errore**, e alla fine si chiude con `close(fd)`. È lo stesso concetto di `fs.openSync` in Node, che restituisce un numero. Qui serve per passare il file a `mmap`. I flag (`O_RDONLY`, …) sono insiemi di bit combinabili con `|`. I descriptor 0, 1 e 2 sono già aperti all'avvio di ogni processo: **stdin, stdout, stderr**. Per questo 0 non è un valore "vuoto" — `close(0)` chiuderebbe lo standard input. La convenzione per "nessun file aperto" è `-1`, lo stesso valore che `open` restituisce in caso di errore. Chiudere il descriptor subito dopo `mmap` è lecito: la mappatura resta valida.

### `fstat` e `struct stat`
`fstat(fd, &st)` riempie una `struct stat` con i metadati del file (definita in `<sys/stat.h>`, documentata in `man 2 stat`); restituisce `0` se ok, `-1` se errore. Il campo che serve qui è `st.st_size`, la dimensione in byte, di tipo `off_t`. Si passa `&st` perché la funzione deve scrivere nella tua variabile.

### mmap
Invece di leggere un file con `fread` in un buffer allocato (`malloc` + copia dei byte), `mmap` mappa il file direttamente nello spazio di indirizzi del programma: il sistema operativo fa finta che il file sia già in memoria, e carica le pagine dal disco solo quando effettivamente le usi (lazy loading). Niente copia esplicita, niente attesa di caricare tutto il file prima di iniziare. Usato qui per caricare i pesi GGUF (2+ GB) senza doverli duplicare in RAM.

`void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)`:

| Parametro | Significato | Cosa passiamo |
|---|---|---|
| `addr` | dove collocare la mappatura (solo un suggerimento) | `NULL` = sceglie il kernel |
| `len` | quanti byte mappare | `file_size` |
| `prot` | cosa è permesso fare con quelle pagine | `PROT_READ`: scrivere darebbe `SIGSEGV`, i pesi non si corrompono per sbaglio |
| `flags` | mappatura privata o condivisa | `MAP_PRIVATE`: eventuali scritture non toccherebbero il file |
| `fd` | quale file mappare | il descriptor di `open` |
| `offset` | da che punto del file partire (multiplo della pagina) | `0` |

Alla fine si rilascia con `munmap(base, len)`. `prot` e `flags` sono insiemi di bit, combinabili con `|`.

### Memoria virtuale e page fault
Gli indirizzi che il programma usa sono **virtuali**, privati del processo; la MMU li traduce in indirizzi fisici tramite le page table del kernel. `mmap` non legge niente dal disco: annota soltanto che un intervallo di indirizzi virtuali corrisponde a quel file, e costa uguale per 1 KB o per 2,4 GB. Quando tocchi un indirizzo la cui pagina non è ancora in RAM scatta un **page fault**: il kernel legge quella pagina dal disco, aggiorna le page table e fa riprendere l'istruzione — trasparente per il codice. Sull'M1 la pagina è di **16 KiB** (`getconf PAGESIZE` → 16384), non 4 KiB come su x86: leggere i primi 4 byte del modello materializza una pagina sola. Le pagine file-backed di sola lettura sono "pulite": sotto pressione di memoria il kernel può scartarle e rileggerle dal file senza usare lo swap — il motivo per cui un modello più grande della RAM può comunque girare.

### MAP_FAILED
Il valore che `mmap` restituisce in caso di errore — è `(void*)-1`, **non** `NULL` come ti aspetteresti per convenzione da altre API C. Va controllato esplicitamente con `== MAP_FAILED`, non con `== NULL`, e prima di castare il risultato a `uint8_t *`.

**Perché `(void*)-1` e non `NULL`**: `void*` è un puntatore "generico" (un indirizzo senza specificare il tipo del dato puntato). Il numero `-1`, su un sistema con rappresentazione in complemento a due (praticamente tutti, M1 incluso), ha tutti i bit a 1 (es. `0xFFFFFFFFFFFFFFFF` su 64 bit). Il cast `(void*)-1` prende quel pattern di bit e lo reinterpreta come indirizzo — un indirizzo "estremo" che non corrisponde mai a un mapping riuscito. È una convenzione storica di `mmap`: hanno evitato `NULL` perché in certi contesti l'indirizzo 0 può essere un mapping valido. La macro `MAP_FAILED` si espande esattamente a questo valore, così nel codice scrivi `if (result == MAP_FAILED)` senza dover ricordare il trucco del cast.

### stdout / stderr, `fprintf`, `perror` ed `errno`
`printf` scrive sempre su **stdout**; `fprintf` prende come primo parametro il flusso di destinazione (`printf(x)` ≡ `fprintf(stdout, x)`), `snprintf` scrive dentro un buffer. `stdout` e `stderr` sono flussi `FILE *` predefiniti da `<stdio.h>`. Convenzione: **output del programma su stdout, messaggi diagnostici su stderr**, così si possono redirigere separatamente (`./engine > out.txt` lascia gli errori a video); inoltre stdout verso file/pipe è bufferizzato mentre stderr no, quindi in caso di crash i messaggi su stdout possono restare nel buffer.

Quando una system call fallisce il motivo finisce nella variabile globale `errno` (`<errno.h>`). `perror("open")` stampa su stderr il prefisso più il messaggio corrispondente: `open: No such file or directory`. Due regole: chiamarlo **subito** dopo il controllo di errore (quasi ogni chiamata di libreria può sovrascrivere `errno`), e guardarlo **solo** se la chiamata ha segnalato errore (in caso di successo contiene residui). Alternativa per messaggi composti: `strerror(errno)`, che restituisce la stringa invece di stamparla.

### printf e specificatori di formato
`printf("testo %d\n", x)` stampa testo e sostituisce ogni `%…` con un valore. A differenza di `console.log`, **il tipo giusto va scelto a mano**: `printf` non sa cosa gli stai passando, si fida dello specificatore. Sbagliarlo è comportamento indefinito — può stampare spazzatura senza errore (`-Wall` avvisa nei casi evidenti).

| Specificatore | Tipo dell'argomento |
|---|---|
| `%d` / `%i` | `int` |
| `%u` | `unsigned int` |
| `%ld` / `%lu` | `long` / `unsigned long` |
| `%lld` / `%llu` | `long long` / `unsigned long long` |
| `%zu` | `size_t` (risultato di `sizeof`, dimensioni, lunghezze) |
| `%f` | `double` (un `float` viene promosso a `double`) |
| `%c` | un carattere singolo |
| `%s` | stringa **terminata da NUL** — non usabile sulle stringhe GGUF, che hanno lunghezza esplicita |
| `%p` | un puntatore (indirizzo in esadecimale) |
| `%x` / `%X` | intero senza segno in esadecimale |
| `%%` | un `%` letterale |

**Flag tra `%` e la lettera**: `%08x` stampa almeno 8 cifre hex riempiendo di zeri a sinistra (utile per il magic GGUF: `0x46554747`); `%-10s` allinea a sinistra in 10 colonne.

**Stampare `uint32_t` / `uint64_t` in modo portabile**: i tipi di `<stdint.h>` non hanno uno specificatore fisso, perché su piattaforme diverse `uint64_t` può essere `unsigned long` o `unsigned long long`. `<inttypes.h>` definisce macro che si espandono in quello giusto: `printf("%" PRIu64 "\n", n)` (le stringhe letterali adiacenti vengono concatenate dal compilatore). Su macOS `%llu` funziona comunque, ma `PRIu64` è la forma corretta.

---

*(si aggiorna man mano che emergono nuovi concetti)*
