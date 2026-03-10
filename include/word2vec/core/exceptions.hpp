class ModelException : public std::runtime_error {
public:
    explicit ModelException(const std::string& msg) : std::runtime_error(msg) {}
};

class FileIOException : public ModelException {
public:
    explicit FileIOException(const std::string& path)
        : ModelException("Cannot read/write file: " + path) {}
};

class VocabularyException : public ModelException {
public:
    explicit VocabularyException(const std::string& msg)
        : ModelException("Vocabulary error: " + msg) {}
};