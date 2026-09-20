# CLAUDE.md — Motore di inferenza LLM in C + Metal

Contratto operativo per qualunque agente (Claude Code o altro) che lavori su questo repository. Indipendente dallo strumento specifico usato — vale per Claude, e allo stesso modo per qualunque altro assistente che in futuro lavori su questo repo.

Il dettaglio teorico completo vive in `docs/theory/theory-summary.md`, incluso lo stato di avanzamento (`Learning state`) di cosa è stato compreso/verificato. Consultalo prima di implementare qualunque componente legato a un concetto teorico.

## Cos'è questo progetto (e cosa non è)

Un motore di inferenza LLM scritto da zero in **C + Metal**, senza framework (niente MLX, niente PyTorch/llama.cpp come dipendenza — solo come riferimento di lettura), ispirato a DS4 di antirez.

**Obiettivo primario: comprensione dimostrabile e portfolio tecnico**, non superare MLX o llama.cpp in benchmark assoluti. Ma "didattico" non vuol dire "senza ottimizzare": la sequenza è

```
correct reference implementation → profiling → optimization → real performance engineering
```

La fase di ottimizzazione, una volta raggiunta la correttezza, va spinta sul serio — kernel Metal su misura, layout di memoria, tutto quello che i dati di profiling suggeriscono. Non è un extra opzionale, è parte dell'obiettivo.

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

Quando Roberto chiede di implementare qualcosa che coinvolge un **concetto architetturale non banale** non ancora segnato come "understood and verified" in `docs/theory/theory-summary.md`:

1. Non implementare subito
2. Spiegare il concetto mancante
3. Porre una breve verifica a risposta multipla
4. Attendere la risposta prima di procedere

Se il concetto è già "understood and verified", procedere direttamente senza rifare la trafila.

**Criterio di applicazione**: questa regola vale per concetti architetturali nuovi (es. GQA, MoE, quantizzazione) — non per dettagli implementativi minori (es. naming di una variabile, scelta di un tipo C) dove fermarsi a fare quiz sarebbe solo un rallentamento inutile. Nel dubbio, propendere per chiedere piuttosto che assumere.

---

## Principi di sviluppo

1. **Correttezza prima, ottimizzazione dopo** — validare numericamente ogni componente (confronto con riferimenti noti) prima di ottimizzarlo.
2. **Ottimizzare solo sulla base di profiling reale** (token/sec, uso RAM, breakdown prefill/decode) — mai per intuizione.
3. **Nessuna dipendenza da framework ML** nel motore. MLX può essere usato esternamente come benchmark di riferimento, mai come dipendenza del codice.
4. **Milestone incrementali**, in ordine: tokenizer BPE → caricamento pesi → forward pass singolo layer (verificato numericamente) → forward pass completo con generazione greedy → generazione multi-token con KV cache → sampling (temperature/top-k/top-p) → quantizzazione → fase di ottimizzazione aggressiva guidata da profiling.
5. **Ogni milestone completata è un potenziale post per il blog** (inglese, GitHub Pages) — segnalarlo esplicitamente quando accade.
6. **Codice leggibile e ben commentato** — è materiale di portfolio, verrà letto e spiegato anche in colloqui.

## Cosa NON fare

- Non introdurre MLX o altri framework "per velocizzare lo sviluppo"
- Non aggiungere supporto multi-modello prematuramente
- Non proporre ottimizzazioni speculative senza dati di profiling a supporto
- Non trattare l'ottimizzazione come opzionale una volta raggiunta la correttezza
- Non semplificare concetti teorici in affermazioni assolute quando andrebbero scoped (es. "l'attenzione è l'unico meccanismo che fa comunicare i token" va inteso nel contesto del Transformer decoder standard qui usato, non come fatto universale)

## Roadmap a più ampio raggio

1. Motore funzionante e ottimizzato per Llama 3.2 1B
2. Generalizzazione per Qwen 3
3. Studio approfondito del codice di llama.cpp
4. Primi contributi open source a llama.cpp (piccoli: bug fix, doc, ottimizzazioni minime scoperte nel proprio lavoro)
5. Blog tecnico in inglese, un post per milestone significativa
