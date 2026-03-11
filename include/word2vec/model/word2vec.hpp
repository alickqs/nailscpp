class Word2VecModel : public ITrainable {
private:
    Vocabulary vocab;
    std::vector<std::vector<int>> training_data;
    std::unique_ptr<NegativeSampler> sampler;  // unique_ptr владеет сэмплером
    double learning_rate;
    std::shared_ptr<std::vector<double>> shared_parameters;  // shared_ptr для демонстрации

public:
    explicit Word2VecModel(const std::string& filename)
        : learning_rate(LEARNING_RATE) {

        // Пример shared_ptr - общие параметры для нескольких компонентов
        shared_parameters = std::make_shared<std::vector<double>>(std::initializer_list<double>{
            LEARNING_RATE, LR_DECAY, NEGATIVE_SAMPLES * 1.0
        });

        load_data(filename);
        initialize_sampler();
    }

    // Загрузка данных с обработкой ошибок через исключения
    void load_data(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw FileIOException(filename);
        }

        std::string line, word;
        int line_num = 0;

        try {
            while (std::getline(file, line)) {
                line_num++;
                std::istringstream iss(line);
                std::vector<int> object;

                while (iss >> word) {
                    int id = vocab.add_word(word);
                    object.push_back(id);
                }

                if (!object.empty()) {
                    training_data.push_back(object);
                }
            }
        } catch (const std::exception& e) {
            throw ModelException("Error parsing line " + std::to_string(line_num) + ": " + e.what());
        }

        std::cout << "Loaded " << vocab.size() << " words, "
                  << training_data.size() << " objects" << std::endl;
    }

    void initialize_sampler() {
        sampler = std::make_unique<NegativeSampler>(vocab.get_frequencies());
    }

    // Реализация чисто виртуальных методов ITrainable
    void train(const std::vector<std::vector<int>>& data) override {
        double current_lr = learning_rate;

        for (int epoch = 0; epoch < EPOCHS; ++epoch) {
            size_t updates = 0;

            for (const auto& object : data) {
                for (size_t i = 0; i < object.size(); ++i) {
                    int target = object[i];

                    for (size_t j = 0; j < object.size(); ++j) {
                        if (i == j) continue;
                        int context = object[j];

                        // Позитивный пример
                        train_pair(target, context, 1.0, current_lr);

                        // Негативные примеры
                        for (int ns = 0; ns < NEGATIVE_SAMPLES; ++ns) {
                            int negative = sampler->sample();
                            train_pair(target, negative, 0.0, current_lr);
                        }

                        updates++;
                    }
                }
            }

            current_lr *= LR_DECAY;
            std::cout << "Epoch " << epoch + 1 << " completed. LR = " << current_lr << std::endl;
        }
    }

    void train_pair(int target, int context, double label, double lr) {
        EmbeddingVector& target_vec = vocab.get_embedding(target);
        EmbeddingVector& context_vec = vocab.get_embedding(context);

        double dot = target_vec.dot(context_vec);
        double pred = sigmoid_constexpr(dot);
        double gradient = pred - label;

        // Временные вектора для градиентов
        EmbeddingVector target_grad(EMBED_DIM);
        EmbeddingVector context_grad(EMBED_DIM);

        for (size_t d = 0; d < EMBED_DIM; ++d) {
            target_grad[d] = gradient * context_vec[d];
            context_grad[d] = gradient * target_vec[d];
        }

        target_vec.sgd_update(target_grad, lr);
        context_vec.sgd_update(context_grad, lr);
    }

    void save(const std::string& path) const override {
        std::ofstream file(path);
        if (!file.is_open()) {
            throw FileIOException(path);
        }

        file << "VOCAB_SIZE " << vocab.size() << "\n";
        file << "EMBED_DIM " << EMBED_DIM << "\n";

        for (size_t i = 0; i < vocab.size(); ++i) {
            file << "WORD " << vocab.get_word(i);
            const auto& emb = vocab.get_embedding(i);
            for (size_t d = 0; d < EMBED_DIM; ++d) {
                file << " " << emb[d];
            }
            file << "\n";
        }
    }

    void load(const std::string& path) override {
        // Заглушка - в реальном проекте была бы реализация
        throw ModelException("Load not implemented in this demo");
    }

    double evaluate(const std::vector<int>& target, const std::vector<int>& context) const override {
        // Заглушка
        return 0.0;
    }

    // Поиск похожих слов с использованием optional
    std::optional<std::vector<std::pair<double, std::string>>> find_similar(
        const std::string& word, int k = TOP_K) {

        auto target_emb_opt = vocab.get_embedding(word);
        if (!target_emb_opt) {
            return std::nullopt;  // Слово не найдено
        }

        const EmbeddingVector& target_emb = target_emb_opt->get();
        std::vector<std::pair<double, std::string>> similarities;

        for (size_t i = 0; i < vocab.size(); ++i) {
            const auto& other_emb = vocab.get_embedding(i);
            double dot = target_emb.dot(other_emb);
            double sim = dot / (target_emb.norm() * other_emb.norm() + 1e-8);
            similarities.emplace_back(sim, vocab.get_word(i));
        }

        std::sort(similarities.begin(), similarities.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        if (similarities.size() > static_cast<size_t>(k)) {
            similarities.resize(k);
        }

        return similarities;
    }

    // Интерактивный режим с обработкой исключений
    void interactive_mode() {
        std::cout << "\n=== Interactive Mode ===\n"
                  << "Enter words to find similar ones, or 'quit' to exit.\n";

        std::string input;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, input);

            if (input == "quit") break;

            try {
                auto result = find_similar(input, TOP_K);

                if (result.has_value()) {
                    std::cout << "Top " << TOP_K << " words similar to '" << input << "':\n";
                    for (const auto& [score, word] : *result) {
                        std::cout << "  " << word << " (similarity: "
                                  << std::fixed << std::setprecision(4) << score << ")\n";
                    }
                } else {
                    std::cout << "Word '" << input << "' not found in vocabulary.\n";
                }
            } catch (const ModelException& e) {
                std::cerr << "Model error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Unexpected error: " << e.what() << std::endl;
            }
        }
    }
};