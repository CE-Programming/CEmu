/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOCKEDWIDGET_H
#define DOCKEDWIDGET_H

class CoreWindow;
class CoreWrapper;

namespace KDDockWidgets
{
class DockWidgetBase;
}

#include <QtCore/QJsonValue>
#include <QtWidgets/QWidget>
QT_BEGIN_NAMESPACE
class QIcon;
QT_END_NAMESPACE

#include <iterator>
#include <limits>

class DockedWidget;

class DockedWidgetList
{
    DockedWidgetList *mPrev, *mNext;

    template<typename Node, typename Value,
             DockedWidgetList *DockedWidgetList::*Dec, DockedWidgetList *DockedWidgetList::*Inc>
    class Iter
    {
        friend class DockedWidgetList;

        Node *node;

    public:
        using   difference_type = std::ptrdiff_t;
        using        value_type = Value;
        using           pointer = value_type *;
        using         reference = value_type &;
        using iterator_category = std::bidirectional_iterator_tag;

        Iter(Node *node = nullptr) noexcept : node{node} {}

        reference operator* () const noexcept { return static_cast<reference>(*node); }
        pointer   operator->() const noexcept { return static_cast<pointer>(node); }

        Iter     &operator--() noexcept { node = node->*Dec; return *this; }
        Iter     &operator++() noexcept { node = node->*Inc; return *this; }
        Iter      operator--(int) noexcept { Iter iter = *this; --*this; return iter; }
        Iter      operator++(int) noexcept { Iter iter = *this; ++*this; return iter; }

        bool      operator==(Iter iter) const noexcept { return node == iter.node; }
        bool      operator!=(Iter iter) const noexcept { return node != iter.node; }

        operator Iter<const Node, const Value, Dec, Inc>() { return node; }
    };

    DockedWidgetList(const DockedWidgetList &) = delete;
    DockedWidgetList(DockedWidgetList &&) = delete;
    DockedWidgetList &operator=(const DockedWidgetList &) = delete;
    DockedWidgetList &operator=(DockedWidgetList &&) = delete;

protected:
    DockedWidgetList(DockedWidgetList *list) noexcept
        : mPrev{list->mPrev}, mNext{list} { mPrev->mNext = mNext->mPrev = this; }

public:
    using              size_type = std::size_t;
    using        difference_type = std::ptrdiff_t;
    using             value_type = DockedWidget;
    using                pointer =       value_type *;
    using          const_pointer = const value_type *;
    using              reference =       value_type &;
    using        const_reference = const value_type &;
    using               iterator = Iter<      DockedWidgetList,       value_type,
                                        &DockedWidgetList::mPrev, &DockedWidgetList::mNext>;
    using         const_iterator = Iter<const DockedWidgetList, const value_type,
                                        &DockedWidgetList::mPrev, &DockedWidgetList::mNext>;
    using       reverse_iterator = Iter<      DockedWidgetList,       value_type,
                                        &DockedWidgetList::mNext, &DockedWidgetList::mPrev>;
    using const_reverse_iterator = Iter<const DockedWidgetList, const value_type,
                                        &DockedWidgetList::mNext, &DockedWidgetList::mPrev>;

     DockedWidgetList() noexcept : mPrev{this}, mNext{this} {}
    ~DockedWidgetList() noexcept { mPrev->mNext = mNext; mNext->mPrev = mPrev; }

                  iterator   begin()       noexcept { return mNext; }
                  iterator   end  ()       noexcept { return  this; }
            const_iterator   begin() const noexcept { return mNext; }
            const_iterator   end  () const noexcept { return  this; }
            const_iterator  cbegin() const noexcept { return mNext; }
            const_iterator  cend  () const noexcept { return  this; }
          reverse_iterator  rbegin()       noexcept { return mPrev; }
          reverse_iterator  rend  ()       noexcept { return  this; }
    const_reverse_iterator  rbegin() const noexcept { return mPrev; }
    const_reverse_iterator  rend  () const noexcept { return  this; }
    const_reverse_iterator crbegin() const noexcept { return mPrev; }
    const_reverse_iterator crend  () const noexcept { return  this; }

    bool      empty()    const noexcept { return cbegin() == cend(); }
    size_type size()     const noexcept { return std::distance(cbegin(), cend()); }
    size_type max_size() const noexcept { return std::numeric_limits<difference_type>::max(); }

          reference front()       { return *  begin(); }
    const_reference front() const { return * cbegin(); }
          reference back ()       { return * rbegin(); }
    const_reference back () const { return *crbegin(); }
};

class DockedWidget : public QWidget, protected DockedWidgetList
{
    friend class DockedWidgetList;

protected:
    DockedWidget(KDDockWidgets::DockWidgetBase *dock, const QIcon &icon, CoreWindow *coreWindow);

public:
    KDDockWidgets::DockWidgetBase *dock();
    CoreWindow *coreWindow();
    CoreWrapper &core();
    virtual void loadFromCore(const CoreWrapper &) {}
    virtual void storeToCore(CoreWrapper &) const {}
    virtual QJsonValue serialize() const { return QJsonValue::Undefined; }
    virtual bool unserialize(const QJsonValue &config) { return config.isUndefined(); }

 private:
    CoreWindow *mCoreWindow;
};

#endif
