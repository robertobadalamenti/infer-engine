# Glossario LLM/inference — infer-engine

Dettagli implementativi LLM/inference emersi scrivendo il codice: cose che la teoria di alto livello non specifica a questo livello. Esempi: convenzioni del formato GGUF, layout in memoria dei tensori multi-dimensionali, naming dei tensori nei modelli Llama, specifiche byte-level dei tipi di quantizzazione.

Per la teoria di base, vedi la [sintesi teorica](../theory/theory-summary.md). Questo file documenta i dettagli implementativi; per i concetti del linguaggio, vedi il [glossario C](c.md).

Ogni voce descrive il concetto, perché conta nel progetto e il collegamento alla teoria quando rilevante.

---

## Modello di riferimento

`models/Llama-3.2-1B-Instruct-f16.gguf` viene da **bartowski/Llama-3.2-1B-Instruct-GGUF** su Hugging Face (SHA256 `1f33ad43…ae146`, verificato contro la pagina del file). La pagina del file su HF mostra una tabella con tutti i metadata letti dal parser GGUF di Hugging Face: è un riferimento indipendente per verificare l'output del nostro parser. Pesi in f16, convertiti dagli originali Meta in bf16 (`meta-llama/Llama-3.2-1B-Instruct`, safetensors, accesso con licenza): gli originali serviranno alla milestone di validazione numerica del forward pass.

## GGUF file format

### Header (24 byte)
Quattro campi a dimensione fissa, senza padding nel file:

| offset | campo | tipo | byte | valore |
|---|---|---|---|---|
| 0 | `magic` | `uint32_t` | `47 47 55 46` | `0x46554747` (ASCII `GGUF` in little-endian) |
| 4 | `version` | `uint32_t` | `03 00 00 00` | 3 |
| 8 | `tensor_count` | `uint64_t` | `93 00 …` | 147 |
| 16 | `metadata_kv_count` | `uint64_t` | `1f 00 …` | 31 |

**Versione**: il parser accetta solo `version == 3` (la corrente della spec, e quella del nostro file). Nella v1 i contatori e le lunghezze delle stringhe erano a 32 bit invece che a 64: un file v1 letto con il layout v3 darebbe valori senza senso. Il controllo va fatto prima di fidarsi dei campi successivi, insieme a quello sulla dimensione minima (almeno 24 byte) e sul magic.

I byte e i valori nelle ultime due colonne sono quelli reali di `Llama-3.2-1B-Instruct-f16.gguf` (`xxd -l 24`), utili come riferimento per verificare il parser. In quest'ordine specifico una struct C non avrebbe padding, ma i campi si leggono comunque uno alla volta con `memcpy` (vedi il glossario C): subito dopo l'header iniziano i metadata a dimensione variabile, dove gli offset non sono più allineati.

### Metadata key-value pair
Record a dimensione variabile: lunghezza della chiave (`uint64_t`), byte della chiave, tipo del valore (`uint32_t`), poi il valore. Le stringhe sono **con lunghezza esplicita** (`uint64_t` + byte, senza terminatore NUL), quindi non si possono passare a `strlen` o `printf("%s")`. La forma del valore dipende dal tipo: numero = dimensione fissa e nessuna lunghezza (es. `uint32_t` = 4 byte), stringa (codice 8) = lunghezza + byte, array = tipo degli elementi + conteggio + elementi. Poiché le dimensioni variano (stringhe corte, numeri, array interi come il vocabolario del tokenizer), la N-esima coppia non si raggiunge direttamente: il parser le percorre in ordine, leggendo `metadata_kv_count` coppie dopo l'header da 24 byte. Chiavi: ASCII, gerarchiche `lower_snake_case.con.punti`, max 65535 byte. Codici tipo: 0-1 u8/i8, 2-3 u16/i16, 4-5 u32/i32, 6 f32, 7 bool (1 byte), 8 stringa, 9 array, 10-11 u64/i64, 12 f64. Lo pseudo-codice della spec non è C valido (array di lunghezza variabile nelle struct): si legge campo per campo. Spec: [gguf.md](https://github.com/ggml-org/ggml/blob/master/docs/gguf.md).

### Array di stringhe nei metadata
Un array ha un tipo degli elementi (`uint32_t`, 4 byte), un conteggio (`uint64_t`, 8 byte), poi gli elementi in sequenza. Il tipo compare una sola volta per l'array. Se gli elementi sono stringhe, ciascuno contiene la propria lunghezza (`uint64_t`, 8 byte) seguita dal testo, senza NUL. Esempio `["ciao", "a"]`: 12 byte di intestazione dell'array + (8 + 4) + (8 + 1) = 33 byte. Non si può moltiplicare il conteggio per la lunghezza del primo elemento: bisogna attraversarli tutti. Se la funzione usata per un elemento aggiorna già il cursore, il ramo array la chiama una volta per elemento e non ripete l'avanzamento sui byte attraversati; il conteggio dei byte deve restare coerente con il cursore.

### Chiavi per architettura e chiavi "di formato"
Gli iperparametri del modello hanno il nome dell'architettura come prefisso: `general.architecture = "llama"` → `llama.block_count`, `llama.embedding_length`, `llama.attention.head_count_kv`, `llama.rope.freq_base`… Un modello Qwen 3 usa gli stessi suffissi con prefisso `qwen3.`: tra architetture della stessa famiglia cambia soprattutto il forward pass (es. la QK-norm di Qwen 3) e i nomi di alcuni tensori, meno il set di metadata. Eccezione al principio "il parser GGUF non interpreta le chiavi": `general.alignment` (u32, default 32 se assente) è una chiave del **formato**, perché decide dove iniziano i dati dei tensori; la deve leggere il parser GGUF stesso.

---

*(si aggiorna man mano che emergono nuovi dettagli implementativi)*
