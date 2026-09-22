# Glossario LLM/inference — infer-engine

Dettagli implementativi LLM/inference emersi scrivendo il codice: cose che la teoria di alto livello non specifica a questo livello. Esempi: convenzioni del formato GGUF, layout in memoria dei tensori multi-dimensionali, naming dei tensori nei modelli Llama, specifiche byte-level dei tipi di quantizzazione.

Per la teoria di base già coperta, vedi @docs/theory/theory-summary.md — questo file cattura solo i dettagli implementativi che emergono scrivendo codice.

Ogni voce: cos'è, perché conta qui, collegamento alla teoria nota se rilevante. Compatto: consolida invece di accumulare (~30-40 righe totali).

## Regole per l'agente

- **Dettaglio implementativo nuovo** (non teoria): spiegalo prima o insieme al codice e aggiungilo qui, sempre, senza chiedere.
- **Teoria di base già studiata** (tokenization, attention/GQA/MQA, RoPE, LayerNorm/RMSNorm, MLP/FFN, KV cache, quantizzazione, MoE, speculative decoding, Flash/Paged Attention, prefill vs decode, roofline): "studiata" non vuol dire "fresca". Prima di usarla, fai un sanity-check di una riga ("ricordi ancora come funziona X, o vuoi un ripasso veloce?") e lascia decidere a Roberto.
- **Se serve il ripasso**: consulta prima `theory-summary.md`, la spiegazione buona è probabilmente già lì. Se il concetto era dimenticato o confuso, annotalo anche qui: nella pratica non era acquisito.
- **Dopo una verifica** (quiz) superata, aggiorna il `Learning state` in `theory-summary.md`.

---

## GGUF file format

### Metadata key-value pair
Record a dimensione variabile: lunghezza della chiave (`uint64_t`), byte della chiave, tipo del valore (`uint32_t`), poi il valore. Le stringhe sono **con lunghezza esplicita** (`uint64_t` + byte, senza terminatore NUL), quindi non si possono passare a `strlen` o `printf("%s")`. La forma del valore dipende dal tipo: numero = dimensione fissa e nessuna lunghezza (es. `uint32_t` = 4 byte), stringa (codice 8) = lunghezza + byte, array = tipo degli elementi + conteggio + elementi. Poiché le dimensioni variano (stringhe corte, numeri, array interi come il vocabolario del tokenizer), la N-esima coppia non si raggiunge direttamente: il parser le percorre in ordine, leggendo `metadata_kv_count` coppie dopo l'header da 24 byte. Chiavi: ASCII, gerarchiche `lower_snake_case.con.punti`, max 65535 byte. Codici tipo: 0-1 u8/i8, 2-3 u16/i16, 4-5 u32/i32, 6 f32, 7 bool (1 byte), 8 stringa, 9 array, 10-11 u64/i64, 12 f64. Lo pseudo-codice della spec non è C valido (array di lunghezza variabile nelle struct): si legge campo per campo. Spec: [gguf.md](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md).

---

*(si aggiorna man mano che emergono nuovi dettagli implementativi)*
