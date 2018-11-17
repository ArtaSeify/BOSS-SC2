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

    size_t size() const;

    bool isEmpty() const;
    bool contains(ActionType action) const;
    void add(ActionType action);
    void add(const ActionSet & set);
    void remove(ActionType action);
    void remove(const ActionSet & set);
    void clear();

          ActionType & operator[] (size_t index);
    ActionType operator[] (size_t index) const;

    const std::string toString() const;
};

}