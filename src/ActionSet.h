/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "Common.h"
#include <vector>

namespace BOSS
{
    class ActionType;

    class ActionSet
    {
        std::vector<ActionType> m_actions;

    public:

        ActionSet();

        int size() const;

        bool isEmpty() const;
        bool contains(ActionType action) const;
        void add(ActionType action);
        void add(const ActionSet & set);
        void remove(ActionType action);
        void remove(const ActionSet & set);
        void clear();

        ActionType & operator[] (int index);
        ActionType operator[] (int index) const;

        const std::string toString() const;
    };
}
