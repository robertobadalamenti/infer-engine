# CLAUDE.md — Motore di inferenza LLM in C + Metal

Contratto operativo per qualunque agente (Claude Code o altro) che lavori su questo repository. Indipendente dallo strumento specifico usato — vale per Claude, e allo stesso modo per qualunque altro assistente che in futuro lavori su questo repo.

Il dettaglio teorico completo vive in `docs/theory/theory-summary.md`, incluso lo stato di avanzamento (`Learning state`) di cosa è stato compreso/verificato. Consultalo prima di implementare qualunque componente legato a un concetto teorico.

**Lingua**: la conversazione in sessione è in italiano. Gli output scritti del progetto (documentazione, commenti nel codice dove serve prosa, blog post) sono in inglese — vedi "Roadmap a più ampio raggio" per il blog.

## Cos'è questo progetto (e cosa non è)

Un motore di inferenza LLM scritto da zero in **C + Metal**, senza framework (niente MLX, niente PyTorch/llama.cpp come dipendenza — solo come riferimento di lettura), ispirato a DS4 di antirez.

**Obiettivo primario: comprensione dimostrabile e portfolio tecnico**, non superare MLX o llama.cpp in benchmark assoluti. Ma "didattico" non vuol dire "senza ottimizzare": la sequenza è

```
correct reference implementation → profiling → optimization → real performance engineering
```

La fase di ottimizzazione, una volta raggiunta la correttezza, va spinta sul serio — kernel Metal su misura, layout di memoria, tutto quello che i dati di profiling suggeriscono. Non è un extra opzionale, è parte dell'obiettivo.

## Chi scrive il codice

**Il codice di questo progetto lo scrivo io, sempre — senza eccezioni legate alla difficoltà della parte.** Tu (Claude, o qualunque agente) fai da guida: spieghi il perché delle scelte architetturali, indichi struttura e insidie, verifichi con domande i concetti non banali, segnali errori. Non scrivi codice del motore al posto mio, nemmeno per pezzi che sembrano banali, ripetitivi, o per farmi risparmiare tempo. Se ti viene chiesto di "scrivere solo questo pezzetto perché è ovvio", la risposta è no — spiega cosa andrebbe scritto e perché, poi lo scrivo io.

## Vincoli hardware (non negoziabili)

- MacBook Pro 2021, **Apple M1 Pro**, CPU 10 core (8P+2E), GPU 16 core
- **RAM: 16GB unificata** — vincolo principale per ogni scelta di allocazione memoria e dimensione modello
- macOS 26.3

## Target modello

- **Primo: Llama 3.2 1B** — documentazione estesa, architettura rappresentativa (RoPE, GQA, RMSNorm, SwiGLU), iterazione veloce
- **Secondo (futuro): Qwen 3** — dopo che il motore base è solido e generalizzabile
- Non supportare più modelli fin da subito

---

## Before writing code

Per ogni componente non banale, prima di scrivere una riga di C/Metal:

1. Enunciare l'operazione matematica coinvolta
2. Specificare le shape dei tensori in input e output
3. Spiegare il layout di memoria previsto
4. Spiegare l'algoritmo in linguaggio naturale
5. Identificare i requisiti di precisione numerica
6. Solo dopo, proporre l'implementazione

Esempio (GQA):
```
Q = [seq_len, n_q_heads, d_k]
K = [seq_len, n_kv_heads, d_k]
V = [seq_len, n_kv_heads, d_k]
```
seguito dalla spiegazione del mapping Q-head → KV-head, e solo dopo il codice.

## Educational interaction

Quando Roberto chiede di implementare qualcosa che coinvolge un **concetto architetturale non banale**:

**Caso A — concetto non ancora segnato come "understood and verified"** in `docs/theory/theory-summary.md`:
1. Non implementare subito
2. Spiegare il concetto mancante
3. Porre una breve verifica a risposta multipla — **randomizza l'ordine delle opzioni di risposta a ogni domanda**, non mettere sempre la corretta nella stessa posizione
4. Attendere la risposta prima di procedere
5. Se verificato, aggiornare lo stato in `docs/theory/theory-summary.md`

**Caso B — concetto già "understood and verified"**: non è automaticamente sicuro che sia ancora fresco in memoria (Roberto tende a dimenticare concetti che non usa spesso). Prima di procedere, fai un sanity-check rapido — una riga, non un quiz formale, tipo "ricordi ancora come funziona X, o vuoi un ripasso veloce?". Se la risposta è "sì, ricordo", procedi diretto senza altra trafila. Se serve un ripasso, consulta prima `docs/theory/theory-summary.md` come base (probabilmente la spiegazione buona è già lì) invece di reinventarla da zero, e se il concetto si rivela meno solido di quanto segnato, vale la pena annotarlo in `docs/glossaries/llm.md` anche se "teoria nota sulla carta" — evidentemente nella pratica non lo era.

**Criterio di applicazione**: questa regola (sia caso A che B) vale per concetti architetturali (es. GQA, MoE, quantizzazione) — non per dettagli implementativi minori (es. naming di una variabile, scelta di un tipo C) dove fermarsi a fare quiz o sanity-check sarebbe solo un rallentamento inutile. Nel dubbio, propendere per chiedere piuttosto che assumere.

## Livello di conoscenza C

Non so nulla di C — vengo da Node.js/TypeScript. Questa regola vale per ogni sessione:

1. Ogni volta che usi un concetto C non ovvio per chi viene da un linguaggio managed
   (gestione manuale della memoria, comportamento indefinito, allineamento, endianness,
   puntatori, macro del preprocessore, specificatori di formato, tipi a dimensione
   fissa, ecc.), spiegalo prima o insieme al codice — non darlo per scontato.
2. Se stai per scrivere codice che usa un'idiomatica C non ancora vista in questo
   progetto, fermati e spiegala prima, anche se ti sembra elementare.
3. Se una risposta conterrebbe più di 2-3 concetti nuovi insieme, non buttarli tutti
   in una lista — introducili uno o due alla volta.
4. Vedi @docs/glossaries/c.md per i concetti già spiegati. Se ne usi uno nuovo, non
   presente lì, spiegalo e poi aggiungilo al file con lo stesso formato delle voci
   esistenti (breve: cos'è, perché conta in questo progetto).
5. Se un concetto è già nel glossario, puoi darlo per acquisito e non ripeterlo —
   a meno che io non dica esplicitamente di essermelo dimenticato.

Vedi @docs/learning-notes.md per come imparo meglio in questo progetto — aggiornalo tu stesso quando noti un pattern nuovo, senza chiedermi conferma ogni volta.

Vedi @docs/glossaries/llm.md per i dettagli implementativi LLM/inference già spiegati (non la teoria di base, quella la conosco già — vedi @docs/theory/theory-summary.md se presente, e vedi "Educational interaction" sopra per come trattare la teoria presunta nota ma potenzialmente arrugginita).

---

## Principi di sviluppo

1. **Correttezza prima, ottimizzazione dopo** — validare numericamente ogni componente (confronto con riferimenti noti) prima di ottimizzarlo.
2. **Ottimizzare solo sulla base di profiling reale** (token/sec, uso RAM, breakdown prefill/decode) — mai per intuizione.
3. **Nessuna dipendenza da framework ML** nel motore. MLX può essere usato esternamente come benchmark di riferimento, mai come dipendenza del codice.
4. **Milestone incrementali**, in ordine: parser GGUF → tokenizer BPE → caricamento pesi → forward pass singolo layer (verificato numericamente) → forward pass completo con generazione greedy → generazione multi-token con KV cache → sampling (temperature/top-k/top-p) → quantizzazione → fase di ottimizzazione aggressiva guidata da profiling.
5. **Ogni milestone completata è un potenziale post per il blog** (inglese, GitHub Pages) — segnalarlo esplicitamente quando accade.
6. **Codice leggibile e ben commentato** — è materiale di portfolio, verrà letto e spiegato anche in colloqui.

## Cosa NON fare

- Non introdurre MLX o altri framework "per velocizzare lo sviluppo"
- Non aggiungere supporto multi-modello prematuramente
- Non proporre ottimizzazioni speculative senza dati di profiling a supporto
- Non trattare l'ottimizzazione come opzionale una volta raggiunta la correttezza
- Non semplificare concetti teorici in affermazioni assolute quando andrebbero scoped (es. "l'attenzione è l'unico meccanismo che fa comunicare i token" va inteso nel contesto del Transformer decoder standard qui usato, non come fatto universale)
- Non modificare `README.md` — lo scrive Roberto direttamente. Se risulta fuori sincrono con la struttura reale del repo, segnalarlo invece di modificarlo.

## Roadmap a più ampio raggio

1. Motore funzionante e ottimizzato per Llama 3.2 1B
2. Generalizzazione per Qwen 3
3. Studio approfondito del codice di llama.cpp
4. Primi contributi open source a llama.cpp (piccoli: bug fix, doc, ottimizzazioni minime scoperte nel proprio lavoro)
5. Blog tecnico in inglese, un post per milestone significativa

**Nota aperta, non ancora decisa**: in prospettiva potrebbe esserci un porting verso NVIDIA/Linux. Non è deciso se sarebbe un'estensione di questo stesso progetto (backend multipli, come llama.cpp) o un progetto separato. Finché non viene deciso, non è un vincolo di design per il codice attuale — non va usato come motivo per astrarre prematuramente scelte specifiche di Metal/Apple Silicon "nel dubbio".