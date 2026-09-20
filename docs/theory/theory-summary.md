# Theory summary — Inferenza LLM

Riferimento teorico dettagliato per il progetto. Consultato da `CLAUDE.md` (contratto operativo) quando serve il dettaglio di un concetto. Aggiornato via via che la teoria avanza.

## Tokenizzazione
- **BPE (Byte Pair Encoding)**: due motivazioni chiave — controllo della dimensione del vocabolario e gestione di parole sconosciute (out-of-vocabulary).

## Embeddings
- Distinzione tra **d_model** (dimensione dello spazio di embedding del token) e **d_k** (dimensione per-head usata nell'attenzione).

## Positional Encoding — RoPE
- **RoPE (Rotary Positional Embedding)** applicato solo a Query (Q) e Key (K), non a Value (V).
- Codifica la posizione **relativa** tra token tramite rotazione nello spazio vettoriale, non posizione assoluta.

## Attenzione
- Formula completa:
  ```
  Attention(Q, K, V) = softmax( (Q · Kᵀ) / √d_k + mask ) · V
  ```
- Lo scaling per **√d_k** previene la saturazione del softmax.
- **Causal masking**: si sommano **-∞** alle posizioni future prima del softmax, così dopo l'esponenziale il loro peso è zero.
- Nel Transformer decoder standard considerato in questo progetto, l'attenzione è il meccanismo che permette la comunicazione diretta tra rappresentazioni di token differenti; la MLP opera invece indipendentemente su ciascun token (vedi sotto).

## Normalizzazione
- **LayerNorm**: `γ · (x − μ) / √(σ² + ε) + β`
- **RMSNorm** (usata da Llama e dai modelli moderni): normalizza solo per la root mean square, senza sottrarre la media — più economica computazionalmente.

## Blocco Transformer
- Due connessioni residuali: una dopo l'attenzione, una dopo la MLP.
- Le connessioni residuali fanno parte dell'architettura del modello e vengono utilizzate anche durante l'inferenza; consentono al segnale di attraversare efficacemente molti layer e contribuiscono alla stabilità dell'architettura.

## MLP
- Elabora ogni token indipendentemente (nessuna comunicazione cross-token).

## Pipeline di output
- Sequenza: **LayerNorm finale → W_unembed → softmax → strategia di decoding** (greedy, sampling, top-k, top-p).

## KV Cache
- Si salvano K e V calcolati ai passi precedenti per non ricalcolarli ad ogni nuovo token.
- Costo di memoria (forma generale, valida anche con GQA):
  ```
  M_KV = 2 × N_layers × N_KV_heads × d_k × seq_len × bytes_per_elemento
  ```
  - Il fattore 2 è per K e V separatamente.
  - **N_KV_heads**, non N_heads totali: con GQA il numero di head KV è minore del numero di head Query — è esattamente il meccanismo con cui GQA riduce la memoria della cache. Usare N_heads qui sarebbe in contraddizione con la definizione di GQA stessa.

## Analisi memory-bound
- L'inferenza LLM è **memory-bound**, non compute-bound: il collo di bottiglia è la banda di memoria per spostare pesi e KV cache, non i FLOP.
- Per questo quantizzazione e gestione della memoria contano più della pura ottimizzazione del calcolo.

---

## Learning state

### Understood and verified
- Tokenization / BPE
- Embeddings (d_model vs d_k)
- RoPE
- Attention (scaling, causal masking)
- RMSNorm vs LayerNorm
- Residual connections
- MLP
- Output projection pipeline
- KV cache (meccanica e formula di memoria)
- Memory-bound analysis / Roofline

### Currently studying
- Quantization (incluso schema asimmetrico 2/8-bit stile DS4)

### Must study before implementation
- GQA (Grouped Query Attention) — necessaria per Llama 3.2, usata nella formula KV cache sopra
- MoE (Mixture of Experts)

*Aggiornare questa sezione ogni volta che un argomento viene verificato — non lasciare che diventi disallineata con lo stato reale.*
