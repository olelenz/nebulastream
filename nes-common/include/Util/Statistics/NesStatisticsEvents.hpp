#pragma once
#include <string>

class NesStatisticsEvents{
    public:
        virtual ~NesStatisticsEvents() = default;
        virtual std::string toCSV() const = 0;
};
class NesBufferAllocateEvent : public NesStatisticsEvents{
public:
    explicit NesBufferAllocateEvent(int queryId);

    std::string toCSV() const override;

private:
    int id;
};
