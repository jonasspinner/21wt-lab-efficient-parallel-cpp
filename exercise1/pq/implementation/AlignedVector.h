#pragma once

template<class T, bool throw_exceptions = false>
class AlignedVector {
public:
    explicit AlignedVector(std::size_t capacity, std::size_t block_size = 1, std::size_t offset = 0)
            : m_capacity(capacity), m_size(0) {
        assert(block_size != 0);
        assert(offset < block_size);
        if (throw_exceptions && block_size == 0) {
            throw std::invalid_argument("block_size is 0");
        }
        if (throw_exceptions && offset >= block_size) {
            throw std::invalid_argument("offset must be less than block_size");
        }
        if (capacity > 0) {
            m_alloc_ptr = static_cast<T *>(std::aligned_alloc(alignof(T) * block_size,
                                                              sizeof(T) * (capacity + block_size)));
            if (m_alloc_ptr == nullptr) {
                throw std::bad_alloc();
            }
            m_elements = m_alloc_ptr + offset;
        }
    }

    AlignedVector(const AlignedVector &other) = delete;

    AlignedVector &operator=(const AlignedVector &other) = delete;

    AlignedVector(AlignedVector &&other) noexcept = delete;

    AlignedVector &operator=(AlignedVector &&other) = delete;

    ~AlignedVector() {
        while (!empty()) {
            pop_back();
        }
        free(m_alloc_ptr);
        m_alloc_ptr = nullptr;
        m_elements = nullptr;
        m_capacity = 0;
        m_size = 0;
    }

    [[nodiscard]] constexpr bool empty() const {
        return m_size == 0;
    }

    [[nodiscard]] constexpr std::size_t size() const {
        return m_size;
    }

    [[nodiscard]] constexpr std::size_t capacity() const {
        return m_capacity;
    }

    T &operator[](std::size_t index) {
        assert(index < m_size);
        return m_elements[index];
    }

    const T &operator[](std::size_t index) const {
        assert(index < m_size);
        return m_elements[index];
    }

    void push_back(const T &value) {
        assert(m_size < m_capacity);
        if (throw_exceptions && m_size >= m_capacity) {
            throw std::length_error("reached capacity");
        }
        new(m_elements + m_size) T(value);
        m_size++;
    }

    void push_back(T &&value) {
        assert(m_size < m_capacity);
        if (throw_exceptions && m_size >= m_capacity) {
            throw std::length_error("reached capacity");
        }
        new(m_elements + m_size) T(std::move(value));
        m_size++;
    }

    void pop_back() {
        assert(!empty());
        m_size--;
        m_elements[m_size].~T();
    }

    T &front() {
        assert(!empty());
        return m_elements[0];
    }

    const T &front() const {
        assert(!empty());
        return m_elements[0];
    }

    T &back() {
        assert(!empty());
        return m_elements[m_size - 1];
    }

    const T &back() const {
        assert(!empty());
        return m_elements[m_size - 1];
    }

private:
    T *m_alloc_ptr{nullptr};
    T *m_elements{nullptr};
    std::size_t m_capacity{0};
    std::size_t m_size{0};
};
