//
// Created by e-al on 27.11.15.
//

#ifndef DMEMBER_TUPLE_H
#define DMEMBER_TUPLE_H

#include <vector>
#include <string>

class Tuple
{
public:
    Tuple() : id(0), data() {}
    Tuple(int newId, const std::vector<std::string>& newData) : id(newId), data(newData) {}
    Tuple(const Tuple& rhs)
    {
        this->id = rhs.id;
        this->data = rhs.data;
    }
    Tuple& operator=(const Tuple& rhs)
    {
        if (&rhs == this) return *this;
        this->id = rhs.id;
        this->data = rhs.data;
        return *this;
    }

    ~Tuple() {}

    int id;
    std::vector<std::string> data;

};


#endif //DMEMBER_TUPLE_H
