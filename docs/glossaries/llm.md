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

*(si aggiorna man mano che emergono nuovi dettagli implementativi)*
