#pragma once
#include "array_ptr.h"
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <iterator>
#include <cassert>

//класс-обёртка для создания вектора с резервированием
class ReserveProxyObj {
public:
    ReserveProxyObj(size_t cap) {
        n_ = cap;
    }
    size_t Res() const {
        return n_;
    }
private:
    size_t n_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    //Если размер нулевой, динамическая память для его элементов выделяться не должна
    explicit SimpleVector(size_t size) {
        if (size != 0) {
            ArrayPtr<Type> tmp(size);
            items_.swap(tmp);
            size_ = size;
            cap_ = size;
        }
    }
    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        SimpleVector tmp(size);
        std::fill(tmp.begin(), tmp.end(), value);
        items_.swap(tmp.items_);
        size_ = size;
        cap_ = size;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        size_t size = init.size();
        Create(init.begin(), init.end(), size);
    }

    SimpleVector(const SimpleVector& other) {
        size_t size = other.GetSize();
        Create(other.begin(), other.end(), size);
    }

    //создание пустого вектора с резервированием памяти
    SimpleVector(const ReserveProxyObj& obj) {
        SimpleVector<Type> tmp(obj.Res());
        this->swap(tmp);
        size_ = 0;
    }

    //конструктор перемещения
    SimpleVector(SimpleVector&& other) noexcept
        :items_{ std::move(other.items_) }
        , size_{ std::exchange(other.size_, 0) }
        , cap_{ std::exchange(other.cap_, 0) }
    {
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this == rhs) {
            return *this;
        }
        SimpleVector<Type> copy_rhs(rhs);
        this->swap(copy_rhs);
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this == &rhs) {
            return *this;
        }
        items_ = std::move(rhs.items_);
        size_ = std::exchange(rhs.size_, 0);
        cap_ = std::exchange(rhs.cap_, 0);
        return *this;
    }

    //задает ёмкость вектора
    void Reserve(size_t new_capacity) {
        if (new_capacity <= cap_) {
            return;
        }
        SimpleVector tmp(new_capacity);
        std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(this->end()), tmp.begin());
        items_.swap(tmp.items_);
        cap_ = new_capacity;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return cap_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0 ? true : false;
    }

    // Возвращает ссылку на элемент с индексом index
    //Для корректной работы оператора индекс элемента массива не должен выходить за пределы массива
    Type& operator[](size_t index) noexcept {
        assert(index <= size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index <= size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("index should be less than size");
        }
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index should be less than size");
        }
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size < size_) {
            size_ = new_size;
            return;
        }
        if (new_size <= cap_) {
            Type n{};
            for (size_t i = size_; i < new_size; ++i) {
                items_[i] = std::move(n);
            }
            size_ = new_size;
            return;
        }
        if (new_size > cap_) {
            SimpleVector tmp(std::max(new_size, cap_ * 2));
            std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(this->end()), tmp.begin());
            items_.swap(tmp.items_);
            size_ = new_size;
            cap_ = std::max(new_size, cap_ * 2);
        }
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (size_ < cap_) {
            items_[size_] = item;
            ++size_;
            return;
        }
        if (cap_ == 0) {
            SimpleVector tmp(1);
            tmp[size_] = item;
            items_.swap(tmp.items_);
            cap_ = 1;
        }
        else {
            SimpleVector tmp(cap_ * 2);
            std::copy(this->begin(), this->end(), tmp.begin());
            tmp[size_] = item;
            items_.swap(tmp.items_);
            cap_ *= 2;
        }
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ < cap_) {
            items_[size_] = std::move(item);
            ++size_;
            return;
        }
        if (cap_ == 0) {
            SimpleVector tmp(1);
            tmp[size_] = std::move(item);
            items_.swap(tmp.items_);
            cap_ = 1;
        }
        else {
            SimpleVector tmp(cap_ * 2);
            std::copy(std::make_move_iterator(this->begin()), std::make_move_iterator(this->end()), tmp.begin());
            tmp[size_] = std::move(item);
            items_.swap(tmp.items_);
            cap_ *= 2;
        }
        ++size_;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        if (cap_ == 0) {
            PushBack(value);
            return begin();
        }
        auto d = std::distance(cbegin(), pos);
        if (size_ < cap_) {
            std::copy_backward(pos, cend(), end() + 1);
            items_[d] = value;
            ++size_;
            return Iterator{ &items_[d] };
        }
        SimpleVector tmp(cap_ * 2);
        std::copy(cbegin(), pos, tmp.begin());
        tmp[d] = value;
        std::copy(pos, cend(), &tmp[d + 1]);
        items_.swap(tmp.items_);
        cap_ *= 2;
        ++size_;
        return Iterator{ &items_[d] };
    }

    Iterator Insert(Iterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        if (cap_ == 0) {
            PushBack(std::move(value));
            return begin();
        }
        auto d = std::distance(begin(), pos);
        if (size_ < cap_) {
            std::copy_backward(std::make_move_iterator(pos), std::make_move_iterator(end()), end() + 1);
            items_[d] = std::move(value);
            ++size_;
            return Iterator{ &items_[d] };
        }
        SimpleVector tmp(cap_ * 2);
        std::copy(std::make_move_iterator(begin()), std::make_move_iterator(pos), tmp.begin());
        tmp[d] = std::move(value);
        std::copy(std::make_move_iterator(pos), std::make_move_iterator(end()), &tmp[d + 1]);
        items_.swap(tmp.items_);
        cap_ *= 2;
        ++size_;
        return Iterator{ &items_[d] };
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    //Не допускается вызывать PopBack, когда вектор пуст
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());
        Iterator pos_c = const_cast<Iterator>(pos);
        auto d = std::distance(begin(), pos_c);
        std::copy(std::make_move_iterator(pos_c + 1), std::make_move_iterator(end()), &items_[d]);
        --size_;
        return Iterator{ &items_[d] };
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(cap_, other.cap_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        Type* b = &items_[0];
        return Iterator{ b };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        Type* e = &items_[size_];
        return Iterator{ e };

    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        const Type* b = &items_[0];
        return ConstIterator{ b };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        const Type* e = &items_[size_];
        return ConstIterator{ e };
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        Type* b = const_cast<Type*>(&items_[0]);
        return ConstIterator{ b };
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        if (cap_ == 0) {
            return nullptr;
        }
        Type* e = const_cast<Type*>(&items_[size_]);
        return ConstIterator{ e };
    }
private:
    ArrayPtr<Type> items_{};
    size_t size_ = 0;
    size_t cap_ = 0;

    template <typename It>
    void Create(It begin, It end, size_t size) {
        SimpleVector<Type> tmp(size);
        auto from = begin;
        auto to = end;
        int i = 0;
        while (from != to) {
            tmp[i] = *from;
            ++from;
            ++i;
        }
        tmp.size_ = size;
        tmp.cap_ = size;
        this->swap(tmp);
    }

};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

    return !(rhs > lhs);
}

