#pragma once

#include <cassert>

template<class T, bool throw_exceptions = false>
class AlignedVector {
public:
    explicit AlignedVector(std::size_t capacity = 0, std::size_t block_size = 1, std::size_t offset = 0)
            : m_block_size(block_size), m_offset(offset) {
        assert(block_size != 0);
        assert(offset < block_size);
        if (throw_exceptions && block_size == 0) {
            throw std::invalid_argument("block_size is 0");
        }
        if (throw_exceptions && offset >= block_size) {
            throw std::invalid_argument("offset must be less than block_size");
        }
        reserve(capacity);
    }

    void reserve(std::size_t new_cap) {
        if (new_cap > m_capacity) {
            const auto alignment = next_power_of_two(block_alignment(m_block_size));
            const auto size = ((sizeof(T) * (new_cap + m_offset) + alignment - 1) / alignment) * alignment;

            assert(is_power_of_two(alignment) && "alignment must be a power of two");
            assert(is_multiple_of(size, alignment) && "size must be an integer multiple of alignment");

            auto *alloc_ptr = static_cast<T *>(std::aligned_alloc(alignment, size));

            if (alloc_ptr == nullptr) {
                throw std::bad_alloc();
            }

            auto *elements = alloc_ptr + m_offset;

            if (m_alloc_ptr != nullptr) {
                std::uninitialized_move_n(m_elements, m_size, elements);

                free(m_alloc_ptr);
                m_alloc_ptr = nullptr;
                m_elements = nullptr;
            }

            m_alloc_ptr = alloc_ptr;
            m_elements = elements;
            m_capacity = size / sizeof(T) - m_offset;
        }
        assert(m_alloc_ptr != nullptr);
        assert(m_elements != nullptr);
        assert(m_capacity >= new_cap);
        assert(m_size <= m_capacity);
    }

    [[nodiscard]] static constexpr std::size_t block_alignment(std::size_t block_size) noexcept {
        return alignof(T) * block_size;
    }

    AlignedVector(const AlignedVector &other) = delete;

    AlignedVector &operator=(const AlignedVector &other) = delete;

    AlignedVector(AlignedVector &&other) noexcept = delete;

    AlignedVector &operator=(AlignedVector &&other) = delete;

    ~AlignedVector() {
        if (m_alloc_ptr != nullptr) {
            std::destroy_n(m_elements, m_size);
            free(m_alloc_ptr);
            m_alloc_ptr = nullptr;
            m_elements = nullptr;
            m_capacity = 0;
            m_size = 0;
        }

        assert(m_alloc_ptr == nullptr);
        assert(m_elements == nullptr);
        assert(m_capacity == 0);
        assert(m_size == 0);
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return m_size == 0;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return m_size;
    }

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return m_capacity;
    }

    [[nodiscard]] constexpr T *data() noexcept {
        return m_elements;
    }

    [[nodiscard]] constexpr const T *data() const noexcept {
        return m_elements;
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
    [[nodiscard]] static constexpr std::size_t next_power_of_two(std::size_t value) noexcept {
        if (value <= 1) {
            return 1;
        }
        std::size_t k = 2;
        value--;
        while (value >>= 1) {
            k <<= 1;
        }
        return k;
    }

    [[nodiscard]] static constexpr bool is_power_of_two(std::size_t value) noexcept {
        return (value & (value - 1)) == 0;
    }

    [[nodiscard]] static constexpr bool is_multiple_of(std::size_t value, std::size_t multiple) noexcept {
        return value == (value / multiple) * multiple;
    }

    T *m_alloc_ptr{nullptr};
    T *m_elements{nullptr};
    std::size_t m_capacity{0};
    std::size_t m_size{0};
    std::size_t m_block_size{1};
    std::size_t m_offset{0};
};
