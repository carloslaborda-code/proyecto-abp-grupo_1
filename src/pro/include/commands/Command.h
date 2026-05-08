#pragma once

class Command {
public:
    virtual ~Command() = default;
    virtual bool execute() = 0;
};
