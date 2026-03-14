# NailsCPP
#### ML-powered service for smart nail design recommendation right in telegram

Role distribution:
Alima Chekueva (alickqs) : word2vec implementation & benchmarks (include/word2vec; src/word2vec; benchmarks directories)
Stoyan Uliana : SQLite database & tests
Mitrichieva Anna : tg bot & dataset

----

Overview:
1. Project Architecture
2. Setup Guide
4. Project Preza

---
## Project Architecture
UI is a tg bot, which works with the SQLite database to store both private and shared repos of manicure designs. Word2vec is trained or data/nail_corpus.txt and retrieves the most similar image from the shared repo

## Setup Guide
Copy the repo & build - that's it!

## Project Preza
https://docs.google.com/presentation/d/1d-7urtjtMP_5g3cQo0AuRQc9DDh6l6djGjACVwfEWIk/edit?usp=sharing
