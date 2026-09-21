# Glossario C — infer-engine

Concetti C spiegati man mano, per chi viene da Node.js/TypeScript e parte da zero. Ogni voce: cos'è, perché conta qui.

---

## Memoria e puntatori

### Puntatore
Una variabile che non contiene un valore, ma **l'indirizzo in memoria** dove si trova un valore. In JS/TS non esistono esplicitamente — ogni oggetto è già "un puntatore" gestito per te dal runtime. In C li scrivi e li maneggi tu: `int* p` è "un puntatore a un intero", cioè una variabile che contiene l'indirizzo dove sta un intero.

### malloc / free
`malloc` chiede al sistema operativo un blocco di memoria e ti restituisce un puntatore al suo inizio. `free` lo restituisce. In JS/TS questo lo fa il garbage collector automaticamente; in C lo fai a mano, e se ti dimentichi `free` hai una memory leak, se lo chiami due volte o usi memoria già liberata hai un bug serio (spesso silenzioso).

### mmap
Invece di leggere un file con `fread` in un buffer allocato (`malloc` + copia dei byte), `mmap` mappa il file direttamente nello spazio di indirizzi del programma: il sistema operativo fa finta che il file sia già in memoria, e carica le pagine dal disco solo quando effettivamente le usi (lazy loading). Niente copia esplicita, niente attesa di caricare tutto il file prima di iniziare. Usato qui per caricare i pesi GGUF (2+ GB) senza doverli duplicare in RAM.

### MAP_FAILED
Il valore che `mmap` restituisce in caso di errore — è `(void*)-1`, **non** `NULL` come ti aspetteresti per convenzione da altre API C. Va controllato esplicitamente con `== MAP_FAILED`, non con `== NULL`.

**Perché `(void*)-1` e non `NULL`**: `void*` è un puntatore "generico" (un indirizzo senza specificare il tipo del dato puntato). Il numero `-1`, su un sistema con rappresentazione in complemento a due (praticamente tutti, M1 incluso), ha tutti i bit a 1 (es. `0xFFFFFFFFFFFFFFFF` su 64 bit). Il cast `(void*)-1` prende quel pattern di bit e lo reinterpreta come indirizzo — un indirizzo "estremo" che non corrisponde mai a un mapping riuscito. È una convenzione storica di `mmap`: hanno evitato `NULL` perché in certi contesti l'indirizzo 0 può essere un mapping valido. La macro `MAP_FAILED` si espande esattamente a questo valore, così nel codice scrivi `if (result == MAP_FAILED)` senza dover ricordare il trucco del cast.

---

## Comportamento del linguaggio

### Undefined Behavior (comportamento indefinito)
In C esistono operazioni che lo standard del linguaggio **non definisce cosa debbano fare**: il compilatore è libero di far succedere qualsiasi cosa (funzionare per caso, andare in crash, produrre risultati sbagliati solo in certe condizioni, ottimizzare in modi imprevedibili). Non è un errore che il compilatore ti segnala sempre — a volte il programma "sembra funzionare" e poi si rompe cambiando compilatore, livello di ottimizzazione, o piattaforma. Per questo va evitato attivamente, non scoperto a runtime.


---

## Tipi e rappresentazione dei dati

### Tipi a dimensione fissa (`uint32_t`, `uint64_t`, ...)
In C il tipo `int` ha una dimensione che dipende dalla piattaforma (di solito 4 byte, ma lo standard non lo garantisce), a differenza del `number` di JS che è sempre un double a 64 bit. I tipi di `<stdint.h>` hanno la dimensione **scritta nel nome**: `uint32_t` è un intero senza segno a 32 bit (4 byte), `uint64_t` a 64 bit (8 byte), `int32_t` è con segno. Qui servono perché i formati binari (GGUF) specificano campi di dimensione esatta: per leggerli servono tipi che hanno la stessa dimensione su ogni macchina.

### Endianness
L'ordine in cui i byte di un numero multi-byte sono scritti in memoria o su file. Il valore `0x12345678` è `12 34 56 78` in **big-endian** (byte più significativo per primo) e `78 56 34 12` in **little-endian** (byte meno significativo per primo). L'M1 è little-endian e il GGUF è little-endian di default (la spec v3 prevede anche file big-endian, che noi non gestiamo), quindi i byte del file si possono leggere senza conversione. Va tenuta presente quando si legge un dump esadecimale: `0300 0000` vale 3, non `0x03000000`. L'inversione è **a livello di byte** (coppie di cifre hex), non di singola cifra: `xxd` raggruppa i byte a coppie, quindi `0300` sono i due byte `03 00`, non "30". Il valore è `03·256⁰ + 00·256¹ + ...`.

---

*(si aggiorna man mano che emergono nuovi concetti)*
